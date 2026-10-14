# RoboCrap — agent instructions

## Mandatory explanation style

- Answer the exact question first. When asked for the next step, explain only that step.
  Be concise by removing repetition, never by omitting the mechanics needed to understand it.
- Be concrete: identify the inputs and where they come from, the actual operations in order,
  and the result and where it is used. Do not substitute abstract labels such as "adaptive",
  "validation", or "decoupling" for an explanation of what the code actually does.
- Use established mathematical and project terminology; do not invent terminology. Define
  every new symbol, index, technical term and quantity immediately, including units where relevant.
  Do not use arrows as a substitute for explaining a calculation or relationship.
- Always denote vectors in explanations with lowercase, regular (non-bold) letters and an overbar,
  for example `\bar{p}`, `\bar{k}`, `\bar{r}`. Do not use uppercase or bold letters or arrow accents
  for vectors.
- Always denote axis unit vectors with lowercase e, an overbar, and the axis subscript x, y,
  or z: `\bar{e}_x`, `\bar{e}_y`, `\bar{e}_z`.
- In formulas, always show all arguments on which each function depends; never omit them
  for brevity (for example, write `\bar{p}_M(\theta)`, not `\bar{p}_M`). Show dependent
  arguments explicitly: if `\theta=\theta(u)`, write `\bar{p}_L(u,\theta(u))` when using
  the two-argument definition, and `\bar{p}_M(\theta(u))` inside it. Distinguish independent
  arguments from dependent quantities and explicitly identify any fixed parameters.
- In Russian answers, use accepted Russian engineering terminology, not literal translations.
  A robot joint is "сочленение", never "сустав". Check unfamiliar terms before using them;
  if uncertain, retain and define the original technical term rather than inventing a translation.
- For proposed changes, state exactly what happens now and what calculation will replace it.
  For failures, identify the failing operation, its inputs, the rejected condition and the reason.
  Ground implementation claims in inspected code; explicitly distinguish uncertainty from fact.
- Use a concrete calculation or example when explaining an algorithm. Prefer actual project
  values; label hypothetical values. Distinguish computation time from movement duration,
  stored animation points from temporary checks and RSI samples, and measurements from estimates.
- Before every answer, check for undefined terms, ambiguous references and missing calculation
  steps. Correct them before replying. These requirements take precedence over default brevity;
  do not make the user repeatedly request specifics or repeat an explanation they already accepted.
  Re-read this section after it changes or context is lost. Explicitly distinguish existing code from
  proposals; explain how example intervals are selected and how many values each operation returns.

## Mandatory C++ style

- Do not use C++ lambdas; use descriptively named helper functions or member methods.
- No project test logic is needed; do not add tests, test harnesses, or test build targets.
- Use `try`/`catch` only to handle exceptions originating in external-library calls, such as
  OCCT's `Standard_Failure`, including standard C++ exceptions raised by those calls. Preserve
  necessary recovery and cleanup; a bare `throw;` may rethrow the same external exception after
  cleanup. Do not define project exception types or throw new exceptions from project code,
  including instances of library exception types. Report expected project failures through
  return values, using existing optional/result conventions.
  This rule does not authorize disabling compiler exception support or changing third-party code.
- Return results through the function return value, never through output or input/output parameters.
  Do not use references, raw pointers, or smart pointers as result channels, including shared result
  objects populated by a worker. Return a cohesive value containing the result and error information
  when both are needed. Input references/pointers and explicit operations on an existing object
  (such as setters or scene insertion) are allowed; do not disguise an output parameter as mutation.
- Functions and constructors must have at most four parameters. Prefer zero to two; three or four
  require a concrete responsibility/API justification, not caller convenience. Group values only
  when they form a meaningful concept; do not introduce parameter bags merely to satisfy the limit.
- Access another object's private state only through its methods: getters for reads and setters
  or named operations for changes. Do not assign fields such as `object->m_geometry`, including
  through friendship. A class may initialize and update its own members inside its implementation.

## Scope and workflow

- Current machining work is limited to animation preparation and playback. Do not change
  or run RSI preparation unless the user explicitly reopens that scope.

- Read the files you will change and inspect affected callers before changing behavior or an API.
  For a changed QML component or component use, inspect its definition and custom base types;
  verify properties, signals, default content, and sizing instead of inferring them from examples.
- Keep changes within the requested task. Preserve unrelated user edits and local formatting.
  Reuse suitable project/Qt helpers; add abstractions only for a current requirement.
- Exclude directories named "build", "libs" and "python" and directories whose names start with old (such as old_imp)
  from routine searches and inspections, unless the task explicitly targets them.
