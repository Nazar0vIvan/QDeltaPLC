# RoboCrap — agent instructions

## Mandatory C++ style

- Do not use C++ lambdas; use descriptively named helper functions or member methods.
- No project test logic is needed; do not add tests, test harnesses, or test build targets.
- Do not use exception-handling syntax in project C++ code: no `try`, `catch`, or `throw`.
  Report expected failures through return values, using existing optional/result conventions.
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
  RoboCrapGeometry (geometry, KR10 numerical kinematics and the previously active path-generation sources), RoboCrapScene
  (scene objects and synchronous importer), RoboCrapDevices (device infrastructure, concrete
  devices and protocols), and RoboCrapBackend (backend QML registration only). Scene and Devices
  link Geometry; Backend links Scene and Devices. Logger and QmlChartBridge compile in robocrap.
  Geometry publicly supplies Eigen because its public headers expose Eigen types. Scene and
  Devices export Qt metatype metadata for backend foreign registration. RDTResponse retains
  its application-module QML registration through qt_generate_foreign_qml_types.
  A backend build includes its implementation dependencies but does not validate the executable,
  application QML, or viewport integration; build robocrap for full integration verification.
- src/3d/CMakeLists.txt owns RoboCrap3D (URI RoboCrap.Viewport3D), the independent OCCT preview.
  It requires the bundled OCCT 8.0.0 SDK and a Windows 64-bit MinGW Qt kit. Its public controller
  header does not expose OCCT math types; internal OCCT code uses namespace RoboCrap3D.
  RoboCrap3D links RoboCrapScene rather than RoboCrapBackend and publicly links RoboCrapGeometry.
  Kr10Kinematics compiles in RoboCrapGeometry and uses the shared Eigen types in geometry/mathtypes.h.
  OccRobotAdapter converts numerical transforms to gp_Trsf; OccScene and OccPart retain OCCT types.
  The former 3d/math copies are removed; RGB conversion lives with OccPartProps.
  RobotViewport.qml hosts its native QWindow; CAD parsing runs on CadLoadWorker instances, while
  robot state, QML properties, and OCCT presentation remain on the GUI thread.
  viewportReady reports an initialized viewer independently of robot assets; ready still reports
  robot-preview readiness for kinematics. Robot attachment replaces only robot parts and preserves
  application presentations. Initial camera fitting stops after a camera gesture on that surface.
  Startup displays the viewcube first; world axes appear only with visible scene parts after
  camera setup. Axes are excluded from fit bounds, and fitting defers redraw until their size is updated.
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
- Device log signals are connected to Logger in main.cpp before devices start, using queued
  connections. Devices depend only on network/common/loggermessage.h, not the Logger singleton.
- Main.qml owns nonmodal PLC and FTS panel windows opened from the Views menu. FTS commands
  remain queued through DeviceRunner. Its UDP connection state means a configured bound socket;
  streaming requires parsed data from the configured peer address/port and ends after 300 ms
  without data. Start waits for data; Record requires a requested active stream. Stop ends recording,
  and Save requires a successful Stop request, confirmed quiet reception and nonempty samples.
  All panel actions require a connected socket. Recording retains the latest sample per 16 ms
  batch and the 7,500-sample cap. JSON retains raw RDT fields with counts_per_unit metadata.
  Saving uses a copied-input async worker and QSaveFile under applicationDirPath()/records with
  unique timestamped names. A device-thread timer collects the return-value result; shutdown
  finishes any outstanding save before deleting the device's timers.
- Methods dispatched by DeviceRunner::invoke(name, args) must be public Q_INVOKABLE methods on
  an AbstractDevice-derived class, return void, and take zero arguments or one QVariantMap.
  Names must be unique: buildApi() indexes by name, not overload signature. Inspect string call sites.
- Register C++ QML types through src/backendqmltypes.h and src/CMakeLists.txt, following
  the existing QML_FOREIGN pattern. Preserve Hub's C++ ownership, engine/thread checks, and lifetime.
  logger is the existing context-property exception; use typed registration for new exposure.
