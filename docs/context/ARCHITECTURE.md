# RoboCrap architecture

Map of the code and hardware for new sessions. Paths are repository-relative; named functions and
files are authoritative. Not repeated here: binding constraints ([AGENT_CONTRACTS.md](AGENT_CONTRACTS.md)),
controller/PC protocols ([RSI_Chamfer_Hole_CONTRACT.md](../../resources/files/rsi/Chamfer/RSI_Chamfer_Hole_CONTRACT.md),
[RSI_Probe_Check_CONTRACT.md](../../resources/files/rsi/ProbeCheck/RSI_Probe_Check_CONTRACT.md)) and operator
procedures ([HARDWARE_SETUP.md](HARDWARE_SETUP.md)). Unverified items are in §7; this document
verifies neither compilation nor runtime behavior.

## 1. Hardware

Supplied by the user unless a source is named.

| Device | Model | Code entry point | Transport |
|---|---|---|---|
| Robot | KUKA KR10 R1420, KR C4 smallsize-2, KSS 8.3.31, RSI 3.3 | `RsiDevice`, key `rsi` | XML over UDP, local 172.31.1.47:5555 |
| PLC | Delta AS332T with AS16AP11P-A and AS16AP11T-A | `PlcDevice`, key `plc` | Custom binary over TCP to 192.168.1.2:5051, fixed source port 1234 |
| Force/torque sensor | Schunk Delta IP68 SI-660-60 (ATI Net F/T electronics) | `FtsDevice`, key `fts` | ATI RDT over UDP, peer 192.168.1.3:49152 |
| Spindle | GDZ-65-800A on an H100-1.5S2-1B frequency inverter (VFD) | none | Started through PLC output Y2.0 |
| Touch probe | WP-500 V6 | none | Probe data enters only as JSON point files (§5.1) |
| Calibration sphere | ⌀25 mm bearing ball, grade G40, ISO 3290 | none | Passive, on a vertical pole on a table in front of the robot; sits 2 mm deep in the top of a ⌀17.8 mm strut, so only the surface above 10.5 mm below its centre is reachable. Strut and table geometry (table top 170.5 mm below the centre, tape-measured 2026-10-29) are in `resources/json/calibration_sphere.json`; the stand CAD `resources/cad/CALIBRATION_STAND.stp` has its origin at the sphere centre |
| EtherCAT I/O | Beckhoff EK1100 coupler, EL1809 (16 DI), EL2809 (16 DO) on the KR C4 | none | No code addresses them; wiring below |
| Network switch | D-Link DGS-1005P (5 Gigabit ports, 4 PoE, unmanaged) | none | Connects the PC to the PLC, sensor and VFD |

### Network

Since 2026-10-23 the PC uses two adapters with separate subnets:

| PC adapter (Windows name) | PC address | Peer |
|---|---|---|
| ASIX AX88179 USB ("KRC4-Direct"), cabled directly to the KR C4 KLI | 172.31.1.47/24 | KR C4 RSI interface 172.31.1.147 (`db.json` RSI endpoint) |
| same adapter | 192.168.2.100/24 | KR C4 Windows interface 192.168.2.200, through which KRL mounts the `RoboCrapJobs` share |
| Intel I219-V ("Devices-Switch"), through the DGS-1005P | 192.168.1.100/24 | PLC 192.168.1.2, sensor 192.168.1.3, VFD 192.168.1.4 |

