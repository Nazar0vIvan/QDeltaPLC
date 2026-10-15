# RoboCrap — subsystem contracts

These are the area-specific requirements referenced by [AGENTS.md](../AGENTS.md).
Read the sections required for the task before changing behavior; inspect current code/callers
to verify implementation claims. User instructions and the root mandatory rules still apply.

## Build and modules

- Root CMakeLists.txt owns robocrap, application QML/resources and QmlChartBridge.
  src/CMakeLists.txt owns RoboCrapGeometry (geometry, KR10 numerics, paths), RoboCrapScene
  (scene objects, synchronous importer), RoboCrapDevices (devices/protocols) and RoboCrapBackend
  (backend QML registration only). Below, Geometry/Scene/Devices/Backend abbreviate these targets.
  Scene/Devices link Geometry; Backend links both.
- Geometry publicly supplies Eigen because public headers expose Eigen types. Scene/Devices
  export Qt metatype metadata for backend foreign registration. Preserve RDTResponse's application
  QML registration through qt_generate_foreign_qml_types.
- src/3d/CMakeLists.txt owns RoboCrap3D, URI RoboCrap.Viewport3D. It requires bundled OCCT 8.0.0
  and a Windows 64-bit MinGW Qt kit, links Scene rather than Backend, and publicly links Geometry.
  Kr10Kinematics belongs to Geometry and uses geometry/mathtypes.h.
- Preserve URIs robocrap_qml_module, RoboCrap.Backend, RoboCrap.Viewport3D, Components and Styles.
  File/type moves or renames require owning CMake source-list and affected-import updates.
  Asset moves require owning .qrc/qt_add_resources and resource-URL updates.
- Register C++ QML types through src/backendqmltypes.h and src/CMakeLists.txt using QML_FOREIGN.
  Preserve Hub's C++ ownership, engine/thread checks and lifetime. Use typed registration for
  new exposure; do not add diagnostic context properties.

## Viewport and CAD

- The public controller header exposes no OCCT math types; internal OCCT code uses RoboCrap3D.
  OccRobotAdapter converts numerical transforms to gp_Trsf; OccScene/OccPart retain OCCT types.
- RobotViewport.qml hosts a native QWindow. CadLoadWorker parses CAD; robot state, QML properties
  and OCCT presentation stay on the GUI thread. viewportReady means viewer initialization,
  independently of robot assets; ready means robot-preview readiness for kinematics.
- Robot attachment replaces only robot parts, preserving application presentations. Stop initial
  camera fitting after a gesture on that surface. Decorative axes are non-selectable and excluded
  from camera-fit bounds.
- Kr10Model groups Kr10KinematicModel and RobotVisualModel from one JSON definition. Kinematics
  receives only the former; presentation receives the latter. Joint data/fixed dimensions live
  in kr10kinematicmodel.h; visual records/complete model in kr10model.h.
- CadLoadWorker accepts absolute source paths and a cache directory, returning ordered shapes/errors.
  Serialize OCCT parsing/cache requests; robot loading includes only seven links. CadLoadResult
  is independent of the worker.
- RobotPreviewState owns the model/current IK seed and creates read-only RobotPose candidates.
  OccController commits a candidate after successful presentation. OccViewWindow retains const
  preview-state access for restoration.
- json.qrc embeds resources/json/kr10.json. cmake/DeployOcct.cmake stages OCCT DLLs/runtime resources
  and resources/cad/kr10 beside the executable and at installation.
- Generated BREPs live in applicationDirPath()/resources/cad/kr10, never QStandardPaths system
  cache folders. CachedShapeLoader accepts STEP/STP basenames in a selected source directory.
  First try <completeBaseName>.brep in deployed CAD without requiring/hashing STEP.
  Missing/unreadable/invalid BREP falls back to STEP conversion and atomic QSaveFile saving.
  Valid BREP takes priority; STEP/content/OCCT-version changes require removing it to regenerate.
  Basenames define cache identity: equal basenames share the same deployed BREP.

## Devices and protocols

Before device/UI integration changes, read src/network/{devicehub,devicerunner,abstractdevice}.{h,cpp}
and src/main.cpp. Also read Build and modules for QML registration changes.

- Backend.Hub.device(key) returns a DeviceRunner on the application/QML thread; keep its
  QQmlPropertyMap there. DeviceHub::add takes a parentless AbstractDevice and moves it to I/O.
  FTS/RSI share ControlIO; PLC uses GeneralIO. main.cpp's FTS-to-RSI signal relies on this;
  recheck connections/data races before changing groups.
- Runner queues startReq, stopReq and invokeReq. Never call device methods directly from QML's
  thread; AbstractDevice's internal direct meta-call already runs on its I/O thread.
