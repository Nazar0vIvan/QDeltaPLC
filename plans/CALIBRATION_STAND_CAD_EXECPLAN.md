# Calibration stand and table in the scene, with build-time STEP to BREP conversion

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`,
`Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is
maintained in accordance with `PLANS.md` at the repository root.

Three rules override `PLANS.md` for this plan. First, the implementing agent never runs a build,
CMake configure, QML lint, the application or any test; when a milestone needs a build or a run,
the agent stops, asks the user to build or run exactly what the milestone names, and continues
from the user's reported result. Second, no git staging, commits, branch switches or stashes are
made unless the user asks for them. Third, no project tests, test harnesses or test build targets
are added. Every rule of `AGENTS.md` (Mandatory change procedure, Mandatory C++ style, Mandatory
code reduction, Mandatory interface stability) applies to every line written for this plan; re-read
those sections before each logical change and after any context loss.


## Purpose / Big Picture

The probe calibration motion (the Calibration submode of Measuring mode, implemented by
`plans/PROBE_SPHERE_CALIBRATION_EXECPLAN.md`) moves the WP-500 probe around a 25 mm calibration
sphere. The sphere stands on a stand (a pole with an adapter) on a table, but the 3D scene shows
only the sphere, so the user cannot see whether the robot or the probe hits the table or the stand
during Dry Run.

After this plan, when the user enters the Calibration submode, the 3D scene shows, together with
the calibration sphere:

1. the calibration stand, read from `resources/cad/CALIBRATION_STAND.stp` and placed with its
   coordinate origin at the fitted sphere centre;
2. the top surface of the table, a 935 mm × 740 mm rectangle 170.5 mm below the sphere centre,
   placed from the user's tape-measure dimensions.

Both are decorative: clicking them selects nothing, and they do not influence camera fitting. They
appear, disappear and are hidden together with the calibration sphere. The user plays Dry Run and
checks visually for collisions.

