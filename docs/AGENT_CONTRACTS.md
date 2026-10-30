# RoboCrap — subsystem contracts

These are the area-specific requirements referenced by [AGENTS.md](../AGENTS.md).
Read the sections required for the task before changing behavior; inspect current code/callers
to verify implementation claims. User instructions and the root mandatory rules still apply.

## Build and modules

- Root CMakeLists.txt owns project/options, dependency discovery, target subdirectories and installation.
  src/application/CMakeLists.txt declares robocrap and all private application C++/QML/resource ownership.
  It links Geometry, Kinematics, Trajectories, Devices and Viewport, plus the existing Qt/QML dependencies.
  Keep QmlChartBridge under Application and its QtCharts requirement; RSI XML streams use QtCore.
  Scene/Backend targets and the aggregate src/CMakeLists.txt no longer exist.
- src/{geometry,kinematics,trajectories}/CMakeLists.txt own RoboCrapGeometry (surfaces/math),
  RoboCrapKinematics (KR10 numerics) and RoboCrapTrajectories (animation and RSI sampling numerics).
  Kinematics publicly links Geometry; Trajectories publicly links Kinematics. Their own public
  includes are limited to include/<capability>; Geometry additionally exports Eigen.
- src/devices/CMakeLists.txt owns RoboCrapDevices (workers/codecs, including RsiDevice in src/devices/rsi).
  GUI DeviceHub/DeviceRunner/DeviceProfileModel belong to private Application. Devices links QtNetwork
  and publicly links Geometry only for the `QVector<V6d>` RSI correction stream; it has no Trajectories dependency.
  PlcMessageManager remains a QObject with dictionary APIs and a published header for PlcDevice/PlcMessage enums.
  Devices owns no QML registration or QQmlPropertyMap. Its metatype metadata is extracted for consumers.
- src/viewport/CMakeLists.txt owns RoboCrap3D, URI RoboCrap.Viewport3D. It requires bundled OCCT 8.0.0
  and a Windows 64-bit MinGW Qt kit. It publicly links Geometry and QtCore/Gui/Qml, privately OCCT,
  and has no Application/Trajectories dependency. Only include/viewport is exported, including the
  existing singleton wrapper needed by Application's C++ instance wiring.
- Application has no public C++ include directory. All five libraries export only their selected include
  directories (plus Eigen from Geometry). No target uses the legacy src include directory; the former
  src/pathgeneration, src/network and src/scene trees are gone. No old-path forwarders remain.
- Saved motions still expose central-motion/arc/path values; retain their published headers until
  consumers/storage can change separately. Kr10Kinematics belongs to Kinematics.
- Preserve URIs robocrap_qml_module, RoboCrap.Viewport3D, Components and Styles. The former
  RoboCrap.Backend URI is retired; its existing element names are registered by private Application's
  backendqmltypes.h. Main uses App; other QML files may retain Backend as an alias for the application URI.
  RDTResponse remains the same queued Q_GADGET value, with rdtResponse registered by an Application foreign wrapper.
- Application singletons (Machining, MachiningPreparation, Playback, Calibration, EndEffectors, Scene, Hub,
  WorkspaceController) return main()-owned instances through create() and the shared claimQmlSingleton check in
  backendqmltypes.h: qFatal on a null instance, wrong thread or second live engine, then CppOwnership.
  Qt 6.8 is the minimum, so QQmlEngine::setExternalSingletonInstance (Qt 6.12) is not available.
  RobotController is not a QML type. Keep construction/lifetime order: main constructs ScenePlayback
  after the document and before machining and calibration, and workspace after its document, machining
  and end-effector owners. Do not add diagnostic context properties.
- Both hot-reload branches declare the same application module. QML_OPTIONS contains build flags only.
  Application QML/config files live under src/application/qml; source paths relative to Application
  preserve the qml/... resource paths and the ON branch's /qml/config.json resource. Root
  hot_reload_config.json selects src/application/qml/Main.qml and mirrors src/application/qml;
  retain its qml entry for Main's cached qml/Modules import path and the Components/Styles mappings.
  Reusable modules remain under qml/Modules. robocrap defaults to the build root
  (plus the normal configuration suffix); preserve explicit CMAKE_RUNTIME_OUTPUT_DIRECTORY overrides.
