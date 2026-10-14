# RoboCrap architecture

A verified map of the current code for future sessions. File references are relative to the
repository root. "Verified" means the claim was checked against constructors, `connect` calls
or call sites; anything else is listed under [Open questions](#7-open-questions).
Snapshot date: 2026-10-14 (working tree at HEAD `3709a12`, including uncommitted edits to
`CMakeLists.txt`, `CMakeLists.txt.user` and `resources/cad/kr10/SEE.stp`). No numerical or QML
files are uncommitted in this snapshot. Current source was checked again for the corrections
in `codex_plans/ARCHITECTURE_REVIEW.md` §7; that review's proposed migrations are not implemented
by this map. Historical "user confirmed" notes record context, not permission to override
current task instructions or `AGENTS.md`.

## 1. Scope and hardware context

Hardware (supplied by the user; not derivable from code):

| Device | Model | Code entry point | Transport (from code) |
|---|---|---|---|
| Robot | KUKA KR10 R1420, KR C4, KSS 8.3.31, RSI 3.3 | `RsiDevice` key `rsi` | XML over UDP, local port 5555 (`resources/json/db.json`) |
| PLC | Delta AS332T with AS16AP11P-A / AS16AP11T-A (8 DI + 8 DO each) | `PlcDevice` key `plc` | Custom binary over TCP to 192.168.1.2:5051, fixed source port 1234 |
| Force/torque sensor | Schunk Delta IP68 SI-660-60 (ATI Net F/T electronics) | `FtsDevice` key `fts` | ATI RDT over UDP, peer port 49152 |
| Touch probe | WP-500 V6 | none | No device code. Probe data enters only as JSON point files (§5.1) |

`db.json` also lists a VFD (`driver: "vfd"`) that has no device class. The Beckhoff manuals in
`D:\Qt\QtProjects\manuals` have no counterpart in `src/`, `qml/` or `resources/json/`.

Two parts of the code base are outside the active build: `old_imp/` is not built, and `python/`
holds analysis scripts.

## 2. Layers and module dependency graph

Verified from `CMakeLists.txt`, `src/CMakeLists.txt`, `src/3d/CMakeLists.txt` and
`qml/Modules/*/CMakeLists.txt`.

```mermaid
graph TD
  robocrap["robocrap (exe)<br/>main.cpp, QmlChartBridge, app QML<br/>URI robocrap_qml_module"]
  Backend["RoboCrapBackend (static QML module)<br/>URI RoboCrap.Backend<br/>backendqmltypes.h only"]
  R3D["RoboCrap3D (static QML module)<br/>URI RoboCrap.Viewport3D<br/>OCCT viewport + chamferprofile CLI"]
  Scene["RoboCrapScene (static)<br/>src/scene/*"]
  Devices["RoboCrapDevices (static)<br/>src/network/**"]
  Geometry["RoboCrapGeometry (static)<br/>src/geometry/*, src/pathgeneration/**,<br/>src/3d/robot/kinematics/*, kr10kinematicmodel.h"]
  Eigen["Eigen 5.0.0 (libs/, header-only)"]
  OCCT["OCCT 8.0.0 TK* (libs/OCCT_8_0_0)"]
  Styles["Styles (QML, NO_PLUGIN)"]
  Components["Components (QML, NO_PLUGIN)"]

  robocrap --> Backend
  robocrap --> R3D
  robocrap --> Scene
  robocrap --> Devices
  robocrap --> Styles
  robocrap --> Components
  Components --> Styles
  Backend -->|PUBLIC| Scene
  Backend -->|PUBLIC| Devices
  R3D -->|PUBLIC| Geometry
  R3D -->|PRIVATE| Scene
  R3D -->|PRIVATE| OCCT
  Scene -->|PUBLIC| Geometry
  Devices -->|PUBLIC| Geometry
  Geometry -->|PUBLIC include| Eigen
```

Library notes:

- **Qt dependencies.**
  - `RoboCrapGeometry` links only `Qt6::Core`.
  - `RoboCrapScene` adds `Qt6::Qml`.
  - `RoboCrapDevices` adds `Qt6::Network` and `Qt6::Qml`.
  - The executable links Core, Gui, Quick, Qml, Network, Charts, Xml and Widgets. Felgo is linked when `USE_HOT_RELOAD=ON`. Charts is used only by the never-instantiated `QmlChartBridge`; Xml is unused (`QXmlStream*` is part of QtCore).
- **Physical vs. logical location.** Directory layout does not match target ownership. `src/3d/robot/kinematics/kr10kinematics.*`, `src/3d/robot/model/kr10kinematicmodel.h` and all of `src/pathgeneration/**` compile into `RoboCrapGeometry`, not `RoboCrap3D` (`src/CMakeLists.txt:4-49`). Scene and pathgeneration code that includes `3d/robot/...` headers therefore does **not** depend on `RoboCrap3D`. There is no target cycle. Geometry, Scene, Devices and 3D nevertheless export the whole `src` include directory (`src/CMakeLists.txt:50,70,101`; `src/3d/CMakeLists.txt:76`), so header access is broader than the target graph suggests.
- **Header include direction.** Scene includes geometry and pathgeneration. Devices include geometry and pathgeneration (`rsidevice.h` includes `chamferjob.h`). Neither scene nor devices include each other; the only scene-to-device coupling is one `connect` in `main.cpp:107`. `RoboCrap3D` includes scene headers in its public headers but links `RoboCrapScene` PRIVATE. In particular, `occcontroller.h:3,71,96` uses `SceneEndEffectors::Tool`, which needs the `SceneEndEffectors` class definition; a forward declaration alone does not declare its nested `Tool`. The executable's direct Scene link, the shared PUBLIC `src` include paths and 3D's own PUBLIC QtCore/Gui/Qml requirements mask the missing public Scene compile requirement (`src/3d/CMakeLists.txt:76-79`).
- **QML module imports.** With `USE_HOT_RELOAD=OFF` the app module declares `IMPORTS Styles Components RoboCrap.Viewport3D` (`CMakeLists.txt:113-123`). `RoboCrap.Backend` is not in that list; it loads only because `RoboCrapBackend` and `RoboCrapBackendplugin` are linked.
- **Foreign registration.** `qt_generate_foreign_qml_types(RoboCrapDevices robocrap)` keeps `RDTResponse` in the application QML module (`CMakeLists.txt:127`).

## 3. Subsystems

### 3.1 Geometry and kinematics (`RoboCrapGeometry`, non-path part)

Plain value types with `std::optional` factories. There are no QObjects, signals or mutable statics.

- **`geometry/mathtypes.h`**:
  - Eigen aliases: `V3d`, `V6d`, `M3d`, `M4d`, `VXd`.
  - `GeomConst::Eps = 1e-9`.
- **`geometry/utils.*`**:
  - `euler2rot(A,B,C)` = Rz(A)·Ry(B)·Rx(C), angles in degrees.
  - `rot2euler` returns two solutions.
  - `makeTransform`, `makeRotation` (principal-axis and arbitrary-axis overloads), `inverseRigidTransform`.
  - `decodeProbeSamples` decodes and validates probe JSON. Object form requires `points`, a finite nonnegative `radius` and numeric `dir` equal to ±1; every point has three finite coordinates. Legacy point arrays use radius 0 and dir 1 (`utils.cpp:67-98`, `utils.h:17-24`). It does not apply compensation.
- **`Pose`**: a validated frame with X/Y/Z/A/B/C order.
- **Fitting factories**:
  - `Plane` and `BoundedPlane`: SVD plane fit.
  - `BoundedCylinder`: Levenberg–Marquardt (LM) fit from 6 seed axes; RMS acceptance threshold max(0.1 mm, 2 % R).
  - `Circle`: circumcircle for 3 points, otherwise an algebraic start refined by LM.
  - `Cylinder::fromAxis`: analytic, not a fit.
- **Probe compensation**: `BoundedPlane::fromSamples` and `BoundedCylinder::fromSamples` fit the original coordinates, then call `applyProbe`. The plane moves by `dir·radius` along its fitted unit normal; the cylinder radius becomes `fittedRadius + dir·radius` and must remain greater than `GeomConst::Eps` (`boundedplane.cpp:72-101`, `boundedcylinder.cpp:246-264`). `Plane::fromJsonFile` applies its plane-offset correction directly (`plane.cpp:54-64`). Circle import reads raw coordinates without decoding or applying probe metadata.
- **`surfaceintersection`**: `intersectSurfaces(BoundedPlane, BoundedCylinder)` returns an analytic ellipse (`EdgeGeometry`), or the status `ParallelCylinderAxis` or `InvalidGeometry`.
- **`Kr10Kinematics`** (`src/3d/robot/kinematics/`) is a closed-form solver for forward kinematics (FK) and inverse kinematics (IK).
  - `solveFK(q)` returns 7 link transforms; the last is the flange.
  - `solveIK(T06, seed)` copies its configuration branch (shoulder, elbow, wrist) and the ±360° winding from the seed. It does not enumerate solutions.
  - It rejects a wrist singularity (|sin q5| ≤ Eps), a target out of reach, and joints outside their limits.
  - It checks the result by FK against `Kr10PosEps = 1e-4` and `Kr10RotEps = 1e-6`.
  - It stores `const Kr10KinematicModel&`, so the model must outlive the solver. `RobotPreviewState` deletes copy and move for this reason.
- **`Kr10KinematicModel`** (`kr10kinematicmodel.h`) holds the joints (axis, limits, rated payload speed in °/s), the IK parameters, `qHome` and `pHome`.
  - `IgnorePreviewJointPositionLimits = false` is a compile-time diagnostic switch that turns off joint-limit checks. It is read in the kinematics, in `RobotPreviewState`, in `chamfermotion.cpp` and in `OccController`.

### 3.2 Scene (`RoboCrapScene`)

All classes live on the GUI thread. They reach QML through singletons in `src/backendqmltypes.h`.

| Class | Responsibility |
|---|---|
| `SceneModel` | Owns `SceneObject`s (QObject parent) and their geometry views. IDs come from `m_nextId`, which starts at 1 and is never reused; 0 means no object. `objects` is a read-only `QQmlListProperty`; the class is not an item model. Signals: `objectAdded`, `objectRemoved(id)`, `objectVisibilityChanged`, `objectDiagnosticOverlaysChanged`, `objectsChanged`, `intersectionFailed`. `intersect()` creates an Edge from one plane and one cylinder. `addMachiningPath()` stores a `SceneMachiningData`. |
| `SceneObject` | Holds id, kind (Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath, Circle), classification, name, visibility, overlays and one geometry QObject. Setters are private; `SceneModel` is a friend and calls named setters. Geometry member assignment occurs inside `SceneObject`'s own setters (`sceneobject.cpp:11-24,67-70`), not through another object's direct field access. |
| `scenegeometry.h` | QObject views: `ScenePlaneGeometry`, `SceneCylinderGeometry`, `SceneCircleGeometry`, `SceneEdgeGeometry` and `SceneMachiningPath`. `SceneMachiningPath` wraps `SceneMachiningData{shared_ptr<const ChamferMotion>, sourceEdgeId, calibrationRevision}`. `SceneMachiningSettings` is a Q_GADGET value type. The views are read-only except `SceneMachiningPath`, whose incompatibility text `SceneMachining::updateCompatibility` sets through `SceneObject::geometry()` (`scenemachining.cpp:272-278`). |
| `SceneSurfaceImporter` | Synchronous, GUI-thread JSON point import. Plane and cylinder files are compensated for the probe; circle files are not. It sets `busy` true and emits `busyChanged` before reading/fitting, then clears it before returning. Synchronous signal handlers can observe true; normal event-loop painting waits for this read/fit call to return (`scenesurfaceimporter.cpp:96-125`). |
| `SceneEndEffectors` | Stores the CAD sources (`MEE.stp`, `SEE.stp`), the measuring stylus data, the collet frame `FLANGE_ER` (initially a hard-coded provisional constant) and the spindle TCP. `applySpindlePoses` composes `FLANGE_ER * ER_TCP`. An ER change commits the coupled calibration before emitting `spindleColletChanged` and `spindleTcpChanged`; a TCP-only change emits `spindleTcpChanged`. Unchanged calibration returns without notifications (`sceneendeffectors.cpp:110-135`). It also converts between spindle (TCP) pose and flange pose. |
| `SceneMachining` | Coordinates Generate and Dry Run (§5.2). It holds the `Kr10KinematicModel` copy received from `OccController`, a calibration revision counter, and the playback state. |
| `SceneMachiningPreparation` | Builds the RSI job ("RSI Sampling"; outside the current animation scope). An `std::async` worker produces a `PreparedChamferJob`. On the GUI thread it then writes `C:/RoboCrapJobs/shared_data.txt` with `QSaveFile` and emits `preparedJobReady`. It watches `<appDir>/rsi-correction-limits.json`. |
| `chamfershareddata` | Free functions. `prepareChamferSharedData` (worker side) and `writeChamferSharedData` (GUI side). |

QML singletons (`backendqmltypes.h`):

- `Backend.Scene`, `Backend.Hub`, `Backend.Machining`, `Backend.MachiningPreparation` and `Backend.EndEffectors` each return a stack object from `main()` through `s_inst`, with `CppOwnership`.
- The Machining, Preparation and EndEffectors wrappers call `qFatal` on a wrong thread or a second engine. Scene and Hub use only `Q_ASSERT`.
- Creatable types: `PlaneImporter` (`SceneSurfaceImporter`), `DeviceProfileModel`, and `SceneModel` (named element without `QML_UNCREATABLE`).

### 3.3 Path generation (`src/pathgeneration`, compiled in `RoboCrapGeometry`)

Numerical code on copied inputs. It calls no scene or device API and has no mutable statics.
IK and FK call counters live in a per-worker `ChamferProfiledKinematics`. The `RoboCrapGeometry`
target still does file I/O in `rsicorrectionlimits.cpp:141`, `offsetjsonwriter.cpp:55`,
`utils.cpp:46-66`, `plane.cpp:54`, `boundedplane.cpp:66-69` and `boundedcylinder.cpp:240-243`.
The last two file factories have live consumers in the profile CLI (`chamferprofile.cpp:73-75`).
TCP means tool center point; LIN denotes a prescribed straight Cartesian transfer, and PTP
denotes a synchronized joint movement.

| Stage | File | Product |
|---|---|---|
| Analytical curve | `chamferpath` | `ChamferPath::evaluate(angleRad)` evaluates the machining curve at an ellipse parameter in radians. `evaluatePhase(phase, progress)` evaluates LeadIn, Machining or LeadOut at dimensionless phase-local progress in [0,1]. Neither parameter is traveled distance or physical seconds. The path also supplies staging point P_s; `create(parameters)` validates the input (`chamferpath.h:83-93`). |
| Arc length | `chamferarclength` | Per-phase map from progress to distance in mm. Uses adaptive Gauss–Kronrod (G7/K15) integration and a quintic inverse per knot interval (the "inverse-map knots"). |
| Central timing | `chamfercentralmotion` | For elapsed seconds, calculate traveled distance in mm, recover phase-local progress through the inverse arc-length map, then evaluate the analytical TCP pose. All three phases use `machiningFeed`; there is an acceleration ramp over `accelerationDistanceFraction` of each lead. `leadInFeed`/`leadOutFeed` are carried through the settings but do not define central timing. |
| Point schedule + IK | `chamfercentralvalidation` | Animation points per span are fixed **before** IK from a curvature/acceleration chord bound (0.01 mm). Each point is solved with seeded IK and checked for joint limits, speed and Cartesian XYZ acceleration. Joint-acceleration magnitude rejection is commented out (temporary exception, see `AGENTS.md`). |
| Whole motion | `chamfermotion` | `ChamferMotion::create(path, robot)` prepares HOME to P_s (PTP), P_s to lead-in start (LIN), the central three phases, lead-out end to P_s (LIN), and P_s to HOME (PTP). It prepares three clamped cubic joint splines: one per LIN and one shared across LeadIn/Machining/LeadOut, each with zero endpoint joint velocities. `evaluate(seconds)` uses quintic smoothstep for PTP and the prepared splines for LIN/central, then FK; it performs no IK. Times 0 and `duration()` return `qHome`. The outer motion stores `ChamferPathParameters`; the complete `ChamferPath` is inside `ChamferCentralMotion`, so parameters are repeated, not two full path objects (`chamfermotion.h:75-81`, `chamfercentralmotion.h:65`). |
| RSI preparation | `chamferjob`, `chamfercartesiansampler`, `rsiposeencoder`, `rsicorrectionlimits` | `PreparedChamferJob` with 4 ms samples of the central part. Each sample runs IK and checks limits, **including per-joint acceleration**. Correction is encoded as an absolute first sample plus per-cycle increments, then checked against the limits file. |
| Legacy RSI demo | `pathgeneration/rsi/*` | `RsiPath::fromSurfPose`, `RsiPath::lin` and `writeOffsetsToJson` are used by `RsiDevice::generateTrajectory` (`rsidevice.cpp:206-220`). The demo remains a callable invokable; its only QML caller is the uninstantiated `KukaPanel`. |

**Fixed animation points and spline checks.** Each straight LIN is split at its acceleration,
cruise and deceleration boundaries. Before IK, each nonzero constant-acceleration span gets
`N = max(1, ceil(T * sqrt(a / (8 * epsilon))))` equal-time intervals: `T` is that span's duration
in seconds, `a` its acceleration magnitude in mm/s², and `epsilon = 0.01 mm`. Cruise uses one
interval. For a hypothetical 0.2 s ramp at the default `a = 20 mm/s²`, this gives
`ceil(0.2 * sqrt(20 / 0.08)) = 4` intervals and five endpoint-inclusive points; the preceding
endpoint is shared, so four points are appended. Central spans use their curvature and timing
bounds to fix their point counts before IK (`chamfercentralvalidation.cpp:155-191`);
LIN uses `chamfermotion.cpp:440-478`. Neither stage inserts extra IK points or stretches time
after an IK/rate failure.

After IK, spline checks evaluate each joint polynomial at interval endpoints and interior
position/speed extrema, checking finite angles/derivatives, joint position and rated speed
(`chamfermotion.cpp:61-95,634-710`). Joint-acceleration magnitude rejection remains disabled
in central validation, LIN checks (including terminal stops) and spline checks; acceleration
finiteness and central XYZ acceleration checks remain. HOME timing still uses configured
joint acceleration. The 0.01 mm point criterion does not bound orientation or the actual TCP
error between saved points after joint interpolation. Dry Run only evaluates the prepared
motion; it constructs no inverse maps or splines and adds no points.

**Repeated preparation.** Each starting joint candidate that reaches central validation calls
`ChamferCentralMotion::create` again, rebuilding its three arc-length maps, timing and point
schedule (`chamfermotion.cpp:286,731-733`; `chamfercentralvalidation.cpp:452-455`;
`chamfercentralmotion.cpp:123-147`). For example, three candidates that each complete all three
maps construct nine maps. This is a source-level work count, not a measured timing bottleneck.
Inverse-map knots, saved animation points and the separately prepared RSI samples are distinct.
Diagnostics separate computation milliseconds from movement seconds and include failed
attempts; nested timings must not be added as if they were disjoint stages.

`src/3d/chamferprofile.*` is a CLI profiler: `robocrap --profile-chamfer in.json out.json`, checked at
`main.cpp:47-55`. It runs before any device or QML object, and before the application's scene objects, are
created. It constructs its own `SceneEndEffectors` (`chamferprofile.cpp:106`). With `previewOnly=true` it
skips RSI preparation; with `previewOnly=false` it runs RSI preparation with default limits.
`in.json` and `out.json` above are argument placeholders, not supplied files. The preview input
named in `AGENTS.md`, `codex_plans/chamfer_profile_spatial_leads_input.json`, is absent in this
snapshot; there is no verified executable baseline using that path. No profiling or RSI
preparation was run for this documentation update.

### 3.4 Devices and network (`RoboCrapDevices`)

| Class | Responsibility |
|---|---|
| `DeviceHub` | Owns the two `QThread`s `ControlIO` and `GeneralIO`, started in the constructor. `startAll`/`stopAll` are public slots, so QML can call them on the `Hub` singleton. `add()` asserts a parentless device, moves it to its thread, connects `QThread::finished` to `deleteLater`, and creates a `DeviceRunner` parented to the hub. `startAll()` asks every runner to start. `stopAll()` is shown in §5.4. |
| `DeviceRunner` | GUI-thread proxy for QML. Properties: `data` (`QQmlPropertyMap`, pre-filled with `connectionConfig`), `socketState`, `isConnected` (Connected or Bound), `isDisconnected`. Every link to the device uses `Qt::QueuedConnection` (`devicerunner.cpp:13-17`). |
| `AbstractDevice` | Public slots `start`, `stop`, `invoke` with an `m_started` guard. `buildApi()` indexes public `Q_INVOKABLE` methods that return `void` and take no arguments or one `QVariantMap`; slots are excluded. Signals: `stateReady`, `socketStateReady`. `attachSocket()` forwards socket state and logs socket errors directly through Qt. |
| `PlcDevice` (GeneralIO) | `QTcpSocket` bound to a local port, then `connectToHost`. Invokables: `connect`, `disconnect`, `setOutput`, `writeMessage`. `PlcMessageManager` builds frames: a 6-byte big-endian header (0xAA55, version, type, transaction id (tid), length) plus a command, and the whole frame is swapped in 16-bit words. Responses are fixed 32-byte frames. Sends SNAPSHOT on connect. State keys: `x1`, `y1`, `x2`, `y2` (8 bools each) and `cellState`. `writeMessage` accepts raw WRITE_RAW/WRITE_REG/SET_VAR frames and is reachable from QML through `invoke()`, although no QML calls it. Pending requests have no deadline and can exhaust all 256 IDs (§7). The framing is defined by the PLC program (not in the repository); the user confirmed on 2026-10-13 that the protocol works. That historical statement does not establish behavior under response loss. |
| `FtsDevice` (ControlIO) | `QUdpSocket`. Invokables: `connect`, `disconnect`, `startStreaming`, `stopStreaming`, `bias`, `startLogRecording`, `stopLogRecording`, `saveLogToDefaultFile`. RDT request: 0x1234, then command, then count = 0. Parses one 36-byte record per datagram; timestamps use the sequence difference divided by the assumed 7000 Hz, and force/torque scaling uses 1e6 counts per unit. Each accepted packet emits `dataSampleHFReady`. While processing packets, elapsed time ≥16 ms triggers `processBatch`, which uses the last sample for recording and GUI publication; this is not a fixed publication timer. First publication includes all six axes; later publications include changes of at least the 0.05 deadband (`FtsDevice::onReadyRead`, `publishState`, `processBatch`, and the local `acceptAxis` helper in `ftsdevice.cpp`). Stream/log control flags are published separately. Recording is saved by `std::async` + `QSaveFile` to `<appDir>/records/`. The 7000 Hz value is not a measured arrival rate or verified sensor setting. |
| `RsiDevice` (ControlIO) | `QUdpSocket`. Answers every received `<Rob>` datagram synchronously in `readyRead`, echoing IPOC and sending `<RKorr>` (`RsiProtocol`). Invokables: `connect`, `disconnect`, `generateTrajectory` (hard-coded demo line; if reached, it dereferences an empty optional from the absent `://files/blank-plane-bottom.json`, causing undefined behavior, `rsidevice.cpp:156`), `startStreaming`, `stopStreaming`. The demo is dormant in the current UI but callable through `DeviceRunner::invoke`. Slots outside the invoke API: `setForce` (stores Fz, never read) and `setPreparedJob` (stores and publishes the `preparedJob*` keys; never arms motion). |
| `RsiHoleProtocol` | Compiled but has no callers. `RSI_Hole_CONTRACT.md` describes its State schema as out of date. |
| `DeviceProfileModel` | Read-only table loaded from `:/json/db.json`. Provides the default endpoints shown in the Network view. |
| Console diagnostics | Device call sites use `qInfo().noquote()` on their owning threads; viewport messages are formatted by `logViewportMessage` in main. Output retains the used `[ERROR]`/`[OK]`/`[INFO]`/`[WRITE]` labels, source names and message text. Qt's message pattern supplies timestamps. Diagnostics have no QML context property, custom message type, or queued GUI logging relay. |
| `QmlChartBridge` | Compiled into `robocrap` but never instantiated (`main.cpp:89-91` is commented out). |

Cross-device wiring in `main.cpp`:

- `FtsDevice::dataSampleHFReady` → `RsiDevice::setForce` (`:85`). This uses AutoConnection, so it is a direct call because both devices are on ControlIO.
- `SceneMachiningPreparation::preparedJobReady` → `RsiDevice::setPreparedJob`, queued (`:107`).

### 3.5 3D viewport (`RoboCrap3D`, namespace `RoboCrap3D`)

| Class | Responsibility |
|---|---|
| `OccController` | QML singleton `Viewport3D.OccController` (`viewportqmltypes.h`). Owns the `OccViewWindow`, a `shared_ptr<RobotPreviewState>`, the robot and tool `CadLoadWorker`s and the tool CAD state. QML uses `loadRobot`, `setSelectedObjects`, `reloadEndEffector`, `detachViewWindow` and its readiness/CAD properties. `solveFK`, `solveIK`, `solveTcpIK`, `jointAngles`, `flangePose`, `tcpPose`, `errorString`, `warningString` and `setDiagnosticOverlays` have no QML or C++ caller. Bridges scene, end effectors and machining to the window (signal table below). An OCCT init error emitted in its constructor is lost, because `main.cpp:118` connects `message` only afterwards. |
| `OccViewWindow` | `QWindow` (OpenGL surface) hosted in QML by `WindowContainer` (`RobotViewport.qml`). On first expose it builds an `OccViewport` from `winId()`. It releases and rebuilds that viewport when the surface is destroyed and re-applies cached state. |
| `OccViewport`, `OccViewer`, `OccScene`, `OccPart`, `OccInputController`, `OccViewCube`, `OccWorldAxes` | OCCT stack: `OpenGl_GraphicDriver` → `V3d_Viewer`/`V3d_View` on a `WNT_Window`, plus `AIS_InteractiveContext`. Rendering is coalesced with a 0 ms single-shot `QTimer` on the GUI thread. There is no render thread and no `QOpenGLContext`. |
| `OccRobotAdapter` | Converts link transforms from Eigen `M4d` to `gp_Trsf`. Places the tool shapes on the flange and draws the spindle TCP and collet axes. |
| `OccSceneAdapter` | Turns `SceneObject`s into OCCT parts: plane and cylinder faces, circle and ellipse edges, a per-phase polyline for each machining path. Draws diagnostic overlays and the machining axis preview. Maps a picked part back to an object id. |
| `CadLoadWorker` / `CachedShapeLoader` / `StepImporter` | `CadLoadWorker` is the `QThread` subclass; a function-local static mutex serializes parsing/cache access. The cache is `<appDir>/resources/cad/kr10/<completeBaseName>.brep`; a readable, valid BREP takes priority without opening or hashing the STEP. Missing, unreadable or invalid BREPs fall back to STEP import and atomic `QSaveFile` storage (`cachedshapeloader.cpp:24-82`). |
| `Kr10Model` / `RobotPreviewState` / `RobotPose` | `:/json/kr10.json` is parsed into `Kr10KinematicModel` and `RobotVisualModel`. The numerical model is OCCT-independent; the complete `Kr10Model` includes `OccPartProps`, whose fields use `gp_Trsf` and `Quantity_Color` (`kr10model.h:8-15`, `occpartprops.h:3-4,29-30`). The state holds the committed pose and uses it as the IK seed. `RobotPose` candidates are read-only; only `OccController` commits them, after synchronous AIS-transform acceptance, before deferred drawing. |
| `ViewportAssets` | Asset paths. `initializeOcct` sets the `CSF_*` environment variables from `<appDir>/resources/occt`. |

`OccController` connections (`occcontroller.cpp`):

| Source signal | Slot | Effect |
|---|---|---|
| `SceneModel::objectAdded`, `objectVisibilityChanged`, `objectDiagnosticOverlaysChanged` (`:134-140`) | `synchronizeSceneObject` | Window/adapter sync, plus preview refresh |
| `SceneModel::objectRemoved` (`:136`) | `removeSceneObject` | Removes the presentation |
| `SceneEndEffectors::activeToolChanged`, `spindleTcpChanged`, `spindleColletChanged`, `*CadSourceChanged` (`:211-224`) | tool sync, frame sync, `reloadEndEffector` | Tool presentation updates or reloads |
| `SceneMachining::inputChanged`, `settingsChanged`, `availabilityChanged` (`:156-160`) | `synchronizeMachiningPreview` | Machining axis preview |
| `SceneMachining::playbackPoseRequested` (`:157-158`, **DirectConnection**) | `applyPlaybackPose` | Dry Run pose |
| `OccController::readyChanged` (`:162`) | `synchronizeMachiningReadiness` | `SceneMachining::setRobotAvailable` |

`finishRobotLoad` also calls `SceneMachining::setRobotModel(kinematics)` (`:493`). The same call
is made in `setMachining` (`:164`).

### 3.6 QML UI (`qml/`)

- **Main window.** `Main.qml` is a single `ApplicationWindow` with no page navigation (no Loader, StackLayout or StackView).
  - It contains a fixed 3-column `SplitView`:
    - Left: `WorkflowPanel`, `ScenePanel`.
    - Centre: `ModeToolBar`, an RSI status label, `RobotViewport`.
    - Right: `PropertiesPanel`, `EndEffectorPanel` ×2 and `MachiningPanel`, switched by `visible`.
  - Tool windows hold the device panels: PLC `DeltaPanel`, FTS `FtsPanel`, Settings/`Network`.
  - A modal calculation window is bound to `Backend.Machining.calculating`.
- **Application decisions in QML.** `Main.qml` holds `workflowMode` (Measuring/Machining), `measurementSubmode` (Rough/Precise), `selectedObjectIds` and the UI-only `dryScanActive`/`dryScanRunning` flags. Its functions select/toggle objects, delete selected objects, intersect them and select the result. Mode changes stop machining playback when leaving Machining, choose the active end effector and synchronize machining input (`Main.qml:22-41,49-103`). These values survive panel switching, but are recreated with `Main.qml`; they are not saved document state.
- **Local UI state.** `chamferEditorOpen` and `toolSetupOpen` choose the right panel. `MachiningPanel` owns draft text, `draftFlipAxis` and captured original settings; `EndEffectorPanel` owns pose draft strings. Focus and `ScenePanel` group expansion are UI state, separate from document/robot state (`MachiningPanel.qml:17-19,42-84`, `EndEffectorPanel.qml:27-69`, `ScenePanel.qml:25-29,138`).
- **Unused files.** `NavigationPanel.qml` and `KukaPanel.qml` are listed in CMake but never instantiated.
- **Device panels.** They use `Backend.Hub.device(key)`, `isConnected`, `data.<key>` and `invoke(name, map)`. Every `invoke` name used in QML exists as a `Q_INVOKABLE` on the target device; `generateTrajectory`'s only caller, `KukaPanel.qml`, is never instantiated.
- **Selection and machining input.** One shared `Main.qml::syncMachiningInput()` chooses the sole selected Edge/MachiningPath in Machining mode, otherwise zero. Mode changes, selected-object changes and component completion call it. Generated objects and viewport picks update selection (`Main.qml:92-145`). `Machining.inputId` is also set by C++ after Generate and on input removal (`scenemachining.cpp:232,290`); active playback retains its own motion rather than following selection.
- **Tools and placeholders.** Dry Scan only toggles `Main.qml` flags; no scan motion is prepared or played. Scan and Start Machining use the toolbar action's default `available: false` and remain disabled (`Main.qml:450-459`, `ModeToolBar.qml:62,197-219,239-244`). Measuring name/ball diameter/stylus length are displayed read-only; spindle ER coordinates are disabled read-only fields, while TCP relative to ER has Apply. Apply passes the original ER coordinates, not their four-decimal display values (`EndEffectorPanel.qml:121-178,261-267`). This UI does not implement measuring calibration or scan execution.
- **Reusable modules.** `Components` contains the `Qx*` wrappers around Qt Quick Controls. `Styles` contains the singletons `Colors`, `Fonts` and `Metrics`. Both are NO_PLUGIN static modules with maintained `qmldir` files.
- **Resources.**
  - `pics.qrc`: SVG icons.
  - `fonts.qrc`: Roboto.
  - `json.qrc`: `kr10.json`, `db.json`.
  - `files.qrc` is empty, so the RSI files under `resources/files/rsi` are not embedded.
  - CAD files are deployed beside the executable by `cmake/DeployOcct.cmake`.

## 4. Threading and ownership model

| Thread | Objects / work | Created by |
|---|---|---|
| GUI (main) | `QApplication`, `DeviceHub`, all `DeviceRunner`s, `SceneModel`, `SceneEndEffectors`, `SceneMachining`, `SceneMachiningPreparation`, `OccController`, `OccViewWindow` and its OCCT viewer/rendering, QML engine; also synchronous JSON import, `ChamferPath::create`, `shared_data.txt` write, Dry Run timer (16 ms) | `main()` stack |
| `ControlIO` (`QThread`) | `FtsDevice`, `RsiDevice` and their sockets/timers | `DeviceHub` |
| `GeneralIO` (`QThread`) | `PlcDevice` and its socket | `DeviceHub` |
| Generate worker (`std::async`) | `ChamferMotion::create` on a copied `ChamferPath` + `ChamferRobotSetup`; polled by a 30 ms GUI timer | `SceneMachining::generate` |
| RSI preparation worker (`std::async`) | `prepareChamferSharedData` on a copied source + limit configuration; polled by a 30 ms timer | `SceneMachiningPreparation::prepare` |
| FTS save worker (`std::async`) | JSON write of a copied sample buffer | `FtsDevice` |
| CAD workers (`QThread` subclass) | STEP/BREP loading, serialized by a static mutex | `OccController` |

Ownership and lifetime rules:

- **Stack objects in `main()`.** The application singletons are stack objects without a QObject parent. The QML engine is declared last, so it is destroyed first. QML gets these objects through `s_inst` with `CppOwnership`.
- **Devices.** Each device is created parentless, then moved to its I/O thread. It is deleted on that thread by `deleteLater` when the thread finishes. Runners are children of the hub.
- **Scene objects.** Each `SceneObject` is a child of `SceneModel`, and each geometry view is a child of its object. Removal emits `objectRemoved`, then calls `deleteLater`.
- **Immutable shared data.** These are shared across threads as `shared_ptr<const ...>`:
  - `ChamferMotion` (scene path, playback, RSI job);
  - `PreparedChamferJob` (GUI thread to ControlIO);
  - `CadLoadResult` (controller, window and adapter).
- **Worker result acceptance.** Results are accepted only on the GUI thread, and stale ones are rejected:
  - Generate compares the input id, the calibration revision, robot availability and the source;
  - preparation compares a generation counter, the source, the motion pointer and the revision;
  - CAD loading compares per-slot generation counters.
- **Thread affinity checks.** `Q_ASSERT` checks thread affinity in `AbstractDevice::start/stop/invoke`, `DeviceRunner::invoke`, `RsiDevice::setPreparedJob`, `OccController::loadRobot` and `SceneMachiningPreparation`.

## 5. Traced workflows

### 5.1 Geometry and CAD import

**Probe point JSON into `SceneModel`.** This is the only path that adds imported geometry to the scene. There is no STEP import into `SceneModel`.

1. `ModeToolBar` emits a plane, circle or cylinder import request. `Main.qml:475-477` opens a `FileDialog` filtered to `*.json` (`:158-180`).
2. `onAccepted` calls `surfaceImporter.load`, `loadCircle` or `loadCylinder(url, Backend.Scene)` (`Main.qml:163,171,179`).
3. `SceneSurfaceImporter::importSurface` (`scenesurfaceimporter.cpp:89`) runs synchronously:
   - `readSamples` reads/parses JSON; for plane/cylinder it calls `decodeProbeSamples` to validate coordinates and probe metadata, without compensating them;
   - `BoundedPlane::fromSamples` or `BoundedCylinder::fromSamples` fits the coordinates and applies probe compensation once; circle import instead reads raw coordinates and calls `Circle::fromPoints`;
   - then `SceneModel::addBoundedPlane`, `addBoundedCylinder` or `addCircle` (`:105-114`).
4. `SceneModel::appendObject` emits `objectAdded`. This triggers `OccController::synchronizeSceneObject` → `OccViewWindow` → `OccSceneAdapter`, which builds a trimmed OCCT face or edge.
5. The importer emits `loaded(object)`, and `Main.qml` selects the new object.
6. Optional: "Intersect Selected" calls `SceneModel::intersect`, which uses `intersectSurfaces` to add an Edge object. That Edge is the Generate input.

**STEP CAD for robot and end effectors.** This goes to the viewport only.

1. `RobotViewport.qml` `Component.onCompleted` calls `OccController.loadRobot()`.
2. `loadRobot` parses `kr10.json`, initializes `RobotPreviewState` (FK at `qHome` must match `pHome`), and starts a `CadLoadWorker` with the 7 link basenames (`occcontroller.cpp:418-465`).
3. `QThread::finished` triggers `finishRobotLoad` (queued). It checks the generation, commits the state, calls `SceneMachining::setRobotModel`, calls `OccViewWindow::setScene`, then emits `readyChanged`.
4. End effectors:
   - `main.cpp:98-99` sets `MEE.stp` and `SEE.stp`; both panels offer Browse and Retry (`Main.qml:212-228,551-555,578-582`). Choosing the same source requests a reload; another source updates its setting. A valid basename-matching BREP still takes priority (§7).
   - That calls `SceneEndEffectors::set*CadSource`, which emits `*CadSourceChanged`.
   - `OccController::reloadEndEffector` runs a `CadLoadWorker` for one file; `finishEndEffectorLoad` then calls `synchronizeEndEffectors` → `OccRobotAdapter`.

### 5.2 Generate → Dry Run

**Generate.**

1. Toolbar "Generate Path" opens/focuses `MachiningPanel` and captures the current settings; it does not generate (`Main.qml:466-472`). The panel's "Generate" commits draft settings through `Backend.Machining.settings` / `SceneMachining::setSettings`, then emits `generateRequested`; `Main.qml:592` calls `Backend.Machining.generate()`. Cancel restores the captured settings and closes the editor (`MachiningPanel.qml:42-79,183-187`).
2. `SceneMachining::generate` (`scenemachining.cpp:159-184`):
   - checks `unavailableReason()`: robot ready, 6-value spindle TCP, an Edge with both source objects still present (one plane and one cylinder), or a saved path. The raw-edge numerical parameters use the edge ellipse plus cylinder origin/axis, but the plane-presence check still gates acceptance (`scenemachining.cpp:116-138`);
   - builds `ChamferPath::create(settings.applyTo(source))` **on the GUI thread**;
   - copies `Kr10KinematicModel`, `flangeToTcp` and the timing into `ChamferRobotSetup`;
   - records the input id and revision;
   - starts `std::async(ChamferMotion::create, path, robot)` and a 30 ms poll timer.
3. `finishCalculation` (`:194-236`), once the future is ready:
   - logs the diagnostics;
   - rejects the result if the input, revision, robot availability or source changed;
   - otherwise calls `SceneModel::addMachiningPath` with `make_shared<const ChamferMotion>`;
   - when regenerating a saved path, copies its name and visibility, then removes the old object;
   - calls `setInputId(newId)` and emits `generated`.
4. The new object reaches the viewport through `objectAdded` (§5.1 step 4).
   Saved motions own geometry/model/TCP/settings snapshots. Their sources may subsequently be
   removed without destroying the motion; regeneration from a saved path uses its parameters.
   Pending Generate from a raw edge is rejected if its required sources disappear before
   acceptance (`scenemachining.cpp:203-220`). Model/TCP changes invalidate playback until regeneration.

**Dry Run.**

1. `ModeToolBar` "Dry Run" calls `Backend.Machining.dryRun()` (`Main.qml:461`).
2. `dryRun` (`:310-338`):
   - requires `canPlay()` (compatible path, robot available) and a connected `playbackPoseRequested`;
   - shares the motion pointer;
   - presents t = 0;
   - starts a `QElapsedTimer` and a 16 ms `PreciseTimer`.
3. Each tick runs `advancePlayback` → `presentPlayback(t)` → `ChamferMotion::evaluate(t)` (no IK, no integration). It then emits `playbackPoseRequested(joints)`, which reaches `OccController::applyPlaybackPose` synchronously (DirectConnection):
   - `RobotPreviewState::forward` builds the pose;
   - `applyPose` applies AIS transformations through the window and then commits the pose. Redraw is scheduled separately; this accepts transforms synchronously, not completed drawing (`occcontroller.cpp:503-517`, `occviewport.cpp:110-114,187-200`);
   - on failure it calls `SceneMachining::failPlayback`.
4. `pause()` stops the timer. Calling `dryRun()` again resumes from `m_elapsed`.
5. `stop()` presents time zero while the robot preview is available, then ends playback (`scenemachining.cpp:398-403`). Time zero returns the saved `qHome` (`chamfermotion.cpp:277,779`), so Stop returns immediately to HOME without an animated return.
6. Playback also ends when t reaches the duration (the last point is `qHome`), and on any of: `aboutToQuit`, robot unavailable, calibration change (`spindleTcpChanged` or `setRobotModel` increments the revision and invalidates saved paths), removal of the active path. Leaving Machining mode also calls `stop()` from QML (`Main.qml:31`), so the robot jumps to HOME.
7. While playback is active, `OccController::solve` refuses manual FK/IK, and `generate()` is unavailable.

### 5.3 Device request from QML and state back

Example: the PLC panel toggles an output.

1. `DeltaModuleAP.qml` calls `plc.invoke("setOutput", {module, index, enabled})`, where `plc` is `Backend.Hub.device("plc")`.
2. `DeviceRunner::invoke` (GUI thread) asserts the thread and emits `invokeReq` (`devicerunner.cpp:31-36`).
3. The queued connection (`:15`) posts the request to GeneralIO. There `AbstractDevice::invoke` checks `m_started`, looks up the name in `m_api`, and calls `QMetaMethod::invoke(..., DirectConnection)` (in `abstractdevice.cpp`).
4. `PlcDevice::setOutput` builds a WRITE_IO frame with `PlcMessageManager`, swaps 16-bit words, and writes it to the TCP socket.
5. `readyRead` collects 32-byte responses. They are matched by transaction id (tid) and command, then decoded into `x1/y1/x2/y2` and `cellState`. The device emits `stateReady(QVariantHash)`.
6. The queued connection (`devicerunner.cpp:16`) calls `DeviceRunner::onStateReady` on the GUI thread. `m_data->insert(vals)` updates the `QQmlPropertyMap`, and QML bindings on `data.x1` and the others update.
7. Socket state follows the same pattern: `QAbstractSocket::stateChanged` → `AbstractDevice::onSocketStateChanged` → `socketStateReady` (queued) → `onSockState` → `socketStateChanged` → `isConnected`/`isDisconnected`.
8. Diagnostics are independent of queued GUI state delivery. Devices write directly through `qInfo().noquote()` on their owning I/O threads, including command rejection and completed FTS save results. Main's viewport-message connection uses the application as its lifetime context and writes through the same Qt output. No device logging signal or custom logging object remains.

Startup:

- `hub.startAll()` (`main.cpp:87`) runs `DeviceRunner::start` → `startReq` (queued) → `AbstractDevice::start` on the I/O thread → `buildApi()` + `startDevice()`.
- `startDevice()` creates the socket. The network connection itself waits for an explicit `invoke("connect", config)` from `Network.qml`.

### 5.4 Application shutdown

1. **`aboutToQuit`.** Slots run in connection order:
   1. `SceneMachining::endPlayback` (connected in its constructor, `scenemachining.cpp:88`).
   2. `SceneMachiningPreparation::shutdown` (`main.cpp:109`). It invalidates its generation, clears any prepared job (emitting `preparedJobReady(nullptr)` to ControlIO if a job existed), and **blocks** on an in-flight preparation future.
   3. `DeviceHub::stopAll` (`main.cpp:110`).
2. **`stopAll`** (`devicehub.cpp:60-79`). It returns at once if both threads are already stopped. Otherwise:
   - for each device: `invokeMethod(dev, &AbstractDevice::stop, BlockingQueuedConnection)`. When a prepared job exists, shutdown posts its null-job invalidation from the GUI thread to `rsiDev` before this same GUI thread posts that device's blocking stop (`main.cpp:107-110`, `scenemachiningpreparation.cpp:173-201`). This orders those queued calls to the same receiver; it is not a universal FIFO claim across unrelated senders, receivers or Qt event kinds. If no job exists, `setPreparedJob({})` emits no redundant invalidation. `FtsDevice::stopDevice` waits for an outstanding save.
   - `quit()` and `wait()` both I/O threads. `QThread::finished` triggers `deleteLater`, which deletes each device on its own thread.
   - `stopAll()` asserts that the caller is outside both I/O threads (`devicehub.cpp:65-66`): a blocking queued stop from its receiving thread would deadlock. The current shutdown caller is the GUI thread; sharing ControlIO does not by itself establish a race or deadlock.
3. **`app.exec()` returns.** Stack objects are destroyed in reverse declaration order:
   1. `QQmlApplicationEngine`. QML is destroyed; `RobotViewport` `onDestruction` calls `detachViewWindow`.
   2. `OccController`. It marks machining unavailable, interrupts and **waits** for the CAD workers (a STEP parse cannot be interrupted mid-file), then deletes the window.
   3. `SceneMachiningPreparation`. Its destructor calls `shutdown()` again, which is a no-op.
   4. `SceneMachining`. There is no explicit wait, but destroying a `std::future` returned by `std::async` blocks until an in-flight `ChamferMotion::create` finishes.
   5. `SceneEndEffectors`, then `SceneModel`.
   6. `DeviceHub`. Its destructor calls `stopAll()`, a no-op because the threads are stopped.
   7. `QApplication`.

## 6. External-library boundaries

- **Qt 6.8 / QML.**
  - Backend and viewport registration uses `QML_FOREIGN` wrappers: `backendqmltypes.h` for `RoboCrap.Backend` and `viewportqmltypes.h` for `RoboCrap.Viewport3D`. `RDTResponse` instead directly declares `QML_VALUE_TYPE(rdtResponse)` (`network/fts/rdtmessage.h:11`); `qt_generate_foreign_qml_types(RoboCrapDevices robocrap)` forwards its metadata into the application module (`CMakeLists.txt:127`). Diagnostics use Qt console output without a QML context property. Registration changes must preserve the existing `rdtResponse` API.
  - Hot reload:
    - `USE_HOT_RELOAD=ON`: app QML is not compiled in; `FelgoHotReload` loads it using `hot_reload_config.json`. CMake requires Felgo/FelgoHotReload and configures the executable with their helpers; the C++ branch uses `USE_FELGO_HOT_RELOAD`.
    - `OFF` (current source default, `CMakeLists.txt:29-33`): QML is embedded and loaded with `engine.loadFromModule("robocrap_qml_module", "Main")`. An existing build cache can choose a different option value.
  - `FAST_QML_BUILD=ON` (default) adds `NO_CACHEGEN`.
  - Devices publicly link QtCore through Geometry, plus QtNetwork and QtQml (`src/CMakeLists.txt:102`). `DeviceRunner` exposes `QQmlPropertyMap`, `RDTResponse` declares its QML value type, and `PlcMessageManager` includes `QQmlEngine`; QML dependencies are present in the current Devices target, not confined to the registration target.
  - Numerical code uses only QtCore: containers, JSON, `QElapsedTimer`.
- **OCCT 8.0.0.**
  - Used by `src/3d/occt`, `src/3d/adapters`, `viewportassets` and `occcontroller.cpp`, and through `OccPartProps` by the complete robot model/loader (`src/3d/robot/model/kr10model.h:8-15`). `Kr10KinematicModel` and `Kr10Kinematics` remain independent of OCCT.
  - The public `OccController` header exposes no OCCT types.
  - Eigen → OCCT conversion is only in `OccRobotAdapter::toOccTransform` and the scene adapter.
  - CMake enforces OCCT 8.0.0 + Windows 64-bit MinGW/GCC; only Release OCCT libraries are mapped.
  - `cmake/DeployOcct.cmake` stages the TK DLLs and their dependencies, the OCCT resources and `resources/cad/kr10` at POST_BUILD and install time.
  - Existing project exception handling is at this boundary: 13 try blocks in eight files under `src/3d`, mostly catching `Standard_Failure`, plus a `std::exception` catch in `cadloadworker.cpp:51` and a catch-all/cleanup/rethrow in `occscene.cpp:247-254`. The current `AGENTS.md` C++ rule prohibits `try`, `catch` and `throw`; an older recorded permission does not authorize new exception syntax. Existing fallback/cleanup is a source fact, not a resolved policy for future edits; returning an optional/status alone would not handle a thrown OCCT exception.
  - Scene-object synchronization started by `SceneModel` signals (`occcontroller.cpp:361` → `OccSceneAdapter::synchronizeObject`) has no catch.
  - `ready` requires the robot CAD to be presented, so Generate depends on OCCT initialization and on the STEP/BREP load. While the native surface is destroyed and not yet rebuilt (on exit or during Felgo hot reload), availability is false; a Generate result arriving then is rejected. The user confirmed this behaviour stays.
- **Eigen 5.0.0.** Header-only, vendored in `libs/`, exported PUBLIC by `RoboCrapGeometry` because `mathtypes.h`, `Pose`, the kinematics and the path types expose Eigen in their headers.
- **Felgo.** Only in hot-reload builds (`FelgoApplication`, `FelgoHotReload` in `main.cpp`).
- **Standard library concurrency.** `std::async` / `std::future` is used for the Generate, RSI preparation and FTS save workers. `QtConcurrent` is not used.

## 7. Open questions

Source-level facts, unresolved decisions and runtime/hardware checks not performed:

**Protocols and controller side**
- **Net F/T.** The ATI Net F/T manual confirms:
  - header 0x1234;
  - commands 0x0000 stop, 0x0002 real-time streaming, 0x0042 software bias;
  - UDP port 49152;
  - an internal rate of 7000 Hz with an RDT output rate configurable from 1 to 7000 Hz;
  - counts per force and counts per torque set by the calibration.

  Unverified in code: the configured RDT rate (the code assumes 7000 Hz for timestamps) and the counts per force/torque (hard-coded to 1e6 for both). With command 0x0002 each datagram carries one record, so parsing 36 bytes is consistent with the manual.
- **RSI.** The RSI 3.3 manual confirms IPO_FAST = 4 ms (12 ms in IPO mode), that a packet is valid only with the echoed IPOC, that late packets are counted and stop the robot, and that `HOLDON` controls output behaviour on late packets. `resources/files/rsi/RSI_Hole.src:77` uses `RSI_ON(#RELATIVE, #IPO_FAST)`.
  - Not available: the `RSI_Hole.rsi` context, which sets the POSCORR correction frame (BASE/TOOL) and limits.
  - Not verified on the controller: `RsiProtocol`'s omission of `<Stop>`, the late-packet count limit, and the stale-datagram behaviour (every queued datagram is answered).
  - `RsiHoleProtocol` is documented as out of date and is unused.
- **RSI execution.** Not connected. `RsiDevice` stores the prepared job but executes only the demo `m_offsets` path. `setForce` stores Fz but nothing reads it.
- **RSI preparation vs. Dry Run.** RSI preparation enforces per-joint acceleration (60 °/s² default), but Dry Run validation does not (temporary exception). A path accepted for Dry Run may therefore fail RSI Sampling. This is inferred from code, not observed.
- **PLC pending lifetime.** `availableTid()` scans all 256 8-bit IDs. If complete socket writes keep succeeding, matching replies never arrive and the connection stays open, pending entries accumulate until every ID is occupied; another write is rejected with "All PLC transaction ids are pending" (`plcdevice.cpp:153-179,206-247`). Matching responses, including matched error replies, remove their entry; a failed/partial write removes its entry, and partial writes also abort the socket. Explicit disconnect, socket transition to UnconnectedState and device stop clear pending requests (`plcdevice.cpp:33-45,113-116,311`). No timeout/retry policy exists. This response-loss condition is derived from code, not an observed hardware incident or a contradiction of the historical working-protocol report.

**Runtime behaviour not exercised**
- Whether `QQmlPropertyMap::insert(QVariantHash)` notifies bindings for every key in practice. Assumed from Qt documentation; not run.
- ControlIO latency. The 4 ms RSI reply loop shares an event loop with FTS datagram handling (timestamps assume 7000 Hz; actual arrival rate is unmeasured), the `setForce` direct calls, the dormant synchronous demo `generateTrajectory` and `FtsDevice::stopDevice` waiting for a save. Latency is unmeasured; the shared thread alone does not prove a race or deadlock.
- Shutdown can block on an in-flight Generate (future destructor), an RSI preparation or a STEP parse. Durations not measured.
- Felgo hot reload with the `qFatal` "one engine" singleton checks. Not run.
- `OccSceneAdapter` reconciles only visibility and overlays for an existing object; a geometry change to an existing object would not be redrawn. No such mutation path was found, but not exhaustively checked.

**Data and units**
- `resources/json/README.md:3-4` states millimeters for the supplied probe datasets. Arbitrary imported files contain no unit tag that the importer validates or converts; point coordinates and probe radius must already use scene units. The README's sample-radius claim also differs from the current `edge_cyl_plane/XP.json` (0) and `XC.json` (1.5); the actual JSON supplies the calculation inputs, not that sentence.
- `SceneEndEffectors` uses a hard-coded collet frame `FLANGE_ER` with a roll explicitly marked provisional (`sceneendeffectors.cpp:11-20`). Source inspection does not establish physical calibration.
- The BREP cache is keyed only by `completeBaseName`, not directory, content hash or OCCT version. A user-chosen `X/MEE.stp` reuses a valid `kr10/MEE.brep`; STEP/content/version changes require removing that BREP to regenerate it (`cachedshapeloader.cpp:36-62`). `applicationDirPath()` may be read-only when installed.
- Both end-effector panels currently allow Browse/Retry (§5.1). The review records a future fixed-configuration intent; whether to remove this existing user choice is unresolved, and that intent is not current behavior.
- `IkSolution::status/turn` resemble KUKA S/T but have no reader and were not checked against the KUKA definition.

**Code hygiene observed (not changed)**
- `SceneModel` is QML-creatable. The Scene and Hub singleton checks are `Q_ASSERT` only.
- Unused compiled/listed files: `QmlChartBridge` is compiled but never constructed; `NavigationPanel.qml` is listed but never instantiated. The listed `files.qrc` contains no resources. These findings do not authorize deletion of public APIs or assets.
- Dormant callable/separate-workflow code: `KukaPanel.qml` is listed but not instantiated, yet contains the `generateTrajectory` invoke. The demo remains callable; `RsiHoleProtocol` is compiled with no active caller, and its controller schema is separately unresolved. Neither is equivalent to an empty file.
- Unused individual APIs/members: no active caller was found for `ChamferPath::sample` or its `ChamferCurvePoint`/`ChamferSampledPath`/`ChamferSamplingResult` output types. `ChamferSamplingParameters` is still used by the RSI Cartesian sampler (`chamfercartesiansampler.cpp:396`); `ChamferPathSample`/`ChamferPathResult` remain live. `FtsDevice::dataBatchReady` and `streamReset` have no active connections; `OccController::warningString` is never set, and `ToolCadState::loadedSource` is assigned/cleared but never read. A repository search finding no consumer cannot rule out external API consumers.
- Device/viewport diagnostics use Qt console output. Additional operation-status/error properties and QML error displays are deferred at the user's request; existing socket/control state channels remain live.
- Retained assets: `.m3d` files are CAD authoring inputs, deployed by `cmake/DeployOcct.cmake:70` but not read by the STEP/BREP runtime. RSI contract/controller/example files are separate workflow assets, despite the empty `files.qrc`; lack of embedding does not make them disposable.
- Existing code that differs from the `AGENTS.md` style rules:
  - `QString& error` output parameters (`RobotPreviewState::forward/inverse`, `ViewportAssets::initializeOcct`);
  - the existing exception handlers described in §6.
- `std::bind` connections are not prohibited by the current mandatory C++ rules; those rules prohibit lambdas. Captured object lifetimes still depend on each connection's context and owner lifetime, not on this style distinction.
- The `devicehub.h:12-17` comment still mentions a single shared I/O thread.