- Preserve public/protected C++ APIs, QML APIs/module URIs, dependencies, and version requirements
  unless the task requests or requires a change. Explain necessary changes and update affected callers.
  If the requested design cannot work, explain the verified conflict and a concrete alternative.
- Git operations that stage, commit, switch branches, stash, rewrite history, or discard changes
  require an explicit user request. This also applies when following PLANS.md.
- For complex features or significant code refactors, read PLANS.md and maintain an ExecPlan.
  Simple edits, documentation-only work, and read-only analysis do not need an ExecPlan.
- All task plans and strategy documents (including ExecPlans) must be stored in the
  repository-root `codex_plans/` folder, never in the repository root or other folders.
- Edit source inputs, not generated build outputs. Treat libs/eigen-5.0.0 as vendored code and
  CMakeLists.txt.user* as local IDE settings; change them only when the task specifically requires it.
- Update only affected rules and architectural constraints in this file. Do not add task history,
  completion reports, tutorials, or descriptions that repeat implementation details.

## Build facts and source boundaries

- This is a Windows Qt Quick application: C++17, Qt 6.8 minimum, CMake 3.16 minimum, bundled Eigen.
  Root and module CMakeLists.txt files are authoritative for requirements, sources, and options.
  Keep compatibility with those minimums; local SDK versions do not authorize raising them.
- Root CMakeLists.txt owns robocrap, application QML, and resource lists. src/CMakeLists.txt owns
  RoboCrapGeometry (geometry, KR10 numerical kinematics and path-generation sources), RoboCrapScene
  (scene objects and synchronous importer), RoboCrapDevices (device infrastructure, concrete
  devices and protocols), and RoboCrapBackend (backend QML registration only). Scene and Devices
  link Geometry; Backend links Scene and Devices. QmlChartBridge compiles in robocrap.
  Geometry publicly supplies Eigen because its public headers expose Eigen types. Scene and
  Devices export Qt metatype metadata for backend foreign registration. RDTResponse retains
  its application-module QML registration through qt_generate_foreign_qml_types.
- src/3d/CMakeLists.txt owns RoboCrap3D (URI RoboCrap.Viewport3D), the independent OCCT preview.
  It requires the bundled OCCT 8.0.0 SDK and a Windows 64-bit MinGW Qt kit. Its public controller
  header does not expose OCCT math types; internal OCCT code uses namespace RoboCrap3D.
  RoboCrap3D links RoboCrapScene rather than RoboCrapBackend and publicly links RoboCrapGeometry.
  Kr10Kinematics compiles in RoboCrapGeometry and uses the shared Eigen types in geometry/mathtypes.h.
  OccRobotAdapter converts numerical transforms to gp_Trsf; OccScene and OccPart retain OCCT types.
  RobotViewport.qml hosts its native QWindow; CAD parsing runs on CadLoadWorker instances, while
  robot state, QML properties, and OCCT presentation remain on the GUI thread.
  viewportReady reports an initialized viewer independently of robot assets; ready still reports
  robot-preview readiness for kinematics. Robot attachment replaces only robot parts and preserves
  application presentations. Initial camera fitting stops after a camera gesture on that surface.
  Decorative axes must remain non-selectable and excluded from camera fit bounds.
  Kr10Model groups Kr10KinematicModel and RobotVisualModel from one JSON definition; Kr10Kinematics
  receives only Kr10KinematicModel, while robot presentation receives RobotVisualModel.
  CadLoadWorker takes absolute source paths and a cache directory and returns ordered shapes/errors;
  requests serialize OCCT parsing/cache access. Robot loading includes only the seven links.
  Joint data and fixed dimensions share kr10kinematicmodel.h; visual records and the complete
  model share kr10model.h.
  CadLoadResult is worker-independent data. RobotPreviewState owns the model and current IK seed;
  it creates read-only RobotPose candidates, and OccController commits them after presentation
  succeeds. OccViewWindow retains const access to preview state for restoration.
  resources/json/kr10.json is embedded by json.qrc. cmake/DeployOcct.cmake stages OCCT DLLs,
  runtime resources, and resources/cad/kr10 beside the executable and during installation.
  Generated BREP files belong in applicationDirPath()/resources/cad/kr10 beside deployed CAD;
  CAD loading does not use QStandardPaths system cache folders. CachedShapeLoader accepts STEP/STP
  basenames in a selected source directory and first reads <completeBaseName>.brep from the deployed
  CAD directory, without requiring or hashing the STEP source. Missing/unreadable/invalid BREPs
  fall back to STEP conversion and are saved atomically with QSaveFile. A valid BREP takes priority;
  STEP/content/OCCT-version changes require removing that BREP to regenerate it. Cache identity
  is the source basename, so sources with the same basename share the same deployed BREP.