- Attach the existing OCCT POST_BUILD deployment helper from Application, the executable's declaring
  directory; root owns deployment setup and installation. Keep deployed CAD/resources beside the executable.
  The helper copies resources/json/calibration_sphere.json to <exe dir>/resources/json/ only when that
  file is absent, so a build never overwrites an edited deployed sphere file; it is not in a qrc.
  File/type moves require owning source-list/import updates; asset moves require resource-URL updates.

## Viewport and CAD

- The public controller header defines concrete drawing values with no OCCT math types or scene/
  machining objects. SceneObject supplies analytic patches/curves/spheres, saved FK TCP phase or
  segment polylines and bounded diagnostic positions; SceneMachining supplies the normalized signed draft axis and length.
  Preserve 256 markers/128 normal positions, 64 ellipse markers and one edge-centre normal. SceneModel
  requests drawing acceptance before publishing renderable objects; failed installation leaves the
  old document object and accepted drawing cache intact. main attaches the empty document first.
- OccController owns the accepted DrawingDocument and DrawingPose; its window borrows both read-only
  and is destroyed first. OccController calls the window's OccViewport directly for document, selection,
  draft-axis and spindle-frame updates, looking it up per call (null without a native surface) and
  re-applying selection after insert/remove/visibility changes. Replay follows stable ID order, restores
  visibility/diagnostics/selection/draft axis, and needs no application QObject pointer, curve
  resampling or FK. IDs are never reallocated by replay.
  RobotController supplies DrawingPose with BASE-relative link, tool and composed TCP/collet matrices.
  Renderer axis setters accept those absolute frames; no tool-transform composition occurs there.
  OccRobotAdapter converts numerical matrices to gp_Trsf; OccScene/OccPart retain OCCT types.
  OccPart applies style/transform inputs at creation and retains its AIS shape/selection/topmost state.
  World/TCP/ER axes use separate OccWorldAxes objects, independent of part presentation.
- OccScene stores only live parts keyed by PartId. Its optional increasing counter starts at zero,
  permits the final representable ID and stays exhausted afterward; removal never resets identity.
  Erase a part only after OCCT removal succeeds. Track selected visible part IDs, including curves,
  and retain unchanged outlines. Same application IDs can resolve to hidden/replaced parts.
  Keep unfinished outline handles/completion state through external failures; remove selection
  before hiding/deleting parts. Preserve selection styling and decorative picking/fit exclusion.
  A part without face boundaries (DrawingSphere: its seam and pole edges are not drawn) gets no
  selection outline; a hidden DrawingPart is not installed. A transparent part gets its own copies of
  the Selected/LocalDynamic highlight styles with its transparency, so its highlight stays translucent.
- RobotViewport.qml hosts a native QWindow; Qt Quick items cannot draw over it. Busy state is
  shown in the status bar; docs/OCCT_SCENE_OVERLAY.md records the verified way to draw over
  the scene if that is required again. CadLoadWorker parses CAD; robot state, QML properties
  and OCCT presentation stay on the GUI thread. viewportReady means viewer initialization,
  independently of robot assets; ready means robot-preview readiness for kinematics.
- main.cpp requests the single robot load once after console wiring; QML recreation does not
  request it. No robot replacement or individual link-deletion workflow exists. Missing startup
  graphics data means wait, not clear. Failed link/tool insertion or initial HOME-transform application
  reports its stage and removes only parts from that installation; readiness requires complete
  success, including both end-effector parts. Cleanup retains part IDs until removal succeeds and
  reports failures without retrying.
- After initialization, a failed playback transform reports a console error and reapplies
  the last accepted matrices once to existing links, tools and axes. Do not delete/recreate links,
  reload CAD or run FK/IK for restoration. Reject the candidate and stop playback without advancing
  elapsed time/phase. If restoration also fails, report it and disable robot readiness; do not
  claim the displayed pose was restored.
- Actual native-surface recreation may install cached graphics in the new empty scene without
  reloading assets or republishing the model. It is not movement-error recovery. Preserve
  application presentations. End-effector models are fixed: no tool CAD selection, Browse or Retry.
  Stop initial camera fitting after a gesture on that surface. Decorative axes are non-selectable
  and excluded from camera-fit bounds.