- Create/use/destroy sockets and timers on their owning I/O thread via startDevice/stopDevice.
  Preserve thread-finished/deleteLater cleanup. Finish device stop calls before quitting/joining
  I/O threads. stopAll uses BlockingQueuedConnection from outside both I/O threads; using it
  inside the receiving thread deadlocks. Preserve shutdown ordering and repeat-call safety.
- Publish values through stateReady, DeviceRunner::onStateReady and data; socket state through
  socketStateReady, onSockState and socketState. Preserve QML-consumed keys/change notifications
  and mutate QML-facing models only on their owning thread.
- Log device diagnostics with qInfo().noquote() on the device thread, retaining category label,
  device name and text. Viewport also uses Qt logging; main.cpp configures timestamps.
  Keep logging independent of QML state: no logging singleton/message type/GUI-thread relay.
- FTS recording/saving retains device-thread sockets/timers, copied-input nonblocking file work
  and completion of outstanding saves before destruction.
- DeviceRunner::invoke(name, args) dispatches public Q_INVOKABLE methods of AbstractDevice subclasses:
  void return, zero arguments or one QVariantMap. Names must be unique because buildApi indexes
  names rather than overload signatures. Inspect string call sites.
- Keep device/protocol processing and substantial numerical work in C++. Do not block the GUI
  or add blocking work to ControlIO receive callbacks. Single-file helpers/constants belong in
  an unnamed namespace; match the edited file's naming, braces and indentation.
- Protocol changes preserve or explicitly update framing, byte order, state keys and endpoint
  validation with their consumers. Validate using offline inputs or local simulation.
  The root hardware-authorization rule applies to equipment connections and outputs.

## Geometry and scene

For numerical geometry/path changes, read geometry/mathtypes.h, geometry/utils.cpp and the affected
producer/consumer. Read Viewport and CAD when changing OCCT adapters or robot presentation.

- Preserve X/Y/Z/A/B/C order, degree-based Euler APIs, Z-Y-X composition, units, tolerances,
  optional/error results and trajectory timing unless explicitly changed by the task.
  Check failed optionals before use; validate finite/degenerate inputs. Explicitly initialize
  Eigen transforms to identity and persistent vectors to zero.
- Robot joint rotations use arbitrary-axis makeRotation without small-component cleanup.
  Preserve cleanup in the principal-axis overload used by trajectories. inverseRigidTransform
  assumes orthonormal rotation without scale/shear. KR10 tolerances remain 1e-4 position,
  1e-6 rotation entries; wrist singularity threshold is GeomConst::Eps (1e-9).
- Angle-only calculateIK returns angles/Status/Turn, using accepted Status and preceding angles.
  It computes only the three-joint transform needed for the wrist. Callers run full FK and
  flange/TCP checks, deriving accepted configuration from those same frames. IkSolution has no
  FK frames. Keep checked solveIK(target, previousAngles) compatible with separate RSI consumers.
- SceneEndEffectors owns tool transforms/calibration on the GUI thread. Compose FLANGE_ER * ER_TCP
  in that order; display rounding must not change stored values. Commit coupled calibration
  values before notifications; unchanged values must not invalidate paths. Preserve flange-based
  FK/IK APIs and conversion of TCP targets using the tool transform.
- SceneModel owns SceneObject/geometry; QML changes scene state through model methods. Preserve
  stable scene-allocated IDs (zero means no object), shared references and GUI-thread mutations.
  Numerical fitting/intersection belongs to Geometry; OCCT presentation belongs to adapters.
  Decorative points/normals/axes do not participate in picking or camera-fit bounds.
- Before import/intersection changes, read numerical geometry factories and
  SceneSurfaceImporter/SceneModel callers; preserve units and probe-compensation semantics.

## Machining and playback

Current work covers animation preparation/playback only; RSI preparation remains closed unless
the user explicitly reopens it. Apply Geometry and scene to numerical changes.

- SceneMachining exposes SceneMachiningSettings as settings. Generate calls Backend.Machining.generate(),
  implemented by SceneMachining::generate. Keep it separate from dryRun, which only plays a prepared
  immutable ChamferMotion. End-effector Apply actions are separate.
- Numerical stages live in src/pathgeneration/chamfer and invoke no scene/device APIs. Inspect
  the relevant producer/consumer: chamferpath, chamferarclength, chamfercentralmotion,
  chamfercentralvalidation and chamfermotion.
- Keep geometric parameter, traveled distance and physical time distinct. Recover TCP position/
  orientation by evaluating the analytical curve at the inverse arc-length parameter; do not
  substitute stored XYZ/ABC interpolation without explicit redesign. Inverse-map knots,
  animation points and RSI cycles are different data.
- Fix central animation points/times before IK using curvature/time. Within one Generate worker,
  share three central arc-length maps, timing and schedule; construct them only when a candidate
  reaches central validation. Cache analytical targets/failures only at reached scheduled indices;
  solve/check joints anew for each seed. Count shared computation once, outside per-attempt times.
