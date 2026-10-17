# Reduce RoboCrap source size without losing current animation features

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is maintained in accordance with `PLANS.md` at the repository root. The repository's `AGENTS.md` rules take precedence over `PLANS.md` where they differ; the differences that matter here are listed in "Context and Orientation".

## Purpose / Big Picture

RoboCrap is a Windows Qt Quick desktop application that prepares and previews robotic hole-edge chamfering with a KUKA KR10 R1420 robot. A user imports a measured plane and a measured cylinder from JSON point files, intersects them to obtain an elliptical edge, configures the spindle tool centre point (TCP), presses Generate to compute a timed robot motion, and plays it back with Dry Run in a 3D view.

The application has grown to roughly 14,600 lines of active C++ and 3,300 lines of application QML for this modest workflow. Much of that code serves features the product owner has now ruled out (replaceable end-effector CAD files, a runtime robot description file, circle import, an unused manual robot pose API) or development-only measurement (a command-line profiler and detailed timing JSON), or it is plumbing that only forwards calls (QML registration wrappers and an extra 3D-view forwarding layer).

After this plan is implemented, the user sees the same workflow: import plane and cylinder, intersect, set TCP, Generate, Dry Run, pause/resume, Stop to HOME, inspect object properties, operate the PLC and force/torque sensor panels. The robot and both end-effector models still appear in the 3D view. What disappears is code, not capability the user relies on: about 2,000 physical lines of C++/QML and several whole files, plus the STEP import dependency of the 3D library. The user can verify the result by running the same workflow before and after, and by comparing the before/after line counts recorded in this plan.

## Progress

- [x] (2026-10-17 03:05 RTZST) Read `AGENTS.md`, `PLANS.md`, `docs/AGENT_CONTRACTS.md`, `docs/ARCHITECTURE.md`; inspect every file named in the milestones; record baseline counts.
- [x] (2026-10-17) Record the product owner's decisions and the selected reduction set in the Decision Log.
- [x] (2026-10-17) Milestone 1 — Remove four uncalled geometry helpers. Active C++ 14565/12333 → 14507/12287 physical/nonblank (−58/−46). Source checks only; no build was requested or run.
- [x] (2026-10-17) Milestone 2 — Remove circle import. Active C++ 104/14507/12287 → 102/14141/11954 files/physical/nonblank; QML 3281/2779 → 3216/2722; build inputs 494/465 → 491/462; `circ.svg` deleted. Source checks only; no build was requested or run.
- [x] (2026-10-17) Milestone 3 — Remove the chamfer profiling command and Generate timing diagnostics. Active C++ 102/14141/11954 → 100/13689/11520 files/physical/nonblank; build inputs 491/462 → 489/460; QML unchanged. Source checks only; no build or Generate run was requested or performed.
- [x] (2026-10-17) Milestone 4 — Load the fixed end-effector models together with the robot from BREP files. Active C++ 100/13689/11520 → 96/13223/11122 files/physical/nonblank; QML 3216/2722 → 3136/2643; build inputs 489/460 → 483/454; `open.svg` deleted. Source checks only; no build or runtime check was requested or performed.
- [x] (2026-10-17) Milestone 5 — Remove the manual robot pose API and RobotController's QML exposure. Active C++ 96/13223/11122 → 96/12984/10905 files/physical/nonblank; QML and build inputs unchanged. Source checks only; no build or runtime check was requested or performed.
- [x] (2026-10-17) Milestone 6 — Compile the fixed KR10 description into the application. Active C++ 96/12984/10905 → 96/12704/10699 files/physical/nonblank; build inputs 483/454 → 482/453; `resources/json/kr10.json` (90 lines) deleted. Source checks only; no build or runtime check was requested or performed.
- [x] (2026-10-17) Milestone 7 — Store object geometry as values and build Properties rows in C++. Active C++ 96/12704/10699 → 96/12537/10554 files/physical/nonblank; QML 3136/2643 → 3041/2561; build inputs unchanged. Source checks only; no build or runtime check was requested or performed.
- [x] (2026-10-17) Milestone 8 — Consolidate QML registration (Option A: one shared singleton check; registrations unchanged). Active C++ 96/12537/10554 → 96/12489/10516 files/physical/nonblank. Source checks only; no build or `robocrap_qmllint` run was requested or performed.
- [x] (2026-10-17) Milestone 9 — Remove the 3D window's forwarding methods. Active C++ 96/12489/10516 → 96/12455/10488 files/physical/nonblank. Source checks only; no build or runtime check (including native-surface recreation) was requested or performed.
- [x] (2026-10-17) Revise the selection: B3 (PLC commands) is no longer executed; the former PLC milestone is removed and the documentation milestone becomes Milestone 10.
- [x] (2026-10-17) Milestone 10 — Update contracts/architecture documents and complete the final audit. `docs/AGENT_CONTRACTS.md` and `docs/ARCHITECTURE.md` describe the reduced code; all milestone searches are clean; frozen RSI and PLC paths are unchanged. Source/document checks only.
- [ ] Not done, pending a user request: CMake build with `USE_HOT_RELOAD=OFF`, `robocrap_qmllint`, and the runtime scenario in "Validation and Acceptance". Nothing has been staged or committed.

## Surprises & Discoveries

- Observation: the STEP-to-BREP fallback can no longer succeed. The STEP sources were deleted from `resources/cad/kr10`; only nine `.brep` files remain (`0_BASE.brep` … `6_WRIST3.brep`, `MEE.brep`, `SEE.brep`).
  Evidence: `ls resources/cad/kr10` lists only `.brep` files, while `src/viewport/occt/cachedshapeloader.cpp:61-62` falls back to `StepImporter::importFile` on the absent `.stp` path.
- Observation: frozen RSI code depends on two trajectory items touched by Milestone 3. `src/pathgeneration/chamfer/chamfercartesiansampler.cpp:3` includes `src/trajectories/chamfer/chamferpreparationdiagnostics.h` and uses `ChamferProfiledKinematics` (line 394); `src/pathgeneration/chamfer/chamferjob.cpp:61` calls `ChamferMotion::evaluate`. Both must remain.
- Observation: `src/scene/scenemachiningpreparation.cpp:43` (frozen RSI) calls `qobject_cast<SceneMachiningPath*>(object->geometry())`. Milestone 7 must keep `SceneObject::geometry()` returning the `SceneMachiningPath` QObject for machining paths.

## Decision Log

- Decision: implement group A (A1 fixed tools, A2 geometry values/Properties rows, A3 compiled KR10 description, A4 registration, A5 viewport forwarding, A6 dead helpers) plus B1 (profiling/diagnostics), B2 (circle import) and B4 (manual robot pose API). B3 (removing the unused PLC commands `READ_REG`, `WRITE_REG`, `WRITE_RAW`) is not executed: the PLC codec and `PlcDevice` stay exactly as they are. B5 (placeholder Dry Scan toggle, never-created `Cone`/`ScanPath` kinds) is excluded and stays as it is.
  Rationale: explicit user selection on 2026-10-17, revised the same day to drop B3.
  Date/Author: 2026-10-17, user; recorded by Claude.
- Decision: end-effector CAD models are fixed; there is no Browse/Retry and no per-tool CAD state. Both tool BREPs load in the same job as the seven robot links.
  Rationale: user answer "fixed" on 2026-10-17. The STEP sources needed for the existing fallback no longer exist, so the fallback is already dead.
  Date/Author: 2026-10-17, user/Claude.
- Decision: a missing or unreadable tool BREP now fails the whole robot load with the same file-specific error used for robot links, instead of leaving the robot usable without that tool.
  Rationale: tools are now part of the fixed robot configuration. A single required installation removes the separate tool loading state, generation counters and status UI. The failure case requires a broken deployment; ordinary behavior is unchanged.
  Date/Author: 2026-10-17, Claude.
- Decision: remove the read-only CAD path row, the loading/error label and the Retry button from `EndEffectorPanel.qml`; keep all pose fields, Apply, Close and TCP/ER axis toggles.
  Rationale: the file path is no longer user-selectable, and its status is reported by the robot load error in the console and `OccController.errorString`. Removing transient content cannot cause layout jumps.
  Date/Author: 2026-10-17, Claude.
- Decision: do not change any PLC code (`src/devices/plc/*`, `src/devices/include/devices/plc/*`). All seven PLC commands and `PlcDevice::writeMessage` remain.
  Rationale: the user decided on 2026-10-17 to leave `READ_REG`, `WRITE_REG` and `WRITE_RAW` in place. An earlier version of this plan had a milestone removing them (about 70 lines); it was deleted.
  Date/Author: 2026-10-17, user.
- Decision: after Milestone 3, Generate writes one console line instead of the timing JSON: on success `Chamfer preparation: <points> points, <seconds> s movement`; on failure `Chamfer preparation failed: <error>`. The user-visible error text in the Machining panel is unchanged.
  Rationale: the JSON exists for development profiling, which the user removed. One line keeps an operator-visible trace.
  Date/Author: 2026-10-17, Claude.
- Decision: keep the arc-length logical work counter and its 800000 limit (`src/trajectories/chamfer/chamferarclength.cpp:14,151-158`). Remove only the separately reported actual-call counter and the result fields that exported both counts.
  Rationale: the logical limit decides whether Generate is rejected; it is behavior, not diagnostics.
  Date/Author: 2026-10-17, Claude.
- Decision: keep `SceneObject::geometry()` and its QML `geometry` property, returning the `SceneMachiningPath` QObject for machining paths and null for every other kind.
  Rationale: `Main.qml:513` and frozen RSI `src/scene/scenemachiningpreparation.cpp:43` read it; changing it would touch RSI.
  Date/Author: 2026-10-17, Claude.
- Decision: all QML singleton factories share one engine/thread check that calls `qFatal`. `Scene` and `Hub` currently use `Q_ASSERT` only, so they become stricter in release builds.
  Rationale: one check replaces six copies. The stricter check only fires on misuse (a second QML engine or a wrong thread), which the application never does.
  Date/Author: 2026-10-17, Claude.
- Decision: in `OccRobotAdapter`, store link and tool part IDs in one list, `m_partIds` (links in model order, then measuring and spindle tool), instead of a separate `std::array<std::optional<PartId>, 2>` as first planned. One private `showActiveTool()` serves both installation and tool switching.
  Rationale: a single list lets the existing reverse cleanup loop in `clear()` remove tools and links together, retaining IDs whose removal fails, without a second copy of that loop. Tool positions follow from the list size and the fixed tool count of two.
  Date/Author: 2026-10-17, Claude.
- Decision: also remove `OccScene::addShapePartWithId` and `resources/pics/open.svg`.
  Rationale: after Milestone 4, tools are created with `createShapePart` plus `setPartVisible`, like links, so `addShapePartWithId` has no caller. The Browse button was the only built user of `open.svg`; the remaining reference is in `old_imp/`, which is outside the build.
  Date/Author: 2026-10-17, Claude.
- Decision: implement Milestone 8 as Option A. Keep all 14 foreign registrations in `backendqmltypes.h` exactly as they are, and replace the six copies of the singleton factory check (five wrappers plus `WorkspaceController::create`) with one inline `claimQmlSingleton(QObject*, QJSEngine*)` in that header. Do not add `qmlsingleton.h/.cpp`.
  Rationale: Qt documentation checked on 2026-10-17. For Qt 6.8, a `QML_FOREIGN` wrapper with a static `create()` returning a stored pointer plus `QJSEngine::setObjectOwnership(..., CppOwnership)` is the documented replacement for `qmlRegisterSingletonInstance`, and it does not support multiple engines. `QQmlEngine::setExternalSingletonInstance` exists only since Qt 6.12, above the project minimum. Registering one class under two QML names (`SceneModel` and `Scene`) is undocumented, so moving registrations into class headers (Option B, about −95 lines) was judged not worth the risk against about −45 lines. Putting the helper in the existing header avoids two new files and a CMake edit. The user chose Option A.
  Date/Author: 2026-10-17, user/Claude.