- Preserve both USE_HOT_RELOAD paths and FAST_QML_BUILD behavior; verification policy is below.
- old_imp/ is outside the active CMake build; python/ contains analysis scripts and data.
  Do not treat either as the current application implementation or an automated test suite.

## Device and C++/QML contracts

Apply these contracts when changing devices or their UI integration, unless the task explicitly
redesigns the mechanism. Start with src/network/{devicehub,devicerunner,abstractdevice}.{h,cpp}
and src/main.cpp.

- QML reaches devices through RoboCrap.Backend's Hub singleton: Backend.Hub.device(key) returns
  a DeviceRunner on the application/QML thread. Keep its QQmlPropertyMap on that thread too.
- DeviceHub::add() takes a parentless AbstractDevice and moves it to its I/O thread. Currently
  FTS and RSI share ControlIO; PLC uses GeneralIO. The FTS-to-RSI signal in main.cpp relies on
  that arrangement. Recheck connection behavior and data races before changing device groups.
- Runner requests use queued startReq, stopReq, and invokeReq signals. Never call device methods
  directly from QML's thread. AbstractDevice's internal direct meta-call already runs on its I/O thread.
- Create, use, and destroy device sockets/timers on their owning I/O thread. Follow startDevice()
  and stopDevice(); preserve DeviceHub's thread-finished/deleteLater device cleanup.
- Shutdown must finish device stop() calls before quitting and waiting for I/O threads.
  stopAll() currently uses BlockingQueuedConnection from outside both I/O threads; calling that
  connection within the receiving thread deadlocks. Preserve shutdown ordering and repeat-call safety.
- Publish value state through stateReady -> DeviceRunner::onStateReady -> data, and socket state
  through socketStateReady -> onSockState -> socketState. Preserve keys consumed by QML and notify
  bindings when values change. Keep QML-facing model mutations on the model's owning thread.
- Device diagnostics use `qInfo().noquote()` directly on the device's owning thread, retaining
  the message's category label, device name, and text. Viewport diagnostics use Qt logging too;
  main.cpp configures Qt's timestamp format. Keep logging independent of QML state and avoid
  introducing a logging singleton, message type, or GUI-thread logging relay.
- For FTS recording/saving, preserve device-thread socket/timer ownership, copied-input
  nonblocking file work and completion of outstanding saves before device destruction.
- Methods dispatched by DeviceRunner::invoke(name, args) must be public Q_INVOKABLE methods on
  an AbstractDevice-derived class, return void, and take zero arguments or one QVariantMap.
  Names must be unique: buildApi() indexes by name, not overload signature. Inspect string call sites.
- Register C++ QML types through src/backendqmltypes.h and src/CMakeLists.txt, following
  the existing QML_FOREIGN pattern. Preserve Hub's C++ ownership, engine/thread checks, and lifetime.
  Use typed registration for new exposure; do not add context properties for diagnostics.
- Keep device/protocol processing and substantial numerical work in C++. Avoid blocking the GUI
  or adding blocking work to ControlIO's receive callbacks. Keep single-file helpers/constants local
  to an unnamed namespace; match the edited file's naming, braces, and indentation.

## QML components and geometry

- Measuring EE and Machining EE toolbar actions open their settings in the right-hand
  Properties area; Close restores object properties. End-effector numeric fields display four
  decimal places and accept at most four fractional digits. Read-only ER display rounding
  does not change the stored calibration.

- Do not add explanatory or tutorial labels to the UI. Use concise field names, units,
  status messages and actionable errors; keep explanations in documentation or chat.

## Machining and scene boundaries

- SceneMachining exposes SceneMachiningSettings through the QML property settings.
  The Generate button calls Backend.Machining.generate(), implemented by
  SceneMachining::generate(). Keep generation separate from dryRun(), which only plays
  an already prepared immutable ChamferMotion. End-effector Apply actions are separate.
- Numerical chamfer geometry, arc-length conversion, timing, IK validation and joint
  playback live in src/pathgeneration/chamfer. Read the relevant producer and consumer
  before changing a stage: chamferpath, chamferarclength, chamfercentralmotion,
  chamfercentralvalidation and chamfermotion. Numerical code must not invoke scene/device APIs.