- RobotDefinition::kr10() (application/robot/robotdefinition.*) builds the fixed KR10 numerical model and
  ordered link filenames/RGB from a constant table without OCCT; changing robot geometry means editing
  that table. RobotController's HOME/FK check and Generate's validRobot guard the values.
  robotgraphics.h supplies neutral graphics, the two tool paths in RobotTool order and RobotTool;
  adapters construct OCCT values. Renderer link count comes from the supplied graphics.
- CadLoadWorker accepts absolute .brep paths and returns ordered shapes/errors. One job loads the seven
  links followed by MEE.brep and SEE.brep; OccRobotAdapter installs them together and shows only the
  active tool. Serialize OCCT parsing requests. CadLoadResult is independent of the worker.
- Application RobotController (application/robot/robotcontroller.*) owns the stable model/solver and accepted pose.
  Keep the checked initial HOME candidate unpublished until graphics acceptance; publish the model
  to ScenePlayback once, and commit later poses only after returned transform acceptance. Renderer
  drawing values/CAD results restore surfaces/windows without a robot-state/model pointer or a
  model revision change.
- Application assetpaths.* selects the deployed CAD and OCCT resource directories; main passes the
  OCCT resource directory to OccController at construction for initialization retry/window recreation.
  Its initializeOcct retains resource checks, environment variables and external Font_FontMgr failure
  handling; no Application header or path selection occurs in Viewport.
- CAD is BREP only: the nine files in resources/cad/kr10 (seven links, MEE, SEE) are read directly from
  applicationDirPath()/resources/cad/kr10. There is no STEP import, BREP cache writing or TKDESTEP link.
  cmake/DeployOcct.cmake stages OCCT DLLs/runtime resources and the *.brep files beside the executable
  and at installation.

## Devices and protocols

Before device/UI integration changes, read src/application/devices/{devicehub,devicerunner}.{h,cpp},
src/devices/abstractdevice.cpp, src/devices/include/devices/abstractdevice.h and src/application/main.cpp.
Also read Build and modules for QML registration changes.

- Application's Hub.device(key) returns a DeviceRunner on the application/QML thread; keep its
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
  FK frames. There is no angles-only checked IK; callers carry the accepted IkSolution as seed.
- SceneEndEffectors owns accepted tool XYZABC and its optional reconstructed transform on the
  GUI thread. Consumers reuse spindleTcpFrame; workers copy its value. Reconstruct from accepted
  coordinates, not the raw Apply product or rounded display values. Compose FLANGE_ER * ER_TCP
  in that order. Commit coupled list/frame values before notifications; unchanged values must not
  invalidate paths. Retain TCP notification on collet changes even when TCP is equivalent, and
  preserve the flange-based Kr10Kinematics FK/IK APIs. There is no manual robot FK/IK command.
  The measuring TCP (ruby ball centre, TOOL_DATA[5] by default) is applied with applyMeasuringTcp,
  reconstructed as measuringTcpFrame and not persisted; an unchanged value emits nothing.
- SceneModel owns SceneObject; QML changes scene state through model methods. SceneObject stores a
  fitted plane, cylinder, intersection edge (with its two source IDs), calibration sphere or
  calibration path (SceneTouchPath, the shared immutable ProbeTouchMotion) by value in a std::variant;
  only a machining path has a child QObject (SceneMachiningPath, exposed as geometry). Its CONSTANT
  properties list supplies the formatted {label, value} rows that the Properties panel renders.
  Preserve stable scene-allocated IDs (zero means no object), shared references and GUI-thread mutations.
  Numerical fitting/intersection belongs to Geometry; OCCT presentation belongs to adapters.
  Decorative points/normals/axes do not participate in picking or camera-fit bounds.
- Before import/intersection changes, read numerical geometry factories and
  SceneSurfaceImporter/SceneModel callers; preserve units and probe-compensation semantics.
- BoundedPlane/BoundedCylinder accept points or decoded ProbeSamples, without file factories.
  SceneSurfaceImporter reads/decodes plane and cylinder inputs; there is no circle import.
