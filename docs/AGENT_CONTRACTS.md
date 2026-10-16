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
  RoboCrapKinematics (KR10 numerics) and RoboCrapTrajectories (animation and retained RSI numerics).
  Kinematics publicly links Geometry; Trajectories publicly links Kinematics. Their own public
  includes are limited to include/<capability>; Geometry additionally exports Eigen.
- src/devices/CMakeLists.txt owns RoboCrapDevices (workers/codecs, including frozen RSI at legacy paths).
  GUI DeviceHub/DeviceRunner/DeviceProfileModel belong to private Application. Devices links QtNetwork
  and publicly retains Trajectories for its frozen RSI interface; its non-RSI headers use QtCore/Network.
  PlcMessageManager remains a QObject with dictionary APIs and a published header for PlcDevice/PlcMessage enums.
  Devices owns no QML registration or QQmlPropertyMap. Its metatype metadata is extracted for consumers.
- src/viewport/CMakeLists.txt owns RoboCrap3D, URI RoboCrap.Viewport3D. It requires bundled OCCT 8.0.0
  and a Windows 64-bit MinGW Qt kit. It publicly links Geometry and QtCore/Gui/Qml, privately OCCT,
  and has no Application/Trajectories dependency. Only include/viewport is exported, including the
  existing singleton wrapper needed by Application's C++ instance wiring.
- Application has no public C++ include directory. All five libraries export only their selected include
  directories (plus Eigen from Geometry). Frozen network/rsi and scene preparation/shared-data files
  remain at their original paths, owned by Devices and Application respectively. Devices/Application
  retain PRIVATE src access for those headers and generated moc; Trajectories limits it to its six
  RSI numerical translation units. These consumers include canonical capability headers directly;
  private socket/diagnostics/machining includes use explicit relative paths. No old-path forwarders remain.
- Saved motions still expose central-motion/arc/path values; retain their published headers until
  consumers/storage can change separately. Kr10Kinematics belongs to Kinematics.
- Preserve URIs robocrap_qml_module, RoboCrap.Viewport3D, Components and Styles. The former
  RoboCrap.Backend URI is retired; its existing element names are registered by private Application's
  backendqmltypes.h. Main uses App; other QML files may retain Backend as an alias for the application URI.
  RDTResponse remains the same queued Q_GADGET value, with rdtResponse registered by an Application foreign wrapper.
- RobotController/robotOperationResult and WorkspaceController remain typed application registrations.
  Keep C++ ownership, guarded single-engine/thread checks and construction/lifetime order. Main constructs
  workspace after its document/machining/robot owners. Do not add diagnostic context properties.
- Both hot-reload branches declare the same application module. QML_OPTIONS contains build flags only.
  Application QML/config files live under src/application/qml; source paths relative to Application
  preserve the qml/... resource paths and the ON branch's /qml/config.json resource. Root
  hot_reload_config.json selects src/application/qml/Main.qml and mirrors src/application/qml;
  retain its qml entry for Main's cached qml/Modules import path and the Components/Styles mappings.
  Reusable modules remain under qml/Modules. robocrap defaults to the build root
  (plus the normal configuration suffix); preserve explicit CMAKE_RUNTIME_OUTPUT_DIRECTORY overrides.
- Attach the existing OCCT POST_BUILD deployment helper from Application, the executable's declaring
  directory; root owns deployment setup and installation. Keep deployed CAD/cache/resources beside the executable.
  File/type moves require owning source-list/import updates; asset moves require resource-URL updates.

## Viewport and CAD

- The public controller header defines concrete drawing values with no OCCT math types or scene/
  machining objects. SceneObject supplies analytic patches/curves, saved FK TCP phase polylines and
  bounded diagnostic positions; SceneMachining supplies the normalized signed draft axis and length.
  Preserve 256 markers/128 normal positions, 64 ellipse markers and one circle normal. SceneModel
  requests drawing acceptance before publishing renderable objects; failed installation leaves the
  old document object and accepted drawing cache intact. main attaches the empty document first.
