# RoboCrap — agent instructions

## Scope and workflow

- Do not use C++ lambdas; use descriptively named helper functions or member methods.
- No project test logic is needed; do not add tests, test harnesses, or test build targets.

- Read the files you will change and inspect affected callers before changing behavior or an API.
  For a changed QML component or component use, inspect its definition and custom base types;
  verify properties, signals, default content, and sizing instead of inferring them from examples.
- Keep changes within the requested task. Preserve unrelated user edits and local formatting.
  Reuse suitable project/Qt helpers; add abstractions only for a current requirement.
- Exclude directories named build and directories whose names start with old (such as old_imp)
  from routine searches and inspections, unless the task explicitly targets them.
- Preserve public/protected C++ APIs, QML APIs/module URIs, dependencies, and version requirements
  unless the task requests or requires a change. Explain necessary changes and update affected callers.
  If the requested design cannot work, explain the verified conflict and a concrete alternative.
- Git operations that stage, commit, switch branches, stash, rewrite history, or discard changes
  require an explicit user request. This also applies when following PLANS.md.
- For complex features or significant code refactors, read PLANS.md and maintain an ExecPlan.
  Simple edits, documentation-only work, and read-only analysis do not need an ExecPlan.
- Edit source inputs, not generated build outputs. Treat libs/eigen-5.0.0 as vendored code and
  CMakeLists.txt.user* as local IDE settings; change them only when the task specifically requires it.
- Update factual statements in this file when an implementation makes them stale; keep research,
  task history, and general tutorials outside the instruction file.

## Build facts and source boundaries

- This is a Windows Qt Quick application: C++17, Qt 6.8 minimum, CMake 3.16 minimum, bundled Eigen.
  Root and module CMakeLists.txt files are authoritative for requirements, sources, and options.
  Keep compatibility with those minimums; local SDK versions do not authorize raising them.
- Root CMakeLists.txt owns robocrap, application QML, and resource lists. src/CMakeLists.txt owns
  RoboCrapBackend and also adds geometry, path generation, and concrete devices directly to robocrap.
  A successful backend-only build does not validate those application sources.
  The plane importer, bounded plane/cylinder numerical data, and existing geometry/plane.cpp
  and geometry/utils.cpp compile in RoboCrapBackend; the trajectory Cylinder and other
  geometry and concrete-device sources still compile in robocrap.
- src/3d/CMakeLists.txt owns RoboCrap3D (URI RoboCrap.Viewport3D), the independent OCCT preview.
  It requires the bundled OCCT 8.0.0 SDK and a Windows 64-bit MinGW Qt kit. Its public controller
  header does not expose OCCT math types; internal OCCT code uses namespace RoboCrap3D.
  RobotViewport.qml hosts its native QWindow; CAD parsing runs on a dedicated worker, while
  robot state, QML properties, and OCCT presentation remain on the GUI thread.
  resources/json/kr10.json is embedded by json.qrc. cmake/DeployOcct.cmake stages OCCT DLLs,
  runtime resources, and resources/cad/kr10 beside the executable and during installation.
  Generated BREP caches belong under QStandardPaths::CacheLocation, not beside source CAD.
- USE_HOT_RELOAD=ON requires Felgo/FelgoHotReload. OFF embeds application QML using
  qt_add_qml_module(). Preserve both paths; use OFF for verification when Felgo is unavailable.
- FAST_QML_BUILD=ON applies NO_CACHEGEN to the application, UI, and viewport modules. Check OFF too
  when build verification is requested for cache-generation behavior; a fast build does not
  validate that compilation path.
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
- Methods dispatched by DeviceRunner::invoke(name, args) must be public Q_INVOKABLE methods on
  an AbstractDevice-derived class, return void, and take zero arguments or one QVariantMap.
  Names must be unique: buildApi() indexes by name, not overload signature. Inspect string call sites.
- Register C++ QML types through src/network/backendqmltypes.h and src/CMakeLists.txt, following
  the existing QML_FOREIGN pattern. Preserve Hub's C++ ownership, engine/thread checks, and lifetime.
  logger is the existing context-property exception; use typed registration for new exposure.