- Keep device/protocol processing and substantial numerical work in C++. Avoid blocking the GUI
  or adding blocking work to ControlIO's receive callbacks. Keep single-file helpers/constants local
  to an unnamed namespace; match the edited file's naming, braces, and indentation.

## QML components and geometry

- Do not add explanatory or tutorial labels to the UI. Use concise field names, units,
  status messages and actionable errors; keep explanations in documentation or chat.

- Numerical internal-chamfer geometry lives in pathgeneration/chamfer/chamferpath.h/.cpp
  in RoboCrapGeometry. ChamferPath validates an ellipse against a cylinder axis line and
  evaluates a BASE-relative surface-attached TCP frame at an ellipse parameter in radians.
  Chamfer size (default 0.5 mm) is the setback along the end plane in each longitudinal
  radial/axis section. chamferAngleDegrees (default 45 degrees) measures the diagonal
  from the outward hole axis; SceneMachiningSettings exposes it as chamferAngle. The wall
  setback follows this angle and must remain positive around the opening. Angles must
  be finite and strictly between 0 and 90 degrees. TCP origin is the midpoint of the endpoints.
  Local Y follows the section diagonal toward the end plane; local Z points inward/toward the opening; local X is radial cross outward axis.
  TCP = local frame * Rx(180 degrees), with coincident origins and opposite Y/Z axes.
  flipAxis reverses the outward axis and traversal. Lead-in/out follow quarter circles
  in normalized angular/axial coordinates, with phi=pi*progress/2: lead-in angle is
  -span*cos(phi) and clear amount is 1-sin(phi); lead-out angle is 2*pi+span*sin(phi)
  and clear amount is 1-cos(phi). Axial clearance and quaternion-interpolated orientation
  use that same clear amount; their clear-end burr Y is opposite the outward
  axis and TCP Z is radially outward. Evaluated lead TCP Y must project negatively on
  the outward axis. The section frame X need not follow the tilted ellipse tangent.
  ChamferPath stores positive finite stagingDistance (default 10 mm). stagingPoint returns
  ellipse center + stagingDistance*outwardAxis; stagingPose(LeadIn/LeadOut) returns that
  origin with the corresponding clear-lead orientation (Y opposite hole Z). Machining
  is not a valid staging side. Scene settings and MachiningPanel preserve the distance
  through Apply and saved-path selection. HOME PTP terminates/starts at P_s; straight fixed-attitude Cartesian transfers connect
  it to the clear lead endpoints using lead-in/out feeds. Transfer/lead corners stop.
  Phase evaluation and adaptive sampling retain shared junctions and the full-turn endpoint.
  SceneMachining connects numerical generation to scene ownership. OccSceneAdapter renders
  seven phase polylines under one object ID (purple HOME, blue leads, green machining).
  Machining paths are displayed by default and follow their scene-object visibility.
  Path visibility is controlled only by each scene object through Scene and Properties panels; no global path-visibility switch remains. OccViewWindow retains selection and axis-preview parameters on recreation.
  The non-selectable magenta outward-axis arrow uses SceneMachining.previewParameters,
  responds to applied settings, and is excluded from camera bounds. New edge inputs default
  outward toward negative scene X for the current XP/XC workpiece; saved paths retain their Flip. MachiningPanel edits parameters and Apply generates/selects paths. SceneMachining owns a GUI-thread timer and monotonic clock for Dry Run; direct C++ pose requests go through OccController and the existing preview-state FK/commit path. Pause freezes elapsed time and pose; Dry Run resumes the retained motion. User Stop resets the preview to HOME and clears active playback; completion ends at HOME. Internal failure/readiness cleanup ends playback without a HOME request. Calibration/model changes, viewport loss, active-path deletion, leaving machining mode and shutdown stop playback. Manual FK/IK and generation are blocked during running or paused playback; selection does not replace the active motion.
  chamfermotion.h/.cpp compiles a timed seven-stage KR10 simulation using the existing
  numerical IK solver, preceding-joint seeds, bounded alternate starting configurations,
  FK validation and direct synchronized joint-space HOME movements. It owns model/TCP
  snapshots and continuous joint angles; it does not call viewport or device APIs.
  Central motion uses monotone cubic joint interpolation, feed/acceleration timing and
  a reported uniform time scale for rate limits. HOME movements use synchronized quintic
  progress. evaluate(seconds) returns a pose independently of display refresh rate.
  IgnorePreviewJointPositionLimits in kr10kinematicmodel.h is false. A1-A6 position checks are
  enforced in analytical IK, chamfer generation/evaluation and preview FK, including manual preview.
  Model limits also guide seed selection and model validation. Finite, singularity, FK accuracy,
  continuity, speed and acceleration checks remain active; device control is unaffected.
  Simulation joint limits for speed/acceleration are explicit settings, not KUKA ratings;
  Cartesian rate and interpolation-error checks are numerical samples, not analytic guarantees.