- fitSphere (Geometry) is the algebraic least-squares sphere fit on mean-centred points (JacobiSVD);
  it rejects fewer than four or nonfinite points, a smallest singular value below 1e-9 times the
  largest and a nonpositive squared radius. SceneCalibration reads the deployed sphere file on the GUI
  thread at every entry into the Calibration submode (version 1, positive sphereDiameter,
  strutDiameter and strutRecess below the diameter, at least four finite roughPoints X/Y/Z; other keys
  ignored). The fitted centre and the ball-centre radius come from the readings; the compensated
  radius subtracts the ruby ball radius; the nominal radius only sizes the touch plan. A changed sphere
  replaces the object. While Calibration is active only the sphere and the calibration path are
  visible and listed; entry records and hides the other visible objects (also ones added meanwhile),
  exit shows the recorded ones that still exist. Neither calibration object can be deleted.

## Machining and playback

Current work covers animation preparation/playback and nominal RSI Sampling/streaming for the T1
RSI_Chamfer_Hole run; force correction is out of scope. Apply Geometry and scene to numerical changes.

- SceneMachining exposes SceneMachiningSettings as settings. Main calls App.Machining.generate(),
  implemented by SceneMachining::generate. Keep it separate from dryRun, which only plays a prepared
  immutable ChamferMotion. End-effector Apply actions are separate.
- Animation implementations live in src/trajectories/chamfer and src/trajectories/probe, built on
  the motion module src/trajectories/motion, with published values under
  src/trajectories/include/trajectories, and invoke no scene/device APIs. Inspect the relevant
  producer/consumer: chamferpath, chamferarclength, chamfercentralmotion, chamfercentralvalidation,
  chamfermotion, probetouchmotion and motion/{timedmotion,motionbuilder,tcpsolver,jointsegments}.
  ChamferPath provides analytical evaluation/differentials; arc maps expose progressAtDistance.
- Keep geometric parameter, traveled distance and physical time distinct. Recover TCP position/
  orientation by evaluating the analytical curve at the inverse arc-length parameter; do not
  substitute stored XYZ/ABC interpolation without explicit redesign. Inverse-map knots,
  animation points and RSI cycles are different data.
- Lead evaluation may skip overwritten machining transform composition only after the existing
  machining axis-sign/basis, positive-depth and finite-position checks. Preserve the final lead
  basis checks and exact complete junction poses. Fixed chamfer trigonometry belongs to each
  immutable path; retain tangent division/reciprocal and copy the cache with its owned parameters.
- Preserve the piecewise quintic Hermite inverse, all nine inverse-check fractions and endpoint
  derivative conditions. Reuse accepted endpoint differentials and the original integral/error;
  retain stored-distance rounding checks. The per-phase 800000 limit counts original logical
  differential calls, including reused work, and rejects the phase when exceeded; it is not exported.
  Charge only reached work at the original check positions.
- Fix central animation points/times before IK using curvature/time. Within one Generate worker,
  share three central arc-length maps, timing and schedule; construct them only when a candidate
  reaches the curve (ChamferCentralPreparation::prepare). Cache analytical targets/failures only at
  reached scheduled indices (CentralPath::target); solve/check joints anew for each seed.
- MotionBuilder builds every generated motion from HOME: appendPtp (joint steps of at most 2°,
  ptpDuration and quintic smoothstep timing) and appendPath for a TimedPath (LinearPath for LIN and
  CIRC, CentralPath for the chamfer curve). appendPath checks, in order: the path start against the
  preceding stored TCP (0.0001 mm, 0.000001 rotation entries); for each scheduled point seeded
  ChamferTcpSolver::solveTcp and the joint speed of the new interval; then the grouped rate
  inspection (secants of at least 1e-4 s from rest to rest: grouped joint speeds, XYZ acceleration of
  the prescribed targets, the stop at the segment end); then the segment's spline. A segment is
  stored only after all its checks pass. Errors read "<phase label>: <message> (t=<path-local s> s)";
  the first failure is the first failing point in point order (user decision, 2026-10-26).