- LIN also fixes points before IK: N=max(1,ceil(T*sqrt(a/(8*0.01mm)))) per constant-acceleration
  span, with span duration T in seconds and acceleration magnitude a in mm/s²; cruise has one
  interval. No recursive subdivision, extra quarter-time IK checks, added central points or
  automatic retiming after IK/rate failures. The position criterion bounds neither orientation
  nor actual TCP error after joint interpolation. Preserve exact shared phase endpoints and
  continuous joint winding; never wrap saved angles modulo 360 degrees.
- During Generate, prepare one independent clamped joint spline per LIN and one shared central
  spline, with zero endpoint joint velocities. Check polynomial position/speed extrema.
  Reject failures without adding points or retiming. Dry Run performs no IK, geometry integration,
  inverse-map/spline construction or pauses to prepare more points. Displayed TCP between saved
  points is approximate; spline smoothness gives no exact Cartesian error guarantee.
  Curvature derivation: codex_plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md.
- Rated joint-speed bounds come from the robot model, without fixed simulation caps. Configured
  accelerations are simulation assumptions, not verified KUKA ratings. User-requested exception:
  joint-acceleration magnitude rejection stays disabled in central preview, LIN (including terminal
  stops) and spline checks. Retain acceleration finiteness, joint position/speed and central XYZ
  acceleration checks. HOME timing still uses configured joint acceleration; RSI checks are separate.
  Do not restore or extend the exception without a user request.
- Generate workers own copied numerical inputs and return cohesive results through futures.
  Accept results/insert scene objects on the GUI thread; reject stale source/model/TCP revisions.
  Publish only complete motions. Insert a successful replacement before removing its old object,
  preserving name/visibility. Wait for remaining workers at shutdown.
- Saved paths own immutable geometry/model/TCP/settings snapshots and survive source deletion.
  Model or TCP calibration changes invalidate playback until regeneration. Preserve pause/resume,
  Stop-to-HOME, completion-at-HOME and cleanup on readiness loss, mode change or path deletion.
  Selection cannot replace running motion; block generation/manual robot edits during playback.
- Separate calculation timings from movement durations and nested totals from disjoint stage times.
  Diagnostics retain attempted-stage failures and distinguish inverse-map knots from animation points.
  Preview-only profiling input: codex_plans/chamfer_profile_spatial_leads_input.json;
  previewOnly=true skips RSI preparation.
- Only for explicitly authorized RSI work, first read resources/files/rsi/RSI_Hole_CONTRACT.md and
  RSI_Hole_DIAGRAM.md, then the current encoder/sampler/coordinator. Preparation, execution and
  controller readiness differ. Numerical/wire checks do not establish installed compatibility:
  PC protocol/minimal XML still need alignment; nominal execution and force correction are unconnected.
  Preparation success never grants permission to execute.
- Preserve queued immutable prepared-job delivery to ControlIO, stale-result rejection and invalidation
  on regeneration/calibration changes. Keep numerical preparation/file I/O outside receive callbacks.
  Shared-file publication is not controller readiness; an old disk file may survive invalidation.
  Read the actual shared-data/KRL contract before edits.

## QML

Inspect changed components and their custom base types, including properties, signals, default
content and sizing. Read device/scene contracts when UI changes touch those APIs, and Build and
modules when changing registration, module names or resources.

- Reuse compatible qml/Modules/Components controls and Styles Colors/Fonts/Metrics.
  Do not add explanatory/tutorial labels; use concise field names, units, statuses and actionable
  errors. Keep explanations in documentation/chat.
- Measuring EE/Machining EE settings open in the right Properties area; Close restores object
  properties. End-effector numeric fields display four decimals and accept at most four fractional
  digits. Read-only ER rounding does not change stored calibration.
- Use implicitWidth/implicitHeight for natural preferred size; preserve useful inherited sizing.
  Before overriding styled Controls, inspect contentItem/background sizes, padding and insets.
  Item wrappers may need their own implicit size.
- A Qt Quick Layout owns immediate-child geometry: use Layout.* and implicit sizes, not concurrent
  x/y/width/height or anchors. Anchors on the layout are valid when its parent is not another layout.
  Check parent/child size dependencies for binding cycles.
- Give reusable components explicit inputs, not caller IDs/context. Use known concrete QML types;
  retain var for heterogeneous data. Require inputs with no valid default and update all creators.
- Delegates with required properties explicitly declare every consumed role/view value:
  index/model/modelData, or TableView row/column. Qualify access through IDs.
  Keep persistent state in models/backend, not disposable delegates.
- Qualify root properties from children (root.title); declare handler parameters explicitly
  (onActivated: index => ...). Keep simple handlers inline; use named functions for reused/substantial
  logic when clearer. Preserve bindings on state updates.
- Components/Styles exports update both module CMakeLists.txt and source qmldir; those qmldir files
  are maintained hot-reload inputs. Keep pragma Singleton, QT_QML_SINGLETON_TYPE and qmldir consistent.