- OccController owns the accepted DrawingDocument; its window borrows it read-only and is destroyed
  first. Replay follows stable ID order, restores visibility/diagnostics/selection/draft axis, and
  needs no application QObject pointer, curve resampling or FK. IDs are never reallocated by replay.
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
- RobotViewport.qml hosts a native QWindow. CadLoadWorker parses CAD; robot state, QML properties
  and OCCT presentation stay on the GUI thread. viewportReady means viewer initialization,
  independently of robot assets; ready means robot-preview readiness for kinematics.
- main.cpp requests the single robot load once after console wiring; QML recreation does not
  request it. No robot replacement or individual link-deletion workflow exists. Missing startup
  graphics data means wait, not clear. Failed link insertion or initial HOME-transform application
  reports its stage and removes only links from that installation; readiness requires complete
  success. Cleanup retains part IDs until removal succeeds and reports failures without retrying.
- After initialization, a failed manual/playback transform reports a console error and reapplies
  the last accepted matrices once to existing links, tools and axes. Do not delete/recreate links,
  reload CAD or run FK/IK for restoration. Reject the candidate and stop playback without advancing
  elapsed time/phase. If restoration also fails, report it and disable robot readiness; do not
  claim the displayed pose was restored.
- Actual native-surface recreation may install cached graphics in the new empty scene without
  reloading assets or republishing the model. It is not movement-error recovery. Preserve
  application presentations and tool CAD Browse/Retry. Stop initial camera fitting after a gesture
  on that surface. Decorative axes are non-selectable and excluded from camera-fit bounds.
- RobotDefinition (application/robot/robotdefinition.*) reads the numerical model and ordered filenames/RGB without OCCT.
  robotgraphics.h supplies neutral graphics and RobotTool; adapters construct OCCT values.
  Renderer link count comes from the supplied graphics.
- CadLoadWorker accepts absolute source paths and a cache directory, returning ordered shapes/errors.
  Serialize OCCT parsing/cache requests; robot loading includes only seven links. CadLoadResult
  is independent of the worker.
- Application RobotController (application/robot/robotcontroller.*) owns the stable model/solver and accepted pose.
  Keep the checked initial HOME candidate unpublished until graphics acceptance; publish the model
  to machining once, and commit later poses only after returned transform acceptance. Renderer
  drawing values/CAD results restore surfaces/windows without a robot-state/model pointer or a
  model revision change.
- Application assetpaths.* selects deployed cache and OCCT resource directories and passes them to
  OccController at construction. OccController owns those directory values for CAD jobs and initialization
  retry/window recreation. Its initializeOcct retains resource checks, environment variables and external
  Font_FontMgr failure handling; no Application header or path selection occurs in Viewport.
- json.qrc embeds resources/json/kr10.json. cmake/DeployOcct.cmake stages OCCT DLLs/runtime resources
  and resources/cad/kr10 beside the executable and at installation.
- Generated BREPs live in applicationDirPath()/resources/cad/kr10, never QStandardPaths system
  cache folders. CachedShapeLoader accepts STEP/STP basenames in a selected source directory.
  First try <completeBaseName>.brep in deployed CAD without requiring/hashing STEP.
  Missing/unreadable/invalid BREP falls back to STEP conversion and atomic QSaveFile saving.
  Valid BREP takes priority; STEP/content/OCCT-version changes require removing it to regenerate.
  Basenames define cache identity: equal basenames share the same deployed BREP.
  Keep default .stp filename keys for BREP selection. Regeneration requires an existing STEP source;
  the fixed KR10/tool resources currently provide nine BREPs without those authoring/STEP sources.

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
  FK frames. Keep checked solveIK(target, previousAngles) compatible with separate RSI consumers.
- SceneEndEffectors owns accepted tool XYZABC and its optional reconstructed transform on the
  GUI thread. Consumers reuse spindleTcpFrame; workers copy its value. Reconstruct from accepted
  coordinates, not the raw Apply product or rounded display values. Compose FLANGE_ER * ER_TCP
  in that order. Commit coupled list/frame values before notifications; unchanged values must not
  invalidate paths. Retain TCP notification on collet changes even when TCP is equivalent, and
  preserve flange-based FK/IK APIs and conversion of TCP targets using the tool transform.
- SceneModel owns SceneObject/geometry; QML changes scene state through model methods. Preserve
  stable scene-allocated IDs (zero means no object), shared references and GUI-thread mutations.
  Numerical fitting/intersection belongs to Geometry; OCCT presentation belongs to adapters.
  Decorative points/normals/axes do not participate in picking or camera-fit bounds.