- ScenePlayback owns Dry Run, the robot model copy and robot availability. It plays a
  std::shared_ptr<const TimedMotion> that aliases the timed() motion of a saved ChamferMotion or
  ProbeTouchMotion and so keeps that owner alive. TimedMotion::evaluateJoints(seconds) returns
  optional angles and the phase index without FK (smoothstep on PTP segments, stored cubics on
  spline segments, joint-linear interpolation on constant-speed segments, exact stored knots; a
  point shared by two segments or phases belongs to the ending one); ScenePlayback calls
  RobotController::applyPlayback directly with those angles and the saved tool transform; main
  attaches its guarded collaborator after both owners are constructed. present keeps a local copy of
  the motion handle, because readiness/pose callbacks may release the active motion. The coordinator owning the
  playing path starts/resumes it; stateChanged reports start/resume/pause/end, playbackChanged every
  presented time, failed(ownerId, error) a failure. RobotController runs one
  FK, checks all link matrices and the saved-tool TCP even when its axes are hidden, and reuses that
  checked TCP in drawing. Commit the robot pose only after drawing acceptance; advance elapsed/phase
  only after returned success while playback remains active. Numerical errors use the robot console
  path; drawing errors retain the renderer console path.
- Saved animation points are nodal points with joint angles and timestamps. Retain FK-derived
  TCP for accepted LIN/central nodes and reuse it when storing them; keep analytical targets
  separate for central XYZ acceleration. FK for interpolated playback angles is separate.
  Saved points retain no velocity field; check HOME derivatives locally with the existing timing error.
- LIN also fixes points before IK: N=max(1,ceil(T*sqrt(a/(8*0.01mm)))) per constant-acceleration
  span, with span duration T in seconds and acceleration magnitude a in mm/s²; cruise has one
  interval. Acceleration 0 means a constant-speed LIN without ramps (one interval). A CIRC
  (circularProfile, arc around a centre, rotation slerp) adds the normal-acceleration bound
  peak²/radius to a of every span. No recursive subdivision, extra quarter-time IK checks, added central points or
  automatic retiming after IK/rate failures. The position criterion bounds neither orientation
  nor actual TCP error after joint interpolation. Preserve exact shared phase endpoints and
  continuous joint winding; never wrap saved angles modulo 360 degrees.
- During Generate, prepare one independent clamped joint spline per LIN/CIRC and one shared central
  spline, with zero endpoint joint velocities; constant-speed probe Approach/Retract have no spline
  and play joint-linearly between their two points. Check polynomial position/speed extrema.
  Write checked intervals into the candidate's final coefficient array; discard the candidate on failure.
  Reject failures without adding points or retiming. Dry Run performs no IK, geometry integration,
  inverse-map/spline construction or pauses to prepare more points. Displayed TCP between saved
  points is approximate; spline smoothness gives no exact Cartesian error guarantee.
  Curvature derivation: plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md.
- Rated joint-speed bounds come from the robot model (equal to the KR C4 machine data), without fixed
  simulation caps. ChamferMotion::create and ProbeTouchMotion::create accept model/tool/timing once
  through ratedRobotSetup and resolve jointSpeed before IK or spline preparation; inner candidates reuse that immutable setup.
  Path and RSI vector speeds share chamferJointSpeedError. The grouped rate inspection skips
  adjacent speed comparisons already accepted while appending; retain checks of grouped intervals.
  The Cartesian XYZ bound defaults to $ACC_MA.CP = 10000 mm/s², the KSS ceiling for
  programmed CP path acceleration; the controller does not monitor it for RSI. The controller has no
  fixed joint-acceleration value (dynamic model), so joint-acceleration magnitude rejection stays
  disabled in the path rate inspection (including terminal stops), spline checks and RSI Sampling. Retain
  acceleration finiteness, joint position/speed and XYZ acceleration checks. HOME timing still uses
  configured joint acceleration. Do not restore joint-acceleration rejection without a user request.
- Generate workers own copied numerical inputs and return cohesive results through futures.
  Accept results/insert scene objects on the GUI thread; reject stale source/model/TCP revisions.
  Publish only complete motions. Insert a successful replacement before removing its old object,
  preserving name/visibility. Wait for remaining workers at shutdown.
- Saved paths own immutable geometry/model/TCP/settings snapshots and survive source deletion.
  Robot model or TCP transform changes invalidate playback until regeneration; for the calibration
  path a sphere replacement, measuring TCP change or robot model change removes the path. Preserve pause/resume,
  Stop-to-HOME, completion-at-HOME and cleanup on readiness loss, mode change or path deletion.
  Selection cannot replace running motion; block generation during playback.