- main.cpp owns SceneMachining after SceneModel/SceneEndEffectors and before the viewport/QML
  engine; Backend.Machining exposes typed machiningSettings drafts and asynchronous apply().
  One edge with live fitted source surfaces, or an existing generated path, supplies input.
  SceneMachiningPath in scenegeometry.h owns shared immutable ChamferMotion and source-edge
  provenance. SceneModel inserts MachiningPath objects using existing IDs/signals. Regeneration
  inserts a complete replacement before removing the original and preserves its name/visibility.
  Saved paths retain geometry/model/TCP/settings snapshots and survive source deletion.
  OccController passes a numerical robot-model copy on successful loading and forwards readiness.
  Model/TCP revisions invalidate old paths until regeneration; viewport unavailability is temporary.
  Apply runs numerical motion compilation on a standard async worker with copied inputs and a
  return-value future. A GUI timer collects the result; scene insertion stays on the GUI thread.
  calculating drives an application-modal native progress window with an indeterminate bar.
  No percentage is estimated. Changed input/calibration or lost robot readiness rejects the result.
  The future waits for any remaining calculation during coordinator destruction.
  Scene playback has no OCCT dependency and invokes no devices.

- SceneEndEffectors in src/scene owns both tool configurations on the GUI thread, independently
  of scene-object selection. main.cpp owns it before the QML engine and exposes Backend.EndEffectors
  through backendqmltypes.h. CAD URLs default to deployed MEE.stp and SEE.stp via ViewportAssets.
  MEE is WP-500 V6 with constant 3 mm ball diameter and 37.2 mm stylus length. Spindle TCP is an
  initially empty QList<double>, set atomically to six finite flange-relative XYZABC values
  (mm/degrees, Z-Y-X rotation). spindleTcpInCollet derives the ER-relative editor pose;
  spindleColletPose exposes the flange-relative ER XYZABC. applySpindlePoses validates ER and
  ER-relative TCP together, composes FLANGE_ER * ER_TCP, and commits both before notifications.
  setSpindleTcpInCollet delegates to that operation with the current ER. Only an effective ER
  change emits spindleColletChanged; ER changes also notify TCP consumers once. TCP-only changes
  preserve the exact ER matrix, and unchanged poses do not notify or invalidate machining paths.
  The hardcoded initial ER origin in flange coordinates is (142.187099, -0.182323, 122.895995) mm;
  ER Y follows normalized (0.999995, -0.000312, -0.003129), provisional ER Z is projected
  flange +Z perpendicular to ER Y, and ER X is Y cross Z. Active tool defaults to Measuring.
  OccController observes CAD source changes and retains separate MEE/SEE shapes, successful source URLs, loading states and
  errors. Superseded load results are discarded; failed replacements retain the last good shape.
  OccRobotAdapter retains both tool presentations, updating both at the flange during pose changes;
  switching only changes visibility. OccViewWindow retains tool shapes/selection across surface
  recreation. Main.qml selects MEE for both measuring submodes and SEE for Machining.
  Main.qml owns two nonmodal tool windows opened by mode-toolbar setup buttons: Measuring EE
  contains EndEffectorPanel for both measuring submodes; Spindle EE contains only the spindle
  EndEffectorPanel. Generate Path opens MachiningPanel in place of Properties in the right
  column. Opening the editor requires one selected edge/path and stopped playback; robot/TCP
  readiness gates Apply, with the reason displayed in the editor. Successful Apply returns to Properties; Cancel discards the draft, restores settings
  after unsuccessful Apply, and returns to Properties. Selection/mode changes close the editor.
  Properties shows cylinder diameter as twice its compensated radius. Tool windows retain drafts
  when closed and hide when leaving their mode. EndEffectorPanel shows fixed MEE data or disabled
  flange-relative ER XYZABC fields under "ER collet pose relative to flange", ER visibility,
  then editable ER-relative TCP XYZABC fields and TCP visibility. One Apply button validates
  and submits both pose drafts; no Reset action remains. HOME PTP simulation is displayed
  as a percentage of configured joint speed/acceleration limits; the backend auxiliaryScale remains in (0,1].
  Timing/sampling limits remain numerical defaults without a tuning UI. Apply remains enabled
  for valid six-value ER and TCP drafts and shows the spindle TCP trihedron; its Visible checkbox controls OccController.showSpindleTcp.
  The separate ER trihedron Visible checkbox controls OccController.showSpindleCollet, initially
  true and usable before TCP calibration; closing Spindle EE does not affect either preference.
  OccViewWindow retains optional flange-relative TCP and ER frames across surface recreation.
  OccRobotAdapter places RGB axes at BASE_FLANGE * FLANGE_TCP and BASE_FLANGE * FLANGE_ER,
  updating both with robot poses. Individual calibration/visibility frame setters refresh only
  the affected trihedron; robot movement, load/clear and tool switching refresh both.
  OccScene shares OccWorldAxes display/sizing operations
  (5% of camera scale); both trihedra are non-selectable, excluded from fit bounds, and hidden
  without a robot or while Measuring is active. Visibility preferences survive mode changes.
  Reapplying unchanged ER-relative TCP does not invalidate paths. Main.qml owns the STEP/STP
  dialog and captures its destination tool when opened. The panel displays per-tool load errors
  and retry; selecting the same source explicitly reloads through the existing cache.
  SceneEndEffectors converts spindle TCP/flange XYZABC poses with existing geometry helpers.
  OccController.tcpPose derives the base-relative TCP pose from committed flange state;
  solveTcpIK converts a TCP target to a flange target and delegates to existing IK.
  These generically named APIs currently use spindleTcp calibration, independently of activeTool.
  ER calibration changes update the ER decoration and derived TCP; TCP-only edits update TCP.
  Calibration edits do not move CAD/robot or reload files. Missing calibration yields empty results;
  no MEE TCP is inferred from stylus length. Existing solveFK/solveIK remain flange-based.

