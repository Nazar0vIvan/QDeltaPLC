# RoboCrap — subsystem contracts

Area-specific requirements referenced by [AGENTS.md](../../AGENTS.md). Read the sections required for
the task before changing behavior; verify implementation claims against current code/callers. User
instructions and the root mandatory rules still apply. The class/file map is in
[ARCHITECTURE.md](ARCHITECTURE.md); this file states only constraints.

## Build and modules

- Root CMakeLists.txt owns project/options, dependency discovery, target subdirectories and
  installation. src/application/CMakeLists.txt declares robocrap and owns all application
  C++/QML/resources; it links Geometry, Kinematics, Trajectories, Devices, Viewport and the existing
  Qt/QML dependencies. QmlChartBridge stays in Application with its QtCharts requirement; RSI XML
  uses QtCore. Application has no public include directory; no target uses the legacy src include
  directory.
- RoboCrapGeometry (surfaces/math), RoboCrapKinematics (KR10 numerics) and RoboCrapTrajectories
  (animation and RSI sampling) are owned by src/{geometry,kinematics,trajectories}/CMakeLists.txt.
  Kinematics publicly links Geometry; Trajectories publicly links Kinematics. Each library exports only
  include/<capability>; Geometry also exports Eigen.
- RoboCrapDevices (src/devices, including src/devices/rsi) links QtNetwork and publicly links Geometry
  only for the `QVector<V6d>` RSI stream; no Trajectories dependency, QML registration or
  QQmlPropertyMap; its metatype metadata is extracted for consumers. DeviceHub, DeviceRunner and
  DeviceProfileModel are private Application code. PlcMessageManager remains a QObject with
  dictionary APIs and a published header for the PlcDevice/PlcMessage enums.
- RoboCrap3D (src/viewport, URI RoboCrap.Viewport3D) requires bundled OCCT 8.0.0 and a Windows
  64-bit MinGW Qt kit; it publicly links Geometry and QtCore/Gui/Qml, privately OCCT, and has no
  Application/Trajectories dependency. It exports only include/viewport, including the singleton
  wrapper used by Application's instance wiring.