- Generate has no profiling command or timing/statistics output. MotionBuilder and RSI sampling share
  ChamferTcpSolver (motion/tcpsolver.h) for checked TCP IK (solveTcp); the chamfer start search uses
  its inverse. Shared PTP/LIN/CIRC/joint-spline numerics and ratedRobotSetup live
  in src/trajectories/motion/jointsegments. After a Generate worker finishes, SceneMachining logs one console line: point count and movement seconds, or
  the error; SceneCalibration logs touches, points, movement seconds and minimum transfer and strut
  clearances, or the error. The candidate's last error string remains the UI error. Keep inverse-map knots,
  animation points and RSI samples distinct.
- Calibration Generate builds createProbeTouchPlan (the fixed 31-touch ring scan; only the sphere
  centre, radii and strut come from inputs) and ProbeTouchMotion::create on a worker: PTP HOME to
  entry, LIN descend, per touch a CIRC transfer on the start shell, constant 1 mm/s LIN Approach and
  Retract, one PTP wrist unwind after T21, LIN retreat, PTP HOME. Every stored point keeps the ball at
  least 3 mm from the strut; every point outside Approach/Retract keeps it at least 3 mm from the
  sphere (0.0001 mm tolerance). Errors start with the phase text ("Touch 22/31: transfer: ..."), which
  Dry Run also shows. No RSI, KRL or real touch exists for calibration.
- Before RSI edits, read resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md, then chamferrsistream,
  scenemachiningpreparation and devices/rsi/rsidevice. Sampling success, a bound socket and a
  delivered reply are not controller readiness and never grant permission to move the robot.
  The RSI Sampling button also requires the existing RSI DeviceRunner.isConnected state.
- sampleChamferRsiStream targets come only from ChamferCentralMotion::evaluate (inverse quintic arc
  map); never interpolate stored poses. One seeded checked IK/FK per rounded reconstructed TCP
  sample; position/speed and XYZ acceleration checks use that reconstructed movement. The chord bound
  relies on every saved central Generate interval being at least 4 ms. Start from the six-decimal
  exported LeadStart pose. XYZ offsets reconcile against the preceding rounded reconstructed position;
  the stream ends with 50 zero settle cycles.
  ABC offsets use POSCORR1 RefCorrSys = World; derive them from desired and preceding TCP rotations
  expressed in WORLD, reconstructing after offset rounding. Do not subtract absolute TCP Euler coordinates.
  Check reconstructed poses against the analytical targets with the existing TCP residual tolerances.
  Export LeadEnd from the composed final pose. CSV targets/exported poses use principal Z-Y-X ABC;
  absolute Euler wrapping does not reject matrix offsets. The summary reports maximum translation
  component and shortest rotation-angle excursions from LeadStart, not a POSCORR MaxRotAngle bound.
  Keep composed TCP feedback separate from FK residual checking. The endpoint retains the saved
  Status and nearest 360-degree joint representatives (differences strictly below 180 degrees);
  pose accuracy retains the TCP residual tolerances. Turn sign bits are not a winding comparison.
- shared_data.txt version 2 has eight CRLF rows: version, PsIn/LeadStart/LeadEnd XYZABC+S/T,
  PsOut S/T, ToolFrame XYZABC, transfer velocity (m/s), transfer acceleration (m/s²).
  Floats use six decimals; S/T are decimal integers from each saved boundary's own joints
  (boundaries 1, 2, 5, 6), classified by existing configuration with one local FK per boundary.
  Check exported poses, including PsOut constructed from PsIn XYZ and LeadEnd ABC, against
  their saved TCP poses with existing position/rotation-entry tolerances before file replacement.
  Keep QSaveFile atomic replacement and install corrections only after export succeeds.
  ChamferMotion::create rejects transfer speed above $VEL_MA.CP = 3000 mm/s and transfer
  acceleration above cartesianAcceleration ($ACC_MA.CP); KRL does not repeat these checks and
  assigns $VEL.CP/$ACC.CP before PTP PsIn, where the controller enforces its live limits.
  The SRC's local READ_JOB requires version 2, reads motion targets with FILE_UTILS READ_POS
  (S 0-7, T 0-63) and ToolFrame (flange-to-TCP FRAME, no S/T) with READ_FRAME, accumulating
  one success flag, and closes the file before returning; failures return before PTP PsIn.
  PsOut is POS: copied from PsIn, ABC from LeadEnd, then its own S/T read. S/T selects PTP
  configuration; LIN ignores it. RSI_RESET at program start replaces RSI_DELETE.