- Preserve the distinction between geometric parameter, traveled distance and physical time.
  Use the analytical curve to recover TCP position/orientation from the inverse arc-length
  parameter; do not replace it with interpolation of stored XYZ/ABC without an explicit redesign.
  Keep inverse-map knots, animation points and RSI cycles separate in code and explanations.
- Determine central animation points and times before IK from the curvature/time criterion.
  Do not add central points or automatically stretch movement time after IK/rate failures.
  Straight LIN also fixes points before IK: N=max(1,ceil(T*sqrt(a/(8*0.01mm)))) per
  constant-acceleration span; cruise has one interval. No recursive subdivision or extra
  quarter-time IK checks. This position criterion does not bound orientation or actual TCP
  error after joint interpolation. Preserve exact shared
  phase endpoints and continuous joint winding; do not wrap saved angles modulo 360 degrees.
- Prepare independent clamped joint splines for each LIN and one shared central spline during
  Generate, with zero endpoint joint velocities. Check polynomial position/speed extrema;
  failures reject the candidate without adding points or retiming. Dry Run must not perform IK, integrate
  geometry, construct inverse maps/splines or pause to prepare more points. The displayed TCP
  between saved points is an approximation; do not claim an exact Cartesian error guarantee
  from joint-spline smoothness. Curvature derivation: codex_plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md.
- Take rated joint-speed bounds from the robot model; do not introduce fixed simulation caps.
  Configured accelerations are simulation assumptions, not verified KUKA controller ratings.
  Temporary user-requested exception: joint-acceleration magnitude rejection is disabled in
  central preview validation, LIN validation (including terminal stops), and playback spline
  checks. Keep acceleration finiteness, joint position/speed and central XYZ acceleration
  checks. HOME timing still uses configured joint acceleration; RSI checks are separate.
  Do not restore or extend this exception without a user request.
- Generate workers own copied numerical inputs and return cohesive results through futures.
  Accept results and insert scene objects only on the GUI thread; reject stale source/model/TCP
  revisions. Publish only complete motions; replace an old object only after inserting its
  successful replacement, preserving name/visibility. Wait for remaining workers on shutdown.
- Saved paths own immutable geometry/model/TCP/settings snapshots and survive source deletion.
  Model or TCP calibration changes invalidate playback until regeneration. Preserve pause/resume,
  Stop-to-HOME, completion-at-HOME and cleanup on readiness loss, mode change or path deletion.
  Selection must not replace a running motion; generation/manual robot edits are blocked during playback.
- Keep calculation timings distinct from movement durations and nested timing totals distinct
  from disjoint stage timings. Diagnostics must retain attempted-stage failures and distinguish
  inverse-map knots from saved animation points. The preview-only profiling input is
  codex_plans/chamfer_profile_spatial_leads_input.json; previewOnly=true skips RSI preparation.
- RSI preparation and execution are different from animation and from controller readiness.
  For explicitly authorized RSI work, start with resources/files/rsi/RSI_Hole_CONTRACT.md
  and RSI_Hole_DIAGRAM.md, then inspect the current encoder, sampler and coordinator.
  Numerical/wire checks do not establish installed controller compatibility. The PC protocol
  and minimal controller XML still require alignment; nominal execution and force correction
  are not connected. Do not silently treat preparation success as permission to execute.
- Preserve queued immutable prepared-job delivery to ControlIO, stale-result rejection and
  job invalidation on regeneration/calibration changes. Keep numerical preparation and file I/O
  outside receive callbacks. Shared-file publication is not controller readiness; a previous
  disk file can remain after invalidation. Read the actual shared-data/KRL contract before edits.
- SceneEndEffectors owns calibration on the GUI thread. Compose FLANGE_ER * ER_TCP in that
  order; displayed rounding must not alter stored calibration. Commit coupled calibration
  values before notifications; unchanged values must not invalidate paths. Preserve the
  distinction between flange-based FK/IK APIs and TCP targets converted using tool calibration.
- SceneModel owns SceneObject and its geometry; QML changes scene state through model methods.
  Preserve stable scene-allocated IDs (zero means no object), shared references and GUI-thread
  mutations. Keep numerical fitting/intersection in Geometry and OCCT presentation in adapters.
  Decorative points/normals/axes must not participate in picking or camera fit bounds.
  Before changing imports or intersections, read the numerical geometry factories and
  SceneSurfaceImporter/SceneModel callers; preserve units and probe-compensation semantics.

## Reusable QML components

- Reuse compatible controls from qml/Modules/Components and values from the Styles module's
  Colors, Fonts, and Metrics singletons.
- Express a component's natural preferred size with implicitWidth/implicitHeight. Preserve useful
  inherited implicit sizing; for styled Controls, check contentItem/background sizes, padding, and
  insets before overriding the control's implicit size. Item wrappers may need their own implicit size.