- Decision: `AGENTS.md` overrides `PLANS.md` on commits and builds. Do not stage, commit, branch or stash unless the user explicitly asks. Do not run CMake configure/build or QML lint unless the user explicitly asks. Do not add tests or test targets.
  Rationale: `AGENTS.md` "Scope and workflow" and "Validation and handoff" sections.
  Date/Author: 2026-10-17, Claude.

## Outcomes & Retrospective

At each milestone record the before/after counts, the compliance findings and what remains.

Milestone 1 (2026-10-17): `readJsonPoints`, `prjPointOnLine`, `polyfit2d` and `deriv2d` were removed from Geometry; 58 physical lines are gone and behavior is unchanged because nothing called them. Remaining: Milestones 2–10.

Milestone 2 (2026-10-17): circle import is gone end to end. That covers the `Circle` fitting class, importer path, `SceneModel::addCircle`, the circle view class and its QML registration, the drawing/diagnostic branches, the toolbar action and dialog, Properties rows, the Scene panel icon and `circ.svg`. It removes 434 physical lines in total (C++ −366, QML −65, build −3) plus the icon. Plane/cylinder import, intersection and drawing are untouched. An intersection edge that is a circle still draws as a circle, because that path uses the ellipse, not `Circle`. Remaining: Milestones 3–10.

Milestone 3 (2026-10-17): the `--profile-chamfer` command and all Generate timing, call-count and cache-statistics JSON are gone (−454 physical lines). The candidate order, every acceptance check, every error string, the arc-length 800000 logical work limit and the target cache are unchanged. Generate now prints one console line, as in the Decision Log. The equivalence of generated motions is argued from the diff (only measurement code and result fields were removed); no executable comparison was run. Remaining: Milestones 4–10.

Milestone 4 (2026-10-17): the seven links and both tools now load from `.brep` files in one background job. STEP import, BREP cache writing, per-tool loading state, Browse/Retry and the tool-CAD status UI are gone, and `RoboCrap3D` no longer links `TKDESTEP`. That removes 552 physical lines of C++/QML/build input, plus `open.svg`. Switching Measuring/Machining still shows only the matching tool; TCP/ER axes and pose editing are unchanged. Deliberate failure-case change (Decision Log): a missing tool BREP now fails the robot load. Remaining: Milestones 5–10.

Milestone 5 (2026-10-17): `RobotController` is no longer a QML singleton and has no manual FK/IK commands or pose properties; none had a QML caller. Its public API is `loadRobot()` and `applyPlayback(joints, flangeToTcp)`, both returning `ViewportOperationResult`. `WorkspaceController` now switches the active tool on `SceneEndEffectors` directly, removing its dependency on `RobotController`. Dry Run, Stop-to-HOME, tool switching and the end-effector panel behave as before. Each Dry Run frame now also skips computing the Euler flange pose and IK configuration that only manual IK read. 239 physical C++ lines were removed. Remaining: Milestones 6–10.

Milestone 6 (2026-10-17): the KR10 geometry, limits, rated speeds, IK parameters, HOME and link files come from `RobotDefinition::kr10()` instead of parsing `:/json/kr10.json`. The values were copied verbatim from the file before it was deleted, and every joint transform is built with the same expression as before. The startup HOME/FK consistency check and the Generate-time model validation still run. 280 C++ lines, one qrc line and the 90-line JSON file were removed. The user was told the trade-off before implementation: robot parameter changes now mean editing the C++ table (the JSON was already compiled in through qrc), and if a calibrated kinematic model is introduced later, a data file may be worth restoring. Remaining: Milestones 7–10.

Milestone 7 (2026-10-17): fitted planes, cylinders and intersection edges are stored as values in `SceneObject` (`std::variant`) instead of child QObject wrappers, and the Properties panel renders a C++-built list of `{label, value}` rows with one `Repeater`. The plane and cylinder rows have the same labels and order, and the formatting rules are the same. The dead `addPlane(coefficients)`/`addObject` API and the plane coefficients-only mode are gone. Machining paths keep their QObject (`geometry`), so `Main.qml` and frozen RSI are unaffected. 262 physical lines were removed (C++ −167, QML −95). Remaining: Milestones 8–10.

Milestone 8 (2026-10-17, Option A): the six singleton factories (`Machining`, `MachiningPreparation`, `EndEffectors`, `Scene`, `Hub`, `WorkspaceController`) now share one check, `claimQmlSingleton`. Every registration name, kind and message is unchanged. `Scene` and `Hub` previously checked only with `Q_ASSERT` (debug builds); like the others, they now abort with `qFatal` on a wrong thread or a second live engine. 48 C++ lines were removed. `OccControllerQml` in the viewport module keeps its own copy, because that module cannot include Application headers. Remaining: Milestones 9–10.

Milestone 9 (2026-10-17): `OccController` calls `OccViewport` directly for insert, remove, visibility, diagnostics, selection, draft axis and spindle frames. `OccViewWindow` now handles only the native surface, input events, robot installation, pose restoration and the active tool. The window no longer keeps its own copy of the pose; it borrows the controller's accepted `DrawingPose`. Net −34 physical C++ lines, below the −45 to −55 estimated, because each direct viewer call in the controller keeps its own "no surface yet" check. The larger variant that would also remove the window's copies of visuals, shapes and active tool was not done. Remaining: Milestone 10.

Final outcome (2026-10-17, after Milestone 10). Measured against the baseline taken before Milestone 1:

    Active C++ (src excluding frozen RSI)  104 files 14565/12333 → 96 files 12455/10488   (−2110 physical, −14.5 %)
    Application QML                         20 files  3281/2779  → 20 files  3041/2561   (−240)
    Build inputs (CMake, cmake/, qrc)       11 files   494/465   → 11 files   482/453    (−12)
    Frozen RSI C++                          23 files  2904/2493  → unchanged
    Deleted resources: kr10.json (90 lines), circ.svg, open.svg; dependency removed: OCCT TKDESTEP

C++, QML and build inputs together went from 18340 to 15978 physical lines (−2362, −12.9 %), close to the initial ≈ −2210 estimate for this scope. Per-milestone physical deltas:

    M1 −58   M2 −434   M3 −454   M4 −552   M5 −239   M6 −281 (+ −90 JSON)   M7 −262   M8 −48   M9 −34

What the user still gets: the same Rough Measuring workflow (plane/cylinder import, intersection), tool calibration, Generate, Dry Run with pause/resume/Stop-to-HOME, the Properties panel and device panels.

What was intentionally removed by user decision: circle import, Browse/Retry for tool CAD, STEP import and BREP cache writing, the manual FK/IK robot API, the `--profile-chamfer` profiler and Generate's timing JSON, and the runtime robot description file.

Behavior changes limited to failure or misuse cases (each in the Decision Log or milestone evidence):
- A missing tool BREP fails the robot load.
- The `Scene` and `Hub` singletons now `qFatal` on misuse in release builds.
- Generate logs one console line instead of the timing JSON.

Lessons: estimates made before reading every caller were optimistic for forwarding-layer removals (M9: −34 against −45 to −55) and pessimistic for dead-feature removals (M4: −552 against ≈ −430). Checking the target Qt version before choosing an API (M8) avoided an unusable Qt 6.12 dependency.

Remaining, outside this plan:
- build/lint/runtime validation on request;
- the optional larger M8 (in-class registration) and M9 (shared presentation state) variants;
- the reduction proposals the user excluded (B3 PLC commands, B5 placeholder UI);
- the application-layer naming question raised before this plan.

## Context and Orientation

The repository root is `D:\Qt\QtProjects\RoboCrap`. All paths below are relative to it. It is a CMake project (C++17, Qt 6.8 or newer, CMake 3.16 or newer). The root `CMakeLists.txt` adds six source targets:

- `src/geometry` builds `RoboCrapGeometry`: Eigen-based numerical types (vectors `V3d`, matrices `M4d`), fitting of measured planes (`BoundedPlane`) and cylinders (`BoundedCylinder`), and their intersection (`intersectSurfaces`, returning an `EdgeGeometry` that holds an ellipse).
- `src/kinematics` builds `RoboCrapKinematics`: the KR10 model value `Kr10KinematicModel` and the forward/inverse kinematics solver `Kr10Kinematics`. FK (forward kinematics) computes link frames from six joint angles; IK (inverse kinematics) computes joint angles for a requested flange frame.
- `src/trajectories` builds `RoboCrapTrajectories`: the analytic chamfer curve `ChamferPath`, arc-length maps `ChamferArcLength`, central timing `ChamferCentralMotion`, point scheduling/validation `ChamferCentralPreparation`, and the complete timed robot motion `ChamferMotion` produced by `ChamferMotion::create`.
- `src/devices` builds `RoboCrapDevices`: socket workers for the PLC (`PlcDevice` with codec `PlcMessageManager`), force/torque sensor (`FtsDevice`) and frozen RSI.
- `src/viewport` builds `RoboCrap3D`, the 3D view on OCCT (Open CASCADE Technology, the CAD kernel used to read and draw shapes). `OccController` is the QML-visible owner; `OccViewWindow` is the native window; `OccViewport` owns the OCCT viewer, the scene and two adapters; `OccRobotAdapter` places robot/tool shapes; `OccSceneAdapter` draws document objects; `CadLoadWorker` reads CAD files on a background thread.
- `src/application` builds the executable `robocrap`, with `main.cpp`, the document (`scene/SceneModel`, `scene/SceneObject`, `scene/scenegeometry.h`), import (`scene/SceneSurfaceImporter`), tool calibration (`scene/SceneEndEffectors`), Generate/Dry Run (`scene/SceneMachining`), robot state (`robot/RobotController`, `robot/RobotDefinition`), mode/selection (`WorkspaceController`), device GUI adapters (`devices/`), QML registration (`backendqmltypes.h`) and the QML user interface under `src/application/qml`.

Terms used in this plan:

- BREP is OCCT's native shape file format (`.brep`). STEP (`.stp`, `.step`) is an exchange CAD format that OCCT converts through the `TKDESTEP` library.
- A QML singleton is a C++ object that QML reads by a global name, for example `App.Machining` in `src/application/qml/Main.qml`. Here, `App` and `Backend` are import aliases of the module `robocrap_qml_module`. A "foreign wrapper" is a small `Q_GADGET` struct with `QML_FOREIGN(SomeClass)` that registers a class it does not own; `src/application/backendqmltypes.h` contains these.
- Frozen RSI means the RSI (KUKA Robot Sensor Interface) preparation and device code in `src/network/rsi`, `src/pathgeneration` and `src/scene`. The user has excluded it from changes. None of its files may be edited by this plan. Where an edit elsewhere affects something RSI reads, this plan keeps that thing.
- A "physical line" is any line in a file; a "nonblank line" is a line containing a non-whitespace character.

Binding rules from `AGENTS.md` that every milestone must obey:

- C++: no lambdas; no project-written templates; no new `throw` and no project exception types (try/catch is allowed only around external-library calls such as OCCT); results are returned by value, never through reference or pointer output parameters; at most four function parameters, and the reason for any three- or four-parameter signature must be recorded; another object's private state is read only through its methods.
- Before adding any helper, class or field, record in this plan in at most three sentences why it is required and why existing code cannot be reused. After each milestone, separately review the changed declarations, bodies and callers for those rules, record file/line violations and fix them before marking the milestone complete.
- Record file and physical/nonblank line counts at the start and end of each milestone, separately for C++, QML, build inputs, and resources/documentation.
- UI changes must not make the interface jump: showing, hiding or updating content must not resize the 3D view, panels or windows.
- Do not edit frozen RSI files, the PLC code in `src/devices/plc/` and `src/devices/include/devices/plc/` (excluded by the user), `libs/`, `python/`, `old_imp/`, `qml/Modules/Components` or `qml/Modules/Styles`.
- Preserve unrelated working-tree edits. At the start of this plan, `git status --short` showed uncommitted user edits in `AGENTS.md`, `resources/pics/generate_path.svg`, `resources/pics/rsi_sampling.svg`, `src/application/qml/Main.qml`, `src/application/qml/MenuBar/MainMenuBar.qml` and `src/application/qml/Views/Workspace/EndEffectorPanel.qml`. Edit around them; never revert them.
- Before changing an area, read the matching sections of `docs/AGENT_CONTRACTS.md`: "Viewport and CAD" (Milestones 4, 6, 9), "Geometry and scene" (1, 2, 7), "Machining and playback" (3, 5), "Devices and protocols" (8, for device QML registration only), "QML" (2, 4, 5, 7, 8), "Build and modules" (4, 8). Milestone 10 rewrites the constraints this plan intentionally changes.