- Before import/intersection changes, read numerical geometry factories and
  SceneSurfaceImporter/SceneModel callers; preserve units and probe-compensation semantics.
- BoundedPlane/BoundedCylinder accept points or decoded ProbeSamples, without file factories.
  SceneSurfaceImporter reads/decodes inputs; src/application/chamferprofile.cpp reuses
  readProbeSamples and fromSamples. Plane::fromJsonFile and utility readers remain legacy RSI exceptions.

## Machining and playback

Current work covers animation preparation/playback only; RSI preparation remains closed unless
the user explicitly reopens it. Apply Geometry and scene to numerical changes.

- SceneMachining exposes SceneMachiningSettings as settings. Main calls App.Machining.generate(),
  implemented by SceneMachining::generate. Keep it separate from dryRun, which only plays a prepared
  immutable ChamferMotion. End-effector Apply actions are separate.
- Animation implementations live in src/trajectories/chamfer, with published values under
  src/trajectories/include/trajectories, and invoke no scene/device APIs. Inspect
  the relevant producer/consumer: chamferpath, chamferarclength, chamfercentralmotion,
  chamfercentralvalidation and chamfermotion.
  ChamferPath provides analytical evaluation/differentials; arc maps expose progressAtDistance.
  Keep ChamferSamplingParameters and saved lead intervals for frozen RSI/QML compatibility.
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
  differential calls, including reused work; return actual/logical counts on success and failure
  and expose both in phase diagnostics. Charge only reached work at the original check positions.
- Fix central animation points/times before IK using curvature/time. Within one Generate worker,
  share three central arc-length maps, timing and schedule; construct them only when a candidate
  reaches central validation. Cache analytical targets/failures only at reached scheduled indices;
  solve/check joints anew for each seed. Count shared computation once, outside per-attempt times.
- Dry Run uses ChamferMotion::evaluateJoints(seconds), returning optional angles/phase without FK.
  SceneMachining calls RobotController::applyPlayback directly with those angles and the saved
  tool transform; main attaches its guarded collaborator after both existing owners are constructed.
  Keep the local immutable motion alive through readiness/pose callbacks. RobotController runs one
  FK, checks all link matrices and the saved-tool TCP even when its axes are hidden, and reuses that
  checked TCP in drawing. Commit the robot pose only after drawing acceptance; advance elapsed/phase
  only after returned success while playback remains active. Numerical errors use the robot console
  path; drawing errors retain the renderer console path. Legacy evaluate(seconds) keeps its TCP
  result and saved endpoint matrices for existing consumers, sharing the same interpolation code.
- Saved animation points are nodal points with joint angles and timestamps. Retain FK-derived
  TCP for accepted LIN/central nodes and reuse it when storing them; keep analytical targets
  separate for central XYZ acceleration. FK for interpolated playback angles is separate.
  Saved points retain no velocity field; check HOME derivatives locally with the existing timing error.
- LIN also fixes points before IK: N=max(1,ceil(T*sqrt(a/(8*0.01mm)))) per constant-acceleration
  span, with span duration T in seconds and acceleration magnitude a in mm/s²; cruise has one
  interval. No recursive subdivision, extra quarter-time IK checks, added central points or
  automatic retiming after IK/rate failures. The position criterion bounds neither orientation
  nor actual TCP error after joint interpolation. Preserve exact shared phase endpoints and
  continuous joint winding; never wrap saved angles modulo 360 degrees.
- During Generate, prepare one independent clamped joint spline per LIN and one shared central
  spline, with zero endpoint joint velocities. Check polynomial position/speed extrema.
  Write checked intervals into the candidate's final coefficient array; discard the candidate on failure.
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
- WorkspaceController owns mode/submode and selected IDs; Main binds them read-only and sends
  selection/delete/intersection/mode requests. C++ connects viewport picks/Delete and selection
  drawing notifications. Before applying new input settings, notify QML so Cancel restores the
  old editor settings; successful Generate closes its editor before requesting selection.
  Main creation resets Measuring/Rough/empty selection using existing Stop-to-HOME behavior.
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