- A Qt Quick Layout owns its immediate children's geometry. Use Layout.* and implicit sizes there;
  do not also drive those children's x/y/width/height or anchors. Anchors on the layout itself are
  valid when its parent is not another layout. Check size dependencies for parent/child binding cycles.
- Give reusable components explicit input properties instead of reaching into caller IDs/context.
  Use a concrete QML type when known; retain var for genuinely heterogeneous data. Use required
  properties for mandatory inputs with no valid default, updating every creator when adding one.
- In delegates with required properties, explicitly declare every consumed model role and relevant
  view-provided value: index/model/modelData, or TableView row/column. Qualify accesses through IDs.
  Persistent application state belongs in the model/backend, not in disposable view delegates.
- Qualify root properties from children (root.title); declare signal parameters explicitly
  (onActivated: index => ...). Keep simple handlers inline; extract named functions for reused or
  substantial logic when that improves clarity. Preserve bindings when updating state.

## CMake, modules, and resources

- Preserve module URIs: robocrap_qml_module, RoboCrap.Backend, RoboCrap.Viewport3D, Components, Styles.
  When a file/type moves or changes name, update its owning CMake source list and all affected imports.
- For Components/Styles exports, update both the module CMakeLists.txt and source qmldir.
  These two qmldir files are maintained hot-reload inputs, not disposable generated files.
  For QML singletons, keep pragma Singleton, QT_QML_SINGLETON_TYPE, and qmldir declarations consistent.
- Update the owning .qrc (or qt_add_resources declaration) and affected resource URLs when assets move.

## Numerical and device behavior

- For geometry/path changes, inspect mathtypes.h, geometry/utils.cpp, and the affected producer and
  consumer. Preserve frame order X/Y/Z/A/B/C, degree-based Euler APIs and Z-Y-X composition,
  existing units, tolerances, optional/error results, and trajectory timing unless the task changes them.
  Check failed optional results before using them; validate finite and degenerate inputs as appropriate.
  Initialize Eigen transforms explicitly to identity and persistent vectors to zero. Robot joint
  rotations use the arbitrary-axis makeRotation overload without small-component cleanup; preserve
  the existing principal-axis overload's cleanup for trajectory callers. inverseRigidTransform
  assumes orthonormal rotation with no scale/shear. KR10 tolerances remain 1e-4 position and 1e-6
  rotation entries; its wrist singularity threshold remains GeomConst::Eps (1e-9).
- For protocol changes, preserve or explicitly update framing, byte order, state keys, and endpoint
  validation together with their consumers. Validate changes using offline inputs or local simulation.
- Connecting to live PLC/FTS/RSI equipment, writing outputs, biasing a sensor, or starting motion
  requires explicit authorization covering that hardware action. A GUI/build check alone is insufficient.

## Validation and handoff

- Inspect git status and the final diff, including newly created/untracked files. Run git diff --check;
  check new files separately because an ordinary diff does not include untracked contents.
- Run CMake configure/build commands and build-backed QML lint targets only when the user
  explicitly requests build verification. Do not run them by default or ask permission to build.
- Reuse a build directory only after checking CMAKE_HOME_DIRECTORY and compiler/Qt paths in its
  CMakeCache.txt when build verification is requested. Discover the installed matching Qt/compiler
  kit; do not invent absolute SDK paths or reconfigure the user's build to a different mode.
  Use a separate build directory when needed.
- USE_HOT_RELOAD=ON requires Felgo/FelgoHotReload; OFF embeds application QML.
  When verifying application packaging, use OFF. FAST_QML_BUILD=ON skips cache generation;
  check OFF when cache-generation verification is requested.
- When build verification is requested, select checks by the change: build the owning target for
  C++/CMake/resource changes, and robocrap for application integration and concrete-device/geometry/path
  changes. For QML changes, run the owning *_qmllint target: Components_qmllint, Styles_qmllint, or
  robocrap_qmllint (hot reload OFF for application QML). Verify targets in the selected build;
  use all_qmllint for multiple modules. Build for QML registration/resource changes too.
- For UI changes, check affected states, resizing, and binding/import warnings in a disconnected
  runtime when available. Build/lint success does not establish visual or hardware correctness.
  Documentation-only changes need content/diff checks.
- Fix failures introduced by the task. Report pre-existing failures separately; change them only
  when necessary to unblock relevant validation. State checks actually run, results, and skipped
  checks with reasons. For reviews, give verified findings with file/line, impact, and correction.