- The stream belongs to its sampled path, not the selection. Drop it and delete shared_data.txt when
  the path is removed, regenerated or incompatible, at a new request, at startup and at shutdown.
  Deliver it to ControlIO as an implicitly shared QVector<V6d> plus its sampling diagnosticId through
  the queued main.cpp link; keep that identifier paired with the active corrections. Every accepted
  sampling request queues an empty stream before its worker starts, even without a held GUI result.
- RsiDevice answers each accepted packet once (newest of a queued burst), advances one increment per
  packet only while Start = 1, and replaces/clears its array immediately on correction updates in
  every state, resetting to Idle and index 0. The first accepted Start = 1 after installation sends
  the new array's first increment; no preceding Start = 0 is required. Normal completion sends Stop
  after the controller echoes Result = 1; Start without a stream sends Stop with Result 0. Keep
  numerical work, file I/O and console output out of receive callbacks: a reply sent later than the
  controller cycle is rejected while the index has already advanced; publish on state transitions/
  correction updates. Correction updates discard buffered datagrams without replying or advancing
  the new array, and reset the learned peer port. Preserve configured endpoints and socket binding.
- RSI diagnostics automatically save analytical targets, joints checked for reconstructed TCP poses
  and increments as CSV on the sampling worker, and
  bounded exact-XML packet captures through owned copied-input save futures after receive silence
  or disconnect. Preserve actual RIst/AIPos fields, monotonic packet/send timing, the reply rows'
  handler/event-loop sleep/wake times, stream indices, write results and capture truncation.
  Keep disk I/O and unfinished-worker waits off ControlIO callbacks; device shutdown joins outstanding saves. Follow the RSI contract for formats/locations.
  Plan and packet filenames share the sampled stream's identifier; repeated packet captures add a
  part suffix. Flush old-ID rows before replacing the active stream, preserving whole exchanges.

## QML

Inspect changed components and their custom base types, including properties, signals, default
content and sizing. Read device/scene contracts when UI changes touch those APIs, and Build and
modules when changing registration, module names or resources.

- Reuse compatible qml/Modules/Components controls and Styles Colors/Fonts/Metrics.
  Do not add explanatory/tutorial labels; use concise field names, units, statuses and actionable
  errors. Keep explanations in documentation/chat.
- Measuring EE/Machining EE settings open in the right Properties area; Close restores object
  properties. Measuring EE edits the ruby ball centre TCP with Apply. End-effector numeric fields display four decimals and accept at most four fractional
  digits. Read-only ER rounding does not change the stored ER transform.
- Use implicitWidth/implicitHeight for natural preferred size; preserve useful inherited sizing.
  Before overriding styled Controls, inspect contentItem/background sizes, padding and insets.
  Item wrappers may need their own implicit size.
- A Qt Quick Layout owns immediate-child geometry: use Layout.* and implicit sizes, not concurrent
  x/y/width/height or anchors. Anchors on the layout are valid when its parent is not another layout.
  Check parent/child size dependencies for binding cycles.
- WorkspaceController owns mode/submode and selected IDs; Main binds them read-only and sends
  selection/delete/intersection/mode requests. C++ connects viewport picks/Delete and selection
  drawing notifications. Before applying new input settings, notify QML so Cancel restores the
  old editor settings; successful Generate closes its editor before requesting selection.
  Main creation resets Measuring/Rough/empty selection using existing Stop-to-HOME behavior.
  In the Calibration submode the toolbar shows only Measuring EE, Generate Path, Dry Run/Pause and
  Stop; Generate Path calls App.Calibration.generate() without an editor; the Scene panel lists only
  the Calibration group; Delete removes nothing; the status bar shows the calibration error, playback
  status or unavailable reason in its existing label. WorkspaceController emits
  calibrationActiveChanged(false) before changing the tool, so calibration playback stops first.
  Keep selected input, captured Generate source and active playback separate; UI drafts/focus/
  panel visibility/group expansion remain QML. Cached renderer replay already restores selection.
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