Baseline counts taken on 2026-10-17 before any change:

    Active C++ (src/**/*.h|cpp excluding src/network, src/pathgeneration, src/scene): 104 files, 14565 physical, 12333 nonblank
    Frozen RSI C++ (src/network, src/pathgeneration, src/scene): 23 files, 2904 physical, 2493 nonblank
    Application QML (src/application/qml/**/*.qml): 20 files, 3281 physical, 2779 nonblank
    Build inputs (CMakeLists.txt files, cmake/, resources/*.qrc): 11 files, 494 physical, 465 nonblank
    resources/json/kr10.json: 90 physical lines

## Plan of Work

The milestones are ordered so that each one removes callers before a later milestone removes what they called. Milestone 3 deletes the profiler before Milestones 5 and 6 delete `SceneEndEffectors::setSpindleTcpInCollet` and `RobotDefinition::fromJson`, which only the profiler still uses. Milestone 2 removes circles before Milestone 7 reshapes geometry storage. Milestone 4 reshapes tool loading before Milestone 9 removes forwarding through the window.

### Milestone 1 — Remove four uncalled geometry helpers

`src/geometry/include/geometry/utils.h` declares, and `src/geometry/utils.cpp` defines, `readJsonPoints` (utils.cpp:46-54), `prjPointOnLine` (:284-294), `polyfit2d` (:307-327) and `deriv2d` (:329-336). A search of all of `src` (including frozen RSI) finds no caller. Delete their declarations and definitions. Keep `jsonArrayToPoints`: `decodeProbeSamples` (utils.cpp:89) and the importer call it. Confirm before deleting:

    cd D:\Qt\QtProjects\RoboCrap
    git grep -n -w -e readJsonPoints -e prjPointOnLine -e polyfit2d -e deriv2d -- src

The only hits must be in `src/geometry/utils.h` and `src/geometry/utils.cpp`. Expected reduction: about 60 lines.

### Milestone 2 — Remove circle import

Circle import is not part of the described workflow and the user removed it. It currently flows as follows. The toolbar action "Circle from JSON" (`src/application/qml/Views/Workspace/ModeToolBar.qml:12,31,154-157`) opens `circleFileDialog` (`Main.qml:88-94`). That calls `SceneSurfaceImporter::loadCircle` (`scenesurfaceimporter.h:20`, `.cpp:84-87`). `importSurface` reads raw points (the `circle` branch of `readSamples`, `.cpp:23-46`, and `ReadError::CircleSamples`, `.cpp:15,62-63`), fits `Circle::fromPoints` (`src/geometry/circle.cpp`, `include/geometry/circle.h`) and calls `SceneModel::addCircle` (`scenemodel.h:32`, `.cpp:222-230`). That stores a `SceneCircleGeometry` (`scenegeometry.h:205-236`) through `SceneObject::setCircleGeometry`/`circle()` (`sceneobject.h:46,69`, `.cpp:42-51`). `SceneObject::drawing` and `drawingDiagnostics` have circle branches (`sceneobject.cpp:113-115,142-182`). The foreign wrapper `SceneCircleGeometryQml` (`backendqmltypes.h:122-128`), the Properties rows (`PropertiesPanel.qml:31,38-39,57,312,365-403`) and the Scene panel icon (`ScenePanel.qml:48`) complete it.

Delete all of the above. Remove the enumerator `Circle` from `SceneObject::Kind` (`sceneobject.h:29`); it is the last enumerator, so the numeric values of the others do not change. In `SceneModel::addObject` (`scenemodel.cpp:168`), change the upper bound to `SceneObject::MachiningPath`; Milestone 7 deletes `addObject` anyway. Remove `circle.h`/`circle.cpp` from `src/geometry/CMakeLists.txt`. Remove `circleImportAvailable`, `circleImportRequested` and the action from `ModeToolBar.qml`, and their bindings in `Main.qml` (lines 387, 422 and the dialog visibility term on line 111). Remove `<file>pics/circ.svg</file>` from `resources/pics.qrc` and delete `resources/pics/circ.svg` once `git grep -n circ.svg` shows no remaining reference. Keep the sample files under `resources/json/circ/` (they are data, not code; deleting them is the user's call). Do not touch `isCircle()` on the ellipse or the circle drawing branch in `src/viewport/adapters/occsceneadapter.cpp:55-60`; an intersection edge can itself be a circle.

Keep the toolbar layout stable: the remaining import actions must not move other toolbar groups. Inspect `ModeToolBar.qml` around line 140-170 and keep the menu or button group's own sizing. Expected reduction: about 380 lines, two C++ files and one SVG.

### Milestone 3 — Remove the chamfer profiling command and Generate timing diagnostics

Generate currently measures itself. `ChamferProfiledKinematics` (`src/trajectories/chamfer/chamferpreparationdiagnostics.h`) wraps `Kr10Kinematics` to count and time IK/FK calls. `ChamferProfileScope` takes timing snapshots. `MotionCompiler` (`chamfermotion.cpp`, around lines 250-430 and 532-541), `ChamferMotion::create` (lines 720-778), `CentralValidator` and `ChamferCentralPreparation` (`chamfercentralvalidation.cpp`, around lines 245-310, 412-415 and 425-506), and `ChamferCentralMotion::create` (`chamfercentralmotion.cpp:122-150`) build `QJsonObject` diagnostics. `ChamferMotionResult::diagnostics`, `ChamferCentralMotionResult::diagnostics` and `ChamferCentralValidationResult::diagnostics` return them. `SceneMachining::generate`/`finishCalculation` (`scenemachining.cpp:179-182,211-213`) log them. The command-line profiler `src/application/chamferprofile.{h,cpp}`, started by `robocrap --profile-chamfer` (`main.cpp:19,49-57`), writes them to a report.

Delete `chamferprofile.h/.cpp`, their entries in `src/application/CMakeLists.txt`, and the `--profile-chamfer` branch and include in `main.cpp`. In `chamfermotion.cpp` and `chamfercentralvalidation.cpp`, replace each `ChamferProfiledKinematics m_solver` member with `RoboCrap3D::Kr10Kinematics m_solver`; the wrapper forwards to the same method names (`solveIK`, `solveFK`, `calculateIK`, `configuration`, `isValid`) with identical arguments, so call sites stay unchanged. Remove every `QElapsedTimer`, `ChamferProfileScope`, `m_attempt`/`m_attempts`/`m_transferSchedule`/`m_segments`/`m_diagnostics` member, `diagnostics()` method, `recordSegment`, nanosecond counter and `m_targetCacheHits`. Then remove the `diagnostics` fields from the three result structs and their `QJsonObject` includes. In `chamferarclength.cpp`, remove `actualEvaluations` from `ArcTableResult` and the `++m_result.actualEvaluations` statement, and remove both count fields from the public `ChamferArcLengthResult`. Keep `logicalEvaluations` inside `ArcTableResult` and `chargeEvaluations`: the 800000 logical limit still rejects work exactly as before. Remove `ChamferProfileScope` from `chamferpreparationdiagnostics.h`, but keep the file and `ChamferProfiledKinematics`, because frozen RSI includes and uses them. Remove the `chamferpreparationdiagnostics.h` include from the two animation files.

Remove `ChamferCentralPreviewSpan::curvatureBound` and `accelerationBound`, and `ChamferCentralPreparation::spans()`, only if a search shows they no longer have non-diagnostic readers. `span.phase` and the span times are still used to build per-phase error messages (`chamfercentralvalidation.cpp:291`) and must stay. In `SceneMachining`, remove `m_geometryConstructionMs` and its timer and replace the JSON log line with the single line chosen in the Decision Log, using `result.motion->points().size()` and `result.motion->duration()`, or `result.error`.

Every error string, candidate order, schedule and acceptance check is unchanged. Only measurement code is removed. Expected reduction: about 420 lines and two files.

### Milestone 4 — Load the fixed end-effector models together with the robot from BREP files

Today the seven robot links load once through `OccController::loadRobot` (`src/viewport/occcontroller.cpp:288-341`), while each tool has its own `ToolCadState` (`occcontroller.h:160-170`). That state includes a worker, generation counter, source URL and error. It is started by `loadEndEffector`/`startEndEffectorLoad`/`finishEndEffectorLoad` (`occcontroller.cpp:101-216`), requested by `RobotController::reloadEndEffector` (`robotcontroller.cpp:60-65,165-171`), fed by `SceneEndEffectors` CAD source properties (`sceneendeffectors.h:16-17,32-33,48-49,60-61,67-68`, `.cpp:49-78`) that `main.cpp:99-102` sets to `MEE.stp`/`SEE.stp`, and shown/edited by `EndEffectorPanel.qml:10-12,21-22,85-121` and `Main.qml:129-146,463-465,473-477,484-486,500-504`. Every CAD file is read by `CadLoadWorker::run` (`src/viewport/occt/cadloadworker.cpp`) through `CachedShapeLoader` (`occt/cachedshapeloader.{h,cpp}`). That loader requires a `.stp`/`.step` name, tries `<basename>.brep`, and otherwise converts STEP with `StepImporter` (`occt/stepimporter.{h,cpp}`) and writes a new BREP.

Replace this with one load of nine BREP files:

1. In `src/viewport/include/viewport/robotgraphics.h`, add `QStringList tools;` to `RobotVisualModel`: absolute paths of the measuring tool and the spindle tool, in `RobotTool` order. Requirement: the renderer must know which loaded shapes are tools. A separate field is smaller than encoding tool roles in `LinkModel`.
2. In `resources/json/kr10.json`, change the seven `filename` values from `.stp` to `.brep` (Milestone 6 later deletes this file).
3. In `RobotController::loadRobot` (`robotcontroller.cpp:191-197`), after resolving link paths, set `visuals.tools` to the deployed `MEE.brep` and `SEE.brep` paths in `assets.cadDirectory`.
4. In `OccController::loadRobot`, append `visuals.tools` to `sources` after the links, so the single `CadLoadWorker` returns nine shapes in order.
5. Rewrite `CadLoadWorker::run` to read each absolute `.brep` path directly with `BRepTools::Read` from a `QFile`, inside the existing try/catch for `Standard_Failure` and `std::exception`. Keep the static `QMutex`, the interruption checks and the absolute-path check. For an unreadable or null shape, set `m_result.error` to `Cannot read CAD file: <path>`. Remove the `cacheDirectory` constructor parameter. Delete `cachedshapeloader.{h,cpp}` and `stepimporter.{h,cpp}`.
6. In `OccRobotAdapter::load` (`occrobotadapter.cpp:22-67`), require `shapes.shapes.size() == visuals.links.size() + visuals.tools.size()`. Create the tool parts (default `OccPartProps`, the same as today's `addShapePartWithId` call) right after the links, inside the same installation and cleanup. Make only the active tool visible. Replace `ToolPart`/`m_tools` with `std::array<std::optional<OccScene::PartId>, 2> m_toolIds`. Replace `setEndEffectors` with `bool setActiveTool(RobotTool active)`, which only toggles visibility and re-synchronizes the spindle axes. `clear()` removes tool parts together with link parts.
7. Replace the `setEndEffectors` chain in `OccViewport`, `OccViewWindow` (`setEndEffectors`, `attachEndEffectors`, `m_toolShapes`) and `OccController::synchronizeEndEffectors` with `setActiveTool(RobotTool)` forwarding. Milestone 9 later removes the window hop. Remove `ToolCadState`, `m_toolCad`, the four tool CAD properties, `endEffectorLoadChanged`, `endEffectorLoaded`, `loadEndEffector`, `startEndEffectorLoad`, `finishEndEffectorLoad`, and `m_cacheDirectory` together with the constructor's `cacheDirectory` parameter. Also remove `cacheDirectory` from `ViewportAssets` (`src/application/assetpaths.{h,cpp}`) and its use in `main.cpp:115`.
8. In `SceneEndEffectors`, remove the CAD source properties, setters, signals, members and `isLocalCadSource`. In `RobotController`, remove `reloadEndEffector` and its four `connect` lines and two constructor calls. In `main.cpp`, remove lines 100-102.
9. In QML, remove `endEffectorFileDialog` and the `cadSource`/`loading`/`loadError`/`onBrowseRequested`/`onRetryRequested` bindings from `Main.qml`. In `EndEffectorPanel.qml`, remove the matching required properties, the `browseRequested`/`retryRequested` signals, the "CAD" row, the status label and the Retry button.
10. Build inputs: remove `TKDESTEP` from `target_link_libraries` in `src/viewport/CMakeLists.txt` and from `runtime_roots` in `cmake/DeployOcct.cmake:22`. Change the CAD copy pattern on line 70 to `PATTERN "*.brep"`. Remove the deleted files from `src/viewport/CMakeLists.txt`. Keep `initializeOcct`'s resource checks unchanged: the deploy step still copies the whole OCCT `resources` folder.

Afterwards the robot is "ready" only when all nine shapes are installed, and switching between Measuring and Machining shows the corresponding tool, as before. Expected reduction: about 430 lines and four files; the 3D library no longer links the STEP translator.

### Milestone 5 — Remove the manual robot pose API and RobotController's QML exposure

`RobotController` exposes `solveFK`, `solveIK`, `solveTcpIK`, `jointAngles`, `flangePose`, `tcpPose`, `ready`, `loading` and `errorString` to QML (`robotcontroller.h:63-68,81-84`). It is registered as the singleton `RobotController` (`robotcontroller.h:122-140`). After Milestone 4, no QML file uses `App.RobotController`; confirm with `git grep -n "RobotController" src/application/qml`. `SceneEndEffectors::spindlePoseFromFlange`/`flangePoseFromSpindle` (`sceneendeffectors.cpp:149-161`) serve only `tcpPose`/`solveTcpIK`, and `setSpindleTcpInCollet` (`.cpp:106-109`) served only the profiler deleted in Milestone 3.

Delete these methods, the Q_PROPERTYs, `RobotControllerQml`, `readValues`, `toList`, `solve`, `inverse`, `onViewportError` and its `connect`, the `errorStringChanged`/`poseChanged`/`tcpPoseChanged` signals with their connects, and `m_endEffectors`' pose helpers. Replace `RobotOperationResult` (a QML value type with an unused `values` list) by the existing `ViewportOperationResult {accepted, error}` from `robotgraphics.h` for `loadRobot` and `applyPlayback`. Update `SceneMachining::presentPlayback` (`scenemachining.cpp:382`) accordingly. Shrink `RobotPose` to the joints and link transforms: `forward` no longer computes `rot2euler` or `configuration`, because only manual IK read them. In `SceneEndEffectors`, make `setSpindleTcp` a private helper of `applySpindlePoses`.

Remove the `WorkspaceController` → `RobotController` dependency at the same time. `WorkspaceController` uses `m_robot` only to call `RobotController::setActiveTool` (`workspacecontroller.cpp:40`), which only forwards to `SceneEndEffectors::setActiveTool` (`robotcontroller.cpp:151-154`). Change the constructor's third parameter to `SceneEndEffectors*`, call `setActiveTool` on it directly, delete `RobotController::setActiveTool`, and update `main.cpp:123`. The three parameters remain three distinct collaborators: document, machining session and tool calibration.

Keep `Kr10Kinematics::solveIK(target, previousAngles)`, `calculateIK` and `matchesFlange`; frozen RSI and Generate use them. Keep the `message` signal and its console connection: playback failures still print through it. Expected reduction: about 150 lines.

### Milestone 6 — Compile the fixed KR10 description into the application

`RobotDefinition::fromJson` (`src/application/robot/robotdefinition.cpp`, 321 lines) parses and validates `resources/json/kr10.json` (`:/json/kr10.json`, listed in `resources/json.qrc`) on every start, although the robot is fixed. Replace it with `static RobotDefinition RobotDefinition::kr10()`, which builds the same values from constants:

- Six joints with offsets `(dx, dy, dz, da, db, dc)`, axis, `min`, `max` and rated speed exactly as in `kr10.json` lines 3-46.
- `localTransform` computed with the same expression as `readJoint` (`makeTranslation(...) * makeRotation(da, Z) * makeRotation(db, Y) * makeRotation(dc, X)`), so the matrices are bit-identical.
- `IkParams {150, 450, 610, 20, 660, 80}`, `qHome {0, -90, 90, 0, 0, 0}`, `pHome {890, 0, 1080, 0, 90, 0}`.
- Seven link entries `0_BASE.brep` … `6_WRIST3.brep` with colours `[0,0,0]`, five times `[255,106,0]`, `[0,0,0]`.

Use one local `constexpr` array of a small aggregate for the joint rows (a data table, as `AGENTS.md` allows). Keep `RobotController::initialize`'s HOME/FK comparison as the startup consistency check. Remove `modelFile` from `ViewportAssets`, the `fromJson` failure branch in `RobotController::loadRobot`, `<file>json/kr10.json</file>` from `resources/json.qrc`, and `resources/json/kr10.json` itself. Before deleting the JSON, run `git grep -n kr10.json` and confirm that no remaining file outside `codex_plans/` and `docs/` refers to it; record the `python/` result separately (it is outside the build).

Expected reduction: about 250 C++ lines plus the 90-line JSON file.

### Milestone 7 — Store object geometry as values and build Properties rows in C++

Each `SceneObject` currently owns a child QObject "view" per geometry kind (`ScenePlaneGeometry`, `SceneCylinderGeometry`, `SceneEdgeGeometry` in `scenegeometry.h:117-261`), found again with `qobject_cast` (`sceneobject.cpp:15-62`, `scenemachining.cpp:123`). Each view has a foreign wrapper (`backendqmltypes.h:106-136`). `PropertiesPanel.qml:33-36,309-450` hand-writes one row per field. `SceneModel::addPlane(coefficients)` and `SceneModel::addObject` (`scenemodel.cpp:160-200`) have no callers, and the coefficient-only plane mode exists only for `addPlane`.

Change `SceneObject` to hold `std::variant<std::monostate, BoundedPlane, BoundedCylinder, SceneEdge> m_shape` plus `SceneMachiningPath* m_machiningPath`. `SceneEdge` is a two-field aggregate `{EdgeGeometry edge; std::array<quint32, 2> sources;}` declared in `scenegeometry.h`. It is required because an edge needs its two source object IDs, and no existing value pairs them. Implement `plane()`, `cylinder()`, `edge()` with `std::get_if`, add `edgeSources()` returning `std::optional<std::array<quint32, 2>>`, and keep `machiningPath()`. Keep `geometry()` and its QML property, returning `m_machiningPath` (see the Decision Log). Replace `qobject_cast<const SceneEdgeGeometry*>` in `SceneMachining::sourceParameters` with `edge()`/`edgeSources()`.

Add `Q_PROPERTY(QVariantList properties READ properties CONSTANT)` to `SceneObject`. Its getter returns one `QVariantMap {"label": QString, "value": QString}` per geometry row, in today's order:

- Plane: Normal, Origin, Width, Height, Points, Offset (d).
- Cylinder: Origin, Axis, Radius, Diameter, Length, Points.
- Edges and machining paths: none, as today.

Format numbers as today's `formatReal`: `QString::number(std::abs(v) < 0.00005 ? 0.0 : v, 'f', 4)`. Format vectors as `[x; y; z]` from those numbers, and point counts as integers. Use two helpers in the unnamed namespace of `sceneobject.cpp`. Translate labels with `tr()`. Requirement: one C++ list replaces three view classes and fifteen QML rows; no existing operation formats these values in C++.

In `PropertiesPanel.qml`, delete `planeGeometry`, `cylinderGeometry`, the per-field rows and the "Geometry" header condition. Add one `Repeater { model: root.selectedObject ? root.selectedObject.properties : [] }` whose delegate is the existing `PropertyValue` component. The delegate declares `required property var modelData` and binds `labelText: modelData.label` and `valueText: modelData.value`. Show the "Geometry" label when the list is non-empty. Keep `labelColumnWidth` and its label list, minus the circle labels removed in Milestone 2, so that the label column width does not change when the selection changes. The per-value `objectName`s (`planeNormalValue` and so on) have no repository consumer and are dropped.

Delete the three view classes, their wrappers, `ScenePlaneGeometry`'s coefficient mode, `setPlaneGeometry(coefficients)`, `addPlane`, `addObject`, and the `QJSEngine::setObjectOwnership` call for non-machining geometry in `SceneModel::appendObject` (`scenemodel.cpp:153`; keep it for `m_machiningPath`). Update `drawing()`/`drawingDiagnostics()` to the accessors; their numeric results do not change. Expected reduction: about 300 lines.

### Milestone 8 — Consolidate QML registration

Revised scope (2026-10-17, see Decision Log): the user chose Option A. Only the singleton factory check is consolidated, into one inline `claimQmlSingleton(QObject*, QJSEngine*)` in `src/application/backendqmltypes.h`. Every registration stays a foreign wrapper in that header with unchanged names, kinds and messages. The remainder of this section describes the larger Option B, which was not implemented; keep it only as a record of the alternative.

`src/application/backendqmltypes.h` (234 lines) registers application classes through foreign wrappers and repeats a singleton factory five times with two different checks; `WorkspaceController::create` (`workspacecontroller.cpp`) is a sixth copy. Now that every registered application class is owned by the `robocrap` target, register classes in their own headers. Keep foreign wrappers only for types owned by `RoboCrapDevices` (`DeviceHub` is in Application, but `PlcMessageManager` and `RDTResponse` are in Devices, which has no QML dependency) and for the `Scene` singleton, because `SceneModel` is also used as the type name `App.SceneModel` (`Main.qml:112`).

Add `src/application/qmlsingleton.h/.cpp` with one function, `QObject* claimQmlSingleton(QObject* instance, QJSEngine* engine)`. It calls `qFatal("%s must be used by one GUI-thread QML engine", className)` (with the class name taken from `instance->metaObject()->className()`) when `instance` is null, when the engine's thread differs from the instance's thread, or when a different engine already claimed a singleton. It records the engine in one file-local `QPointer<QJSEngine>`, sets `QJSEngine::CppOwnership` and returns `instance`. Requirement: it replaces six copies of the same check. Two parameters suffice because the class name comes from the instance.

Each singleton class (`SceneMachining` as `Machining`, `SceneMachiningPreparation` as `MachiningPreparation`, `SceneEndEffectors` as `EndEffectors`, `DeviceHub` as `Hub`, `WorkspaceController`) gets `QML_NAMED_ELEMENT(<name>) QML_SINGLETON`. It also gets a static instance pointer set in `main.cpp` and a one-line `static T* create(QQmlEngine*, QJSEngine* e) { return static_cast<T*>(claimQmlSingleton(s_instance, e)); }`.

`SceneMachiningPreparation` lives in frozen `src/scene`. Keep its foreign wrapper in a reduced form instead of editing that file. Non-singleton application classes get `QML_ELEMENT`/`QML_NAMED_ELEMENT` and `QML_UNCREATABLE` (or `QML_VALUE_TYPE(machiningSettings)` for `SceneMachiningSettings`) in their own headers: `SceneObject`, `SceneMachiningPath` (`MachiningPath`), `SceneModel` (`SceneModel`, now uncreatable), `SceneSurfaceImporter` (`PlaneImporter`, creatable), `DeviceRunner` (uncreatable), `DeviceProfileModel` (creatable). Every QML element name stays the same. Rename the remaining wrapper file to `src/application/foreignqmltypes.h` only if that removes confusion; otherwise keep the name.

Check that every name QML uses (`git grep -n -E "(App|Backend)\.[A-Z][A-Za-z]+" src/application/qml`) is still registered. Expected reduction: about 120 lines.

### Milestone 9 — Remove the 3D window's forwarding methods

Each document operation travels `OccController` → `OccViewWindow` → `OccViewport` → `OccSceneAdapter`: insert, remove, visibility, diagnostics, selection, draft axis, spindle frames and active tool (`occviewwindow.cpp:49-122`). The window adds two behaviors: it re-applies the selection after insert/remove/visibility changes, and it keeps a copy of the pose for replay after surface recreation. `OccController` already owns the accepted `DrawingDocument` (including the selection) and the accepted `DrawingPose`.

Give `OccViewWindow` a `OccViewport* viewport() const` getter (null when no native surface exists). Change its constructor to borrow both `const DrawingDocument&` and `const DrawingPose&` from `OccController`; this is two parameters. In `OccController`, call `m_window->viewport()` directly for the operations above, and re-apply `m_document.selected` after successful insert/remove/visibility changes, exactly as the window did. Delete the window's forwarding methods, `m_pose` and `setSpindleFrames`; `initializeViewport` reads the borrowed pose. Keep `applyTransforms` (it implements the one-time restoration and `m_presentationFailed`), `setScene`, `attachRobot`, `releaseSurface`, readiness and all input/surface event handlers in the window.

Native-surface recreation still replays the document (the `OccViewport` constructor already replays `m_document.objects`), the draft axis, selection, spindle frames and the robot. Expected reduction: about 100 lines.

### Milestone 10 — Update contracts and architecture documents; final audit

Update only the constraints this plan intentionally changed:

- `docs/AGENT_CONTRACTS.md`, "Viewport and CAD": CadLoadWorker reads BREP only; no STEP fallback or cache writing; tools load with the robot; no tool Browse/Retry; robot description is compiled in.
- "Machining and playback": diagnostics/profiling bullets now state that only the logical 800000 limit remains; remove the profiling-input reference.
- "Geometry and scene": geometry stored as values; no circle import.
- "QML": registration location and the shared singleton check.
- "Build and modules": `TKDESTEP` is no longer linked; registration headers.
- Remove `chamferprofile.cpp`/`readProbeSamples` mentions where obsolete.

Update `docs/ARCHITECTURE.md` sections 3.2, 3.3, 3.5, 3.6, 5.1 and 6 to match. Do not add completion reports to those documents. Record final counts and the compliance review in this plan.

## Concrete Steps

All commands run from `D:\Qt\QtProjects\RoboCrap` in Git Bash. Capture counts at the start and end of each milestone with:

    count(){ files=$(git ls-files "$@"; git ls-files --others --exclude-standard "$@"); n=$(echo "$files" | grep -c .); p=$(cat $files | wc -l); nb=$(cat $files | grep -cv '^\s*$'); echo "$n files, $p physical, $nb nonblank"; }
    count $(git ls-files 'src/*.h' 'src/*.cpp' | grep -v '^src/network/\|^src/pathgeneration/\|^src/scene/')
    count $(git ls-files src/application/qml | grep '\.qml$')
    count CMakeLists.txt 'src/*CMakeLists.txt' cmake resources/pics.qrc resources/fonts.qrc resources/json.qrc

The baseline output was:

    104 files, 14565 physical, 12333 nonblank
    20 files, 3281 physical, 2779 nonblank
    11 files, 494 physical, 465 nonblank

Before and after each milestone, confirm frozen RSI files are untouched; this must print nothing:

    git diff --name-only -- src/network src/pathgeneration src/scene

After each milestone run:

    git status --short
    git diff --check

Inspect new untracked files separately, for example `cat src/application/qmlsingleton.h`.

Searches that must return no hits when the named milestone is complete:

    M2:  git grep -n -i -e loadCircle -e SceneCircleGeometry -e circleImport -e "circ.svg" -- src resources
    M3:  git grep -n -e profile-chamfer -e ChamferProfileScope -e "\.diagnostics" -- src/trajectories src/application
    M4:  git grep -n -e StepImporter -e CachedShapeLoader -e loadEndEffector -e reloadEndEffector -e CadSource -e TKDESTEP -- src cmake
    M5:  git grep -n -e solveFK -e solveTcpIK -e RobotControllerQml -e flangePoseFromSpindle -- src
    M6:  git grep -n -e "RobotDefinition::fromJson" -e modelFile -e kr10.json -- src resources
    M7:  git grep -n -e ScenePlaneGeometry -e SceneCylinderGeometry -e SceneEdgeGeometry -e addObject -- src/application

Only when the user explicitly requests a build: find an existing build directory whose `CMakeCache.txt` has `CMAKE_HOME_DIRECTORY` equal to this repository, check its Qt and compiler paths and `USE_HOT_RELOAD` value, and then run, for example:

    cmake --build <that build directory> --target robocrap

Use `USE_HOT_RELOAD=OFF` to compile application QML, and run `robocrap_qmllint` when QML changed. Do not invent SDK paths.

## Validation and Acceptance

Source-level acceptance, required for every milestone: the milestone's search above returns no hits; `git diff --check` is clean; frozen RSI paths show no diff; counts are recorded; and the separate compliance review (declarations, bodies, callers, parameter counts, result channels, ownership, added helpers) is written into `Artifacts and Notes` with any violation fixed.

Runtime acceptance, performed only when the user requests a build and a disconnected runtime is available. Start `robocrap` with no devices connected and do the following:

1. In Rough Measuring, import `resources/json/edge_cyl_plane/XP.json` as a plane and `resources/json/edge_cyl_plane/XC.json` as a cylinder. Both appear in the Scene panel and the 3D view. Selecting each shows Properties rows with the same labels and four-decimal values as before the change (note the values before starting Milestone 7).
2. Select both and press Intersect Selected; an Edge appears.
3. Switch to Machining. The spindle tool is visible on the flange; in Measuring, the measuring tool is visible.
4. Open Machining EE, enter a TCP and press Apply. Select the edge, open Generate Path and press Generate. The console prints `Chamfer preparation: <N> points, <T> s movement`. A machining path appears. N and T must equal the `retainedPoints` and `movementSeconds` printed by the pre-Milestone-3 build for the same inputs and settings.
5. Press Dry Run, Pause, Dry Run (resume), then Stop; the robot returns to HOME. Delete the path during playback; playback stops.
6. Open the PLC, FTS and Network panels. They open and bind as before; this plan changes no device code, only how `Hub`, `DeviceRunner` and `DeviceProfileModel` are registered with QML (Milestone 8). No live PLC connection is part of this check; connecting to equipment requires separate user authorization.

Opening Measuring EE/Machining EE and changing selection must not change the 3D view size or move toolbar controls.

## Idempotence and Recovery

Each milestone is a set of deletions and local edits. It can be redone by restoring the affected files with `git diff` review, never with `git checkout` or `git stash` unless the user asks. User edits already present in `Main.qml`, `MainMenuBar.qml`, `EndEffectorPanel.qml` and `AGENTS.md` must be preserved. If a milestone is interrupted, record in `Progress` which files are done and which remain, and continue from there. No step writes outside the repository except a requested build directory. Deployed copies of deleted DLLs or files in an existing build directory are not purged automatically; that is harmless.

## Artifacts and Notes

Estimated reductions, from reading the code; replace them with measured counts as each milestone completes:

    M1 dead helpers ≈ -60   M2 circle ≈ -380   M3 diagnostics ≈ -420   M4 fixed tools ≈ -430
    M5 manual pose API ≈ -150   M6 compiled KR10 ≈ -250 (+ -90 JSON)   M7 geometry values ≈ -300
    M8 registration ≈ -120   M9 window forwarding ≈ -100
    Total ≈ -2,210 C++/QML lines plus the 90-line JSON file.

Milestone 1 evidence (2026-10-17). Caller search before the change:

    git grep -n -w -e readJsonPoints -e prjPointOnLine -e polyfit2d -e deriv2d
    -> only src/geometry/utils.h, src/geometry/utils.cpp and a historical note in codex_plans/ARCHITECTURE_EXECPLAN.md:1084

After the change the same search restricted to `src` prints nothing. Counts (physical/nonblank): `src/geometry/utils.cpp` 357/279 → 304/237; `src/geometry/include/geometry/utils.h` 49/39 → 44/35. Active C++ scope: 104 files, 14565/12333 → 104 files, 14507/12287. QML, build inputs and resources unchanged. `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check -- src` is clean.

Compliance review. Declarations: four declarations removed; no signature changed or added. Bodies: four definitions removed; no other body edited. Callers: none in `src`. Includes: `utils.cpp` still needs `QFile`/`QJsonDocument` for `readProbeSamples`, and `utils.h` still needs `QString`/`QJsonDocument` for `readProbeSamples`/`decodeProbeSamples`; nothing became unused. No helper, class, field, lambda, template or exception was added. No violations found.

Milestone 2 evidence (2026-10-17). The completion search prints nothing:

    git grep -n -i -e loadCircle -e SceneCircleGeometry -e circleImport -e "circ.svg" -e CircleGeometry -e "SceneObject.Circle" -e addCircle -e "geometry/circle.h" -- src resources

The remaining `circle` mentions in active `src` are the edge-curve classification (`SceneEdgeGeometry::CurveKind::Circle`, `EdgeGeometry::Ellipse::isCircle`), a comment in `chamferpath.cpp` and the OCCT circle-edge drawing in `occsceneadapter.cpp`. All of these belong to ellipse edges and are intentionally kept.

Deleted files: `src/geometry/circle.cpp` (241/221), `src/geometry/include/geometry/circle.h` (30/24), `resources/pics/circ.svg` (3/3); none are staged. Edited files, physical/nonblank before → after:

    scenesurfaceimporter.h 32/26 → 31/25     scenesurfaceimporter.cpp 124/109 → 100/86
    scenemodel.h 64/56 → 63/55               scenemodel.cpp 267/236 → 257/227
    sceneobject.h 85/75 → 83/73              sceneobject.cpp 206/187 → 188/171
    scenegeometry.h 261/237 → 227/207        backendqmltypes.h 234/203 → 226/196
    geometry/utils.cpp 304/237 → 308/239     geometry/utils.h 44/35 → 43/34
    geometry/CMakeLists.txt 24/24 → 22/22    resources/pics.qrc 29/29 → 28/28
    Main.qml 519/458 → 509/449               ModeToolBar.qml 247/210 → 238/202
    PropertiesPanel.qml 453/393 → 408/354    ScenePanel.qml 304/256 → 303/255

`git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` is clean apart from Git's existing LF/CRLF notices for the user's two edited SVGs. The user's pre-existing edits in `Main.qml` are kept; only the circle dialog, its visibility term and its two toolbar bindings were removed.

Additional reduction found during the milestone: after circle import was removed, `jsonArrayToPoints`' only caller was `decodeProbeSamples` in the same file. It was moved into the unnamed namespace of `src/geometry/utils.cpp` and its public declaration removed. This explains the +4 lines in `utils.cpp`: an unnamed-namespace wrapper replacing one header line. `readSamples` lost its `circle` parameter (2 → 1 parameter), and `ReadError::CircleSamples`, `Surface::Circle` and the `QJsonObject` include in the importer were removed.

Compliance review. Declarations: `loadCircle`, `addCircle`, `circle()`, `setCircleGeometry`, `SceneCircleGeometry`, `SceneCircleGeometryQml`, `Circle` and the public `jsonArrayToPoints` declaration were removed; `readSamples(const QUrl&)` dropped one parameter; no declaration was added. Bodies: `drawingDiagnostics` lost only the circle branches; for edges its sample count is unchanged, because edge normals already used one centre sample. Callers: no remaining reference (search above). `SceneObject::Kind::Circle` was the last enumerator, so the other values are unchanged. No lambda, template, exception, output parameter or new helper was added. No violations found. Interface stability: the toolbar row is one action shorter by deliberate removal; no state-dependent sizing was introduced, and the Properties label-width list keeps every label still shown.

Milestone 3 evidence (2026-10-17). The completion search below prints only the 3D view's own drawing-diagnostics fields (`occsceneadapter.cpp`) and `ChamferProfiledKinematics::diagnostics()` in `chamferpreparationdiagnostics.h`; frozen RSI still uses both of those, and they are intentionally kept:

    git grep -n -e profile-chamfer -e ChamferProfileScope -e "\.diagnostics" -e "diagnostics()" -e computationNanoseconds -e actualEvaluations -e knotCount -e m_geometryConstructionMs -- src/trajectories src/application src/viewport src/kinematics src/geometry

Deleted files: `src/application/chamferprofile.cpp` (154/148) and `chamferprofile.h` (5/4); not staged. Edited files, physical/nonblank before → after:

    main.cpp 184/139 → 174/129                       application/CMakeLists.txt 135/130 → 133/128
    scenemachining.cpp 422/393 → 420/391             scenemachining.h 103/97 → 102/96
    chamferarclength.cpp 358/331 → 354/327           chamferarclength.h 57/47 → 54/44
    chamfercentralmotion.cpp 206/190 → 187/171       chamfercentralmotion.h 69/57 → 67/55
    chamfercentralvalidation.cpp 517/482 → 415/383   chamfercentralvalidation.h 74/64 → 63/54
    chamfermotion.cpp 829/783 → 717/675              chamfermotion.h 90/79 → 88/77
    chamferpreparationdiagnostics.h 90/79 → 65/57

`git diff --stat` for the milestone: 15 files, 20 insertions, 474 deletions. `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check -- src` is clean.

What was removed and why behavior is unchanged:

- `MotionCompiler`, `CentralValidator`: the solver member is now `RoboCrap3D::Kr10Kinematics` instead of the counting wrapper `ChamferProfiledKinematics`. The wrapper forwarded the same const calls (`calculateIK`, `solveFK`, `configuration`) with identical arguments and returned the solver's results unchanged, so every IK/FK result is identical. Both borrow the model exactly as the wrapper's inner solver did: `MotionCompiler` from `boundedRobot` in `ChamferMotion::create`, `CentralValidator` from the `robot` argument of `validate`, and both outlive the solver.
- The wrapper functions `compile`/`compileImpl`, `run`/`runImpl`, `appendPtp`/`appendPtpImpl` and `appendTransfer`/`appendTransferImpl` were merged. The outer functions only measured and recorded the inner one's result; the inner bodies are unchanged apart from deleted measurement statements.
- `ChamferMotion::create` loops over the same `compiler.starts()` list in the same order and returns the first complete candidate with the same error precedence.
- Removed from public results: `ChamferMotionResult::diagnostics`, `ChamferCentralMotionResult::diagnostics`, `ChamferCentralValidationResult::diagnostics`, `ChamferArcLengthResult::actualEvaluations`/`logicalEvaluations`. Removed as unused: `ChamferArcLength::knotCount`, `ChamferCentralPreparation::computationNanoseconds`/`diagnostics`, `ChamferCentralPreviewSpan::accelerationBound`/`curvatureBound`, and `ChamferProfileScope`. `ChamferCentralPreviewSpan::phase` is kept because it builds error text.
- Kept: the private `ArcTableResult::logicalEvaluations` and `chargeEvaluations`, so the 800000 logical limit rejects exactly as before.
- `SceneMachining::finishCalculation` logs `Chamfer preparation: <points> points, <seconds> s movement` or `Chamfer preparation failed: <error>`; the UI error path (`setError`) is unchanged.
- `ChamferProfiledKinematics` and `chamferpreparationdiagnostics.h` remain for the frozen RSI sampler (`src/pathgeneration/chamfer/chamfercartesiansampler.cpp:3,394`).

Compliance review. Declarations: no function or type added; three result structs and one span struct lost fields; `MotionCompiler::compile` and `CentralValidator::run` keep their one-parameter signatures. Bodies: only measurement statements and wrappers removed; one range-for now iterates `compiler.starts()` directly instead of a named copy. Callers: `ChamferMotion::create` is the only `compile` caller and `ChamferCentralPreparation::validate` the only `run` caller; `SceneMachining` is the only reader of `ChamferMotionResult` outside RSI, and RSI reads only `motion`/`error`. No lambda, template, exception, output parameter or new helper. No violations found. Line endings: edited files keep CRLF; Git prints its usual `autocrlf` notice for `chamfercentralvalidation.h`.

Milestone 4 evidence (2026-10-17). The completion search prints nothing:

    git grep -n -e StepImporter -e CachedShapeLoader -e loadEndEffector -e reloadEndEffector -e CadSource -e TKDESTEP -e addShapePartWithId -e setEndEffectors -e cacheDirectory -e "\.stp" -- src cmake resources

Deleted files, none staged: `src/viewport/occt/stepimporter.cpp` (100) and `.h` (53); `src/viewport/occt/cachedshapeloader.cpp` (85) and `.h` (19); `resources/pics/open.svg`. Scope totals before → after: active C++ 100 files 13689/11520 → 96 files 13223/11122 physical/nonblank (−466/−398); QML 3216/2722 → 3136/2643 (−80/−79); build inputs 489/460 → 483/454 (−6/−6). `resources/json/kr10.json` keeps 90 lines; only the seven link file names changed from `.stp` to `.brep`. `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing. `git diff --check` reports only Git's `autocrlf` notices. Files rewritten with LF endings during the milestone were converted back to the working tree's CRLF (index endings stay LF); their diffs contain only content changes.

What changed and how:

- `CadLoadWorker(QStringList)` reads each absolute `.brep` path through `QFile` bytes into `BRepTools::Read`, as `CachedShapeLoader` did for its BREP branch. It keeps the static mutex, the interruption checks, the absolute-path check and the `Standard_Failure`/`std::exception` handlers. An unreadable or null shape yields `Cannot read CAD file: <path>`.
- `RobotController::loadRobot` sets `visuals.tools` to `<cad>/MEE.brep` and `<cad>/SEE.brep`. `OccController::loadRobot` appends them after the link paths, so one worker returns nine shapes.
- `OccRobotAdapter::load` creates links (displayed, coloured) and then tools (default `OccPartProps`, undisplayed) in one installation. After the HOME transforms succeed it displays only the active tool, and any failure cleans every created part through the existing reverse removal loop. `applyTransforms` applies `pose.tool` to both tool parts, as before. `setActiveTool` replaces `setEndEffectors` and only toggles tool visibility.
- The `setEndEffectors` chain (`OccController::synchronizeEndEffectors` → `OccViewWindow` → `OccViewport` → adapter) is now `setActiveTool`. `OccController` lost `ToolCadState`, the four tool CAD properties, two signals and three loading methods; its constructor takes only the OCCT resource directory. `ViewportAssets::cacheDirectory` and `main.cpp`'s CAD-source setup are removed.
- `SceneEndEffectors` lost the CAD source properties/setters/signals/members and `isLocalCadSource`. `RobotController` lost `reloadEndEffector`, its four connects and its two constructor calls.
- QML: `Main.qml` lost `endEffectorFileDialog` and the six CAD bindings; `EndEffectorPanel.qml` lost the CAD row, the loading/error label, the Retry button, three required properties and two signals. The user's earlier uncommitted edits in both files are preserved.
- Build/deploy: `TKDESTEP` removed from `src/viewport/CMakeLists.txt` and from `cmake/DeployOcct.cmake` runtime roots (its DLL dependencies were resolved from that root, so they are no longer deployed); CAD deployment copies `*.brep` only. `initializeOcct`'s resource checks are unchanged, because the whole OCCT `resources` folder is still copied.

Compliance review. Declarations: added `RobotVisualModel::tools` (planned) and the private `OccRobotAdapter::showActiveTool()`. That helper is required because installation and tool switching both apply the same visibility rule; one function replaces two loops. `CadLoadWorker` and `OccController` constructors went from 2/3 parameters to 1/2. Bodies: no lambda, template, new exception type or new `throw`; the only removed try block was the catch-all/rethrow in `addShapePartWithId`. Result channels: unchanged (`ViewportOperationResult`, `bool`). Ownership: tool parts are now owned by the installation and removed with it; the worker generation check and shutdown wait are unchanged. Interface stability: the panel lost a transient status label and button that previously appeared and disappeared, so no new layout shift is possible. No violations found.

Milestone 5 evidence (2026-10-17). The completion search finds only the kinematics solver's own `solveFK`/`solveIK`, which Generate and frozen RSI still use; `git grep -n RobotController -- src/application/qml` prints nothing:

    git grep -n -e solveFK -e solveIK -e solveTcpIK -e RobotControllerQml -e flangePoseFromSpindle -e spindlePoseFromFlange -e setSpindleTcpInCollet -e RobotOperationResult -e robotOperationResult -e "RobotPose\b" -e reloadEndEffector -- src

Edited files, physical/nonblank before → after:

    robot/robotcontroller.h 139/123 → 60/48         robot/robotcontroller.cpp 328/290 → 192/170
    scene/sceneendeffectors.h 62/52 → 59/48         scene/sceneendeffectors.cpp 134/114 → 115/98
    workspacecontroller.h 67/58 → 67/58              workspacecontroller.cpp 115/99 → 114/98
    main.cpp 169/124 → 168/123

Active C++ scope: 96 files, 13223/11122 → 12984/10905 (−239/−217). `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` reports only Git's `autocrlf` notices. The two rewritten files were converted back to CRLF working-tree endings.

What changed and how:

- Removed from `RobotController`: `solveFK`, `solveIK`, `solveTcpIK`, `solve`, `inverse`, `readValues`, `toList`, `setActiveTool`, `isLoading`, `onViewportError`; the `ready`, `loading`, `errorString`, `jointAngles`, `flangePose` and `tcpPose` properties; the `readyChanged`, `loadingChanged`, `errorStringChanged`, `poseChanged` and `tcpPoseChanged` signals; and the `RobotControllerQml` singleton wrapper with its QML engine includes. None had a QML caller or a C++ listener (`git grep` for each name outside `src/application/robot` was empty).
- `RobotOperationResult` (a QML value type whose `values` list was only filled by the manual commands) is replaced by the existing `ViewportOperationResult {accepted, error}`. `SceneMachining::presentPlayback` reads the same two fields unchanged.
- The `RobotPose` class, which held joints, IK configuration, Euler flange coordinates and link transforms, is replaced by `QList<M4d>` link transforms. Only manual IK read the other three values. `forward` keeps every check (finite joints, joint limits, finite FK transforms) and no longer computes `rot2euler` or `configuration`. `RobotPoseResult` now carries `std::optional<QList<M4d>> links`.
- `setError` keeps its de-duplicated console message but no longer emits a property signal. `onViewportError` used to copy viewport error text into `m_error`; that only affected de-duplication of a robot message identical to a viewport message. The numerical messages (`Joint %1 must be ...`, `Robot preview is not ready for playback.`, `The saved trajectory TCP transform is not finite.`, load failures) never match viewport texts, so console output is unchanged.
- `SceneEndEffectors`: removed `spindlePoseFromFlange`, `flangePoseFromSpindle` and `setSpindleTcpInCollet` (no remaining caller after Milestone 3); `setSpindleTcp` became a private helper of `applySpindlePoses`. QML still uses `spindleTcp`, `spindleTcpInCollet`, `spindleColletPose`, `applySpindlePoses` and `setActiveTool`, and these are unchanged.
- `WorkspaceController(SceneModel*, SceneMachining*, SceneEndEffectors*)` calls `SceneEndEffectors::setActiveTool` directly instead of through `RobotController::setActiveTool`, which only forwarded. `main.cpp` passes `&endEffectors` and no longer sets `RobotControllerQml::s_inst`.
- Kept: `Kr10Kinematics::solveIK`, `calculateIK`, `configuration` and `matchesFlange` for Generate and frozen RSI.

Compliance review. Declarations: no function or type added. `RobotController`'s constructor keeps its three collaborators (renderer, tool calibration, machining session), and `WorkspaceController` keeps three (document, machining session, tool calibration); each is a distinct owner it calls. Result channels: return values only. Ownership: unchanged (`m_kinematics` borrows `m_model.kinematics`; copy/move stays disabled). Behavior removed by request: manual FK/IK, the "Stop Dry Run before changing the robot pose" guard (it only protected manual commands), and the QML-visible robot pose. No lambda, template, exception or output parameter. No violations found.

Milestone 6 evidence (2026-10-17). The completion search prints nothing outside `src/geometry/utils.cpp` (where `jsonValueToPoint` is now file-local), and `git grep -n kr10.json -- . ':!codex_plans' ':!docs' ':!old_imp'` prints nothing, `python/` included:

    git grep -n -e "RobotDefinition::fromJson" -e modelFile -e kr10.json -e jsonValueToPoint -- src resources

Values transcribed from the deleted `kr10.json` (offsets in mm and Z-Y-X degrees, signed local Z axis, limits in degrees, rated speed in degrees per second):

    J1   0      0     227    | 0  0   0  | -1 | -170..170 | 220
    J2   150   -90.4  223    | 0  0 -90  | +1 | -185..65  | 210
    J3   0     -610  -11.1   | 90 0   0  | +1 | -137..163 | 270
    J4  -20    -357   101.5  | 0  0 -90  | +1 | -185..185 | 381
    J5   0     -39   -303    | 0  0  90  | +1 | -120..120 | 311
    J6   0     -80   -39     | 0  0  90  | -1 | -350..350 | 492
    IkParams sx 150, sz 450, a 610, bx 20, by 660, dF 80
    qHome (0, -90, 90, 0, 0, 0)   pHome (890, 0, 1080, 0, 90, 0)
    links 0_BASE, 6_WRIST3 black (0,0,0); 1_ROTATING_COLUMN .. 5_WRIST2 orange (255,106,0); all .brep

Edited files, physical/nonblank before → after:

    robot/robotdefinition.h 20/14 → 16/12            robot/robotdefinition.cpp 321/243 → 62/54
    robot/robotcontroller.h 60/48 → 60/48            robot/robotcontroller.cpp 192/170 → 182/160
    assetpaths.h 16/11 → 15/10                       assetpaths.cpp 18/14 → 17/13
    resources/json.qrc 6/6 → 5/5                     geometry/utils.cpp 308/239 → 304/237
    geometry/include/geometry/utils.h 43/34 → 42/33  resources/json/kr10.json 90 → deleted

Active C++ scope: 96 files, 12984/10905 → 12704/10699 (−280/−206). `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` reports only Git's `autocrlf` notices. Rewritten files were converted back to CRLF working-tree endings.

Why the model is numerically identical: each `JointModel::localTransform` is computed by `makeTranslation(V3d{dx, dy, dz}) * makeRotation(da, Z) * makeRotation(db, Y) * makeRotation(dc, X)`, the same calls and order as the former `readJoint`. The decimal literals (`-90.4`, `-11.1`, `101.5`) are correctly rounded to the same doubles that Qt's JSON parser produced, and the integers are exact. The axes `(0, 0, ±1)` equal the former `normalize` result exactly. Colours are the same integers.

What was removed or simplified: `RobotDefinition::fromJson` with its eight file-local readers and their `qWarning`s; the unused `RobotDefinition::name`; `ViewportAssets::modelFile`; the "Cannot read the robot definition" and "invalid CAD filename" branches of `RobotController::loadRobot` (both checked constant data); the public `jsonValueToPoint` declaration (now file-local, its only remaining caller being `jsonArrayToPoints` in the same file). `RobotController::m_model` is now `const RobotDefinition m_model = RobotDefinition::kr10();`. `Kr10Kinematics` still borrows `m_model.kinematics`, and copy/move stays disabled.

What still guards the data: `RobotController::initialize` (`Kr10Kinematics::isValid()` and FK(qHome) against pHome within 1e-4 mm and 1e-6 per rotation entry) at robot load, and `validRobot` in `chamfermotion.cpp` and `chamfercentralvalidation.cpp` (rigid joint transforms, ±Z axes, ordered finite limits, positive speeds) at Generate.

Compliance review. New declarations: `RobotDefinition::kr10()` (no parameters), replacing `fromJson`; the file-local aggregate `JointRow` with one `constexpr std::array` table of six rows. These are required because the six joint rows are a data mapping, which `AGENTS.md` allows as a table, and no existing structure holds them. Bodies: no lambda, template, exception or output parameter. Callers: `RobotController` is the only user of `RobotDefinition` (the profiler was removed in Milestone 3). No violations found.

Milestone 7 evidence (2026-10-17). The completion search prints nothing:

    git grep -n -e ScenePlaneGeometry -e SceneCylinderGeometry -e SceneEdgeGeometry -e addObject -e "addPlane\b" -e setPlaneGeometry -e setCylinderGeometry -e setEdgeGeometry -e PlaneGeometry -e CylinderGeometry -e "m_geometry\b" -- src/application

Edited files, physical/nonblank before → after:

    scene/sceneobject.h 83/73 → 89/79              scene/sceneobject.cpp 188/171 → 190/174
    scene/scenegeometry.h 227/207 → 120/114        scene/scenemodel.h 63/55 → 58/50
    scene/scenemodel.cpp 257/227 → 218/192         scene/scenemachining.cpp 420/391 → 420/391
    backendqmltypes.h 226/196 → 202/175            qml/Views/Workspace/PropertiesPanel.qml 408/354 → 313/272

Scope totals: active C++ 12704/10699 → 12537/10554 (−167/−145); QML 3136/2643 → 3041/2561 (−95/−82). `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` reports only Git's `autocrlf` notices; edited files keep CRLF.

What changed and how:

- `scenegeometry.h`: `ScenePlaneGeometry`, `SceneCylinderGeometry` and `SceneEdgeGeometry` were deleted, along with the coefficients-only plane mode and the unused `curveKind`/`sourceIds` QML properties. Added `struct SceneEdge { EdgeGeometry edge; std::array<quint32, 2> sources{}; }`.
- `SceneObject`: `Shape m_shape` (`std::variant<std::monostate, BoundedPlane, BoundedCylinder, SceneEdge>`) plus `SceneMachiningPath* m_machiningPath`. `plane()`, `cylinder()` and `edge()` are `std::get_if` lookups. `edge()` now returns `const SceneEdge*`, so its one caller outside the class, `SceneMachining::sourceParameters`, reads `edge->sources` and `edge->edge` directly. One private `setShape(Shape)` replaces the three geometry setters. `geometry()` returns the machining-path QObject or null. `drawing()` and `drawingDiagnostics()` keep their arithmetic; only their accessors changed.
- `SceneObject::properties()` (Q_PROPERTY, CONSTANT, because geometry is set before the object is published) returns the plane rows Normal, Origin, Width, Height, Points, Offset (d) or the cylinder rows Origin, Axis, Radius, Diameter, Length, Points, and nothing for other kinds. These are the same labels and order the QML showed for bounded planes (coefficient-only planes no longer exist). `formatReal` reproduces the QML rule: |v| < 0.00005 shows `0.0000`, otherwise 4 decimals. `formatVector` gives `[x; y; z]`, and point counts are integers. The only possible difference is rounding of an exact binary tie in the fifth decimal (values that are odd multiples of 1/32). There, QML's `toFixed` rounds half up and `QString::number` may not.
- `SceneModel`: removed `addPlane(coefficients)` and `addObject` (no callers) and the `<cmath>`/`QVariantList` includes they needed. `addBoundedPlane`, `addBoundedCylinder` and `intersect` call `setShape`.
- `backendqmltypes.h`: removed the three geometry wrapper registrations.
- `PropertiesPanel.qml`: removed `planeGeometry`, `cylinderGeometry`, `formatReal`, `formatVector`, the `valueObjectName` alias and the twelve per-field rows. Added `geometryRows` and one `Repeater` whose `PropertyValue` delegate declares `required property var modelData`. The fixed label-width list is unchanged, so the label column does not change width with the selection.

Compliance review. New declarations and why they are needed:
- `SceneEdge`: an edge needs its two source IDs, and no existing value pairs them.
- `SceneObject::Shape`: a plain alias of one `std::variant` instantiation, not a project template.
- `setShape`: replaces three setters.
- `properties()` with three file-local helpers (`formatReal`, `formatVector`, `propertyRow`): they replace twelve QML rows and two QML functions, and nothing else formats these values in C++. Each helper has one or two parameters.

Result channels are return values. Ownership: `SceneMachiningPath` stays a child of its `SceneObject`, and the variant values are owned by value. No lambda, template, exception or output parameter. No violations found.

Milestone 8 evidence (2026-10-17). The registration macros in `backendqmltypes.h` were extracted before and after the change and compared: 37 macro lines, identical (same 14 `QML_FOREIGN` wrappers, names, `QML_SINGLETON`, `QML_UNCREATABLE` messages and value-type names). `WorkspaceController` keeps `QML_NAMED_ELEMENT(WorkspaceController)` and `QML_SINGLETON`. `git grep -n "s_engine\|must be used by one GUI-thread" -- src` now finds only the shared helper and the viewport module's own `OccControllerQml`, which is out of scope.

Edited files, physical/nonblank before → after:

    backendqmltypes.h 202/175 → 158/141      workspacecontroller.h 67/58 → 66/57      workspacecontroller.cpp 114/98 → 111/95

Active C++ scope: 12537/10554 → 12489/10516 (−48/−38). `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` reports only Git's `autocrlf` notices; the rewritten header was converted back to CRLF.

What `claimQmlSingleton(instance, engine)` does, in order:

1. It calls `qFatal` if `instance` is null, if the engine's thread differs from the instance's thread, or if a different engine that is still alive has already claimed a singleton.
2. It records the engine in one function-local `QPointer<QJSEngine>`.
3. It sets `QJSEngine::CppOwnership` so the engine never deletes the `main()` stack object.
4. It returns `instance`.

Each `create()` keeps the signature `T* create(QQmlEngine*, QJSEngine*)` that `QML_SINGLETON` requires and casts the result back to its own type. Behavior differences, all only on misuse:

- `Scene` and `Hub` used `Q_ASSERT`, so release builds did not check them; now they `qFatal` like the rest.
- The single engine record is shared by all singletons and, like the former `QPointer` copies, is released when that engine is destroyed. The former raw-pointer records for `Scene` and `Hub` were never released.
- The fatal message now names the C++ class (for example `SceneMachining`) instead of the QML name.

Compliance review. One new function, needed because six copies of the same check had diverged. It has two parameters and returns its result. The function-local static `QPointer` replaces six static engine records. `WorkspaceController::s_engine` was removed. No registration, ownership or lifetime order changed. No lambda, template, exception or output parameter. No violations found. Validation still pending: a build with `USE_HOT_RELOAD=OFF` plus `robocrap_qmllint`, both only on the user's request.

Milestone 9 evidence (2026-10-17). `git grep` for calls to the removed `OccViewWindow` methods (`insertObject`, `removeObject`, `setObjectVisible`, `setObjectDiagnostics`, `setSelectedObjects`, `setDraftAxis`, `setSpindleFrames`) through `m_window->` or as `OccViewWindow::` definitions prints nothing. Edited files, physical/nonblank before → after:

    occt/occviewwindow.h 73/61 → 66/54             occt/occviewwindow.cpp 271/237 → 225/198
    occcontroller.cpp 267/237 → 283/252            include/viewport/occcontroller.h 161/138 → 164/141

Active C++ scope: 12489/10516 → 12455/10488 (−34/−28). `git diff --name-only -- src/network src/pathgeneration src/scene` prints nothing; `git diff --check` reports only Git's `autocrlf` notices; `occcontroller.cpp` was converted back to CRLF after a `sed` edit.

What changed and why behavior is unchanged:

- `OccViewWindow(const DrawingDocument&, const DrawingPose&)` borrows the controller's accepted document (as before) and pose (new). Its `viewport()` getter returns the current `OccViewport*`, or null while no native surface exists. Removed: the seven forwarding methods, the `pose` parameter of `setScene`, and the window's own `DrawingPose` copy.
- `OccController::viewport()` (private) looks the viewer up on every call and never stores it, because the viewer is destroyed with the native surface. Each operation keeps the former "no surface → accepted, nothing drawn" result. After a successful insert or visibility change, and after removal, the controller re-applies `m_document.selected`, in the same place the window did. The order is unchanged: draw, then re-apply selection, then update the accepted document.
- `OccViewWindow::applyTransforms` no longer copies the accepted pose. `OccController::applyRobotTransforms` assigns `m_pose` only after acceptance, so the borrowed `m_pose` is still the last accepted pose when a failed update is restored once.
- One timing detail. If the native surface appears after `loadRobot` but before the CAD worker finishes, `initializeViewport` now passes the controller's TCP/collet frames to the new viewer instead of the window's then-empty copy. `OccRobotAdapter::synchronizeSpindleFrames` shows those axes only after the robot is installed, and installation (`setRobot` with the same pose) applies them anyway. The visible result is the same.

Compliance review. New declarations:
- `OccViewWindow::viewport()` replaces seven forwarding methods.
- `OccController::viewport()` (no parameters) replaces seven repeated null-checked window calls.
- The window constructor now takes two borrowed references; the document and the pose are the two pieces of accepted state it replays.

No ownership change: the window still owns `OccViewport` and is destroyed before the controller's state. No lambda, template, exception or output parameter. No violations found. Validation still pending, and only on the user's request: build, then check that objects, selection, draft axis, TCP/ER axes and the robot reappear after native-surface recreation.

Milestone 10 evidence (2026-10-17). Documentation updated in place, without completion reports:

- `docs/AGENT_CONTRACTS.md`:
  - Build and modules: singleton registration through `claimQmlSingleton`; the Qt 6.8 limit on `setExternalSingletonInstance`; RobotController no longer a QML type.
  - Viewport and CAD: borrowed pose and direct `OccViewport` calls; fixed tools with no Browse/Retry; `RobotDefinition::kr10()`; BREP-only loading of nine files; no STEP, cache or TKDESTEP.
  - Geometry and scene: value geometry and the `properties` rows; no circle import; no manual FK/IK.
  - Machining and playback: the 800000 limit is internal; no profiler or timing output; the one-line Generate log.
- `docs/ARCHITECTURE.md`:
  - §2 drawing/assets; §3.1 (no `Circle`, KR10 table); §3.2 (`SceneObject`, `scenegeometry.h`, singletons); §3.3 (solver, arc limit, no diagnostics, no profiler, RobotController);
  - §3.5 (OccController/OccViewWindow/OccRobotAdapter/CadLoadWorker/RobotDefinition/ViewportAssets); §3.6 (WorkspaceController collaborators, resources, Properties panel);
  - §4, §5.1, §5.2 and §6/§7 entries for STEP, Browse/Retry, the BREP cache and singleton checks.
  - The header notes that some `file:line` references in unchanged paragraphs predate the reduction.
- `AGENTS.md` was not edited: it contains the user's own uncommitted changes. Its routing table still says "CAD/cache loading", which now only means CAD loading.

Final audit commands, all run from the repository root:

    every milestone completion search (M1–M9)               -> clean
    git diff --name-only -- src/network src/pathgeneration src/scene   -> (empty)
    git status --short --untracked-files=all -- src/network src/pathgeneration src/scene -> (empty)
    git diff --name-only -- src/devices/plc src/devices/include/devices/plc            -> (empty)
    git diff --check                                         -> only Git autocrlf notices
    git ls-files --others --exclude-standard                 -> codex_plans/CODE_REDUCTION_EXECPLAN.md
    deleted tracked files: resources/json/kr10.json, resources/pics/circ.svg, resources/pics/open.svg,
      src/application/chamferprofile.{h,cpp}, src/geometry/circle.cpp, src/geometry/include/geometry/circle.h,
      src/viewport/occt/cachedshapeloader.{h,cpp}, src/viewport/occt/stepimporter.{h,cpp}

`git diff --shortstat` reports 73 files, 567 insertions, 3032 deletions. These figures include documentation and the user's own pre-existing uncommitted edits (`AGENTS.md`, two SVGs, parts of `Main.qml`, `MainMenuBar.qml`, `EndEffectorPanel.qml`), which were preserved.

New items to be justified at implementation (each already justified above): `RobotVisualModel::tools` (M4), `SceneEdge` and `SceneObject::properties` with two formatting helpers (M7), `claimQmlSingleton` (M8), `OccViewWindow::viewport` (M9), `RobotDefinition::kr10` with its joint table (M6, replacing `fromJson`).

## Interfaces and Dependencies

At the end of the plan these signatures exist (C++17, Qt 6.8):

    // src/viewport/include/viewport/robotgraphics.h
    struct RobotVisualModel { QList<LinkModel> links; QStringList tools; };

    // src/viewport/occt/cadloadworker.h
    CadLoadWorker(QStringList sourcePaths);              // absolute .brep paths

    // src/viewport/include/viewport/occcontroller.h
    OccController(QString occtResources, QObject* parent = nullptr);
    void setActiveTool(RobotTool tool);                  // unchanged name, now only toggles visibility

    // src/application/robot/robotcontroller.h
    RobotController(OccController* viewport, SceneEndEffectors* effectors, SceneMachining* machining);
    ViewportOperationResult loadRobot();
    ViewportOperationResult applyPlayback(const V6d& joints, const M4d& flangeToTcp);

    // src/application/robot/robotdefinition.h
    static RobotDefinition RobotDefinition::kr10();

    // src/application/workspacecontroller.h
    WorkspaceController(SceneModel* scene, SceneMachining* machining, SceneEndEffectors* effectors);

    // src/application/scene/sceneobject.h
    const BoundedPlane* plane() const; const BoundedCylinder* cylinder() const;
    const EdgeGeometry* edge() const; std::optional<std::array<quint32, 2>> edgeSources() const;
    QVariantList properties() const;                     // Q_PROPERTY properties, CONSTANT

    // src/application/qmlsingleton.h
    QObject* claimQmlSingleton(QObject* instance, QJSEngine* engine);

    // src/viewport/occt/occviewwindow.h
    OccViewWindow(const DrawingDocument& document, const DrawingPose& pose);
    OccViewport* viewport() const;

Dependencies removed: the OCCT `TKDESTEP` library (STEP translation) from `RoboCrap3D` and from deployment; the runtime file `:/json/kr10.json`. No dependency is added, and no minimum version changes.

Revision note (2026-10-17): initial plan written from the user's selection of reduction proposals A1–A6 and B1–B4. B5 is excluded, and `SET_VAR` is kept, which also keeps `writeMessage`.

Revision note (2026-10-17, completion): Milestones 1–10 are implemented in source. Milestone 8 was executed as Option A, and its Plan of Work section is marked as describing the unexecuted Option B. Final counts and the outcome are recorded in "Outcomes & Retrospective"; build/lint/runtime validation remains pending a user request.

Revision note (2026-10-17, second revision): the user decided not to execute B3. The PLC-command milestone and its Decision Log entry were removed, PLC code is now explicitly out of scope, the documentation/audit milestone was renumbered from 11 to 10, and the estimated total fell from about 2,280 to about 2,210 lines. B1, B2 and B4 remain in scope; B5 remains excluded.