- Keep device/protocol processing and substantial numerical work in C++. Avoid blocking the GUI
  or adding blocking work to ControlIO's receive callbacks. Keep single-file helpers/constants local
  to an unnamed namespace; match the edited file's naming, braces, and indentation.

## QML components and geometry

- Scene data is owned in C++ by src/scene. main.cpp owns ApplicationScene and exposes it as
  RoboCrap.Backend's Scene singleton; SceneModel owns SceneObject children and their
  PlaneGeometry or CylinderGeometry.
  QML receives a read-only object list and edits names/visibility through model methods. Preserve
  stable IDs, shared object references, and GUI-thread mutations. Fitting remains in src/geometry.
  OccController observes the application scene directly in C++ and carries it through viewport
  recreation; viewport-local presentation handles are keyed by stable object IDs. Bounded imported
  planes and cylinders render as finite OCCT faces from their authoritative frames and bounds.
  Other scene geometry is not rendered yet, and scene objects are not connected to trajectory generation.
  Imported planes retain immutable BoundedPlane data (original points, centered rectangle,
  tangent axes and bounds). Imported cylinders retain immutable BoundedCylinder data (original
  points, fitted axis, midpoint origin, radius and finite length). PlaneImporter.load(url, scene)
  and loadCylinder(url, scene) fit on the same worker lifecycle, insert directly into the
  destination scene on the GUI thread, and emit loaded(SceneObject*); QML never forwards geometry.
  The coefficient-only addPlane API remains unbounded. Properties displays plane bounds only
  when PlaneGeometry.hasBounds is true. CylinderGeometry is shown only for fitted cylinders;
  metadata-only cylinder rows have no OCCT presentation.
  Main.qml owns the selected application IDs and the Show Points/Show Normals preferences.
  OccController forwards them to each viewport; CAD picks return stable application IDs to QML.
  Point and normal overlays use original samples, are bounded display-only OCCT parts, and do
  not participate in picking. Parent visibility controls overlays.

- Reuse compatible controls from qml/Modules/Components and values from the Styles module's
  Colors, Fonts, and Metrics singletons.
  Check input, output, sizing, and interaction contracts before reusing or extending a component.
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
  When build verification is requested, validate application QML packaging with hot reload OFF;
  the ON path omits the root QML_FILES list.

## Numerical and device behavior

- For geometry/path changes, inspect mathtypes.h, geometry/utils.cpp, and the affected producer and
  consumer. Preserve frame order X/Y/Z/A/B/C, degree-based Euler APIs and Z-Y-X composition,
  existing units, tolerances, optional/error results, and trajectory timing unless the task changes them.
  Check failed optional results before using them; validate finite and degenerate inputs as appropriate.
- For protocol changes, preserve or explicitly update framing, byte order, state keys, and endpoint
  validation together with their consumers. Validate changes using offline inputs or local simulation.
- Connecting to live PLC/FTS/RSI equipment, writing outputs, biasing a sensor, or starting motion
  requires explicit authorization covering that hardware action. A GUI/build check alone is insufficient.

## Validation and handoff

- Inspect git status and the final diff, including newly created/untracked files. Run git diff --check;
  check new files separately because an ordinary diff does not include untracked contents.
- Run CMake configure/build commands and build-backed QML lint targets only when the user
  explicitly requests build verification. Do not run them by default after code changes.
- Reuse a build directory only after checking CMAKE_HOME_DIRECTORY and compiler/Qt paths in its
  CMakeCache.txt when build verification is requested. Discover the installed matching Qt/compiler
  kit; do not invent absolute SDK paths or reconfigure the user's build to a different mode.
  Use a separate build directory when needed.
- Commands below run from the repository root in PowerShell. Set $buildDir to the selected build.
  To configure a new Ninja build, also set $qtPrefix to the installed Qt kit and activate its matching
  compiler environment first; the configure command is unnecessary for an existing valid build.

```powershell
cmake -S . -B "$buildDir" -G Ninja "-DCMAKE_PREFIX_PATH=$qtPrefix" -DUSE_HOT_RELOAD=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build "$buildDir" --target robocrap --parallel
cmake --build "$buildDir" --target Components_qmllint
```

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