- Numerical intersections live in geometry/surfaceintersection.h/.cpp in RoboCrapGeometry.
  intersectSurfaces supports only plane-cylinder and returns a status and optional immutable EdgeGeometry containing a complete ellipse/circle using the infinite plane and axially extended cylinder.
  Displayed surface bounds do not clip the intersection; parallel-axis cases return no edge. SceneModel::canIntersect/intersect
  accept only one fitted plane and one fitted cylinder; intersect inserts one Edge and returns its ID list, or emits
  intersectionFailed without insertion. SceneEdgeGeometry owns immutable geometry and source IDs,
  exposed as uncreatable Backend.EdgeGeometry. OccSceneAdapter renders complete cyan ellipse/circle edges with existing picking and visibility. Main.qml wires Intersect Selected in both measuring submodes through canIntersect/intersect, selects successful results and shows intersectionFailed messages;
  see INTERSECTIONS_EXECPLAN.md.

- The Scene Abstraction Layer lives in src/scene. main.cpp owns SceneModel and exposes it as
  RoboCrap.Backend's Scene singleton; SceneModel owns SceneObject children and their
  ScenePlaneGeometry, SceneCylinderGeometry or SceneCircleGeometry (all in scenegeometry.h).
  QML receives a read-only object list and edits names/visibility through model methods. Preserve
  scene-allocated quint32 IDs (zero means no object), shared object references and GUI-thread mutations.
  Startup creates no demo scene objects. SceneModel::removeObjects emits objectRemoved for targeted
  viewport/overlay removal and defers QObject destruction; source deletion does not cascade to edges.
  Delete works from the scene UI and native viewport, with text editing excluded from the shortcut.
  The viewport background is neutral sRGB #A5A5A5.
  SceneModel indexes IDs with QHash, emits objectAdded/objectVisibilityChanged for targeted rendering,
  and retains objectsChanged only as the QML collection notification. Fitting remains in src/geometry.
  OccController observes the application scene directly in C++ and carries it through viewport
  recreation. OccSceneAdapter in src/3d/adapters translates typed scene geometry, overlays, selection and picks
  into OccScene operations, grouping viewport-local parts by stable object ID. OccViewport owns
  OccSceneAdapter and OccRobotAdapter and coordinates input and rendering. Full scene installation
  replaces presentations; ordinary object changes update only the affected presentation. Bounded imported
  planes and cylinders render as finite OCCT faces from their authoritative frames and bounds.
  Fitted circles render as complete OCCT circular edges. Other scene geometry is not rendered yet,
  and intersection edges supply chamfer trajectory generation through SceneMachining.
  BoundedPlane and BoundedCylinder have private construction and read-only accessors; validated
  fromPoints/fromSamples factories create them. Both retain original samples as QVector<V3d>;
  fromPoints takes that container by value and fromSamples takes ProbeSamples by value so the
  importer can transfer ownership. Fitting reads shared samples through const access; centered
  numerical workspaces remain separate. Point arrays describe the small surface frames only.
  Imported planes retain immutable BoundedPlane data
  (original points, centered rectangle, tangent axes and bounds). Imported cylinders retain immutable BoundedCylinder data (original
  points, fitted axis, midpoint origin, radius and finite length). Imported circles retain immutable
  Circle data (center, radius, canonical unit normal, original points and 3D fit RMS).
  Circle::fromPoints uses a direct circumcircle for three non-collinear points and geometric
  least-squares fitting for larger sets. SceneSurfaceImporter in
  scenesurfaceimporter.* retains the QML name PlaneImporter.
  load(url, scene), loadCylinder(url, scene) and loadCircle(url, scene) synchronously decode, fit and insert on the GUI
  thread, then emit loaded(SceneObject*); there is no import worker or event pumping. QML never
  forwards geometry. Source URLs are provenance metadata in the scene, validated only for file import.
  The coefficient-only addPlane API remains unbounded. Properties displays plane bounds only
  when the PlaneGeometry QML view hasBounds is true. The CylinderGeometry QML view is shown only
  for fitted cylinders; metadata-only cylinder rows have no OCCT presentation.
  Main.qml owns the selected application IDs. SceneObject owns independent Show Points/Show Normals
  preferences, edited through SceneModel from Properties for a single selected surface or edge.
  OccController observes targeted overlay changes; viewport recreation restores them from scene
  objects. Its compatibility overlay setter affects selected objects only. CAD picks return stable
  application IDs to QML.
  Point overlays use original samples; plane/cylinder normals originate at samples and a circle's
  single normal originates at its center. Analytic intersection edges use 64 display-only curve
  markers and one curve-plane normal at the center, not probe samples or machining normals.
  Overlays are bounded display-only OCCT parts and do
  not participate in picking. Parent visibility controls overlays.
  Plane and cylinder JSON imports accept {"points": [[x,y,z], ...], "radius": r, "dir": s} for
  probe-ball compensation after fitting. Radius is finite and nonnegative in point units;
  dir is exactly +1 or -1. Planes shift by dir*radius along their canonical unit normal
  (both coefficients and bounded origin); cylinders add dir*radius to their fitted radius,
  which must remain positive. Original samples and pre-compensation fit residuals are retained.
  Legacy point arrays remain supported without compensation; fromPoints APIs fit raw points.
  Circle JSON imports read raw point arrays or an object's points array; radius/dir metadata is
  ignored and no probe compensation is applied. Manual circle examples live in resources/json/circ.

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

- Precise Measuring uses QML-only Dry Scan/Pause/Stop states in Main.qml. Plane/cylinder/cone selection enables Dry Scan; mode/submode changes reset its state. Machining toolbar uses Backend.Machining playbackActive/playing/canPlay for Dry Run/Pause/Stop. Scan and Start Machining remain disabled.