- tools/cadstep2brep is the only target linking TKDESTEP. robocrap_deploy_occt (cmake/DeployOcct.cmake)
  converts each resources/cad/*.stp through cmake/ConvertStep.cmake into
  <build>/generated/cad/<name>.brep only when inputs are newer; a failed conversion leaves no BREP.
  robocrap depends on that target and lists the BREPs in LINK_DEPENDS, so a changed BREP relinks it
  and reruns the POST_BUILD deployment. No STEP file is deployed.
- Saved motions expose central-motion/arc/path values; keep those published headers until consumers
  and storage change separately.
- Preserve URIs robocrap_qml_module, RoboCrap.Viewport3D, Components and Styles. The retired
  RoboCrap.Backend element names are registered by Application's backendqmltypes.h. Main uses App;
  other QML may keep Backend as the application-URI alias. RDTResponse stays the same queued
  Q_GADGET, with rdtResponse registered by an Application foreign wrapper.
- Application singletons (Machining, MachiningPreparation, Playback, Calibration, EndEffectors, Scene,
  Hub, WorkspaceController) return main()-owned instances through create() and claimQmlSingleton:
  qFatal on a null instance, wrong thread or second live engine, then CppOwnership.
  QQmlEngine::setExternalSingletonInstance (Qt 6.12) is unavailable at the Qt 6.8 minimum.
  RobotController is not a QML type. main constructs ScenePlayback after the document and before
  machining and calibration, and the workspace after its document, machining and end-effector owners.
  Add no diagnostic context properties.
- Both hot-reload branches declare the same application module; QML_OPTIONS holds build flags only.
  Application QML/config live in src/application/qml; paths relative to Application keep the qml/...
  resource paths and the ON branch's /qml/config.json. Root hot_reload_config.json selects
  src/application/qml/Main.qml and mirrors src/application/qml; keep its qml entry (Main's cached
  qml/Modules import path) and the Components/Styles mappings. Reusable modules stay in qml/Modules.
  robocrap outputs to the build root (plus the configuration suffix); preserve explicit
  CMAKE_RUNTIME_OUTPUT_DIRECTORY overrides.
- Application, the executable's declaring directory, attaches the OCCT POST_BUILD deployment helper;
  root owns deployment setup and installation. Deployed CAD/resources stay beside the executable.
  resources/json/calibration_sphere.json is copied to <exe dir>/resources/json/ only when absent (an
  edited deployed file is never overwritten) and is not in a qrc. File/type moves require owning
  source-list/import updates; asset moves require resource-URL updates.

## Viewport and CAD

- The public controller header defines concrete drawing values without OCCT math types or
  scene/machining objects. SceneObject supplies analytic patches/curves/spheres, saved FK TCP
  phase/segment polylines and bounded diagnostics; SceneMachining supplies the normalized signed draft
  axis and length. Preserve 256 markers/128 normal positions, 64 ellipse markers and one edge-centre
  normal. SceneModel requests drawing acceptance before publishing; failed installation keeps the old
  object and accepted drawing cache. main attaches the empty document first.
- OccController owns the accepted DrawingDocument and DrawingPose; its window borrows both read-only
  and is destroyed first. OccController calls the window's OccViewport directly (looked up per call,
  null without a native surface) for document, selection, draft-axis and spindle-frame updates and
  re-applies selection after insert/remove/visibility changes. Replay follows stable ID order, restores
  visibility/diagnostics/selection/draft axis, never reallocates IDs, and needs no application QObject
  pointer, curve resampling or FK.
- RobotController supplies DrawingPose with BASE-relative link, tool and composed TCP/collet matrices;
  renderer axis setters take those absolute frames and compose no tool transforms. OccRobotAdapter
  converts matrices to gp_Trsf; OccScene/OccPart keep OCCT types. OccPart applies style/transform at
  creation and keeps its AIS shape/selection/topmost state. World/TCP/ER axes are separate
  OccWorldAxes objects.
- OccScene stores only live parts keyed by PartId; its optional increasing counter starts at zero,
  permits the final representable ID, stays exhausted afterward and is never reset by removal. Erase a
  part only after OCCT removal succeeds. Track selected visible part IDs (curves included) and keep
  unchanged outlines; equal application IDs can resolve to hidden/replaced parts. Keep unfinished
  outline handles/completion state through external failures; remove selection before hiding/deleting.
  Preserve selection styling and decorative picking/fit exclusion. A part without face boundaries
  (DrawingSphere: seam and pole edges not drawn) gets no outline; a hidden DrawingPart is not
  installed; a transparent part gets its own Selected/LocalDynamic style copies with its transparency.
- RobotViewport.qml hosts a native QWindow; no Qt Quick item draws over it; busy state goes to the
  status bar (docs/OCCT_SCENE_OVERLAY.md records the verified overlay method). CadLoadWorker parses
  CAD; robot state, QML properties and OCCT presentation stay on the GUI thread. viewportReady means
  viewer initialization; ready means robot-preview readiness for kinematics.
- main.cpp requests the single robot load once after console wiring; QML recreation does not. There
  is no robot replacement or link deletion. Missing startup graphics data means wait, not clear. A
  failed link/tool insertion or initial HOME transform reports its stage and removes only that
  installation's parts; readiness requires complete success including both end-effector parts.
  Cleanup keeps part IDs until removal succeeds and reports failures without retrying.
- After initialization, a failed playback transform reports a console error and reapplies the last
  accepted matrices once to existing links, tools and axes, without deleting/recreating links,
  reloading CAD or running FK/IK; reject the candidate and stop playback without advancing elapsed
  time/phase. If restoration fails, report it and disable robot readiness; never claim restoration.
- Native-surface recreation may install cached graphics in the new empty scene without reloading
  assets or republishing the model; it is not movement-error recovery. Preserve application
  presentations. End-effector models are fixed:
  no tool CAD selection, Browse or Retry. Initial camera fitting stops after a gesture on that
  surface. Decorative parts (axes, markers, normals, calibration table and stand) are non-selectable
  and excluded from fit bounds: OccScene marks their presentation structures infinite after display
  (OccPartProps::fitExcluded), never their AIS objects, because AIS_Shape computes an infinite
  object's shaded presentation as wireframe.
- RobotDefinition::kr10() builds the model and ordered link filenames/RGB without OCCT; robot geometry
  changes mean editing its constant table. RobotController's
  HOME/FK check and Generate's validRobot guard the values. robotgraphics.h supplies neutral graphics
  and the two tool paths in RobotTool order; adapters construct OCCT values; renderer link count comes
  from the supplied graphics.
- CadLoadWorker accepts absolute .brep paths and returns ordered shapes/errors (CadLoadResult is
  worker-independent); OCCT parsing is serialized. One robot job loads seven links, then
  EE_WP-500.brep (measuring) and EE_GDZ-65-800A.brep (spindle); OccRobotAdapter installs them together
  and shows only the active tool.
- A DrawingModel part (absolute BREP path, translation-only origin in {B}) loads once per path on a
  separate worker, one path at a time; finished loads, failures included, stay in
  DrawingDocument::models. Insertion accepts an object whose model is loading or failed and skips that
  part; a successful load redraws its users. A failure is reported once, never retried and never
  affects robot readiness.
- RobotController owns the stable model/solver and accepted pose. Keep the checked initial HOME
  candidate unpublished until graphics acceptance; publish the model to ScenePlayback once; commit
  later poses only after transform acceptance. Surfaces/windows are restored from drawing values/CAD
  results without a robot-state/model pointer or model revision change.
- Application assetpaths.* selects the deployed CAD and OCCT resource directories; main passes the
  OCCT resource directory to OccController at construction. initializeOcct keeps resource checks,
  environment variables and external Font_FontMgr failure handling; Viewport selects no paths.
- The application reads BREP only from applicationDirPath()/resources/cad (seven hand-maintained link
  BREPs in kr10/, plus BREPs generated from resources/cad/*.stp); no STEP import, runtime BREP cache or
  TKDESTEP link. cmake/DeployOcct.cmake stages OCCT DLLs/resources and both BREP sets beside the
  executable and at installation.

## Devices and protocols

Before device/UI integration changes, read src/application/devices/{devicehub,devicerunner}.{h,cpp},
src/devices/abstractdevice.cpp, src/devices/include/devices/abstractdevice.h and
src/application/main.cpp; for QML registration also Build and modules.

- Hub.device(key) returns a DeviceRunner on the QML thread; keep its QQmlPropertyMap there.
  DeviceHub::add takes a parentless AbstractDevice and moves it to I/O. FTS/RSI share ControlIO and
  PLC uses GeneralIO; main.cpp's FTS-to-RSI signal relies on this, so recheck connections/data races
  before changing groups.
- The runner queues startReq, stopReq and invokeReq. Never call device methods from QML's thread;
  AbstractDevice's internal direct meta-call already runs on its I/O thread.
- Create/use/destroy sockets and timers on their I/O thread via startDevice/stopDevice. Preserve
  thread-finished/deleteLater cleanup, finish device stops before quitting/joining I/O threads, and
  keep shutdown order and repeat-call safety. stopAll uses BlockingQueuedConnection from outside both
  I/O threads; calling it inside the receiving thread deadlocks.
- Publish values through stateReady → DeviceRunner::onStateReady → data, socket state through
  socketStateReady → onSockState → socketState. Preserve QML-consumed keys/notifications; mutate
  QML-facing models only on their owning thread.
- Log device diagnostics with qInfo().noquote() on the device thread with category label, device name
  and text; main.cpp configures timestamps. No logging singleton, message type or GUI-thread relay.
- FTS recording keeps device-thread sockets/timers, copied-input nonblocking file work and completion
  of outstanding saves before destruction.
- DeviceRunner::invoke(name, args) dispatches public Q_INVOKABLE methods of AbstractDevice subclasses
  returning void with zero arguments or one QVariantMap. Names must be unique (buildApi indexes names,
  not signatures); inspect string call sites.
- Keep device/protocol processing and substantial numerics in C++; never block the GUI or ControlIO
  receive callbacks. Single-file helpers/constants go in an unnamed namespace; match the edited file's
  naming, braces and indentation.
- Protocol changes preserve or explicitly update framing, byte order, state keys and endpoint
  validation with their consumers; validate with offline inputs or local simulation. The root
  hardware-authorization rule applies.

## Geometry and scene

For numerical geometry/path changes, read geometry/mathtypes.h, geometry/utils.cpp and the affected
producer/consumer; for OCCT adapters or robot presentation also Viewport and CAD.

- Preserve X/Y/Z/A/B/C order, degree-based Euler APIs, Z-Y-X composition, units, tolerances,
  optional/error results and trajectory timing unless the task changes them. Check failed optionals
  before use; validate finite/degenerate inputs. Initialize Eigen transforms to identity and persistent
  vectors to zero.
- Joint rotations use arbitrary-axis makeRotation without small-component cleanup; keep cleanup in
  the principal-axis overload used by trajectories. inverseRigidTransform assumes orthonormal rotation
  without scale/shear. KR10 tolerances: 1e-4 mm position, 1e-6 rotation entries; wrist singularity
  threshold GeomConst::Eps (1e-9).
- Angle-only calculateIK returns angles/Status/Turn from accepted Status and preceding angles and
  computes only the three-joint transform needed for the wrist. Callers run full FK and flange/TCP
  checks and derive the accepted configuration from those frames. IkSolution has no FK frames; there
  is no angles-only checked IK; callers carry the accepted IkSolution as seed.
- SceneEndEffectors owns accepted tool XYZABC and its optional reconstructed transform on the GUI
  thread; consumers reuse spindleTcpFrame, workers copy it. Reconstruct from accepted coordinates,
  not the raw Apply product or rounded display values; compose FLANGE_ER * ER_TCP in that order.
  Commit coupled list/frame values before notifications; unchanged values must not invalidate paths;
  a collet change still emits the TCP notification even when the TCP is equivalent. Preserve the
  flange-based Kr10Kinematics FK/IK APIs; there is no manual FK/IK command. The measuring TCP (TOOL_DATA[5]
  by default) is applied with applyMeasuringTcp, reconstructed as measuringTcpFrame, not persisted;
  an unchanged value emits nothing.
- SceneModel owns SceneObject; QML changes scene state through model methods. SceneObject holds a
  plane, cylinder, intersection edge (with its two source IDs), calibration sphere or calibration path
  (the shared immutable ProbeTouchMotion) by value in a std::variant; only a machining path has a child
  QObject (exposed as geometry). Its CONSTANT properties list supplies the {label, value} rows of the
  Properties panel. Preserve stable scene-allocated IDs (0 = none), shared references and GUI-thread
  mutations. Numerical fitting/intersection belongs to Geometry; OCCT presentation to adapters.
  Decorative points/normals/axes take no part in picking or fit bounds. The calibration sphere's
  drawing adds the table top (sphere file `table`) and CALIBRATION_STAND as decorative parts that share
  its visibility; diagnostic overlay updates keep them.
- Before import/intersection changes read the geometry factories and SceneSurfaceImporter/SceneModel
  callers; preserve units and probe-compensation semantics. BoundedPlane/BoundedCylinder accept points
  or decoded ProbeSamples, without file factories; SceneSurfaceImporter reads/decodes plane and
  cylinder inputs; there is no circle import.
- fitSphere rejects fewer than four or nonfinite points, a smallest singular value below 1e-9 of the
  largest, and a nonpositive squared radius. SceneCalibration reads the deployed sphere file on the GUI
  thread at every entry into Calibration: version 1, positive sphereDiameter, strutDiameter and
  strutRecess below the diameter, at least four finite roughPoints X/Y/Z; other keys ignored. Centre
  and ball-centre radius come from the readings; the compensated radius subtracts the ruby ball radius;
  the nominal radius only sizes the touch plan. A changed sphere replaces the object. While
  Calibration is active only the sphere and calibration path are visible and listed; entry records and
  hides other visible objects (also ones added meanwhile), exit shows the recorded ones that still
  exist. Neither calibration object can be deleted.

## Machining and playback

Scope: animation preparation/playback and nominal RSI Sampling/streaming for the T1 RSI_Chamfer_Hole
run; force correction is out of scope. Apply Geometry and scene to numerical changes.

- SceneMachining exposes SceneMachiningSettings as settings; Main calls App.Machining.generate()
  (SceneMachining::generate), separate from dryRun, which only plays a prepared immutable
  ChamferMotion. End-effector Apply actions are separate.
- Animation code lives in src/trajectories/{chamfer,probe} on the motion module src/trajectories/motion,
  with published values in src/trajectories/include/trajectories, and calls no scene/device APIs.
  Inspect the relevant producer/consumer among chamferpath, chamferarclength, chamfercentralmotion,
  chamfercentralvalidation, chamfermotion, probetouchmotion and motion/{timedmotion,motionbuilder,
  tcpsolver,jointsegments}.
- Keep geometric parameter, traveled distance and physical time distinct. Recover TCP pose by
  evaluating the analytical curve at the inverse arc-length parameter; never substitute stored XYZ/ABC
  interpolation without explicit redesign. Inverse-map knots, animation points and RSI cycles are
  different data.
- Lead evaluation may skip overwritten machining transform composition only after the existing
  axis-sign/basis, positive-depth and finite-position checks. Preserve the final lead basis checks and
  exact complete junction poses. Fixed chamfer trigonometry belongs to each immutable path; keep
  tangent division/reciprocal and copy the cache with its owned parameters.
- Preserve the piecewise quintic Hermite inverse, all nine inverse-check fractions and endpoint
  derivative conditions. Reuse accepted endpoint differentials and the original integral/error; keep
  stored-distance rounding checks. The per-phase 800000 limit counts original logical differential
  calls, including reused work, rejects the phase when exceeded and is not exported. Charge only
  reached work at the original check positions.
- Fix central points/times before IK from curvature/time. Within one Generate worker share three
  central arc-length maps, timing and schedule, constructed only when a candidate reaches the curve
  (ChamferCentralPreparation::prepare). Cache analytical targets/failures only at reached scheduled
  indices (CentralPath::target); solve/check joints anew for each seed.
- MotionBuilder builds every motion from HOME with appendPtp (joint steps ≤ 2°, ptpDuration, quintic
  smoothstep) and appendPath for a TimedPath (LinearPath for LIN/CIRC, CentralPath for the chamfer
  curve). appendPath checks in order: path start against the preceding stored TCP (0.0001 mm,
  0.000001 rotation entries); per scheduled point seeded ChamferTcpSolver::solveTcp and the new
  interval's joint speed; the grouped rate inspection (secants ≥ 1e-4 s from rest to rest: grouped
  joint speeds, XYZ acceleration of prescribed targets, the stop at the segment end); the segment's
  spline. A segment is stored only after all checks pass. Errors read
  "<phase label>: <message> (t=<path-local s> s)"; the first failure is the first failing point in
  point order (user decision, 2026-10-26).
- ScenePlayback owns Dry Run, the robot model copy and availability. It plays a
  std::shared_ptr<const TimedMotion> aliasing the timed() motion of a saved ChamferMotion or
  ProbeTouchMotion, keeping the owner alive. TimedMotion::evaluateJoints(seconds) returns optional
  angles and the phase index without FK (smoothstep on PTP, stored cubics on splines, joint-linear on
  constant-speed segments, exact knots; a point shared by two segments/phases belongs to the ending
  one). ScenePlayback calls RobotController::applyPlayback with those angles and the saved tool
  transform; main attaches this collaborator after both exist. present keeps a local copy of the motion
  handle because callbacks may release the active motion. The coordinator owning the path starts/resumes
  it; stateChanged reports start/resume/pause/end, playbackChanged each presented time,
  failed(ownerId, error) a failure. RobotController runs one FK, checks all link matrices and the
  saved-tool TCP even with hidden axes, and reuses that TCP in drawing. Commit the pose only after
  drawing acceptance; advance elapsed/phase only after success while playback stays active. Numerical
  errors use the robot console path; drawing errors the renderer path.
- Saved animation points are nodal joint angles with timestamps, no velocity field. Keep FK-derived
  TCP for accepted LIN/central nodes and reuse it when storing them; keep analytical targets separate
  for central XYZ acceleration; FK of interpolated playback angles is separate. Check HOME derivatives
  locally with the existing timing error.
- LIN also fixes points before IK: N = max(1, ceil(T·sqrt(a/(8·0.01 mm)))) per constant-acceleration
  span (T in s, a in mm/s²); cruise has one interval; acceleration 0 is constant-speed LIN without ramps
  (one interval). CIRC (circularProfile: arc about a centre with rotation slerp, used for the probe
  transfers on the start shell) adds peak²/radius to a of every span, so its cruise is subdivided too.
  No recursive subdivision, quarter-time IK checks, added central points or automatic retiming after
  IK/rate failures. The position criterion bounds neither orientation nor actual TCP error after joint
  interpolation. Keep exact shared phase endpoints and continuous joint winding; never wrap saved
  angles modulo 360°.
- During Generate prepare one clamped joint spline per LIN/CIRC and one shared central spline, with
  zero endpoint joint velocities; constant-speed probe Approach/Retract have no spline and play
  joint-linearly. Check polynomial position/speed extrema and write checked intervals into the
  candidate's final coefficient array; discard the candidate on failure without adding points or
  retiming. Dry Run performs no IK, integration, inverse-map/spline construction or preparation pauses.
  Displayed TCP between saved points is approximate; spline smoothness gives no exact Cartesian error
  guarantee.
- Rated joint speeds come from the robot model (equal to KR C4 machine data), without simulation caps.
  ChamferMotion::create and ProbeTouchMotion::create take model/tool/timing once through
  ratedRobotSetup and resolve jointSpeed before IK or splines; inner candidates reuse that setup. Path
  and RSI speeds share chamferJointSpeedError. The grouped inspection skips adjacent speed comparisons
  already accepted while appending but rechecks grouped intervals. The XYZ bound defaults to
  $ACC_MA.CP = 10000 mm/s² (KSS ceiling for programmed CP acceleration; not monitored for RSI). The
  controller has no fixed joint-acceleration value (dynamic model), so joint-acceleration magnitude
  rejection stays disabled in path rate inspection (terminal stops included), spline checks and RSI
  Sampling; do not restore it without a user request. Keep acceleration finiteness, joint
  position/speed and XYZ acceleration checks. HOME timing still uses configured joint acceleration.
- Generate workers own copied inputs and return cohesive results through futures. Accept results and
  insert scene objects on the GUI thread; reject stale source/model/TCP revisions; publish only
  complete motions; insert a replacement before removing its old object, keeping name/visibility. Wait
  for remaining workers at shutdown.
- Saved paths own immutable geometry/model/TCP/settings snapshots and survive source deletion. Robot
  model or TCP changes invalidate playback until regeneration; a sphere replacement, measuring TCP
  change or robot model change removes the calibration path. Preserve pause/resume, Stop-to-HOME,
  completion at HOME and cleanup on readiness loss, mode change or path deletion. Selection cannot
  replace running motion; block generation during playback.
- Generate has no profiling command or statistics output. After a worker finishes, SceneMachining logs
  one line (point count and movement seconds, or the error); SceneCalibration logs touches, points,
  movement seconds and minimum transfer/strut clearances, or the error. The candidate's last error
  string remains the UI error. MotionBuilder and RSI sampling share ChamferTcpSolver::solveTcp; the
  chamfer start search uses its inverse.
- Calibration Generate builds createProbeTouchPlan (fixed uniform 37-touch grid, per-touch tilt and
  roll, one wrist configuration; only sphere centre, radii and strut are inputs) and
  ProbeTouchMotion::create: PTP HOME to entry, LIN descend, per touch a CIRC transfer on the start shell
  and constant 1 mm/s LIN Approach/Retract, LIN retreat, PTP HOME. Every stored point keeps the ball
  ≥ 3 mm from the strut, and every point outside Approach/Retract ≥ 3 mm from the sphere (0.001 mm
  tolerance). Errors start with the phase text ("Touch 22/37: transfer: ..."), also shown by Dry Run.
  No RSI, KRL or real touch exists for calibration.
- RSI: before edits read resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md (frames, signals, PC
  behavior, offset reconstruction, diagnostics, shared_data.txt and the KRL reader), then
  chamferrsistream, scenemachiningpreparation and devices/rsi/rsidevice. The constraints below are not
  repeated there:
  - Sampling success, a bound socket or a delivered reply is not controller readiness and never grants
    permission to move the robot. The RSI Sampling button also requires RSI DeviceRunner.isConnected.
  - sampleChamferRsiStream targets come only from ChamferCentralMotion::evaluate (inverse quintic arc
    map), never from interpolated stored poses. The chord bound relies on every saved central Generate
    interval being ≥ 4 ms. Keep composed TCP feedback separate from FK residual checking.
  - ChamferMotion::create rejects transfer speed above $VEL_MA.CP and acceleration above
    cartesianAcceleration; KRL does not repeat these checks and the controller enforces its live
    limits when KRL assigns $VEL.CP/$ACC.CP before PTP PsIn. Keep QSaveFile replacement and install
    corrections only after export succeeds.
  - The stream belongs to its sampled path, not the selection. Deliver it to ControlIO as an implicitly
    shared QVector<V6d> plus its diagnosticId through the queued main.cpp link; keep that identifier
    paired with the active corrections.
  - The RSI test LIN (resources/files/rsi/RSI_Probe_Lin_CONTRACT.md) is the only other stream:
    prepareTestLin drops any held stream and shared_data.txt like a new request and queues 25000
    increments of +0.004 mm WORLD X plus 50 zeros with a `test_lin_` ID through the same link; a later
    Sampling request, startup or shutdown drops it.
  - Keep numerical work, file I/O, console output and unfinished-worker waits out of ControlIO receive
    callbacks; publish on state transitions/correction updates; device shutdown joins outstanding
    saves. Plan and packet filenames share the stream identifier; flush old-ID rows before replacing
    the active stream, keeping whole exchanges.

## QML

Inspect changed components and their custom base types (properties, signals, default content,
sizing). Read device/scene contracts when UI changes touch those APIs, and Build and modules for
registration, module names or resources.

- Reuse qml/Modules/Components controls and Styles Colors/Fonts/Metrics. No explanatory/tutorial
  labels: concise field names, units, statuses and actionable errors; explanations go to docs/chat.
- Measuring EE/Machining EE settings open in the right Properties area; Close restores object
  properties. Measuring EE edits the ruby ball centre TCP with Apply. End-effector numeric fields show
  four decimals and accept at most four fractional digits; read-only ER rounding never changes the
  stored ER transform.
- Use implicitWidth/implicitHeight for natural size and keep useful inherited sizing. Before
  overriding styled Controls inspect contentItem/background sizes, padding and insets; Item wrappers
  may need their own implicit size.
- A Qt Quick Layout owns its immediate children's geometry: use Layout.* and implicit sizes, not
  concurrent x/y/width/height or anchors. Anchors on a layout are valid when its parent is not a
  layout. Check parent/child size dependencies for binding cycles.
- WorkspaceController owns mode/submode and selected IDs; Main binds them read-only and sends
  selection/delete/intersection/mode requests. C++ connects viewport picks/Delete and selection
  drawing notifications. Before applying new input settings notify QML so Cancel restores the old
  editor settings; a successful Generate closes its editor before requesting selection. Main creation
  resets Measuring/Rough/empty selection with the existing Stop-to-HOME behavior.
- In Calibration the toolbar shows only Measuring EE, Generate Path, Dry Run/Pause and Stop; Generate
  Path calls App.Calibration.generate() without an editor; the Scene panel lists only the Calibration
  group; Delete removes nothing; the status bar's existing label shows the calibration error, playback
  status or unavailable reason. WorkspaceController emits calibrationActiveChanged(false) before
  changing the tool, so calibration playback stops first.
- Keep selected input, captured Generate source and active playback separate; UI drafts, focus, panel
  visibility and group expansion stay in QML. Cached renderer replay already restores selection.
- Reusable components take explicit inputs, not caller IDs/context. Use known concrete QML types;
  keep var for heterogeneous data. Require inputs without a valid default and update all creators.
- Delegates with required properties declare every consumed role/view value (index/model/modelData,
  or TableView row/column) and qualify access through IDs. Persistent state lives in models/backend,
  not disposable delegates.
- Qualify root properties from children (root.title); declare handler parameters
  (onActivated: index => ...). Keep simple handlers inline, named functions for reused/substantial
  logic. Preserve bindings on state updates.
- Components/Styles exports update both module CMakeLists.txt and the source qmldir (maintained
  hot-reload inputs). Keep pragma Singleton, QT_QML_SINGLETON_TYPE and qmldir consistent.