The plan also changes how CAD files reach the application. STEP files (`*.stp`) in
`resources/cad/` are converted to BREP files (OCCT's native text format, `*.brep`) during the
build, by a small converter program built from this repository. A STEP file that changes is
converted again on the next build. Only BREP files are copied into the build directory; the
application keeps reading BREP only. The robot links stay hand-maintained BREP files in
`resources/cad/kr10/`. The two end effectors are now built from `resources/cad/EE_WP-500.stp`
(the measuring tool, formerly `MEE.brep`) and `resources/cad/EE_GDZ-65-800A.stp` (the spindle tool,
formerly `SEE.brep`).


## Progress

Record every stopping point here with a UTC timestamp, splitting a partially completed step into
"completed" and "remaining". Record physical/nonblank line counts of every touched file at the
start and end of each milestone.

- [x] (2026-10-29) Milestone 1: converter program `cadstep2brep`, build-time conversion of `resources/cad/*.stp`
  and deployment of the generated BREPs (completed 2026-10-29: source edits, compliance pass, and
  a clean build by the user in a deleted-and-recreated
  `build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug` (Ninja): `.ninja_log` records the three
  conversions, `generated/cad/` and `resources/cad/` both hold `CALIBRATION_STAND.brep` (71030
  bytes), `EE_GDZ-65-800A.brep` (6510004) and `EE_WP-500.brep` (224678), no `.stp` or `.tmp` file
  exists under `resources/`. The application then reported the expected
  `Cannot read CAD file: .../resources/cad/kr10/MEE.brep` (see Surprises), which Milestone 2
  removes. Incremental check, 2026-10-29: the user re-saved `EE_GDZ-65-800A.stp` (15:34:37) and
  built; `.ninja_log` records only `generated/cad/EE_GDZ-65-800A.brep` (15:35:13) and a relink of
  `robocrap.exe` (15:36:56), and the deployed `resources/cad/EE_GDZ-65-800A.brep` carries the new
  time. No-change check, 2026-10-29: the user ran Qt Creator's Rebuild (clean, then build), which
  removed and regenerated all three BREPs (15:42:44–15:42:47) and relinked `robocrap.exe`, as a
  clean build must; the following Run built again and `.ninja_log` records only
  `CMakeFiles/cmake.verify_globs`, with no conversion and no relink. Milestone 1 is complete.
  Line counts, physical/nonblank. Build inputs before: CMakeLists.txt 69/54,
  cmake/DeployOcct.cmake 82/78. After: CMakeLists.txt 71/56, cmake/DeployOcct.cmake 117/111,
  new cmake/ConvertStep.cmake 17/16, new tools/cadstep2brep/CMakeLists.txt 3/3 (+73/+71 build
  inputs in total). C++: new tools/cadstep2brep/main.cpp 76/68. src/application/CMakeLists.txt
  is unchanged (131/126). The growth is the required conversion: the converter (STEP read, BREP
  write, argument and error handling), one custom command per STEP file, the conversion target,
  the relink dependency and the deployment copy. The smaller alternative, a runtime cache, was
  rejected by the user's requirement that only BREP files reach the build.
  Compliance pass: main.cpp has no lambda, template, project exception or output parameter;
  `readStep` returns shape and error together (`StepReadResult`), `writeBrep` returns the error;
  at most two parameters per function; try/catch only around the OCCT/std calls. The CMake rules
  reuse the function's `compiler_bin`, `occt_prefix` and `OCCT_RUNTIME_SEARCH_DIRS` and the
  existing directory check. One finding fixed during the pass: the converter's DLL directories
  listed `compiler_bin` in addition to `OCCT_RUNTIME_SEARCH_DIRS`, which already defaults to it
  and must contain it for the existing deployment to resolve the MinGW runtime; removed.
- [x] (2026-10-29) Milestone 2: end effectors loaded from the generated BREPs; obsolete BREPs removed from the
  repository (the user rebuilt and ran the application on 2026-10-29 and reported that
  everything is in order: the robot loads and the tools appear correctly; completed earlier the same day: source edits, deletion of `resources/cad/EE_WP-500.brep` and
  `resources/cad/EE_GDZ-65-800A.brep` (byte-identical to `HEAD:resources/cad/kr10/MEE.brep` and
  `SEE.brep`; the user had already deleted the `kr10` copies), compliance pass; remaining: the
  user's build and run with the visual tool check of Validation).
  Line counts, physical/nonblank: src/application/assetpaths.cpp 17/13 to 17/13,
  src/application/robot/robotcontroller.cpp 182/160 to 183/161. The added line is the `kr10`
  link directory, now separate from the CAD root that holds the end effectors.
  Compliance pass: `ViewportAssets::cadDirectory` keeps its name and type; its only caller,
  `RobotController::loadRobot`, was updated; no new function, field or parameter; the comment
  above `visuals.tools` names the new files and their `RobotTool` order.
- [x] (2026-10-29) Milestone 3: viewport support for decorative parts that survive diagnostic updates, and for
  CAD model parts loaded on demand (`DrawingModel`) (source edits and compliance pass; the user
  built and confirmed the regression checks on 2026-10-29: draft axis in Machining mode,
  points/normals toggles of an imported plane, calibration sphere and path display).
  Line counts, physical/nonblank, before to after: include/viewport/occcontroller.h 171/147 to
  186/160, occcontroller.cpp 283/252 to 334/298, adapters/occsceneadapter.h 37/30 to 41/34,
  adapters/occsceneadapter.cpp 274/257 to 297/277, occt/occviewport.cpp 201/173 unchanged (one
  initializer changed), occt/occviewwindow.{h,cpp} unchanged; total +93/+83. Growth: the
  `DrawingModel` value and its variant entry, the model queue and worker with their start/finish
  operations and redraw of waiting objects, the borrowed model map in the adapter, the moved model
  shape, and the separate decoration list. Reductions in the same files: `removeParts` takes one
  list (its nested loop over two lists is gone) and `setObjectVisible` iterates one list; the
  plan's `requestModels` method was not added because its five lines run only in `insertObject`.
  Compliance pass: no lambda, template, exception or output parameter; at most two parameters
  (`OccSceneAdapter` constructor, `modelShape`, `usesModel`); the adapter borrows the map as a
  const input reference whose owner (`OccController::m_document`) outlives the window, viewport
  and adapter; the worker is joined in the destructor before the window is deleted; new state is
  only `DrawingDocument::models`, `m_modelQueue` and `m_modelWorker`, each needed to load one path
  at a time, report a failure once and redraw after loading. No findings remained.
- [x] (2026-10-29) Milestone 4: table data in the sphere file, stand and table drawn with the calibration
  sphere (completed 2026-10-29: source edits, repository sphere file, compliance pass; remaining:
  the user adds the `table` object to the deployed
  `build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/resources/json/calibration_sphere.json`,
  which has no `table` key, then builds, runs and performs the Validation checks).
  Line counts, physical/nonblank, before to after: scene/scenegeometry.h 129/120 to 149/138,
  scene/scenecalibration.cpp 273/244 to 298/268, scene/sceneobject.cpp 255/234 to 262/241 (C++
  +52/+49); resources/json/calibration_sphere.json 18/18 to 19/19. Growth: the five table values
  with their comparison, their validation and error message, the stand path, and the two drawn
  parts. Smaller alternative considered: hard-coded table constants (no struct, reader or JSON
  key, about 30 lines less); rejected because the table is measured setup data like the strut
  values already read from the same file (Decision Log).
  Compliance pass: `calibrationTable(root)` has one parameter, reuses `positiveNumber` and returns
  `std::optional`; no lambda, template, exception or output parameter; `SceneSphere` stays an
  aggregate built by value in `readSphereFile`; `activate` sets `standModel` on its own local
  result before passing it to `replaceSphere`; `operator==` compares the new fields, so a changed
  table or path replaces the sphere object on the next entry. No findings remained.
  First run, 2026-10-29: the deployed sphere file lacked `table` (the build directory had been
  recreated from the old repository copy), so Calibration showed the table error; the agent added
  the `table` line to the deployed file, preserving its CRLF endings. Second run: table and stand
  appeared at the expected place but as wireframe (see Surprises). Fix: decorative parts are
  excluded from fitting through their presentation structures instead of the AIS object (Decision
  Log). Line counts of the fix, before to after: occt/occpartprops.h 37/29 to 38/30,
  occt/occpart.h 35/27 to 37/29, occt/occpart.cpp 64/56 to 65/57, occt/occscene.cpp 374/308 to
  379/313, adapters/occsceneadapter.cpp 297/277 to 299/279 (+10/+10). Compliance: one new
  `OccPartProps`/`OccPart` field with a getter, set by the adapter and read by `OccScene`; no
  lambda, template or output parameter. Table made opaque at the user's request (style
  `{{190, 190, 190}, 0.0, true}`); the stand was already opaque. The user built, checked and
  accepted the result on 2026-10-29.
- [x] (2026-10-29) Milestone 5: documentation updates and the final `AGENTS.md` compliance pass.
  Documentation, physical/nonblank lines: docs/AGENT_CONTRACTS.md 361 (HEAD) to 377/360 (Build
  and modules: converter target and conversion/relink rule; Viewport and CAD: decorative fit
  exclusion through presentation structures, new tool names, `DrawingModel` loading, BREP-only
  application with build-time STEP conversion; Geometry and scene: table and stand as decorative
  sphere parts). docs/ARCHITECTURE.md 486/396 (hardware row, `OccController`, `OccSceneAdapter`,
  Resources, CAD workers, §5.1 robot CAD paragraph and new calibration stand paragraph, shutdown,
  §6 OCCT, §7 data and retained assets; the file also carries the user's earlier uncommitted
  edits, which were preserved). docs/ROBOT_PROBE_SPHERE_CALIBRATION.md 336/266 (the `table` keys
  and the stand model). Final compliance pass over every changed C++ and build file: no lambda,
  project template, project exception, `throw` or output parameter (search for `[](`, `[&]`,
  `[=]`, `template <`, `throw ` in the touched files found none); every new function has at most
  two parameters; new state is limited to `DrawingDocument::models`, `m_modelQueue`,
  `m_modelWorker`, `OccPartProps::fitExcluded`/`OccPart::m_fitExcluded`, `CalibrationTable`,
  `SceneSphere::table` and `SceneSphere::standModel`, each recorded with its requirement in this
  plan; `git diff --check` reports no whitespace errors.


## Surprises & Discoveries

- Observation: earlier code imported STEP at runtime and cached BREP. It was removed in commit
  `6b0df5a` (2026-10-17); the last version is in its parent, `6b0df5a^`, in
  `src/viewport/occt/stepimporter.{h,cpp}` and `src/viewport/occt/cachedshapeloader.{h,cpp}`.
  Evidence: `git log -S "STEPControl_Reader" -- src` lists `6b0df5a` and `07fb03d`.
- Observation: the old cache never detected a changed STEP file. `CachedShapeLoader::loadStpWithCache`
  read `<cache>/<name>.brep` whenever it existed and opened the STEP source only when the BREP was
  missing or unreadable. The old deployment copied `*.stp` and `*.brep` beside the executable.
- Observation: a decorative part of a scene object is currently removed whenever that object's
  diagnostic overlay changes. In `src/viewport/adapters/occsceneadapter.cpp`,
  `createPresentation` puts every part whose selection mode is `OccSelectionMode::None` into
  `Presentation::diagnostics`, which holds both decorative parts (`DrawingStyle::decoration`) and
  the diagnostic markers/normal lines; `setObjectDiagnostics` then removes all of
  `it->diagnostics` and installs only the new markers and normal lines. The calibration sphere has
  a points overlay (its `roughPoints`), so toggling it would delete the table and the stand. Milestone 3
  fixes this.
- Observation: the stand STEP file is an assembly. Before the user's correction on 2026-10-29 it
  contained the products `stand`, `adapter` and `datum-sphere-25`; the `stand` placement was
  `CARTESIAN_POINT(..., (0, 0, -170.5))`, so the stand base lies 170.5 mm below the sphere centre,
  on the table surface. After the correction it contains only `stand` and `adapter`; one
  `SPHERICAL_SURFACE` of radius 12.5 mm remains, which is the adapter's spherical seat.


- Observation: when Milestone 1 started, the user had already deleted
  `resources/cad/kr10/MEE.brep` and `resources/cad/kr10/SEE.brep` from the working tree
  (`git status` shows them as `D`). An existing build directory still holds their deployed copies
  in `<build>/resources/cad/kr10`, so the robot preview keeps working until Milestone 2; a fresh
  build directory has no tool BREPs there and the robot preview fails to load until Milestone 2.
  Evidence: `git status --short` on 2026-10-29.

- Observation: the relink forced by `LINK_DEPENDS` after a STEP change is slow in the Debug
  build: the conversion of `EE_GDZ-65-800A.stp` took 2.7 s and the relink of the 190 MB Debug
  `robocrap.exe` about 105 s.
  Evidence: `.ninja_log` entries `65 2763 ... EE_GDZ-65-800A.brep` and
  `2763 107760 ... robocrap.exe`. STEP files change rarely, so the cost is accepted.

- Observation: the generated tool BREPs are not the old `MEE.brep`/`SEE.brep`. The old files (and
  the repository copies `EE_WP-500.brep`, `EE_GDZ-65-800A.brep`, byte-identical to them) begin their
  location table with a rotation (first rows `-1 ~0 ~0 0`, `~0 ~0 1 0`, `~0 1 ~0 0`), while the
  generated `EE_WP-500.brep` begins with the identity. The first table entry is not necessarily the
  top-level placement, so this does not prove that the tool appears elsewhere, but Milestone 2's
  visual check against the flange and TCP axes is required before relying on the new files.
  Evidence: sizes 224712 vs 224678 and 6510103 vs 6510004 bytes; `diff` of the first 300 bytes.

- Observation: decorative surfaces (the table plane and the stand) were drawn as wireframe in the
  part colour although their display mode is `AIS_Shaded`. The adapter excluded decorative parts
  from camera fitting with `AIS_Shape::SetInfiniteState(true)`, which makes `AIS_Shape` compute
  its shaded presentation as wireframe for infinite objects. Before this plan every decorative
  part was a line or marker set, which is wireframe anyway, so the effect was invisible.
  Evidence: the user's screenshot of 2026-10-29 (white table outline, grey stand edges, shaded
  sphere). `Graphic3d_Structure::SetInfiniteState` is an inline setter of the structure's own flag
  (`Graphic3d_Structure.hxx`), and `Graphic3d_CView::MinMaxValues` excludes infinite structures
  from `FitAll`.

## Decision Log

- Decision: decorative parts are excluded from camera fitting by marking their presentation
  structures infinite (`handle->Presentations()`, each `SetInfiniteState(true)`) in
  `OccScene::displayPart` after `AIS_InteractiveContext::Display`, driven by a new
  `OccPartProps::fitExcluded` flag (decorative object parts, markers and normal lines). The AIS
  object itself stays finite.
  Rationale: keeps shaded decorations shaded and keeps them out of `V3d_View::FitAll`. Doing it in
  `displayPart` covers parts created hidden and shown later, because their presentation is first
  computed there. Robot parts also use `OccSelectionMode::None`, so the selection mode cannot
  replace the flag. Alternative rejected: a project-computed fit box (more code and a second
  bounding-box path). Known limit: if OCCT recomputes a presentation without a new `Display`, its
  structure might lose the flag and the part would then take part in fitting; nothing in the
  application recomputes these parts.
  Date/Author: 2026-10-29, agent.
- Decision: finished model loads are stored in `DrawingDocument::models` instead of a separate
  `OccController::m_models` passed through new `OccViewWindow` and `OccViewport` constructor
  parameters (Milestone 3 text describes the original idea).
  Rationale: the window and the viewport already borrow the controller's `DrawingDocument` by
  const reference, and the viewport replays it on native-surface recreation; loaded models are
  part of that accepted drawing state. Only the viewport's `m_application(m_scene,
  document.models)` initializer and the adapter constructor change; the window and viewport
  signatures stay unchanged. The public header names `CadLoadResult` only by forward declaration,
  so no OCCT type becomes public.
  Date/Author: 2026-10-29, agent.
- Decision: in `OccSceneAdapter::createPresentation`, a shape is a diagnostic when its index is at
  or after the first marker/normal shape (`diagnosticsStart`), a decoration when it comes from
  `object.parts` with `OccSelectionMode::None`, otherwise a surface.
  Rationale: markers and normal lines are always appended after the object parts, so the index
  separates them without a per-shape group field.
  Date/Author: 2026-10-29, agent.
- Decision: the conversion rules (STEP glob, one custom command per file, the target
  `robocrap_cad`, `add_dependencies` and `LINK_DEPENDS`) live in `robocrap_deploy_occt` in
  `cmake/DeployOcct.cmake`, not in `src/application/CMakeLists.txt` as first planned.
  Rationale: that function already receives the target and the OCCT prefix and computes the
  compiler directory and `OCCT_RUNTIME_SEARCH_DIRS`, and is called in the directory that declares
  `robocrap`, which custom-command outputs and `add_dependencies` require. Placing the rules in
  the application file would repeat those values.
  Date/Author: 2026-10-29, agent.
- Decision: `cmake/ConvertStep.cmake` converts the DLL directory list to backslashes with
  `string(REPLACE)` instead of `file(TO_NATIVE_PATH)`.
  Rationale: the result goes into `PATH`; a plain character replacement has no list or quoting
  behavior to depend on.
  Date/Author: 2026-10-29, agent.

- Decision: STEP files are converted to BREP at build time by a repository-built converter
  executable, not at application runtime.
  Rationale: the user requires that only BREP files go into the build directory. A runtime cache
  needs the STEP file beside the executable to notice a change. A build-time conversion keeps the
  application BREP-only (no `TKDESTEP` link in the application, no cache writing at runtime), and
  the build system reruns the conversion when the STEP file is newer than its BREP. The user chose
  this option (b) on 2026-10-29.
  Date/Author: 2026-10-29, user and agent.
- Decision: change detection uses the build system's timestamp comparison (a custom command's
  output is rebuilt when a `DEPENDS` file is newer), not a content hash.
  Rationale: it needs no extra code or stored state. A STEP file replaced by an older copy with an
  older timestamp is the only case it misses; touching the file fixes it.
  Date/Author: 2026-10-29, agent.
- Decision: from the old code, keep only the STEP reading sequence of `StepImporter::importFile`
  (`STEPControl_Reader::ReadFile`, `TransferRoots`, `OneShape`, its status names and its
  null-shape checks). Drop `CachedShapeLoader`, `CadImportResult::success/failure` factories, the
  `QElapsedTimer` debug timing, the separate `exists`/`isFile` checks (ReadFile already fails on a
  missing file and the status is reported), the `TKDESTEP` link of `RoboCrap3D` and the STEP
  deployment.
  Rationale: the cache logic is replaced by the build dependency; the rest is diagnostics without
  a current requirement.
  Date/Author: 2026-10-29, agent.
- Decision: the converter does not link Qt. It writes the BREP to `<output>.tmp`; a CMake script
  renames it to `<output>` only after the converter succeeds.
  Rationale: a converter that links Qt would also need Qt's DLLs on its search path at build time.
  The rename makes a failed or interrupted conversion leave no BREP that the build system would
  treat as up to date.
  Date/Author: 2026-10-29, agent.
- Decision: generated BREP files are produced in `<build>/generated/cad/` and copied into
  `<executable directory>/resources/cad/` by the existing post-build/install deployment script
  `cmake/DeployOcct.cmake`. The executable `robocrap` lists the generated BREPs in its
  `LINK_DEPENDS` property, so a changed BREP relinks `robocrap` and reruns its post-build
  deployment.
  Rationale: a post-build step runs only when its target is relinked. Without `LINK_DEPENDS`, a
  STEP-only change would update `<build>/generated/cad/` but not the copy beside the executable.
  Copying directly from the conversion target into the executable directory would need
  `$<TARGET_FILE_DIR:robocrap>` in a target that `robocrap` depends on, which creates a dependency
  cycle, and CMake 3.16 (the project minimum) does not accept generator expressions in
  `add_custom_command(OUTPUT ...)`.
  Date/Author: 2026-10-29, agent.
- Decision: the stand is drawn through a new public drawing value `DrawingModel { QString path;
  V3d origin; }`, a part of the calibration sphere's `DrawingObject`. The viewport loads each model
  path once, on demand, on a `CadLoadWorker` separate from the robot job, and redraws the objects
  that use it when the load finishes.
  Rationale: requirement: draw a CAD shape placed at a scene position, as part of the sphere
  object so that it shares the sphere's visibility. Reuse fails because `DrawingPart` has only
  analytic geometry and polylines, and the only CAD loading is the robot job, whose failure
  disables the robot preview. Rejected alternative: loading the stand inside the robot job; a
  missing or broken stand file would then make the robot unavailable, and the robot controller
  would have to know about calibration fixtures.
  Date/Author: 2026-10-29, agent.
- Decision: the model is placed by translation only: `origin` is the position of the model's
  coordinate origin, expressed in the robot base coordinate system {B}, in mm; the model's axes
  stay parallel to the axes of {B}.
  Rationale: the stand STEP is saved in the calibration-sphere coordinate system {S}, whose origin
  is the sphere centre and whose axes are parallel to {B} (`docs/ROBOT_PROBE_SPHERE_CALIBRATION.md`).
  No current requirement needs rotation.
  Date/Author: 2026-10-29, agent.
- Decision: the table and the stand are decorative parts (`DrawingStyle::decoration = true`) of
  the calibration sphere's drawing, not separate scene objects.
  Rationale: the user asked for parts that are not selectable and not used for camera fitting, and
  that appear and disappear with the sphere (2026-10-29). Decorative parts already get
  `OccSelectionMode::None` and `SetInfiniteState(true)`, which excludes them from picking and from
  `V3d_View::FitAll`.
  Date/Author: 2026-10-29, user and agent.
- Decision: decorative parts get their own list `Presentation::decorations` in `OccSceneAdapter`,
  separate from `Presentation::diagnostics`.
  Rationale: see the corresponding Surprise; diagnostic updates must replace only markers and
  normal lines.
  Date/Author: 2026-10-29, agent.
- Decision: the table dimensions are stored in `calibration_sphere.json` as a required object
  `table`, not as constants in code.
  Rationale: the table is physical setup data measured by the user, like `strutDiameter` and
  `strutRecess`, which are already required keys of the same file. The deployment never
  overwrites the deployed sphere file, so the user adds the object to the deployed copy by hand
  (Concrete Steps).
  Date/Author: 2026-10-29, agent.
- Decision: table orientation in {B}: in the user's top view (`top_view.jpg`, 2026-10-29) the
  {B} x-axis points up and the y-axis points left. The 740 mm side runs along x, the 935 mm side
  along y; the sphere centre is 160 mm from the table edge at the smaller x value and 135 mm from
  the edge at the smaller y value; the table edges are parallel to the x- and y-axes of {B}.
  Rationale: user drawing with axes. With the rough sphere centre near x = 1053 mm,
  y = −332 mm, the table spans x 893…1633 mm and y −467…468 mm, almost symmetric about y = 0.
  Date/Author: 2026-10-29, user.
- Decision: `initializeOcct` keeps its `XSTEPResource` check and `CSF_STEPDefaults` variable,
  although the application no longer reads STEP.
  Rationale: `docs/AGENT_CONTRACTS.md` requires retaining its resource checks and environment
  variables; removing them is unrelated to this task.
  Date/Author: 2026-10-29, agent.


## Outcomes & Retrospective

2026-10-29, all five milestones complete. The Calibration submode now shows, with the sphere,
the opaque table top placed from the tape-measure dimensions and the calibration stand loaded from
`resources/cad/CALIBRATION_STAND.stp`; both are decorative and share the sphere's visibility. STEP
files in `resources/cad` are converted to BREP during the build by `tools/cadstep2brep`; only
BREP files reach the build directory, a changed STEP file is reconverted on the next build, and
the end effectors are now built from `EE_WP-500.stp` and `EE_GDZ-65-800A.stp`. The user verified
the clean, incremental and no-change builds, the tool placement, the viewport regressions and the
calibration display.

Lessons: the old runtime STEP cache never detected a changed STEP file, so only its reading
sequence was reused. Two existing viewport behaviors stayed invisible until shaded decorative
parts appeared: a diagnostic update deleted decorative parts, and an infinite AIS object is drawn
as wireframe; both were fixed. A relink of the 190 MB Debug executable (about 105 s) is the cost of
redeploying a changed BREP and was accepted.

Remaining outside this plan: `docs/ARCHITECTURE.md` §3.2 does not yet describe `SceneCalibration`
(an omission that predates this plan); the table clearance during the touch motion is checked
only visually.


## Context and Orientation

The repository root is `E:\Qt\QtProjects\RoboCrap` (or `D:\Qt\QtProjects\RoboCrap` on the user's
second computer). All paths below are relative to it. The application is a Windows Qt Quick
program (C++17, Qt ≥ 6.8, CMake ≥ 3.16, MinGW 64-bit) that drives a KUKA KR10 R1420 robot. OCCT
(Open CASCADE Technology 8.0.0, bundled in `libs/OCCT_8_0_0/install-gcc1310_64`) draws the 3D
scene. Terms used below:

- BREP: OCCT's native shape file format, read with `BRepTools::Read` and written with
  `BRepTools::Write`. STEP: the ISO 10303 CAD exchange format, read with `STEPControl_Reader` from
  the OCCT library `TKDESTEP` (present in the bundled SDK as `bin/libTKDESTEP.dll`).
- {B}: the robot base coordinate system; all scene coordinates are in {B}, in mm. {S}: the
  calibration-sphere coordinate system, origin at the sphere centre, axes parallel to {B}.
- Custom command / custom target: CMake rules (`add_custom_command`, `add_custom_target`) that run
  programs during the build. `POST_BUILD`: a custom command attached to a target that runs after
  the target is linked.
- Decorative part: a drawn shape that cannot be picked with the mouse and is excluded from the
  camera's fit-all bounds.

How CAD reaches the screen today. `src/application/robot/robotcontroller.cpp`, in
`RobotController::loadRobot`, takes `ViewportAssets::applicationAssets().cadDirectory`
(`src/application/assetpaths.cpp`, currently `<exe dir>/resources/cad/kr10`), makes the seven link
file names from `RobotDefinition::kr10()` (`src/application/robot/robotdefinition.cpp`, for
example `0_BASE.brep`) absolute, sets `visuals.tools` to `MEE.brep` and `SEE.brep` in the same
directory, and calls `OccController::loadRobot` (`src/viewport/occcontroller.cpp`). That starts one
`CadLoadWorker` (`src/viewport/occt/cadloadworker.cpp`, a `QThread` subclass whose `run` reads each
absolute `.brep` path with `BRepTools::Read`, serialized by a function-local static mutex, and
returns `CadLoadResult { QList<TopoDS_Shape> shapes; QString error; }`). `finishRobotLoad` hands
the shapes to `OccViewWindow::setScene`, and `OccRobotAdapter` installs links and tools.

Deployment. `cmake/DeployOcct.cmake` defines `robocrap_deploy_occt(target occt_prefix)`, called at
the end of `src/application/CMakeLists.txt`. It generates `occt-deploy-<config>.cmake` holding the
source paths, attaches a `POST_BUILD` command that runs the same script with
`ROBOCRAP_DEPLOY_CONFIG` and `ROBOCRAP_DEPLOY_DEST`, and registers it for `install`. The script
copies OCCT DLLs, OCCT resources, `resources/cad/kr10/*.brep` into
`<dest>/resources/cad/kr10`, and `resources/json/calibration_sphere.json` only when the
destination file does not exist. `robocrap` is written to the build root unless
`CMAKE_RUNTIME_OUTPUT_DIRECTORY` is set (`src/application/CMakeLists.txt`, lines 38–40). OCCT is
found in `src/viewport/CMakeLists.txt`, which marks every OCCT imported target `IMPORTED_GLOBAL`
and maps Debug to the Release libraries; the root `CMakeLists.txt` adds `src/viewport` before
`src/application`.

Current CAD source files. `resources/cad/kr10/` holds the seven link BREPs (the user may still have
`MEE.brep` and `SEE.brep` there). `resources/cad/` holds `CALIBRATION_STAND.stp`,
`EE_WP-500.stp`, `EE_GDZ-65-800A.stp`, and BREPs `EE_WP-500.brep`, `EE_GDZ-65-800A.brep` that this
plan treats as generated outputs and removes from the source tree.

How scene objects are drawn. `SceneObject::drawing()` (`src/application/scene/sceneobject.cpp`)
returns a `RoboCrap3D::DrawingObject` (public header `src/viewport/include/viewport/occcontroller.h`):
an id, a list of `DrawingPart`s, visibility and `DrawingDiagnostics`. A `DrawingPart` holds a
`std::variant` geometry (`DrawingPlane`, `DrawingCylinder`, `DrawingCurve`, `DrawingSphere`,
polyline, line list), a `DrawingStyle { color, lineWidth, decoration, transparency }` and a
`visible` flag. `SceneModel` passes it to `OccController::insertObject`, which stores it in its
`DrawingDocument m_document` and forwards it to `OccViewport::insertObject` and
`OccSceneAdapter::insertObject` (`src/viewport/adapters/occsceneadapter.cpp`). There
`createPresentation` builds one OCCT shape per visible part (`makeShape`) and creates OCCT parts;
parts with `decoration` get `OccSelectionMode::None` and `SetInfiniteState(true)`. When a native
window is recreated, `OccViewport`'s constructor replays every object of the borrowed document.
`OccController` owns the document; `OccViewWindow` and `OccViewport` borrow it by const reference
and are destroyed first.

The calibration sphere. `SceneCalibration` (`src/application/scene/scenecalibration.cpp`) reads
`<exe dir>/resources/json/calibration_sphere.json` in `activate()` (entering the Calibration
submode) through `readSphereFile(path, ballRadius)`, which validates `version` 1,
`sphereDiameter`, `strutDiameter`, `strutRecess` and `roughPoints`, fits the sphere and returns a
`SceneSphere` (`src/application/scene/scenegeometry.h`: `center`, `radius`, `ballCenterRadius`,
`compensatedRadius`, `rms`, `roughPoints`, `strutRadius`, `strutRecess`, and `operator==`).
`replaceSphere` inserts a new scene object only when the value changed. `SceneObject::drawing()`
draws a sphere object as two `DrawingSphere` parts. Leaving the submode hides the sphere and the
calibration path; the Scene panel can hide them too.


## Plan of Work

### Milestone 1: build-time STEP to BREP conversion

At the end of this milestone, building `robocrap` produces
`<build>/generated/cad/CALIBRATION_STAND.brep`, `EE_WP-500.brep` and `EE_GDZ-65-800A.brep` and
copies them into `<exe dir>/resources/cad/`. No `.stp` file is copied into the build. Touching one
STEP file and building again converts only that file and copies it again. The application itself
is unchanged in this milestone.

Create `tools/cadstep2brep/main.cpp`, a command-line program `cadstep2brep <input.stp>
<output.brep>`. In an unnamed namespace define `struct StepReadResult { TopoDS_Shape shape;
std::string error; };`, `const char* statusName(IFSelect_ReturnStatus status)` (the switch of
the old `StepImporter::statusToString`), `StepReadResult readStep(const char* path)` (construct
`STEPControl_Reader`, `ReadFile(path)`; if the status is not `IFSelect_RetDone` return
"cannot read <path>: <status name>"; if `TransferRoots()` returns 0 return "no transferable roots
in <path>"; take `OneShape()`; if it is null return "empty shape in <path>") and
`std::string writeBrep(const TopoDS_Shape& shape, const char* path)` (open `std::ofstream` in
binary mode, `BRepTools::Write(shape, stream)`, flush, return "cannot write <path>" if the stream
failed, otherwise an empty string). `main` checks `argc == 3` (otherwise prints the usage line to
`std::cerr` and returns 2), calls both functions inside one `try` that catches
`Standard_Failure` and `std::exception` (both originate in OCCT/std calls), prints any error to
`std::cerr` and returns 1, and on success prints `cadstep2brep: <input> -> <output>` and returns 0.
`STEPControl_Reader` applies assembly placements during `TransferRoots`, so the stand and the
adapter come out in the file's top-level coordinate system ({S}). Paths reach the program as
narrow `argv` strings in the Windows ANSI code page; the repository and build paths are ASCII,
which this plan assumes.

Create `tools/cadstep2brep/CMakeLists.txt` with `add_executable(cadstep2brep main.cpp)` and
`target_link_libraries(cadstep2brep PRIVATE TKernel TKBRep TKDESTEP)`. Add
`add_subdirectory(tools/cadstep2brep)` to the root `CMakeLists.txt` directly after
`add_subdirectory(src/viewport)`, because the OCCT imported targets are created there.

Create `cmake/ConvertStep.cmake`, run in script mode (`cmake -P`). It receives `CONVERTER`,
`SOURCE`, `OUTPUT`, `OCCT_RESOURCES` and `RUNTIME_DIRS` (`<OCCT prefix>/bin` followed by the
`OCCT_RUNTIME_SEARCH_DIRS` cache value, which defaults to the MinGW compiler directory,
`|`-separated so that no CMake list separator crosses the command line). It sets `ENV{PATH}` to
those directories with backslashes, followed by the old `PATH`, joined with `;` (the converter's
OCCT and MinGW runtime DLLs must be found when the build runs it), sets `ENV{CSF_STEPDefaults}` to `${OCCT_RESOURCES}/XSTEPResource`, removes
`${OUTPUT}.tmp`, runs `execute_process(COMMAND "${CONVERTER}" "${SOURCE}" "${OUTPUT}.tmp"
RESULT_VARIABLE result)`, and on a nonzero result removes the temporary file and stops with
`message(FATAL_ERROR "STEP conversion failed: ${SOURCE}")`; otherwise it renames
`${OUTPUT}.tmp` to `${OUTPUT}` with `file(RENAME ...)`.

In `cmake/DeployOcct.cmake`, inside `robocrap_deploy_occt` after `OCCT_RUNTIME_SEARCH_DIRS` is
defined (see the Decision Log for why there and not in `src/application/CMakeLists.txt`): collect
`file(GLOB cad_steps CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/resources/cad/*.stp")`; for each,
compute `${CMAKE_BINARY_DIR}/generated/cad/<name without extension>.brep` and add
`add_custom_command(OUTPUT <brep> COMMAND "${CMAKE_COMMAND}" -DCONVERTER=$<TARGET_FILE:cadstep2brep>
... -P "${PROJECT_SOURCE_DIR}/cmake/ConvertStep.cmake" DEPENDS <stp> cadstep2brep
"${PROJECT_SOURCE_DIR}/cmake/ConvertStep.cmake" COMMENT "Converting <name>.stp to BREP" VERBATIM)`
with a preceding `file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/cad")`. Then
`add_custom_target(robocrap_cad DEPENDS <all breps>)`, `add_dependencies(robocrap robocrap_cad)`
and `set_property(TARGET robocrap APPEND PROPERTY LINK_DEPENDS <all breps>)` (written with the
function's `${target}`). `OCCT_RESOURCES` is `<occt_prefix>/share/opencascade/resources`.

In the same file, add `set(cad_generated [==[${cad_generated}]==])` to the generated
configuration and, after the KR10 copy, `file(COPY "${cad_generated}/" DESTINATION
"${ROBOCRAP_DEPLOY_DEST}/resources/cad" FILES_MATCHING PATTERN "*.brep")` (the KR10 copy already
creates `resources/cad`). Add `IS_DIRECTORY "${cad_generated}"` to the existing directory check
and its `FATAL_ERROR` text. Update the `COMMENT` of the post-build command to say "CAD assets".

Proof: the user configures and builds `robocrap`, then lists the directories named in Validation.

### Milestone 2: end effectors from the generated BREPs

At the end of this milestone the robot preview loads the measuring tool from
`<exe dir>/resources/cad/EE_WP-500.brep` and the spindle tool from
`<exe dir>/resources/cad/EE_GDZ-65-800A.brep`.

In `src/application/assetpaths.cpp`, `ViewportAssets::applicationAssets()` returns
`<exe dir>/resources/cad` as `cadDirectory` (the field name stays; it now names the CAD root
directory, of which `kr10` is a subdirectory). In `RobotController::loadRobot`, build the link paths from
`QDir(cadDirectory.filePath(QStringLiteral("kr10")))` and set `visuals.tools` to
`cadDirectory.filePath(QStringLiteral("EE_WP-500.brep"))` and
`cadDirectory.filePath(QStringLiteral("EE_GDZ-65-800A.brep"))`, in `RobotTool` order (Measuring,
Spindle); update the comment above it. Delete `resources/cad/EE_WP-500.brep` and
`resources/cad/EE_GDZ-65-800A.brep` (generated now) and, if present,
`resources/cad/kr10/MEE.brep` and `resources/cad/kr10/SEE.brep` (replaced by the STEP sources; git
history keeps them). An existing build directory still holds stale copies of `MEE.brep`/`SEE.brep`
in `resources/cad/kr10`; they are unused.

Proof: the user runs the application. The probe appears on the flange in Measuring mode and the
spindle in Machining mode, in the same place as before. If a tool appears displaced or rotated, the
STEP file is not saved in the flange coordinate system; stop and report it to the user, because
the application does not transform tool models.

### Milestone 3: decorative parts and CAD model parts in the viewport

At the end of this milestone the viewport can draw a `DrawingModel` part and keeps decorative parts
through diagnostic updates. Nothing in the application uses `DrawingModel` yet; the draft axis and
the existing objects behave as before.

In `src/viewport/include/viewport/occcontroller.h` add, after `DrawingSphere`:

    // A CAD model file placed by translation; its axes stay parallel to {B}.
    struct DrawingModel {
      QString path; // Absolute BREP path.
      V3d origin = V3d::Zero(); // Model coordinate origin in {B}, mm.
    };

and add `DrawingModel` to the `DrawingPart::geometry` variant after `DrawingSphere`. Add to
`OccController` the private members `QHash<QString, std::shared_ptr<const CadLoadResult>> m_models;`
(a finished load per path, including failures, whose `error` is set and `shapes` empty),
`QStringList m_modelQueue;` (paths waiting or being loaded; the first is the running one) and
`std::unique_ptr<CadLoadWorker> m_modelWorker;`, and the private methods
`void requestModels(const DrawingObject& object);`, `void startModelLoad();` and
`void finishModelLoad();`.

In `src/viewport/occcontroller.cpp`: `insertObject` calls `requestModels(object)` after an
accepted insertion. `requestModels` appends each `DrawingModel::path` of the object's parts that is
neither in `m_models` nor in `m_modelQueue`, then calls `startModelLoad`. `startModelLoad` returns
when shutting down, when `m_modelWorker` exists or when the queue is empty; otherwise it creates
`CadLoadWorker(QStringList{m_modelQueue.first()})`, connects `QThread::finished` to
`finishModelLoad` with `Qt::QueuedConnection` (a member-function pointer, no lambda) and starts it.
`finishModelLoad` returns when shutting down; otherwise it waits for the worker, stores
`std::make_shared<const CadLoadResult>(m_modelWorker->takeResult())` under `m_modelQueue.takeFirst()`,
resets the worker, emits `message(error, true)` for a failed load (once; a failed path is never
retried, because it stays in `m_models`), and for a successful one reinserts through `viewport()`
every document object that has a `DrawingModel` part with that path (helper in the unnamed
namespace: `bool usesModel(const DrawingObject& object, const QString& path)`), then re-applies
`m_document.selected` and calls `startModelLoad` for the next path. The destructor interrupts and
waits for `m_modelWorker` as it does for `m_worker`. `CadLoadWorker`'s static mutex already
serializes the model job with the robot job.

Pass the model map by const reference down the existing borrowing chain: `OccViewWindow(const
DrawingDocument&, const DrawingPose&, const QHash<QString, std::shared_ptr<const CadLoadResult>>&)`
(stored as `m_models` beside `m_document`), `OccViewport(Aspect_Handle, const DrawingDocument&,
const QHash<...>&)` and `OccSceneAdapter(OccScene&, const QHash<...>&)`. `OccController::createWindow`
passes `m_models`. The controller owns the map and outlives the window, as it already does for the
document.

In `OccSceneAdapter`: add `std::vector<OccScene::PartId> decorations;` to `Presentation` and a
member `std::vector<OccScene::PartId> parts() const` that returns surfaces, decorations and
diagnostics concatenated. Change `removeParts` to take one vector. In `createPresentation`, a
`DrawingModel` part whose path has no loaded shape (absent or failed) is skipped, so the object is
accepted and the model appears when `finishModelLoad` reinserts it; a loaded one uses
`shapes.first().Moved(TopLoc_Location(trsf))` with `trsf.SetTranslation(gp_Vec(origin))` (a moved
copy shares the geometry). Every shape from `object.parts` with `decoration` goes to `decorations`,
the markers and normal lines to `diagnostics`, the rest to `surfaces`; record the target list with
each shape instead of deducing it from `selectionMode`. `insertObject`, `removeObject` and
`setObjectVisible` use `parts()`; `setObjectDiagnostics` replaces only `diagnostics`;
`setDraftAxis` keeps `replacement.decorations` in `m_axisParts`. `setSelectedObjects` and
`objectIdFor` keep using `surfaces`.

Proof: build only; Milestone 4 exercises the new paths. The user checks that the draft axis in
Machining mode and the points/normals toggles of an imported plane still work.

### Milestone 4: table and stand with the calibration sphere

In `src/application/scene/scenegeometry.h`, add before `SceneSphere`:

    // Table top under the calibration sphere, measured from the sphere centre (mm).
    struct CalibrationTable
    {
      double lengthX = 0.0; // Table side along the x-axis of {B}.
      double widthY = 0.0; // Table side along the y-axis of {B}.
      double sphereFromMinusX = 0.0; // Sphere centre to the table edge at the smaller x.
      double sphereFromMinusY = 0.0; // Sphere centre to the table edge at the smaller y.
      double sphereAboveTop = 0.0; // Sphere centre above the table top.
      bool operator==(const CalibrationTable& other) const;
    };

(define `operator==` inline comparing all five values) and add to `SceneSphere` the fields
`CalibrationTable table;` and `QString standModel; // Absolute BREP path of the stand, origin at
center.`, both compared in `SceneSphere::operator==`.

In `scenecalibration.cpp`, add `std::optional<CalibrationTable> calibrationTable(const QJsonObject&
root)`, which reads the object `table` with the existing `positiveNumber` for the keys `lengthX`,
`widthY`, `sphereFromMinusX`, `sphereFromMinusY`, `sphereAboveTop`, and returns `std::nullopt`
unless all five are finite and positive and each `sphereFrom…` value is smaller than the side it
lies on. `readSphereFile` calls it after the strut checks and fails with "table needs finite
positive lengthX, widthY, sphereFromMinusX, sphereFromMinusY and sphereAboveTop, with the sphere
over the table."; it stores the table in the returned `SceneSphere`. In `activate`, set
`standModel` of the read sphere to `QDir(ViewportAssets::applicationAssets().cadDirectory)
.filePath(QStringLiteral("CALIBRATION_STAND.brep"))` before `replaceSphere`. Update the file-format
comment above `readSphereFile`.

In `SceneObject::drawing()`, in the sphere branch, append two decorative parts after the two
spheres. The table: `DrawingPlane` with `origin = center + (lengthX/2 − sphereFromMinusX,
widthY/2 − sphereFromMinusY, −sphereAboveTop)`, `normal` = the z unit vector, `axisU` = the x unit
vector, `width = lengthX`, `height = widthY` (`makeShape` spans `width` along `axisU`), style
`{{190, 190, 190}, 0.0, true}` (opaque, user request 2026-10-29). The stand: `DrawingModel{standModel, center}`, style
`{{140, 140, 140}, 0.0, true}`. With the user's values the table centre is
`center + (210, 332.5, −170.5)` mm.

Add to `resources/json/calibration_sphere.json`, after `strutRecess`:

    "table": {"lengthX": 740.0, "widthY": 935.0, "sphereFromMinusX": 160.0,
              "sphereFromMinusY": 135.0, "sphereAboveTop": 170.5},

and ask the user to add the same object to the deployed copy
`<exe dir>/resources/json/calibration_sphere.json`, which the deployment never overwrites.

Proof: see Validation.

### Milestone 5: documentation and compliance

Update only the affected constraints. In `docs/AGENT_CONTRACTS.md`: "Build and modules" gains the
`tools/cadstep2brep` target (links `TKDESTEP`; the only STEP reader) and the `robocrap_cad`
conversion/`LINK_DEPENDS` rule; "Viewport and CAD" replaces "CAD is BREP only … no STEP import,
BREP cache writing or TKDESTEP link" with: the application reads BREP only; STEP sources in
`resources/cad` are converted at build time into `<build>/generated/cad` and deployed to
`resources/cad`; robot links are hand-maintained BREPs in `resources/cad/kr10`; the robot job loads
the seven links followed by `EE_WP-500.brep` and `EE_GDZ-65-800A.brep`; `DrawingModel` paths load
once on a separate worker, failures are reported once and not retried, and failure never affects
robot readiness. "Geometry and scene" extends the decorative rule to the table and model parts and
states that diagnostic updates keep decorative parts. In `docs/ARCHITECTURE.md` update §3.5
(`CadLoadWorker`, `OccController`), §3.6 Resources, §5.1 (BREP CAD paragraph), §6 (OCCT), §7
"Data and units" (the BREP sources sentence) and the hardware table row for the calibration sphere
(stand on a table, 170.5 mm). In `docs/ROBOT_PROBE_SPHERE_CALIBRATION.md`, mention the `table` keys
next to the other `calibration_sphere.json` contents. Then perform the `AGENTS.md` compliance pass
over all changed declarations, bodies and callers and record findings and line counts here.


## Concrete Steps

Before each milestone, record line counts from the repository root (Git Bash):

    cd /e/Qt/QtProjects/RoboCrap
    for f in <touched files>; do printf '%s %s %s\n' "$f" "$(wc -l < "$f")" "$(grep -cv '^\s*$' "$f")"; done

After each milestone, inspect `git status --short`, `git diff --check` and the contents of new
untracked files (`tools/cadstep2brep/*`, `cmake/ConvertStep.cmake`, this plan).

Milestone 1, the user runs in Qt Creator (or a shell with the kit) a CMake configure and a build of
`robocrap` in the existing build directory, then reports the build output lines containing
"Converting" and the listing of:

    <build>/generated/cad/
    <build>/resources/cad/

Expected: three `Converting <name>.stp to BREP` lines on the first build; both directories contain
`CALIBRATION_STAND.brep`, `EE_GDZ-65-800A.brep`, `EE_WP-500.brep`; `<build>/resources/cad/` and its
subdirectories contain no `.stp`. Then the user changes the modification time of one STEP file
(for example by saving it again) and builds: exactly one "Converting" line appears and `robocrap`
relinks. A build without changes prints no "Converting" line.

Milestones 2 and 4: the user builds `robocrap` and runs it (disconnected from hardware; no robot
motion is involved).


## Validation and Acceptance

Milestone 1 is accepted on the directory listings and build lines above. If the converter fails to
start with a missing DLL (exit code 0xC0000135), the missing library's directory must be added to
`OCCT_RUNTIME_SEARCH_DIRS`; record it in Surprises.

Milestone 2: after start, the robot appears at HOME with the probe on the flange (Measuring mode);
switching to Machining shows the spindle. The probe tip lies at the TCP axes as before the change.

Milestone 4, in a running application: enter Measuring → Calibration. The scene shows the sphere,
the stand under it and a grey table top. The stand's base lies on the table plane and the sphere
sits in the adapter. The table's long side (935 mm) runs left–right as seen from the robot, and the
sphere is near the table's corner closest to the robot on the robot's right (smaller y). Clicking
the table or the stand does not change the selection. Hiding the sphere in the Scene panel hides
the table and the stand; showing it shows them again. Toggling the sphere's points overlay keeps
the table and the stand. Leaving Calibration hides all three. Generate Path and Dry Run show the
probe's movement relative to the table and the stand. Removing the `table` object from the deployed
sphere file and re-entering Calibration shows the table error message and no sphere. Renaming
`<exe dir>/resources/cad/CALIBRATION_STAND.brep` and restarting shows the sphere and table, a
console error naming the missing file, and a working robot preview.

Milestone 3 has no separate visible behavior beyond the regression checks named in its proof.


## Idempotence and Recovery

All source edits are additive or local replacements and can be reapplied. The conversion is
idempotent: an up-to-date BREP is not regenerated. A failed conversion leaves no BREP (only a
removed `.tmp`), so the next build retries. Deleting `<build>/generated/cad/` forces a full
reconversion. The deleted repository BREPs remain in git history (`git show HEAD:<path>`). The
deployed sphere file is never overwritten; keep a copy before editing it by hand.


## Artifacts and Notes

Old STEP reading sequence kept for the converter (from `6b0df5a^:src/viewport/occt/stepimporter.cpp`):

    STEPControl_Reader reader;
    const IFSelect_ReturnStatus readStatus = reader.ReadFile(path);
    if (readStatus != IFSelect_RetDone) -> error with status name
    if (reader.TransferRoots() <= 0) -> error "no transferable roots"
    TopoDS_Shape shape = reader.OneShape(); if (shape.IsNull()) -> error

Table placement with the user's values, in {B}, for a sphere centre (x, y, z):
x from x − 160 to x + 580, y from y − 135 to y + 800, top at z − 170.5; table centre
(x + 210, y + 332.5, z − 170.5).


## Interfaces and Dependencies

New executable `cadstep2brep` (`tools/cadstep2brep`), linking `TKernel`, `TKBRep`, `TKDESTEP`;
usage `cadstep2brep <input.stp> <output.brep>`, exit 0 on success, 1 on conversion failure, 2 on
wrong arguments. New script `cmake/ConvertStep.cmake`. New custom target `robocrap_cad`.

`src/viewport/include/viewport/occcontroller.h`: `struct DrawingModel { QString path; V3d origin; }`
in the `DrawingPart::geometry` variant; `DrawingDocument::models`
(`QHash<QString, std::shared_ptr<const CadLoadResult>>`). The `OccSceneAdapter` constructor gains
the borrowed model map (two parameters); `OccViewWindow` and `OccViewport` constructors are
unchanged. No QML API changes.

`src/application/scene/scenegeometry.h`: `struct CalibrationTable`; `SceneSphere::table`,
`SceneSphere::standModel`. `calibration_sphere.json` version 1 gains the required `table` object.


Revision note (2026-10-29): first version, written after the user chose build-time conversion
(option b), supplied `top_view.jpg` with the {B} axes, chose decorative presentation, and removed
the sphere from `CALIBRATION_STAND.stp`.