RSI bypasses the switch: through the DGS-1005P (store-and-forward, 802.3az EEE, 802.3x flow
control, 802.1p QoS per D-Link's B1 datasheet) KR C4 requests sometimes arrived up to 19 ms late and
several at once, and the controller rejected the late replies; over a direct cable none was late
(measured 2026-10-22 with `python/rsi_packet_timing.py` and Wireshark; the cause inside the switch
was not examined). RSI also requires a Windows firewall rule and a Private profile on
"KRC4-Direct" ([HARDWARE_SETUP.md](HARDWARE_SETUP.md)).

### Key device specifications

Datasheet values per component, not measurements of this installation, from the manuals folder
(`E:\Qt\QtProjects\manuals` on DESKTOP-31IFLFK, `D:\Qt\QtProjects\manuals` on the second PC;
checked 2026-10-24) unless marked as user-supplied.

| Device | Key data | Source |
|---|---|---|
| KR C4 smallsize-2 | KLI: Ethernet socket of the KUKA PC for RSI, the Windows interface and WorkVisual. X69 (KSI, cabinet door) is a separate service bus, not used by RSI. RSI cycle 4 ms (`#IPO_FAST`). | Spez KR C4 smallsize-2 §3.7, §6.6.6; RSI 3.3 §5.1, §7.1.4 |
| Delta AS332T-A | 24 VDC; Ethernet, 2× RS-485, USB, micro-SD; onboard 16 DI X0.0–X0.15 (response < 2.5 µs for X0.0–X0.11, < 50 µs for X0.12–X0.15) and 16 DO Y0.0–Y0.15 (sinking transistor, 5–30 VDC); up to 1024 I/O; 40K steps/ms; constant scan cycle 1–32000 ms in 1 ms steps; 128K steps program. | AS Hardware and Operation Manual §2 |
| AS16AP11P-A / AS16AP11T-A | 8 DI + 8 DO each. Inputs: 24 VDC, 5 mA, 4.7 kΩ, sinking or sourcing; ON > 15 V, OFF < 5 V; hardware response < 20 µs on, < 200 µs off, **plus a software filter of 0–25 ms per module, default 10 ms**. Outputs: transistor (P-A sourcing, T-A sinking), 5–30 VDC, 0.5 A per output, 4 A per COM, response ≤ 0.5 ms, ≤ 100 Hz resistive load. | AS Module Manual §2.1.1 |
| Beckhoff EK1100 | EtherCAT coupler, 100 Mbit/s, 24 VDC, E-bus supply 2000 mA, delay ≈ 1 µs. | Datasheet EK1100 |
| Beckhoff EL1809 | 16 DI, 24 VDC (−15 %/+20 %), EN 61131-2 type 1/3; "0" −3…+5 V, "1" 11…30 V; typ. 3 mA; **input filter typ. 3.0 ms**; reference ground 0 V power contact. | Datasheet EL1809 |
| Beckhoff EL2809 | 16 DO, 24 VDC, 0.5 A per channel, short-circuit-proof; **switching typ. 60 µs on, 300 µs off**. | Datasheet EL2809 |
| SI-660-60 (ATI Net F/T) | Ranges Fx, Fy ±660 N; Fz ±1980 N; Tx, Ty, Tz ±60 Nm; loads must stay within range on all six axes. RDT streaming over UDP port 49152. | FT Transducer manual §5.10 |
| WP-500 | Wired optical probe; body Ø42 mm, stylus thread M3, standard stylus 30 mm with Ø3 mm ruby ball; repeatability 0.001 mm; overtravel XY 20°, Z 3 mm; IP67; CR2032 (≈ 6 months standby); 3 m two-core cable with Type-C connector. User-supplied: normally open NPN output, conducting on trigger, which cannot drive EL1809 inputs and is therefore wired to PLC input X2.0 (working, verified 2026-10-24); an LED on the probe lights on contact, so jog touches need no signal. **The user states the probe can run on 24 V; the manual (ENG and CHN) does not mention a 24 V supply.** | WP-500 manual (values matched by row order from garbled PDF text) |
| H100-1.5S2-1B | X1–X6 are multi-function digital inputs; F044 sets the X1 function (factory 02 forward run, 01 RUN); jumper J1 selects NPN or PNP wiring. The F044 and J1 settings of this inverter are not recorded. | H100 manual §3.4, §6.3 |

### Additional hardware and Delta I/O wiring

Supplied by the user. Delta expansion modules:

| Module | Inputs (8 DI) | Outputs (8 DO) |
|---|---|---|
| AS16AP11P-A | X1.0–X1.7; sourcing; S/S to 0 V | Y1.0–Y1.7; sourcing; C0 to +24 V |
| AS16AP11T-A | X2.0–X2.7; sinking; S/S to +24 V, which suits the NPN WP-500 output pulling the input to 0 V | Y2.0–Y2.7; sinking; C0 to 0 V |

All connected PLC terminals (2026-11-06; every other terminal, including the onboard X0.x/Y0.x, is
unconnected):

| PLC terminal | Connected to |
|---|---|
| X2.0 | WP-500 NPN output |
| Y1.7 | EL1809 input 8 (`$IN[8]`); the PLC program copies X2.0 to Y1.7 |
| Y2.0 | VFD terminal X1 |
| Y2.3, Y2.4, Y2.5, Y2.6, Y2.7 | Door panel lamps LED_RUN, LED_GREEN, LED_RED3, LED_RED2, LED_RED1 |

WorkVisual maps EL1809 input n to `$IN[n]` and EL2809 output n to `$OUT[n]` for n = 1–16. Only
EL1809 input 8 is wired; no `$OUT` reaches the PLC.

Probe trigger path: WP-500 → X2.0 → PLC program → Y1.7 → EL1809 input 8 → `$IN[8]`, TRUE while the
probe is deflected (since 2026-11-06; from 2026-10-29 it was Y1.5 → input 11 → `$IN[11]`). Datasheet
delays in order: AS16AP11T-A input (< 20 µs, then the software filter, default 10 ms), the PLC scan,
Y1.7 (≤ 0.5 ms), the EL1809 filter (typ. 3.0 ms), EtherCAT. The AS filter dominates on the PLC side,
the EL1809 filter on the KR C4 side.

Automatic External is switched off and none of its signals uses `$IN`/`$OUT` 1–16; its signal
assignments are in [HARDWARE_SETUP.md](HARDWARE_SETUP.md).

### Controller tool data

TOOL_DATA[5] is the WP-500 TCP (2026-10-24): ruby ball centre X −0.048, Y −0.097, Z 127 mm,
A = B = C = 0° relative to the flange. A CMM measured the probe clamped in its adapter, off the robot,
in the adapter frame {AD} defined by the adapter's flange-side locating surfaces; {AD} is assumed to
coincide with the flange frame {F}. Mounting clearance and the difference between the physical flange
and the controller's FLANGE are not included; the sphere calibration
(`docs/calibration/ROBOT_PROBE_SPHERE_CALIBRATION_METHOD_RU.md`) determines the actual value. With TOOL_DATA[5]
and base $NULLFRAME, HOME is X 1017.00, Y −0.1, Z 1080.05 mm, A 0°, B 90°, C 0°, S 110, T 111010
(smartPAD bits; decimal S 6, T 58): the stylus points along +X of WORLD.

## 2. Modules

Target ownership, links, exported includes, QML registration and resources: "Build and modules" in
AGENT_CONTRACTS.md. `old_imp/` is not built; `python/` holds analysis scripts.

| Target | Folder | Contents |
|---|---|---|
| `robocrap` (exe; URI `robocrap_qml_module`) | src/application | Document, robot, machining, device GUI adaptation, QML. Links Qt Core/Gui/Quick/Qml/Network/Charts/Widgets, plus Felgo in hot-reload builds |
| `RoboCrapGeometry` | src/geometry | Surfaces, fitting, math; exports Eigen 5.0.0 (vendored, header-only) |
| `RoboCrapKinematics` | src/kinematics | KR10 FK/IK |
| `RoboCrapTrajectories` | src/trajectories | Animation motions, RSI sampling |
| `RoboCrapDevices` | src/devices | Device workers and codecs, including RSI |
| `RoboCrap3D` (URI `RoboCrap.Viewport3D`) | src/viewport | OCCT 8.0.0 drawing and CAD loading |
| `Components`, `Styles` (NO_PLUGIN) | qml/Modules | `Qx*` wrappers around Qt Quick Controls; `Colors`, `Fonts`, `Metrics` singletons |
| `cadstep2brep` | tools/cadstep2brep | Build-time STEP → BREP converter |

## 3. Subsystems

### 3.1 Geometry and kinematics

Plain value types with `std::optional` factories; no QObjects, signals or mutable statics.

- `geometry/mathtypes.h`: Eigen aliases `V3d`, `V6d`, `M3d`, `M4d`, `VXd`; `GeomConst::Eps = 1e-9`.
- `geometry/utils.*`: `euler2rot(A,B,C)` = Rz(A)·Ry(B)·Rx(C) in degrees; `rot2euler` returns two
  solutions; `makeTransform`, `makeRotation` (principal- and arbitrary-axis), `inverseRigidTransform`;
  `decodeProbeSamples` validates probe JSON: object form with `points`, finite nonnegative `radius`
  and `dir` = ±1, or a legacy point array (radius 0, dir 1); no compensation.
- Fitting: `Plane`/`BoundedPlane` by SVD; `BoundedCylinder` by Levenberg–Marquardt from 6 seed axes,
  accepted at RMS ≤ max(0.1 mm, 2 % R). `fromSamples` fits original coordinates, then `applyProbe`
  moves the plane by `dir·radius` along its normal or sets the cylinder radius to
  `fittedRadius + dir·radius` (must stay > Eps). No circle fitting or import.
- `intersectSurfaces(BoundedPlane, BoundedCylinder)` returns an analytic ellipse (`EdgeGeometry`)
  or `ParallelCylinderAxis`/`InvalidGeometry`.
- `fitSphere(points)`: algebraic least squares ‖p‖² = 2·pᵀc + k on mean-centred points (JacobiSVD);
  returns centre, radius and RMS.
- `Kr10Kinematics` (src/kinematics, headers in `include/kinematics/`), closed form: `solveFK(q)`
  returns 7 link transforms, the last being the flange; `calculateIK(T06, previous)` returns six
  angles in degrees and Status/Turn for a BASE-relative flange target (Status chooses
  shoulder/elbow/wrist, angles take the nearest permitted ±360° equivalents, Turn records angle signs,
  not revolutions); `configuration(q, transforms)` derives Status/Turn from computed frames;
  `matchesFlange` checks the flange residual without FK. It rejects wrist singularity
  (|sin q5| ≤ Eps), out-of-reach targets and joint limits. It stores `const Kr10KinematicModel&`;
  `RobotController` disables copy/move and destroys the solver before the model.
- `Kr10KinematicModel` holds joints (axis, limits, rated speed °/s), IK parameters, `qHome`, `pHome`,
  filled from the constant table in `RobotDefinition::kr10()` (no robot JSON).
  `IgnorePreviewJointPositionLimits = false` is a compile-time diagnostic switch read by the
  kinematics, `RobotController` and `chamfermotion.cpp`.

### 3.2 Scene (src/application/scene, GUI thread)

| Class | Responsibility |
|---|---|
| `SceneModel` | Owns `SceneObject`s (QObject parent); IDs from `m_nextId`, starting at 1, never reused. `objects` is a read-only `QQmlListProperty`, not an item model. Signals `objectAdded`, `objectRemoved(id)`, `objectVisibilityChanged`, `objectDiagnosticOverlaysChanged`, `objectsChanged`, `intersectionFailed`. `intersect()`, `addMachiningPath()`, `addCalibrationObject(shape, name)`. |
| `SceneObject` | Id, kind (Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath, Sphere, CalibrationPath), classification, name, visibility, overlays. Private setters (`SceneModel` is a friend). `drawing()` supplies analytic dimensions/styles or saved TCP polylines; `drawingDiagnostics()` bounded markers and normals; `properties` the Properties rows (4 decimals, `[x; y; z]`). A sphere also stores the visibility of its ball-centre and compensated spheres. |
| `scenegeometry.h` | `SceneMachiningSettings` (Q_GADGET), `SceneMachiningData{shared_ptr<const ChamferMotion>, sourceEdgeId, robotSetupRevision}`, `SceneMachiningPath` QObject, `SceneEdge`, `SceneSphere` (fitted centre, nominal/ball-centre/compensated radii, RMS, readings, strut radius and recess), `SceneTouchPath{shared_ptr<const ProbeTouchMotion>}`. |
| `SceneSurfaceImporter` | Synchronous JSON plane/cylinder import with probe compensation; `busy` is set before reading and cleared before returning, so painting waits for the call. |
| `SceneEndEffectors` | Active tool, measuring stylus data, collet frame `FLANGE_ER`, spindle XYZABC with `spindleTcpFrame`, measuring TCP with `measuringTcpFrame` (`applyMeasuringTcp`, `measuringTcpChanged`). A collet change emits `spindleColletChanged` then `spindleTcpChanged`. |
| `ScenePlayback` | Dry Run (`App.Playback`): `shared_ptr<const TimedMotion>`, owner path id, 16 ms PreciseTimer, pause/resume, Stop to HOME, phase text, the robot model copy and availability. |
| `SceneMachining` | Chamfer Generate and Dry Run (§5.2); robot-setup revision (robot model, spindle TCP); signed draft-axis direction with 5–40 mm display length. |
| `SceneCalibration` | Calibration submode (`App.Calibration`, §5.5): sphere file, object hiding, Generate worker, calibration path id and revision (sphere, measuring TCP, robot model). |
| `SceneMachiningPreparation` | RSI Sampling (`App.MachiningPreparation`): `sampleChamferRsiStream` worker, `shared_data.txt`, `correctionsReady(QVector<V6d>, QString diagnosticId)`; QML reads `canPrepare`, `unavailableReason`, `preparing`, `error`, `summary`. `prepareTestLin` (Network page, "RSI test LIN") installs the `RSI_Probe_Check` stream. |

QML singletons from `application/backendqmltypes.h`: `App.Scene`, `App.Hub`, `App.Machining`,
`App.MachiningPreparation`, `App.Playback`, `App.Calibration`, `App.EndEffectors` and
`App.WorkspaceController`, each a `main()` stack object returned through `claimQmlSingleton`
(QML_FOREIGN + `create()`, the Qt 6.8 replacement for `qmlRegisterSingletonInstance`; one engine
only). Creatable: `PlaneImporter` (`SceneSurfaceImporter`), `DeviceProfileModel`, `SceneModel`
(used only as a type annotation). Uncreatable: `SceneObject`, `MachiningPath`, `DeviceRunner`,
`PlcMessage`.

### 3.3 Path generation (`RoboCrapTrajectories`)

Implementations in src/trajectories/{chamfer,probe,motion}; published headers in
src/trajectories/include/trajectories.

| Stage | File | Product |
|---|---|---|
| Analytical curve | `chamferpath` | `evaluate(angleRad)` at an ellipse parameter; `evaluatePhase(phase, progress)` for LeadIn/Machining/LeadOut at progress in [0,1]; staging point P_s; `create(parameters)` validates input. |
| Arc length | `chamferarclength` | Per-phase progress → distance (mm) by adaptive Gauss–Kronrod G7/K15 and a piecewise quintic Hermite inverse ([study](../CHAMFER_ARCLENGTH_STUDY.md)); `progressAtDistance` only. |
| Central timing | `chamfercentralmotion` | Elapsed seconds → distance → progress → analytical TCP pose; all phases use `machiningFeed`, ramped over `accelerationDistanceFraction` of each lead. |
| Point schedule | `chamfercentralvalidation` | `ChamferCentralPreparation` (per `ChamferMotion::create`, owned by `MotionCompiler`) builds maps, timing and the 0.01 mm curvature/acceleration schedule; `CentralPath` is the `TimedPath` with phases Lead-in, Machining, Lead-out. |
| Whole motion | `chamfermotion` | `ChamferMotion::create(path, robot)` tries start configurations; per attempt one `MotionBuilder` appends PTP HOME→P_s, LIN P_s→lead-in start, the central curve (one spline), LIN lead-out end→P_s, PTP P_s→HOME. Stores `timed()`, 8 phase boundaries, `ChamferPathParameters` and `ChamferCentralMotion`. |
| Motion module | `timedmotion`, `motion/motionbuilder`, `motion/tcpsolver` | `TimedMotion` (published): timed joint points, smoothstep/spline/joint-linear segments, labelled phases, spline coefficients, `evaluateJoints(seconds)`. `MotionBuilder`: `appendPtp`, `appendPath(TimedPath, interpolation)`; `LinearPath` adapts a LIN/CIRC `LinearProfile`. `ChamferTcpSolver`: checked TCP IK. |
| Motion numerics | `motion/jointsegments` | `ptpDuration`, quintic smoothstep, `linearProfile`/`circularProfile` (trapezoidal, triangular, or constant speed at acceleration 0), `linearSchedule`, `clampedJointSpline`, `ratedRobotSetup`. |
| Probe touch plan | `probetouchplan` | `createProbeTouchPlan(parameters)`: 6 calibration passes (114 touches) and 2 verification passes (44 touches); each pass has purpose, group (fixed probe orientation), repetition, entry pose, touches (start/contact poses, normal, tilt, roll) and exit pose. |
| Probe touch motion | `probetouchmotion` | `ProbeTouchMotion::create(plan, robot)`: one `MotionBuilder`, pass transitions, clearance checks after each step, one phase per step, minimum clearances; keeps the plan. |
| RSI sampling | `chamferrsistream` | `sampleChamferRsiStream(motion)`: one correction per 4 ms plus 50 zero cycles; start/composed-end/staging frames and excursion summary. |

### 3.4 Devices (`RoboCrapDevices`; GUI adaptation in src/application/devices)

| Class | Responsibility |
|---|---|
| `DeviceHub` | Owns I/O threads `ControlIO` and `GeneralIO`, started in the constructor; public slots `startAll`/`stopAll` (§5.4). `add()` moves a parentless device to its thread, connects `QThread::finished` to `deleteLater` and creates a child `DeviceRunner`. |
| `DeviceRunner` | GUI-thread QML proxy: `data` (`QQmlPropertyMap` pre-filled with `connectionConfig`), `socketState`, `isConnected` (Connected or Bound), `isDisconnected`; all device links queued. |
| `AbstractDevice` | Slots `start`, `stop`, `invoke` guarded by `m_started`; signals `stateReady`, `socketStateReady`; `attachSocket()` forwards state and logs socket errors. |
| `PlcDevice` (GeneralIO) | `QTcpSocket` bound to a local port, then `connectToHost`. Invokables `connect`, `disconnect`, `setOutput`, `writeMessage` (raw WRITE_RAW/WRITE_REG/SET_VAR frames; no QML caller). `PlcMessageManager`: 6-byte big-endian header (0xAA55, version, type, transaction id, length) plus command, whole frame swapped in 16-bit words; strict 8/16-bit readers; fixed 32-byte responses. SNAPSHOT on connect. State keys `x1`, `y1`, `x2`, `y2` (8 bools each), `cellState`. The framing is defined by the PLC program (not in the repository); the user confirmed it works on 2026-10-13. |
| `FtsDevice` (ControlIO) | `QUdpSocket`; invokables `connect`, `disconnect`, `startStreaming`, `stopStreaming`, `bias`, `startLogRecording`, `stopLogRecording`, `saveLogToDefaultFile`. RDT request 0x1234, command, count 0; one 36-byte record per datagram; timestamps from sequence difference at an assumed 7000 Hz; 1e6 counts per unit. Each packet emits `dataSampleHFReady`; after ≥ 16 ms `processBatch` publishes the last sample (all axes first, then changes ≥ 0.05). Recording saves through `std::async` + `QSaveFile` to `<appDir>/records/`. |
| `RsiDevice` (ControlIO, src/devices/rsi) | `QUdpSocket`; answers the newest valid `<Rob>`; states Idle → Streaming → Finished → Stopping. Invokables `connect`, `disconnect`; slot `setCorrections(QVector<V6d>, QString)`; publishes `rsiState`, `rsiCorrections`. Behavior: RSI_Chamfer_Hole_CONTRACT.md "PC behavior". |
| `DeviceProfileModel` | Read-only table from `:/json/db.json`: default endpoints for the Network view. |
| `QmlChartBridge` | Compiled into `robocrap` with QtCharts; its construction in `main.cpp` is commented out. |

Console output uses `[ERROR]`/`[OK]`/`[INFO]`/`[WRITE]` labels with source names; Qt's message
pattern adds timestamps; `logViewportMessage` in main formats viewport messages. `main.cpp` connects
`SceneMachiningPreparation::correctionsReady` queued to `RsiDevice::setCorrections` and registers
`QVector<V6d>`.

### 3.5 3D viewport (`RoboCrap3D`)

| Class | Responsibility |
|---|---|
| `OccController` | Viewport3D singleton: native window, robot CAD worker, model CAD worker (`DrawingModel` BREPs, one path at a time, results in `DrawingDocument::models`), neutral graphics, accepted `DrawingDocument`/`DrawingPose`, tool display state. |
| `OccViewWindow` | QWindow in RobotViewport's WindowContainer: native surface, input, robot installation, active tool, `viewport()`; first expose builds the viewport. |
| `OccViewport`, `OccViewer`, `OccScene`, `OccPart`, `OccInputController`, `OccViewCube`, `OccWorldAxes` | `OpenGl_GraphicDriver` → `V3d_Viewer`/`V3d_View` on a `WNT_Window` plus `AIS_InteractiveContext`. Rendering is coalesced by a 0 ms single-shot `QTimer` on the GUI thread, except robot installation (`OccViewport::setRobot`), drawn immediately so the robot shows before `ready`. No render thread, no `QOpenGLContext`. |
| `OccRobotAdapter` | Eigen → gp_Trsf (`toOccTransform`); one installation creates seven links then measuring and spindle tools (`m_partIds` in that order). |
| `OccSceneAdapter` | Analytic patches/curves/spheres, polylines, loaded `DrawingModel` shapes (translated; skipped until loaded) and diagnostics → OCCT parts; picking returns the application ID. Surfaces, decorative parts and diagnostics are separate lists per object. |
| `CadLoadWorker` | `QThread` subclass; a function-local static mutex serializes parsing; reads `.brep` bytes through `QFile` into `BRepTools::Read`; returns ordered shapes or `Cannot read CAD file: <path>`. |
| `RobotDefinition` / `RobotController` (application) | Constant KR10 table → model and link filenames/RGB. RobotController owns the model, solver, checked initial HOME candidate and accepted pose; not a QML type; `loadRobot()` and `applyPlayback(joints, flangeToTcp)` return `ViewportOperationResult`, internal `initialize`/`forward` return `RobotPoseResult`. |
| `ViewportAssets` (application assetpaths.*) | Selects the CAD and OCCT resource directories beside the executable. |

### 3.6 QML UI (src/application/qml; reusable modules in qml/Modules)

- `Main.qml`: one `ApplicationWindow`, no page navigation; a 3-column `SplitView`: left
  `WorkflowPanel`, `ScenePanel`; centre `ModeToolBar`, RSI status label (`data.rsiState` while bound),
  `RobotViewport`; right `PropertiesPanel`, two `EndEffectorPanel`s and `MachiningPanel`, switched by
  `visible`. Tool windows hold the device panels: PLC `DeltaPanel`, FTS `FtsPanel`, Settings/`Network`.
- Status bar (right-aligned): "Loading…" while `OccController.ready` is false without
  `errorString`; "Calculating trajectory…" while machining or calibration calculates; "RSI Sampling…";
  in Calibration the calibration error, playback status ("Playing · Calibration G2 pass 1, touch
  5/19: approach · 34.12 / 246.19 s") or the reason Generate is unavailable. A `QxBusyIndicator` is a child of the label,
  outside the layout. Nothing draws over the native OCCT window (`docs/OCCT_SCENE_OVERLAY.md`).
  Generate has no modal window; while calculating only Generate, Dry Run and RSI Sampling are
  unavailable.
- `WorkspaceController` (src/application/workspacecontroller.*) owns mode (Measuring/Machining),
  submode (Calibration/Rough/Precise) and selection, emits `calibrationActiveChanged` (connected to
  `SceneCalibration::setActive`) and makes the sole selected Edge/MachiningPath the machining input.
  Main keeps the UI-only `dryScanActive`/`dryScanRunning`, `chamferEditorOpen` and `toolSetupOpen`;
  `MachiningPanel` owns draft text, `draftFlipAxis` and captured settings; `EndEffectorPanel` owns
  pose drafts.
- Device panels use `App.Hub.device(key)`, `isConnected`, `data.<key>` and `invoke(name, map)`.
  `DeltaPanel` labels the PLC terminals and the door panel lamps (§1 wiring).
- Placeholders: Dry Scan only toggles Main flags; Scan and Start Machining are disabled
  (`available: false`); Cone import and the Precise submode do nothing.
- In Calibration the Scene panel lists only the "Calibration" group; sphere Geometry rows show the
  three radii with check boxes for the fitted spheres, the centre, RMS and point count.
- Resources: `pics.qrc` (SVG icons), `fonts.qrc` (Roboto), `json.qrc` (`db.json`). CAD: seven link
  BREPs in `resources/cad/kr10` (no STEP sources) and `EE_WP-500.stp`, `EE_GDZ-65-800A.stp`,
  `CALIBRATION_STAND.stp` in `resources/cad`, converted at build time; `resources/files` keeps
  blade/RSI inputs outside any qrc.
- `PropertiesPanel.qml` renders `selectedObject.properties` with a fixed-width label column.

## 4. Threads and ownership

| Thread | Objects / work | Created by |
|---|---|---|
| GUI | `QApplication`, `DeviceHub`, `DeviceRunner`s, scene classes, `OccController`/`OccViewWindow` and OCCT rendering, QML engine; JSON import, `ChamferPath::create`, `shared_data.txt` write, 16 ms Dry Run timer | `main()` stack |
| `ControlIO` | `FtsDevice`, `RsiDevice`, their sockets/timers | `DeviceHub` |
| `GeneralIO` | `PlcDevice`, its socket | `DeviceHub` |
| Generate (`std::async`) | `ChamferMotion::create` on a copied `ChamferPath` + `ChamferRobotSetup`; 30 ms GUI poll | `SceneMachining::generate` |
| Calibration (`std::async`) | `createProbeTouchPlan` + `ProbeTouchMotion::create` on copied inputs; 30 ms poll | `SceneCalibration::generate` |
| RSI sampling (`std::async`) | `sampleChamferRsiStream` on a copied `shared_ptr<const ChamferMotion>`; 30 ms poll | `SceneMachiningPreparation::prepare` |
| FTS save (`std::async`) | JSON write of a copied buffer | `FtsDevice` |
| CAD (`QThread` subclasses) | Robot job (seven links, two tools) and `DrawingModel` jobs; reads serialized by a static mutex | `OccController` |

- Application singletons are parentless `main()` stack objects; the QML engine is declared last and
  destroyed first.
- Devices are deleted on their own thread by `deleteLater` when it finishes; runners are hub children.
- A `SceneObject` is a `SceneModel` child; removal emits `objectRemoved`, then `deleteLater`.
- Shared immutable data: `ChamferMotion`, `ProbeTouchMotion`, their aliased `TimedMotion` and
  `CadLoadResult` as `shared_ptr<const ...>`; the RSI stream as an implicitly shared `QVector<V6d>`.
- `Q_ASSERT` thread checks: `AbstractDevice::start/stop/invoke`, `DeviceRunner::invoke`,
  `OccController::loadRobot`, `RobotController::loadRobot/applyPlayback`, `SceneMachiningPreparation`;
  `claimQmlSingleton` `qFatal`s on a wrong-thread engine.

## 5. Traced workflows

### 5.1 Import

Probe JSON: `ModeToolBar` request → `FileDialog` (`*.json`) → `surfaceImporter.load`/`loadCylinder(url,
App.Scene)` → `readSamples` + `decodeProbeSamples` → `BoundedPlane`/`BoundedCylinder::fromSamples` →
`SceneModel::addBoundedPlane`/`addBoundedCylinder` → `appendObject` asks OccController to install
`drawing()`; only accepted insertion publishes `objectAdded`/`objectsChanged` → `loaded(object)` →
`WorkspaceController::selectObject`. "Intersect Selected" → `SceneModel::intersect` adds the Edge
that Generate uses.

Robot CAD: main calls `RobotController::loadRobot` once; it checks HOME and supplies the nine BREP
paths (seven links, `EE_WP-500.brep`, `EE_GDZ-65-800A.brep`), RGB and matrices; one installation
inserts them; on graphics readiness RobotController accepts HOME and hands the model to
ScenePlayback. Mode switches call `SceneEndEffectors::setActiveTool`; OccController only toggles tool
visibility.

Calibration stand: `SceneCalibration::activate` sets the sphere's `standModel` to
`resources/cad/CALIBRATION_STAND.brep`; `SceneObject::drawing()` adds the table top (`DrawingPlane`
at centre + (lengthX/2 − sphereFromMinusX, widthY/2 − sphereFromMinusY, −sphereAboveTop)) and the
stand (`DrawingModel` at the centre); the sphere is redrawn when the stand load finishes.

### 5.2 Generate → Dry Run

1. Toolbar "Generate Path" opens `MachiningPanel` and captures settings; its "Generate" commits them
   to `App.Machining.settings` and emits `generateRequested`; Main calls `App.Machining.generate()`.
   Cancel restores the captured settings.
2. `SceneMachining::generate` checks `unavailableReason()` (robot ready, 6-value spindle TCP, an Edge
   whose two sources exist, or a saved path; the parameters use only the ellipse and the cylinder
   origin/axis, but the plane must still exist), builds `ChamferPath::create(settings.applyTo(source))`
   on the GUI thread, copies model/`flangeToTcp`/timing into `ChamferRobotSetup`, records input id and
   revision, and starts the worker.
3. `finishCalculation` logs one line, rejects stale results, calls `SceneModel::addMachiningPath`;
   a regenerated path copies name/visibility and removes the old object; then `setInputId(newId)`,
   `generated`.
4. "Dry Run" → `App.Machining.dryRun()` (`canPlay()`) → `ScenePlayback::start(timed, pathId)`:
   presents time zero, then each 16 ms tick `evaluateJoints(seconds)` →
   `RobotController::applyPlayback(joints, savedFlangeToTcp)` (one FK); failure emits
   `failed(ownerId, error)`. `pause()` stops the timer; `dryRun()` resumes; `stop()` presents `qHome`
   and ends.
5. Playback also ends at its duration, on `aboutToQuit`, robot unavailability, a revision change of
   machining-owned playback, removal of the active path and leaving Machining mode.

### 5.3 Device request and state

`DeltaModuleAP.qml` → `plc.invoke("setOutput", {module, index, enabled})` → `DeviceRunner::invoke`
emits `invokeReq` (queued) → GeneralIO `AbstractDevice::invoke` looks up `m_api` and calls the method
directly → `PlcDevice::setOutput` writes a WRITE_IO frame → `readyRead` matches 32-byte responses by
transaction id and command → `stateReady(QVariantHash)` (queued) → `DeviceRunner::onStateReady`
inserts into `data`. Socket state: `stateChanged` → `onSocketStateChanged` → `socketStateReady` →
`onSockState` → `isConnected`. Startup: `hub.startAll()` → `startReq` → `buildApi()` +
`startDevice()` creates sockets; connecting waits for `invoke("connect", config)` from `Network.qml`.

### 5.4 Shutdown

1. `aboutToQuit`, in connection order: `ScenePlayback::end`;
   `SceneMachiningPreparation::shutdown` (blocks on a sampling future, emits an empty
   `correctionsReady`, deletes `shared_data.txt`); `DeviceHub::stopAll`.
2. `stopAll` (returns at once if stopped; asserts it runs outside both I/O threads): a
   `BlockingQueuedConnection` stop per device, then `quit()`/`wait()` both threads. The empty
   `setCorrections` from step 1 is posted by the same GUI thread before the RSI stop, so it arrives
   first; this is not a general FIFO guarantee. `FtsDevice::stopDevice` waits for its save.
3. After `app.exec()`, reverse declaration order: QML engine (`RobotViewport` calls
   `detachViewWindow`); RobotController marks the robot unavailable and OccController waits for both
   CAD workers (a BREP read cannot be interrupted) and deletes the window;
   `SceneMachiningPreparation` (no-op `shutdown`); `SceneCalibration`, `SceneMachining` (future
   destructors block on running workers); `ScenePlayback`, `SceneEndEffectors`, `SceneModel`;
   `DeviceHub` (no-op `stopAll`); `QApplication`.

### 5.5 Calibration Generate → Dry Run

Measuring → Calibration emits `calibrationActiveChanged(true)`; `SceneCalibration::setActive(true)`
reads `<exe dir>/resources/json/calibration_sphere.json`, fits the ten readings, inserts or keeps the
sphere (a changed sphere removes the path) and hides/records other visible objects. "Generate Path"
→ `App.Calibration.generate()` (no calculation or playback running; robot model and preview,
measuring TCP and sphere present) copies sphere, ruby ball radius, strut, model and
`measuringTcpFrame`, records the revision and starts the worker. `finishCalculation` logs
`Probe calibration preparation: …`, rejects a changed revision, inserts the `CalibrationPath`
(hidden if Calibration was left) and removes the previous path. "Dry Run" plays it through
`ScenePlayback`. Leaving Calibration stops its playback at HOME and restores the recorded objects.

## 6. External-library boundaries

- Qt 6.8/QML: `USE_HOT_RELOAD` defaults to OFF (QML embedded, `engine.loadFromModule(
  "robocrap_qml_module", "Main")`); an existing cache may differ. `FAST_QML_BUILD=ON` (default) adds
  `NO_CACHEGEN`. Numerical code uses only QtCore.
- OCCT 8.0.0: only src/viewport/occt, src/viewport/adapters and OccController's implementation; Release
  libraries only; `cadstep2brep` alone links `TKDESTEP` and runs through `cmake/ConvertStep.cmake`
  with OCCT/MinGW DLLs on `PATH`. While the native surface is destroyed (exit, Felgo hot reload),
  availability is false and a Generate result is rejected (user-confirmed).
- Eigen 5.0.0: header-only in `libs/`, exported PUBLIC by Geometry because its headers expose Eigen.
- Felgo: hot-reload builds only (`FelgoApplication`, `FelgoHotReload` in `main.cpp`).
- Concurrency: `std::async`/`std::future` for all workers except CAD; no `QtConcurrent`.

## 7. Open questions

Controller-side RSI items are listed in the RSI contracts.

- **Net F/T.** The manual confirms header 0x1234, commands 0x0000 stop / 0x0002 streaming / 0x0042
  bias, UDP 49152, internal 7000 Hz with RDT output 1–7000 Hz, and calibration-defined counts per
  unit. Unverified: the configured RDT rate (code assumes 7000 Hz) and counts (hard-coded 1e6).
- **PLC pending lifetime.** `availableTid()` scans 256 IDs; if writes succeed but replies never come
  on an open connection, pending entries fill all IDs and writes fail with "All PLC transaction ids
  are pending". Matched replies (errors included), failed/partial writes, disconnect, socket
  UnconnectedState and device stop clear entries; no timeout/retry. Derived from code, not observed.
- **VFD endpoint.** `db.json` names the 192.168.1.4 device "VFD ATV320U40N4C" (`driver: "vfd"`,
  Modbus TCP) with no device class, while the spindle inverter is the H100-1.5S2-1B.
- **Not exercised at runtime:** binding notification for every key of
  `QQmlPropertyMap::insert(QVariantHash)`; ControlIO latency with RSI and FTS sharing one event loop;
  shutdown blocking durations (Generate, sampling, BREP reads); Felgo hot reload with the one-engine
  `qFatal` checks; OccScene outline reuse, picking, replacement, partial OCCT failures and replay.
- **Calibration clearance.** Generate checks only the ball centre against sphere and strut; links are
  not checked against the fixture. The fixed probe orientations never point the stylus upward; link
  clearance to the table and stand is not computed, only seen in Dry Run (user reported it works,
  2026-11-11).
- **Data and units.** Imported files carry no validated unit; coordinates and probe radius must be mm.
  `FLANGE_ER` is hard-coded from CMM data (ER y-axis and origin; roll from projecting the flange
  z-axis), only a reference for entering the spindle TCP (user, 2026-10-24). Link BREPs have no STEP
  sources; OCCT compatibility is checked only by `BRepTools::Read`. End-effector STEP files must be in
  the flange frame and the stand in {S}; models are only translated. `IkSolution::status` selects the
  next IK branch; the bit mapping's match with installed KUKA semantics is unverified.
- **Retained interfaces.** `SceneModel` is registered as `SceneModel` and singleton `Scene` (not a
  documented Qt pattern). `FtsDevice::dataSampleHFReady`, `dataBatchReady`, `streamReset` and the
  `RDTResponse` registrations are kept for future force control (user decision);
  `ChamferPathSample`/`ChamferPathResult` serve analytical evaluation. Device/viewport error
  properties and QML displays are deferred (user). Link BREPs, STEP sources and `resources/files`
  are retained assets. The `devicehub.h` class comment still mentions one shared I/O thread.
