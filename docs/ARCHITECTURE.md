# RoboCrap architecture

A verified map of the current code for future sessions. File references are relative to the
repository root. "Verified" means the claim was checked against constructors, `connect` calls
or call sites; anything else is listed under [Open questions](#7-open-questions).
Snapshot date: 2026-10-13 (working tree on `master`, including uncommitted user edits).

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
  robocrap["robocrap (exe)<br/>main.cpp, Logger, QmlChartBridge, app QML<br/>URI robocrap_qml_module"]
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
- **Physical vs. logical location.** Directory layout does not match target ownership. `src/3d/robot/kinematics/kr10kinematics.*`, `src/3d/robot/model/kr10kinematicmodel.h` and all of `src/pathgeneration/**` compile into `RoboCrapGeometry`, not `RoboCrap3D` (`src/CMakeLists.txt:4-49`). Scene and pathgeneration code that includes `3d/robot/...` headers therefore does **not** depend on `RoboCrap3D`. There is no target cycle.
- **Header include direction.** Scene includes geometry and pathgeneration. Devices include geometry and pathgeneration (`rsidevice.h` includes `chamferjob.h`). Neither scene nor devices include each other; the only scene-to-device coupling is one `connect` in `main.cpp:113`. `RoboCrap3D` includes scene headers in its public headers but links `RoboCrapScene` PRIVATE. This works because `robocrap` links Scene directly.
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
  - `decodeProbeSamples` reads the probe JSON: `points`, optional `radius` and `dir` = ±1.
- **`Pose`**: a validated frame with X/Y/Z/A/B/C order.
- **Fitting factories**:
  - `Plane` and `BoundedPlane`: SVD plane fit.
  - `BoundedCylinder`: Levenberg–Marquardt (LM) fit from 6 seed axes; RMS acceptance threshold max(0.1 mm, 2 % R).
  - `Circle`: circumcircle for 3 points, otherwise an algebraic start refined by LM.
  - `Cylinder::fromAxis`: analytic, not a fit.
- **Probe compensation**: `applyProbe` shifts a plane by `dir·radius` along its normal and changes a cylinder radius by `dir·radius`. Circles are not compensated.
- **`surfaceintersection`**: `intersectSurfaces(BoundedPlane, BoundedCylinder)` returns an analytic ellipse (`EdgeGeometry`), or the status `ParallelCylinderAxis` or `InvalidGeometry`.
- **`Kr10Kinematics`** (`src/3d/robot/kinematics/`) is a closed-form solver.
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
| `SceneObject` | Holds id, kind (Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath, Circle), classification, name, visibility, overlays and one geometry QObject. Setters are private; `SceneModel` is a friend. |
| `scenegeometry.h` | QObject views: `ScenePlaneGeometry`, `SceneCylinderGeometry`, `SceneCircleGeometry`, `SceneEdgeGeometry` and `SceneMachiningPath`. `SceneMachiningPath` wraps `SceneMachiningData{shared_ptr<const ChamferMotion>, sourceEdgeId, calibrationRevision}`. `SceneMachiningSettings` is a Q_GADGET value type. The views are read-only except `SceneMachiningPath`, whose incompatibility text `SceneMachining::updateCompatibility` sets through `SceneObject::geometry()` (`scenemachining.cpp:272-278`). |
| `SceneSurfaceImporter` | Synchronous, GUI-thread JSON point import. Plane and cylinder files are compensated for the probe; circle files are not. Its `busy` property is set and cleared inside one call, so it is never observable as true (`scenesurfaceimporter.cpp:98-121`). |
| `SceneEndEffectors` | Stores the CAD sources (`MEE.stp`, `SEE.stp`), the measuring stylus data, the collet frame `FLANGE_ER` (hard-coded provisional constant) and the spindle TCP. `applySpindlePoses` composes `FLANGE_ER * ER_TCP`, commits both values, then emits `spindleColletChanged` and `spindleTcpChanged`. It also converts between spindle (TCP) pose and flange pose. |
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
target still does file I/O in a few places: `rsicorrectionlimits.cpp:141`, `offsetjsonwriter.cpp:55`,
`utils.cpp:46-66` and `plane.cpp:54`.

| Stage | File | Product |
|---|---|---|
| Analytical curve | `chamferpath` | `ChamferPath`: TCP pose as a function of the ellipse parameter (rad) for the LeadIn, Machining and LeadOut phases, plus the staging point P_s. `create()` validates the input. |
| Arc length | `chamferarclength` | Per-phase map from progress to distance in mm. Uses adaptive Gauss–Kronrod (G7/K15) integration and a quintic inverse per knot interval (the "inverse-map knots"). |
| Central timing | `chamfercentralmotion` | Time → distance → progress → pose for lead-in, machining and lead-out. All three phases use `machiningFeed`; there is an acceleration ramp over `accelerationDistanceFraction` of each lead. `leadInFeed`/`leadOutFeed` are carried through the settings but never read. |
| Point schedule + IK | `chamfercentralvalidation` | Animation points per span are fixed **before** IK from a curvature/acceleration chord bound (0.01 mm). Each point is solved with seeded IK and checked for joint limits, speed and Cartesian XYZ acceleration. Joint-acceleration magnitude rejection is commented out (temporary exception, see `AGENTS.md`). |
| Whole motion | `chamfermotion` | `ChamferMotion::create`: HOME→P_s (PTP) → P_s→lead-in start (LIN) → central → lead-out end→P_s (LIN) → P_s→HOME (PTP), then a cubic joint spline over the central knots. `ChamferMotion::evaluate(t)` does no IK: smoothstep for PTP, spline for central, linear for LIN, then FK. t = 0 and t = duration both give `qHome`. A saved motion also holds a second `ChamferPath` copy inside its `ChamferCentralMotion`. |
| RSI preparation | `chamferjob`, `chamfercartesiansampler`, `rsiposeencoder`, `rsicorrectionlimits` | `PreparedChamferJob` with 4 ms samples of the central part. Each sample runs IK and checks limits, **including per-joint acceleration**. Correction is encoded as an absolute first sample plus per-cycle increments, then checked against the limits file. |
| Legacy RSI demo | `pathgeneration/rsi/*` | `RsiPath::lin` and `writeOffsetsToJson`. Used only by `RsiDevice::generateTrajectory`. |

`src/3d/chamferprofile.*` is a CLI profiler: `robocrap --profile-chamfer in.json out.json`, checked at
`main.cpp:48-56`. It runs before any device or QML object, and before the application's scene objects, are
created. It constructs its own `SceneEndEffectors` (`chamferprofile.cpp:106`). With `previewOnly=true` it
skips RSI preparation; with `previewOnly=false` it runs RSI preparation with default limits.

### 3.4 Devices and network (`RoboCrapDevices`, plus `Logger` in the executable)

| Class | Responsibility |
|---|---|
| `DeviceHub` | Owns the two `QThread`s `ControlIO` and `GeneralIO`, started in the constructor. `startAll`/`stopAll` are public slots, so QML can call them on the `Hub` singleton. `add()` asserts a parentless device, moves it to its thread, connects `QThread::finished` to `deleteLater`, and creates a `DeviceRunner` parented to the hub. `startAll()` asks every runner to start. `stopAll()` is shown in §5.4. |
| `DeviceRunner` | GUI-thread proxy for QML. Properties: `data` (`QQmlPropertyMap`, pre-filled with `connectionConfig`), `socketState`, `isConnected` (Connected or Bound), `isDisconnected`. Every link to the device uses `Qt::QueuedConnection` (`devicerunner.cpp:13-17`). |
| `AbstractDevice` | Public slots `start`, `stop`, `invoke` with an `m_started` guard. `buildApi()` indexes public `Q_INVOKABLE` methods that return `void` and take no arguments or one `QVariantMap`; slots are excluded. Signals: `stateReady`, `socketStateReady`, `logMessage`. `attachSocket()` forwards socket state and error changes. |
| `PlcDevice` (GeneralIO) | `QTcpSocket` bound to a local port, then `connectToHost`. Invokables: `connect`, `disconnect`, `setOutput`, `writeMessage`. `PlcMessageManager` builds frames: a 6-byte big-endian header (0xAA55, version, type, transaction id (tid), length) plus a command, and the whole frame is swapped in 16-bit words. Responses are fixed 32-byte frames. Sends SNAPSHOT on connect. State keys: `x1`, `y1`, `x2`, `y2` (8 bools each) and `cellState`. `writeMessage` accepts raw WRITE_RAW/WRITE_REG/SET_VAR frames and is reachable from QML through `invoke()`, although no QML calls it. The framing is defined by the PLC program (not in the repository); the user confirmed on 2026-10-13 that the protocol works. |
| `FtsDevice` (ControlIO) | `QUdpSocket`. Invokables: `connect`, `disconnect`, `startStreaming`, `stopStreaming`, `bias`, `startLogRecording`, `stopLogRecording`, `saveLogToDefaultFile`. RDT request: 0x1234, then command, then count = 0. Parses one 36-byte record per datagram; assumes 7000 Hz and 1e6 counts per unit. Every 16 ms it publishes `fx`…`tz`, but only axes that changed by more than the 0.05 deadband; flags are published only on state change (`ftsdevice.cpp:30,358-396`). Recording is saved by `std::async` + `QSaveFile` to `<appDir>/records/`. |
| `RsiDevice` (ControlIO) | `QUdpSocket`. Answers every received `<Rob>` datagram synchronously in `readyRead`, echoing IPOC and sending `<RKorr>` (`RsiProtocol`). Invokables: `connect`, `disconnect`, `generateTrajectory` (hard-coded demo line; dereferences an empty optional from the non-existent `://files/blank-plane-bottom.json`, `rsidevice.cpp:182`), `startStreaming`, `stopStreaming`. Slots outside the invoke API: `setForce` (stores Fz, never read) and `setPreparedJob` (stores and publishes the `preparedJob*` keys; never arms motion). |
| `RsiHoleProtocol` | Compiled but has no callers. `RSI_Hole_CONTRACT.md` describes its State schema as out of date. |
| `DeviceProfileModel` | Read-only table loaded from `:/json/db.json`. Provides the default endpoints shown in the Network view. |
| `Logger` | Meyers singleton on the GUI thread. `push(LoggerMessage)` emits `logAdded`, which nothing consumes, so device and viewport messages are discarded. Exposed as the context property `logger`, which no QML file reads. |
| `QmlChartBridge` | Compiled into `robocrap` but never instantiated (`main.cpp:95-97` is commented out). |

Cross-device wiring in `main.cpp`:

- `FtsDevice::dataSampleHFReady` → `RsiDevice::setForce` (`:91`). This uses AutoConnection, so it is a direct call because both devices are on ControlIO.
- `SceneMachiningPreparation::preparedJobReady` → `RsiDevice::setPreparedJob`, queued (`:113`).

### 3.5 3D viewport (`RoboCrap3D`, namespace `RoboCrap3D`)

| Class | Responsibility |
|---|---|
| `OccController` | QML singleton `Viewport3D.OccController` (`viewportqmltypes.h`). Owns the `OccViewWindow`, a `shared_ptr<RobotPreviewState>`, the robot and tool `CadLoadWorker`s and the tool CAD state. QML uses `loadRobot`, `setSelectedObjects`, `reloadEndEffector`, `detachViewWindow` and its readiness/CAD properties. `solveFK`, `solveIK`, `solveTcpIK`, `jointAngles`, `flangePose`, `tcpPose`, `errorString`, `warningString` and `setDiagnosticOverlays` have no QML or C++ caller. Bridges scene, end effectors and machining to the window (signal table below). An OCCT init error emitted in its constructor is lost, because `main.cpp:124` connects `message` only afterwards. |
| `OccViewWindow` | `QWindow` (OpenGL surface) hosted in QML by `WindowContainer` (`RobotViewport.qml`). On first expose it builds an `OccViewport` from `winId()`. It releases and rebuilds that viewport when the surface is destroyed and re-applies cached state. |
| `OccViewport`, `OccViewer`, `OccScene`, `OccPart`, `OccInputController`, `OccViewCube`, `OccWorldAxes` | OCCT stack: `OpenGl_GraphicDriver` → `V3d_Viewer`/`V3d_View` on a `WNT_Window`, plus `AIS_InteractiveContext`. Rendering is coalesced with a 0 ms single-shot `QTimer` on the GUI thread. There is no render thread and no `QOpenGLContext`. |
| `OccRobotAdapter` | Converts link transforms from Eigen `M4d` to `gp_Trsf`. Places the tool shapes on the flange and draws the spindle TCP and collet axes. |
| `OccSceneAdapter` | Turns `SceneObject`s into OCCT parts: plane and cylinder faces, circle and ellipse edges, a per-phase polyline for each machining path. Draws diagnostic overlays and the machining axis preview. Maps a picked part back to an object id. |
| `CadLoadWorker` / `CachedShapeLoader` / `StepImporter` | `QThread` subclass. A function-local static mutex serializes all parsing. The cache is `<appDir>/resources/cad/kr10/<basename>.brep`; a BREP that exists is used without checking the STEP. Otherwise it runs a STEP import and saves the BREP atomically with `QSaveFile`. |
| `Kr10Model` / `RobotPreviewState` / `RobotPose` | `:/json/kr10.json` is parsed into `Kr10KinematicModel` and `RobotVisualModel`. The state holds the committed pose and uses it as the IK seed. `RobotPose` candidates are read-only; only `OccController` commits them, after presentation succeeds. |
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
- **Workflow state.** `workflowMode`, `selectedObjectIds` and similar state lives in `Main.qml`, not in a backend.
- **Unused files.** `NavigationPanel.qml` and `KukaPanel.qml` are listed in CMake but never instantiated.
- **Device panels.** They use `Backend.Hub.device(key)`, `isConnected`, `data.<key>` and `invoke(name, map)`. Every `invoke` name used in QML exists as a `Q_INVOKABLE` on the target device; `generateTrajectory`'s only caller, `KukaPanel.qml`, is never instantiated.
- **Application state in QML.** Workflow mode and selection live in `Main.qml`; `Machining.inputId` is written both by QML (`Main.qml:94`) and by C++ (`scenemachining.cpp:232,290`).
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
| GUI (main) | `QApplication`, `DeviceHub`, all `DeviceRunner`s, `Logger`, `SceneModel`, `SceneEndEffectors`, `SceneMachining`, `SceneMachiningPreparation`, `OccController`, `OccViewWindow` and its OCCT viewer/rendering, QML engine; also synchronous JSON import, `ChamferPath::create`, `shared_data.txt` write, Dry Run timer (16 ms) | `main()` stack |
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
   - `readSamples` → `decodeProbeSamples`, which applies probe compensation for plane and cylinder;
   - then `BoundedPlane::fromSamples`, `BoundedCylinder::fromSamples` or `Circle::fromPoints`;
   - then `SceneModel::addBoundedPlane`, `addBoundedCylinder` or `addCircle` (`:105-114`).
4. `SceneModel::appendObject` emits `objectAdded`. This triggers `OccController::synchronizeSceneObject` → `OccViewWindow` → `OccSceneAdapter`, which builds a trimmed OCCT face or edge.
5. The importer emits `loaded(object)`, and `Main.qml` selects the new object.
6. Optional: "Intersect Selected" calls `SceneModel::intersect`, which uses `intersectSurfaces` to add an Edge object. That Edge is the Generate input.

**STEP CAD for robot and end effectors.** This goes to the viewport only.

1. `RobotViewport.qml` `Component.onCompleted` calls `OccController.loadRobot()`.
2. `loadRobot` parses `kr10.json`, initializes `RobotPreviewState` (FK at `qHome` must match `pHome`), and starts a `CadLoadWorker` with the 7 link basenames (`occcontroller.cpp:418-465`).
3. `QThread::finished` triggers `finishRobotLoad` (queued). It checks the generation, commits the state, calls `SceneMachining::setRobotModel`, calls `OccViewWindow::setScene`, then emits `readyChanged`.
4. End effectors:
   - `main.cpp:104-105` sets `MEE.stp` and `SEE.stp`; the user can choose another STEP (`Main.qml:212-228`).
   - That calls `SceneEndEffectors::set*CadSource`, which emits `*CadSourceChanged`.
   - `OccController::reloadEndEffector` runs a `CadLoadWorker` for one file; `finishEndEffectorLoad` then calls `synchronizeEndEffectors` → `OccRobotAdapter`.

### 5.2 Generate → Dry Run

**Generate.**

1. In `MachiningPanel`, "Generate" writes the draft settings to `Backend.Machining.settings`, which is `SceneMachining::setSettings`. `Main.qml:592` then calls `Backend.Machining.generate()`.
2. `SceneMachining::generate` (`scenemachining.cpp:159-184`):
   - checks `unavailableReason()`: robot ready, 6-value spindle TCP, an Edge with a plane and a cylinder source, or a saved path;
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

**Dry Run.**

1. `ModeToolBar` "Dry Run" calls `Backend.Machining.dryRun()` (`Main.qml:461`).
2. `dryRun` (`:310-338`):
   - requires `canPlay()` (compatible path, robot available) and a connected `playbackPoseRequested`;
   - shares the motion pointer;
   - presents t = 0;
   - starts a `QElapsedTimer` and a 16 ms `PreciseTimer`.
3. Each tick runs `advancePlayback` → `presentPlayback(t)` → `ChamferMotion::evaluate(t)` (no IK, no integration). It then emits `playbackPoseRequested(joints)`, which reaches `OccController::applyPlaybackPose` synchronously (DirectConnection):
   - `RobotPreviewState::forward` builds the pose;
   - `applyPose` presents it on the window and then commits it;
   - on failure it calls `SceneMachining::failPlayback`.
4. `pause()` stops the timer. Calling `dryRun()` again resumes from `m_elapsed`.
5. `stop()` presents t = 0, which is `qHome` (`chamfermotion.cpp:277,816`), so the robot jumps to HOME without an animated return. Then playback ends.
6. Playback also ends when t reaches the duration (the last point is `qHome`), and on any of: `aboutToQuit`, robot unavailable, calibration change (`spindleTcpChanged` or `setRobotModel` increments the revision and invalidates saved paths), removal of the active path. Leaving Machining mode also calls `stop()` from QML (`Main.qml:31`), so the robot jumps to HOME.
7. While playback is active, `OccController::solve` refuses manual FK/IK, and `generate()` is unavailable.

### 5.3 Device request from QML and state back

Example: the PLC panel toggles an output.

1. `DeltaModuleAP.qml` calls `plc.invoke("setOutput", {module, index, enabled})`, where `plc` is `Backend.Hub.device("plc")`.
2. `DeviceRunner::invoke` (GUI thread) asserts the thread and emits `invokeReq` (`devicerunner.cpp:31-36`).
3. The queued connection (`:15`) posts the request to GeneralIO. There `AbstractDevice::invoke` checks `m_started`, looks up the name in `m_api`, and calls `QMetaMethod::invoke(..., DirectConnection)` (`abstractdevice.cpp:35-62`).
4. `PlcDevice::setOutput` builds a WRITE_IO frame with `PlcMessageManager`, swaps 16-bit words, and writes it to the TCP socket.
5. `readyRead` collects 32-byte responses. They are matched by transaction id (tid) and command, then decoded into `x1/y1/x2/y2` and `cellState`. The device emits `stateReady(QVariantHash)`.
6. The queued connection (`devicerunner.cpp:16`) calls `DeviceRunner::onStateReady` on the GUI thread. `m_data->insert(vals)` updates the `QQmlPropertyMap`, and QML bindings on `data.x1` and the others update.
7. Socket state follows the same pattern: `QAbstractSocket::stateChanged` → `AbstractDevice::onSocketStateChanged` → `socketStateReady` (queued) → `onSockState` → `socketStateChanged` → `isConnected`/`isDisconnected`.
8. Logging follows the same pattern: `logMessage` → `Logger::push`, queued (`main.cpp:79-81`).

Startup:

- `hub.startAll()` (`main.cpp:93`) runs `DeviceRunner::start` → `startReq` (queued) → `AbstractDevice::start` on the I/O thread → `buildApi()` + `startDevice()`.
- `startDevice()` creates the socket. The network connection itself waits for an explicit `invoke("connect", config)` from `Network.qml`.

### 5.4 Application shutdown

1. **`aboutToQuit`.** Slots run in connection order:
   1. `SceneMachining::endPlayback` (connected in its constructor, `scenemachining.cpp:88`).
   2. `SceneMachiningPreparation::shutdown` (`main.cpp:115`). It invalidates its generation, emits `preparedJobReady(nullptr)` (queued to ControlIO), and **blocks** on an in-flight preparation future.
   3. `DeviceHub::stopAll` (`main.cpp:116`).
2. **`stopAll`** (`devicehub.cpp:60-79`). It returns at once if both threads are already stopped. Otherwise:
   - for each device: `invokeMethod(dev, &AbstractDevice::stop, BlockingQueuedConnection)`. The null job queued earlier is processed first, because the thread's event queue is FIFO. `FtsDevice::stopDevice` waits for an outstanding save.
   - `quit()` and `wait()` both I/O threads. `QThread::finished` triggers `deleteLater`, which deletes each device on its own thread.
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
  - C++ types reach QML only through `QML_FOREIGN` wrappers: `backendqmltypes.h` for `RoboCrap.Backend` and `viewportqmltypes.h` for `RoboCrap.Viewport3D`. `logger` is the one context property.
  - Hot reload:
    - `USE_HOT_RELOAD=ON` (default): app QML is not compiled in; `FelgoHotReload` loads it using `hot_reload_config.json`. The C++ guard is the macro `USE_FELGO_HOT_RELOAD`, defined by the Felgo build (`USE_FELGO_HOT_RELOAD=1` in `build.ninja`).
    - `OFF`: QML is embedded and loaded with `engine.loadFromModule("robocrap_qml_module", "Main")`.
  - `FAST_QML_BUILD=ON` (default) adds `NO_CACHEGEN`.
  - Devices use only QtCore/QtNetwork (QML-facing state goes through `DeviceRunner`).
  - Numerical code uses only QtCore: containers, JSON, `QElapsedTimer`.
- **OCCT 8.0.0.**
  - Confined to `src/3d/occt`, `src/3d/adapters`, `viewportassets` and `occcontroller.cpp`.
  - The public `OccController` header exposes no OCCT types.
  - Eigen → OCCT conversion is only in `OccRobotAdapter::toOccTransform` and the scene adapter.
  - CMake enforces OCCT 8.0.0 + Windows 64-bit MinGW/GCC; only Release OCCT libraries are mapped.
  - `cmake/DeployOcct.cmake` stages the TK DLLs and their dependencies, the OCCT resources and `resources/cad/kr10` at POST_BUILD and install time.
  - All project `try`/`catch` is at this boundary (`Standard_Failure`): 13 try blocks in 8 files under `src/3d`, plus one rethrow (`occscene.cpp:254`). The user confirmed (2026-10-13) that this handling stays: OCCT reports failures by exceptions.
  - Scene-object synchronization started by `SceneModel` signals (`occcontroller.cpp:361` → `OccSceneAdapter::synchronizeObject`) has no catch.
  - `ready` requires the robot CAD to be presented, so Generate depends on OCCT initialization and on the STEP/BREP load. While the native surface is destroyed and not yet rebuilt (on exit or during Felgo hot reload), availability is false; a Generate result arriving then is rejected. The user confirmed this behaviour stays.
- **Eigen 5.0.0.** Header-only, vendored in `libs/`, exported PUBLIC by `RoboCrapGeometry` because `mathtypes.h`, `Pose`, the kinematics and the path types expose Eigen in their headers.
- **Felgo.** Only in hot-reload builds (`FelgoApplication`, `FelgoHotReload` in `main.cpp`).
- **Standard library concurrency.** `std::async` / `std::future` is used for the Generate, RSI preparation and FTS save workers. `QtConcurrent` is not used.

## 7. Open questions

Not verified, or verifiable only on hardware:

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

**Runtime behaviour not exercised**
- Whether `QQmlPropertyMap::insert(QVariantHash)` notifies bindings for every key in practice. Assumed from Qt documentation; not run.
- ControlIO latency. The 4 ms RSI reply loop shares an event loop with the 7 kHz FTS datagram handling, the `setForce` direct calls, the blocking demo `generateTrajectory` and `FtsDevice::stopDevice` waiting for a save. Not measured.
- Shutdown can block on an in-flight Generate (future destructor), an RSI preparation or a STEP parse. Durations not measured.
- Felgo hot reload with the `qFatal` "one engine" singleton checks. Not run.
- `OccSceneAdapter` reconciles only visibility and overlays for an existing object; a geometry change to an existing object would not be redrawn. No such mutation path was found, but not exhaustively checked.

**Data and units**
- The units of imported probe JSON are assumed to be mm; the importer does not state them.
- `SceneEndEffectors` uses a hard-coded collet frame `FLANGE_ER` with a roll marked provisional in the code.
- The BREP cache is keyed only by basename. A user-chosen `X/MEE.stp` reuses `kr10/MEE.brep`, and STEP edits are ignored until the BREP is deleted. `applicationDirPath()` may be read-only when installed.
- `IkSolution::status/turn` resemble KUKA S/T but have no reader and were not checked against the KUKA definition.

**Code hygiene observed (not changed)**
- `SceneModel` is QML-creatable. The Scene and Hub singleton checks are `Q_ASSERT` only.
- Dead or unused code:
  - `QmlChartBridge`;
  - `RsiHoleProtocol`;
  - `ChamferPath::sample()`;
  - `FtsDevice::dataBatchReady` and `streamReset` (no connections);
  - `NavigationPanel.qml` and `KukaPanel.qml` (not instantiated);
  - the `logger` context property (not read by QML);
  - `OccController::warningString` (never set);
  - `.m3d` files deployed but never read.
- Existing code that differs from the `AGENTS.md` style rules:
  - `QString& error` output parameters (`RobotPreviewState::forward/inverse`, `ViewportAssets::initializeOcct`);
  - `std::bind` connections.
- The `devicehub.h:12-17` comment still mentions a single shared I/O thread.
