# One-hole-edge machining: agreed strategy and implementation handoff

Prepared on 2026-10-07. Repository: D:/Qt/QtProjects/RoboCrap.

Completeness audit: checked against all 33 numbered answers in clarifications.txt, the subsequent force/lead-out/FTS/Stop clarifications, and the final explicit +e_z^T instruction. Section 18 preserves the answer-by-answer record. Section 2.1 governs how to proceed without asking the user the same questions again.

This document preserves the user's decisions and explains how to connect the current chamfer trajectory to KUKA RSI, with a later force-dependent normal retreat. It is intended to be sufficient context for another implementation agent without the conversation history.

**This is a strategy and handoff document, not a claim that robot control is implemented or validated.** Creating this document authorizes no source changes or hardware operation. A subsequent implementation request should use it as the design baseline and maintain an ExecPlan as required by AGENTS.md and PLANS.md.

## 1. Read this first

The most important behavioral requirements are:

1. Process one internal hole edge using the existing generated Lead-in + Machining + Lead-out trajectory.
2. KRL performs the auxiliary HOME and staging movements. The PC supplies the entire central motion through RSI; force correction is added to that nominal central motion.
3. Initial target: T1, KR10 R1420, KR C4, KSS 8.3.31, RSI 3.3, Cartesian POSCORR, relative corrections, 4 ms cycle. A 12 ms alternative is a deliberate later configuration change, not an automatic fallback.
4. The nominal feeds come from the saved trajectory's application settings. Do not substitute the unrelated demonstration trajectory currently in RsiDevice.
5. When the chosen force measure exceeds its threshold during Machining, move the tool away from the material along **positive TCP Z, +e_z^T**, while continuing nominal tangential progress.
6. When force falls below the threshold, retain the accumulated retreat. Do not spring back toward the nominal path during Machining.
7. Carry that displacement continuously into Lead-out, then remove it smoothly after sufficient clearance. Lead-in does not use force correction; Lead-out does not make new force-feedback decisions.
8. Stop terminates the current machining execution. There is no automatic recovery, Lead-out, return to P_s, or HOME movement after an abort.
9. Compute the ATI Tool Transformation once for a particular calibrated TCP and enter it in the active Net F/T configuration. Consume the already transformed wrench in the application; do not transform it twice.
10. Implement nominal execution first, with force correction disabled. Keep the numerical interfaces ready to add the force-dependent offset.

The final +e_z^T direction above supersedes the earlier reference to +e_z^i. Do not ask the user to choose that direction again or silently reverse it to match an older diagram interpretation.

## 2. Decision status and scope

Use these distinctions throughout implementation:

- **Agreed:** behavior explicitly settled with the user, summarized above and detailed below.
- **Verified current code:** observations from the repository at the preparation date. Recheck affected sources before editing; the repository may evolve.
- **Recommended implementation:** a concrete way to realize the agreed behavior, not a previously approved class name, packet extension, gain, or controller configuration.
- **Unresolved commissioning detail:** information required before enabling the corresponding hardware behavior. Represent it explicitly; do not invent a value or describe a proposal as already verified.

The workpiece is steel. The intended nominal result is an approximately 0.5 mm by 45 degree chamfer, with no particular surface-roughness target at this stage. The cutter is a cylindrical burr. The sensor is a Schunk Delta IP68 SI-660-60 used with ATI Net F/T electronics. The sensor supports the spindle assembly as shown in the supplied end-effector drawing.

The user has chosen to keep cyclic processing in the Windows Qt application initially. KR C4 and Net F/T share an Ethernet switch. Preserve that scope. Windows/Qt execution is not a demonstrated hard-real-time guarantee; measure deadlines and handle failures instead of promising deterministic 4 ms scheduling.

Outside the initial implementation scope: automatic mode, multiple holes, tangential feed reduction due to force, pause-and-relieve machining, force-driven orientation changes, bidirectional constant-force regulation, automatic recovery after abort, and a full dynamic compensation model with invented mass/inertia data.

The shared-folder mechanism already works. Its future role is to transfer auxiliary points and job metadata before motion. The user explicitly deferred file work. Do not make implementation of a new file protocol a prerequisite for the numerical nominal-motion/force pipeline, and do not create controller files as a side effect of reading this document.

### 2.1 Continuation policy: avoid repeated clarification

When the user authorizes implementation, begin the applicable stages using this document. Do not respond with another general questionnaire, ask permission to choose ordinary class/file names, or require the user to restate the strategy. The selected model or reasoning level does not change these requirements.

Treat sections 1, 8 and 18 as the settled behavioral record. New explicit user instructions can change it. Older attachment wording does not overrule the final direction, continuous-feed choice, retained retreat, lead-out blending or abort behavior. Proposed filtering/gains, packet names and controller mappings remain proposals until implemented and verified; do not mislabel them as user-supplied settings.

For apparent missing information, follow this order:

1. Read this document's answer record and the relevant source files. Geometry, TCP composition, feeds, joint limits, device threading and existing UI commands are discoverable here; they are not questions for the user.
2. Read the supplied manuals and any available installed object documentation. Controller/API semantics are the implementer's research task, not a request for the user to explain RSI mathematics.
3. Make routine implementation choices within the recorded responsibilities and document them in the ExecPlan. In particular, design the new KRL/RSI context and protocol extensions; the user already stated that no strict job-specific context/program exists. Do not ask for a missing .rsi/.src that the user has asked us to design.
4. Keep genuinely unknown equipment values as explicit unavailable configuration. Implement preparation and other independent work without inventing hardware settings. Lack of a live Net F/T export, force threshold or load data must not block nominal software preparation.
5. Only when the next dependent stage requires a fact that cannot be derived from these sources, request that precise fact once. State which operation it enables and keep independent work moving. The likely later inputs are a current configuration export or measured/tuned parameters, not another choice of overall strategy.

Missing physical data is not approval to run equipment, and time elapsed without an answer is not a value. Conversely, a commissioning requirement must not be presented as a prerequisite for every offline stage. Keep the distinction visible in the plan and user-facing status.

## 3. Repository rules that remain binding

Read [AGENTS.md](D:/Qt/QtProjects/RoboCrap/AGENTS.md) before implementing. For a complex implementation, read [PLANS.md](D:/Qt/QtProjects/RoboCrap/PLANS.md) and maintain the required ExecPlan. This document does not override either file. In particular:

- C++17, Qt 6.8 minimum, CMake 3.16 minimum; preserve existing module URIs and dependencies. The project context includes Felgo 4.4.0, OCCT 8.0.0 and bundled Eigen. Root/module CMake files are authoritative.
- No C++ lambdas; use named functions/member methods. No try/catch/throw in project C++.
- Return cohesive results through return values; do not introduce output parameters or mutate a shared result object from a worker.
- At most four function/constructor parameters; three or four require a responsibility-based reason. Group genuinely related concepts, not arbitrary parameter bags.
- Do not access another object's private members directly, including through friendship.
- Do not add project tests, test harnesses, or test build targets. Validation below describes acceptance evidence, not permission to add those facilities.
- Build/configure/build-backed lint only when explicitly requested. No git staging, commits, branch changes or destructive operations without explicit user instruction. These AGENTS.md constraints take precedence over conflicting generic suggestions in PLANS.md.
- Do not inspect old_imp, directories beginning with old, python, libs or build for this analysis. Do not edit generated or vendored files.
- Live PLC/FTS/RSI connections, output writes, sensor biasing and robot motion require explicit authorization covering that hardware action. A software implementation request is not by itself authorization to operate equipment.

Existing code has historical interfaces that do not meet every current style rule. Do not copy those patterns into new APIs, and do not turn this feature into an unrelated cleanup.

## 4. Existing architecture and exact integration points

### 4.1 Modules and ownership

The following map comes from actual CMake dependencies and code usage:

| Layer | Existing components | Responsibility and boundary |
| --- | --- | --- |
| Application/QML | main.cpp, Main.qml, MachiningPanel.qml, tool panels | Application composition, user settings, selection and commands; GUI thread. |
| Scene/application state | SceneModel, SceneObject, SceneMachiningPath, SceneMachining, SceneEndEffectors | Own saved geometry, calibrated TCP, immutable motion snapshots and preview coordination. |
| Numerical geometry | ChamferPath, ChamferMotion, Kr10Kinematics, geometry/mathtypes.h and utils.cpp | Geometry, poses, kinematics and trajectory timing; no socket or OCCT presentation dependency. |
| Device infrastructure | DeviceHub, DeviceRunner, AbstractDevice | Thread ownership, queued commands, state publication and lifetime. |
| Device/protocol | RsiDevice, RsiProtocol, FtsDevice, RDTResponse, PlcDevice | UDP protocols, sample reception and future execution session. |
| Visualization | OccController, OccSceneAdapter, OccRobotAdapter, OccViewport | OCCT presentation and preview poses; never the source of cyclic hardware commands. |
| Robot controller | KRL, RSI Visual context, ETHERNET and POSCORR objects | Auxiliary robot movements and execution of cyclic Cartesian corrections. |

In src/CMakeLists.txt, RoboCrapGeometry owns the numerical chamfer and RSI path sources. RoboCrapScene and RoboCrapDevices each link Geometry. RoboCrapBackend registers the QML-facing types and links Scene and Devices. RoboCrap3D links Scene and Geometry. Do not introduce a Scene-to-Devices-to-Scene dependency cycle or move numerical control into the viewport.

main.cpp owns SceneModel, then SceneEndEffectors, then SceneMachining, then the viewport controller and QML engine. SceneModel owns SceneObject children; a saved SceneMachiningPath owns a shared immutable ChamferMotion through SceneMachiningData. Saved paths retain geometry, robot model, TCP and settings snapshots and survive deletion of their source edge. Model/TCP changes mark paths incompatible until regenerated.

DeviceHub owns runners on the application thread and moves parentless devices to I/O threads. FTS and RSI share ControlIO; PLC uses GeneralIO. In main.cpp, FtsDevice::dataSampleHFReady connects directly through Qt's automatic connection mechanism to RsiDevice::setForce; because both devices run on ControlIO, the call currently executes on that same thread. Preserve or explicitly rework this contract if thread grouping changes.

QML must use Backend.Hub.device(key), which returns a DeviceRunner, rather than calling a device across threads. Runner startReq, stopReq and invokeReq are queued. Its QQmlPropertyMap stays on the GUI thread. Device methods available through invoke must be uniquely named public Q_INVOKABLE void functions taking zero arguments or one QVariantMap.

Sockets and timers are created, used and destroyed on their device thread. DeviceHub::stopAll completes device stops before quitting/waiting for threads. Its blocking queued calls must not originate inside the receiving I/O thread. Preserve thread-finished/deleteLater cleanup and repeat-call safety.

### 4.2 Files to read before editing

All links below refer to the current workspace. Inspect affected callers as well as definitions.

| Files | Important behavior |
| --- | --- |
| [src/main.cpp](D:/Qt/QtProjects/RoboCrap/src/main.cpp) | Object lifetimes, device grouping and FTS-to-RSI connection. |
| [src/CMakeLists.txt](D:/Qt/QtProjects/RoboCrap/src/CMakeLists.txt) | Owning targets and QML registration dependencies. |
| [src/backendqmltypes.h](D:/Qt/QtProjects/RoboCrap/src/backendqmltypes.h) | Existing QML_FOREIGN registration and singleton exposure. |
| [src/scene/scenemachining.h](D:/Qt/QtProjects/RoboCrap/src/scene/scenemachining.h), [implementation](D:/Qt/QtProjects/RoboCrap/src/scene/scenemachining.cpp) | Settings conversion, apply, copied-input async compilation, revision rejection, saved-path insertion and Dry Run. |
| [src/scene/scenegeometry.h](D:/Qt/QtProjects/RoboCrap/src/scene/scenegeometry.h) | SceneMachiningSettings, SceneMachiningData and SceneMachiningPath. |
| [src/scene/sceneendeffectors.cpp](D:/Qt/QtProjects/RoboCrap/src/scene/sceneendeffectors.cpp) | applySpindlePoses and flange/ER/TCP composition. |
| [src/pathgeneration/chamfer/chamferpath.h](D:/Qt/QtProjects/RoboCrap/src/pathgeneration/chamfer/chamferpath.h), [implementation](D:/Qt/QtProjects/RoboCrap/src/pathgeneration/chamfer/chamferpath.cpp) | Analytic nominal poses, phases, staging poses and adaptive geometric sampling. |
| [src/pathgeneration/chamfer/chamfermotion.h](D:/Qt/QtProjects/RoboCrap/src/pathgeneration/chamfer/chamfermotion.h), [implementation](D:/Qt/QtProjects/RoboCrap/src/pathgeneration/chamfer/chamfermotion.cpp) | Timed seven-stage simulation, IK/FK checks, continuous joints and evaluate(seconds). |
| [src/pathgeneration/rsi/rsipath.cpp](D:/Qt/QtProjects/RoboCrap/src/pathgeneration/rsi/rsipath.cpp) | Existing linear/polyline offset helpers; limitations described below. |
| [src/network/rsi/rsidevice.cpp](D:/Qt/QtProjects/RoboCrap/src/network/rsi/rsidevice.cpp) | Current demonstration path, UDP callback, offset index and incomplete stop behavior. |
| [src/network/rsi/rsiprotocol.h](D:/Qt/QtProjects/RoboCrap/src/network/rsi/rsiprotocol.h), [implementation](D:/Qt/QtProjects/RoboCrap/src/network/rsi/rsiprotocol.cpp) | Sender validation, XML/IPOC decoding and reply encoding. |
| [src/network/fts/ftsdevice.cpp](D:/Qt/QtProjects/RoboCrap/src/network/fts/ftsdevice.cpp), [rdtmessage.h](D:/Qt/QtProjects/RoboCrap/src/network/fts/rdtmessage.h) | Raw high-frequency samples, scaling assumptions, display batching, recording and status/sequence fields. |
| [src/network/devicehub.cpp](D:/Qt/QtProjects/RoboCrap/src/network/devicehub.cpp), [devicerunner.cpp](D:/Qt/QtProjects/RoboCrap/src/network/devicerunner.cpp), [abstractdevice.cpp](D:/Qt/QtProjects/RoboCrap/src/network/abstractdevice.cpp) | Threading and queued command contracts. |
| [src/geometry/mathtypes.h](D:/Qt/QtProjects/RoboCrap/src/geometry/mathtypes.h), [utils.cpp](D:/Qt/QtProjects/RoboCrap/src/geometry/utils.cpp) | Eigen types, transform and Euler conventions. |
| [qml/Views/Workspace/MachiningPanel.qml](D:/Qt/QtProjects/RoboCrap/qml/Views/Workspace/MachiningPanel.qml) | Draft parameters, including lead-in, machining and lead-out feeds in mm/s. |
| [qml/Views/Dashboard/KukaPanel/KukaPanel.qml](D:/Qt/QtProjects/RoboCrap/qml/Views/Dashboard/KukaPanel/KukaPanel.qml) | Existing string-dispatched demonstration RSI commands. |
| [qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml](D:/Qt/QtProjects/RoboCrap/qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml) | PLC spindle command appears as RUN MOT. |

### 4.3 Current user/application workflow

The user imports fitted plane/cylinder data, intersects the selected surfaces to create an edge, selects that edge, configures the spindle TCP and applies machining settings through Generate Path. SceneMachining builds a ChamferPath and asynchronously compiles a ChamferMotion using copied numerical inputs. A GUI-thread timer collects the result and inserts the saved path into SceneModel. No scene QObject is accessed from the numerical worker.

Dry Run uses a GUI timer and monotonic clock, evaluates the retained ChamferMotion and sends joint poses through OccController to the preview. It does not invoke devices. Preview Stop returns the animation to HOME. **That preview behavior must not be reused for hardware Stop**, which must not initiate a return motion.

The future hardware action should prepare a numerical execution job from a selected compatible saved path. It must not reconstruct geometry from OCCT display polylines, use whatever is currently visible as its source, or read changing QML draft fields during execution.

The final user workflow remains surface JSON import -> Intersect Selected -> select edge -> configure TCP -> Generate Path/Apply -> prepare that saved path for machining. In the present UI, Generate Path opens the settings editor and Apply creates the path. Do not replace this with mandatory manual point entry or the separate KukaPanel demonstration button. Actual machining execution remains a distinct deliberate action; generating or selecting a path must not start hardware motion.

Stage A now supplies the separate Prepare action for a selected compatible saved path. Backend.MachiningPreparation captures that immutable motion, validates its boundaries on a numerical worker, reports the central duration/time scale and sends a typed job to RSI through a queued connection in main.cpp. Selection or compatibility loss invalidates that prepared job. RsiDevice stores it using separate preparedJob* state; the job is not consumed by the demonstration motion or any execution session. This implements offline preparation only; complete 4 ms sampling and controller encoding remain subsequent work.

### 4.4 Verified gaps in the current RSI implementation

- RsiDevice::generateTrajectory constructs unrelated hardcoded geometry and a demonstration RsiPath::lin motion. It is not connected to SceneMachiningPath.
- RsiPath::lin/polyline use a hardcoded 0.004 s period and distances in a six-component XYZABC vector. That mixes millimetres and degrees and linearly interpolates Euler coordinates. Do not use that metric to time a general chamfer pose trajectory.
- Those helpers round each increment independently. Repeated rounded relative increments can accumulate endpoint error.
- RsiDevice::setForce only stores sample.Fz divided by 1,000,000 in m_fz. The value is not used by the current motion calculation.
- tickMotion increments its vector index when a valid incoming request invokes the callback. There is no explicit duplicate/stale IPOC execution policy.
- RsiTxFrame has shouldStop, but RsiProtocol::encode does not serialize it. Current stopStreaming only ends local offset playback; that is not a complete controller-level job abort.
- RsiProtocol::decode validates XML/IPOC and optional RIst/AIPos/MACur fields. Its Response retains optional RIst, but the current reply callback receives only IPOC. Do not assume all robot telemetry is already exposed to the controller or QML.
- A bound RSI UDP socket is not evidence that a controller motion session is running or that any correction was accepted.

## 5. Frames, geometry and calibration

### 5.1 Pose conventions

Use B for robot BASE, H for the hole frame, F for the sensor's factory frame, T for calibrated TOOL/TCP, ER for the collet editor frame, and i for the local chamfer construction frame. A transform B_T maps T coordinates into B. Its rotation columns are the TCP axes expressed in BASE; its translation is the TCP origin in BASE.

Internal numerical positions are millimetres. Public XYZABC angles are degrees and use:

    R(A,B,C) = Rz(A) * Ry(B) * Rx(C)

Trigonometric path parameters are radians. Use the existing numerical helpers and explicitly initialized Eigen transforms. Do not subtract complete homogeneous matrices to obtain robot corrections.

SceneEndEffectors already composes:

    FLANGE_TCP = FLANGE_ER * ER_TCP

The robot's selected $TOOL must correspond to that full flange-relative TCP, not ER_TCP alone. Match the robot's active $BASE to the frame in which imported geometry and the prepared trajectory are expressed. Do not assume a tool number or base number; these controller settings have not been provided.

The nominal path is already evaluated in BASE. H is useful geometry notation, but a second runtime hole-frame convention is unnecessary for the first implementation. The recommended correction-coordinate orientation is BASE. This must actually be configured in POSCORR when using IPO_FAST; an intention in C++ is not controller configuration.

### 5.2 What the existing trajectory represents

ChamferPath validates the plane-cylinder intersection ellipse and cylinder axis. Chamfer size is the setback along the end plane in each longitudinal radial/axis section. The angle is measured from the outward hole axis to the section diagonal. The nominal TCP origin is the midpoint of that diagonal, not the original edge point. There is no separate implemented machining-allowance parameter.

The local frame has X = radial direction cross outward hole axis, Y along the section diagonal toward the end plane, and Z = X cross Y. The tool frame is:

    B_T = B_i * Rx(180 degrees)
    e_x^T = e_x^i
    e_y^T = -e_y^i
    e_z^T = -e_z^i

The burr axis is TCP Y. TCP Z is the selected cylindrical-burr surface normal used by this strategy. ChamferPathSample::radialNormal is the hole's transverse radial direction; it is **not** a substitute for TCP Z.

The user's latest instruction defines positive TCP Z as the retreat direction. Preserve that sign. When commissioning, confirm that the actual calibrated axis and generated geometry produce the intended physical separation; a software equation alone cannot verify mounting. Do not use the older +e_z^i statement to change the final requirement.

Geometric progress is not time or arc length. For Machining, theta runs through a full 2*pi revolution. Keep the final full-turn endpoint even if its geometric pose equals the starting pose; the continuous robot joint winding need not be identical.

The existing leads follow quarter-circle laws in normalized angular/axial coordinates. With phase progress u in [0,1] and phi = pi*u/2:

    Lead-in:
      theta = -spanIn*cos(phi)
      clear = 1 - sin(phi)
    Lead-out:
      theta = 2*pi + spanOut*sin(phi)
      clear = 1 - cos(phi)

Axial displacement is leadClearance*clear. Orientation interpolates between the machining and clear attitudes using that same clear amount. At the clear endpoint, TCP Y is opposite the outward hole axis and TCP Z is radially outward. Reuse evaluatePhase; do not replace these leads with linear ramps.

P_s is the ellipse centre plus stagingDistance times the outward hole axis. stagingPose(LeadIn) and stagingPose(LeadOut) use that same point with the corresponding clear-lead attitude. The poses can therefore have identical XYZ and different ABC.

flipAxis reverses the selected outward hole axis and traversal. New edge inputs currently default the outward axis toward negative scene X for the XP/XC workpiece; saved paths retain their own setting. Preserve the saved value rather than reapplying a UI default when preparing a job. The local section X axis need not be tangent to a tilted ellipse; neither local X nor radialNormal should be substituted for the selected TCP-Z normal.

### 5.3 Numerical values already available in the project

Do not ask the user to supply the existing robot joint-position limits. The authoritative numerical model is [resources/json/kr10.json](D:/Qt/QtProjects/RoboCrap/resources/json/kr10.json), loaded by the robot model code. At this audit its limits, in degrees, are:

| Joint | Minimum | Maximum |
| --- | --- | --- |
| A1 | -170 | 170 |
| A2 | -185 | 65 |
| A3 | -137 | 163 |
| A4 | -185 | 185 |
| A5 | -120 | 120 |
| A6 | -350 | 350 |

The same model's qHome is [0, -90, 90, 0, 0, 0] degrees. These are project model values; keep the separate real-controller HOME/configuration verification described in section 6. Read the model at preparation time rather than duplicating these numbers as new constants.

Current numerical defaults are chamferSize = 0.5 mm, chamferAngleDegrees = 45, stagingDistance = 10 mm, each lead clearance = 5 mm, each lead span = 30 degrees and each lead seed interval count = 16. Default lead-in/machining/lead-out feeds are 10/5/10 mm/s. Default Cartesian acceleration is 20 mm/s^2; simulation joint speed and acceleration are 30 deg/s and 60 deg/s^2 per joint. The geometric sampling defaults are 0.01 mm position and 0.1 degree orientation error tolerances.

Those are existing defaults, not the required values of every job and not new commissioning recommendations. A saved path's parameters and timing take precedence. Seed intervals and adaptive display/geometric samples do not determine the 4 ms execution sample count. Do not ask for a new feed when the selected path already supplies it, and do not confuse simulation limits with missing force-control or RSI correction limits.

## 6. Complete robot workflow

The user's five logical parts map to seven existing preview phases:

| Order | Existing phase | Execution owner | Force behavior |
| --- | --- | --- | --- |
| 1 | Approach: HOME to P_s,in | KRL PTP | Disabled |
| 2 | TransferIn: P_s,in to clear lead-in start | KRL LIN, fixed attitude | Disabled |
| 3a | LeadIn | PC trajectory through RSI_MOVECORR | r = 0 |
| 3b | Machining | PC nominal trajectory plus normal retreat through RSI_MOVECORR | Accumulate retreat on excess force |
| 3c | LeadOut | PC trajectory plus carried/blended displacement through RSI_MOVECORR | No new force-feedback update |
| 4 | TransferOut: clear lead-out end to P_s,out | KRL LIN, fixed attitude, after normal completion only | Disabled |
| 5 | ReturnHome: P_s,out to HOME | KRL PTP, after normal completion only | Disabled |

The original question allowed PTP/LIN transfers. The current trajectory design uses straight fixed-attitude Cartesian transfers and stops at transfer/lead corners; retain LIN for the initial strategy. Avoid blending the KRL/RSI handoff corners. Do not stop at every geometric sample or split the three central phases into unrelated start/stop sessions.

KRL LIN feed settings must convert application mm/s to the controller API's required units; for $VEL.CP this is m/s. The preview's HOME percentage scales simulation joint limits and is not automatically a calibrated real-controller PTP velocity setting.

FRAME records contain XYZABC only. They do not encode a verified robot configuration/winding for PTP. Before hardware execution, choose validated controller positions/configuration data, such as suitable E6POS S/T information or taught/validated axis targets. Do not assume that a preview IK solution or PTP of an arbitrary FRAME forces the same real robot branch. The preview HOME is not automatically the robot's taught HOME.

## 7. Nominal motion preparation and time base

### 7.1 Recommended first implementation

Prepare an immutable numerical job from the saved ChamferMotion, outside ControlIO's receive callback. Preserve its calibrated robot/TCP snapshot, saved settings and compatibility identity. A new selection or draft edit must not replace an executing job. Prevent effective calibration/configuration changes while a job is armed or executing, or explicitly invalidate/abort that session; never rebase its accumulated corrections onto a new TCP or BASE silently.

ChamferMotion::boundaries has eight indices for seven phases. The central RSI interval is:

    tStart = points[boundaries[2]].time   // start of LeadIn
    tEnd   = points[boundaries[5]].time   // end of LeadOut

Use ChamferMotion::evaluate(tStart + t) as the saved timing reference. Its TCP is obtained from interpolated joints through FK; it is not an exact analytic ChamferPath evaluation at every intermediate time. Under the subsequently confirmed Cartesian sampling decision, prepared targets come from ChamferPath::evaluatePhase instead. Recover phase-local progress by projecting the timed reference position onto the analytic curve within its saved parameter interval. This preserves timestamps and centralTimeScale while using the mathematical lead/machining geometry and analytic orientation. Check agreement with the reference, monotone progress, continuous IK/FK, positions, sampled rates and interpolation error. Reject a failed check rather than silently changing feeds or retiming the job. No display polyline defines the targets.

Boundary detail verified in evaluate(): exact internal junction times are labelled with the following phase because its phase search uses time >= nextBoundaryTime. Therefore evaluate(tEnd) returns the correct final lead-out pose but labels it TransferOut. Do not discard that endpoint or begin streaming the KRL transfer because of the label. The prepared central job must own its explicit phase/time intervals and terminal endpoint semantics. Its machining interval starts at points[boundaries[3]].time and ends at points[boundaries[4]].time, before subtracting tStart to obtain job-relative times.

If a 4 ms interval crosses a phase boundary, account for the boundary inside the numerical update; do not apply a whole cycle of force accumulation to a non-machining portion. The duration eligible for a force-driven update is the overlap of that cycle interval with the Machining interval. Carry the resulting state continuously into the next phase while still returning one target/reply per controller cycle. This is not permission to insert extra network cycles or duplicate the shared geometric junction.

ChamferMotion is a simulation, not a KRL interpolator or a hardware-safety certificate. It contains simulation joint speed/acceleration limits and can apply a uniform centralTimeScale. Record that scale and the effective duration/feed in the prepared job; do not claim to execute the raw requested feed if the saved motion has been slowed. Retaining the existing time scale is a numerical limit decision, not the forbidden force-dependent feed reduction.

An analytic path-specific timing compiler can replace this sampling approach later if required, but must preserve the selected geometry, nominal feeds, accelerations, phase junctions and calibrated poses. Do not change preview behavior as an incidental consequence of introducing hardware preparation.

### 7.2 Sampling rules

Use one explicit cycle period in job preparation and execution: initially dt = 0.004 s. Define the initial pose at t = 0, then target poses at t_k = min(k*dt, duration). Include the endpoint once and hold it after completion. For a duration not divisible by dt, the last target is the exact endpoint, reached on the final controller cycle.

The first relative increment is from the validated start pose to the first target, not an absolute position measured from BASE origin. While waiting for readiness/start authorization, send zero relative increments and do not advance phase time. Keep the start pose and phase ownership at shared junctions explicit.

Use controller request cycles to advance execution, not a GUI timer or host wall-clock catch-up. A delayed PC callback must not skip ahead to a distant pose or transmit several increments as a burst to make up time. Validity, ordering, session identity and duplicate handling are part of the execution gate.

The nominal schedule continues during force relief. This means its path parameter continues; it does not guarantee that the total speed of the corrected TCP remains exactly the nominal feed. Changing the normal and retreat adds motion, so bound/check the combined command separately.

## 8. Normal-retreat model: definitive sign convention

Let p_nom(s), R_nom(s) be the nominal TCP pose, expressed in BASE. Define:

    n(s) = R_nom(s) * [0, 0, 1]^T        // positive TCP Z in BASE
    r >= 0                              // accumulated retreat, millimetres
    p_cmd(s) = p_nom(s) + r*n(s)
    R_cmd(s) = R_nom(s)

Only position changes in the initial force strategy. The normal follows the current nominal TCP orientation; it is not one constant BASE vector saved at job start.

To retain the user's earlier phrase "d can only decrease", use d = -r:

    d <= 0
    p_cmd(s) = p_nom(s) - d*n(s)

Never combine d <= 0 with p_nom + d*n; that would reverse the agreed physical direction. A retreat r = 0.1 mm means a displacement of +0.1 mm along TCP Z. Prefer r in implementation because its magnitude is positive and directly describes retreat.

During Machining, r starts at zero, never decreases, increases when the selected force measure exceeds its threshold, and is retained after the force falls. There is no attractor returning the tool to the nominal surface during that phase. "Equidistant" here means a displacement of magnitude r along the prescribed pointwise normal; while r changes it is not a single constant-distance curve. For tilted geometry, this construction is not a proof of shortest-distance offset from a swept surface.

The nominal pass is intended to produce the finished chamfer. A pass that retreats can leave additional material; preserving the nominal finish after retreat has not been established. Do not add an automatic finishing pass or return-to-nominal motion to compensate: neither was requested, and either would change the agreed one-direction strategy.

### 8.1 Convert complete corrected targets into relative increments

For BASE-oriented Cartesian translation, the required increment is:

    delta_p[k] = p_cmd[k] - p_cmd[k-1]
               = (p_nom[k] - p_nom[k-1])
                 + (r[k]*n[k] - r[k-1]*n[k-1])

Do not send r*n as an increment every cycle: RSI relative mode would accumulate it repeatedly. Do not use only delta_r*n either: when r is held constant and n rotates, the term r*(n[k]-n[k-1]) is still required to remain on the displaced path.

Store the previous commanded target as execution state. Do not replace it with the latest measured RIst and send target-minus-measured-position as a feedforward increment; that would create a different feedback controller and can repeatedly integrate tracking lag.

### 8.2 Force-to-retreat law: proposed starting point, not a tuned decision

The user has agreed on retreat behavior, not a particular force scalar, gain or numerical threshold. Keep force measurement and the retreat update separate. One candidate is a bounded velocity law:

    excess = max(0, F_control - F_limit)
    v_retreat_target = min(v_retreat_max, K_retreat*excess)
    r_next = r + v_retreat*dt

Here force is in N, K_retreat is in mm/(N*s), velocity is in mm/s, and r is in mm. v_retreat is a suitably bounded/smoothed realization of the target. No gain, cutoff, threshold, retreat limit or derivative limit has been agreed numerically.

Use filter/threshold behavior that avoids persistent noise-driven retreat: a one-sided accumulator can drift outward on repeated positive noise excursions. Hysteresis or a dead zone can be evaluated, but do not silently introduce a reverse correction or feed pause. Reaching a configured retreat/command limit while overload persists must be an explicit fault/abort policy, not an unbounded continued command.

The requirements "smooth motion" and "hold r below threshold" need a defined discrete transition: rate/acceleration smoothing may create a short decay of retreat velocity. Specify and report that behavior during tuning rather than claiming instantaneous zero velocity and bounded acceleration simultaneously.

### 8.3 Lead-out transition

At the Machining-to-LeadOut boundary, preserve the full displacement vector and commanded pose. Disable new force-driven accumulation, but do not set r to zero at that boundary.

A recommended construction is to continue the offset using the lead-out TCP normal, initially at the retained amplitude, and then multiply its amplitude by a smooth blend to zero after a defined clearance condition:

    p_cmd,out(u) = p_nom,out(u) + r_end*b(u)*n_out(u)
    b(0) = 1
    b = 1 until the clearance condition is satisfied
    b = 0 at the completed clearing transition

The clearance condition and blend interval are not yet specified numerically. They must account for the burr envelope, workpiece, allowed retreat and changing orientation; being somewhere in LeadOut alone does not prove clearance. If a proposed blend cannot finish within the existing lead-out while satisfying motion bounds, preparation must report that conflict rather than snapping the offset to zero.

Match position continuously and design velocity/acceleration transitions at the phase boundary. If the incoming retreat velocity is nonzero, a frozen scalar alone does not establish derivative continuity; use a bounded transition from that state without new force-feedback decisions in LeadOut. A quintic smoothstep can shape a zero-derivative blend interval, but does not by itself solve a nonzero incoming derivative.

Finish the displacement removal before normal KRL TransferOut begins, so the actual end target matches the validated auxiliary path. Abort bypasses this normal lead-out procedure entirely.

## 9. FTS configuration, force interpretation and filtering

### 9.1 Fixed transformation at the sensor

The agreed ATI Tool Transformation is fixed for a particular TCP relative to the sensor. Align both the reported origin and its axes with TCP. Recompute/re-enter it only when that relationship changes. Robot motion does not require rewriting those six values every cycle.

This sensor-to-TCP transformation is not the robot's flange-to-TCP $TOOL transformation; their source frames differ. Do not copy KUKA XYZABC directly into ATI Dx/Dy/Dz/Rx/Ry/Rz. Derive the intended rigid transform and use ATI's documented rotation/order/units convention when setting it.

If the sensor reports the wrench in TCP axes, Fz is the signed component along TCP Z. That identifies a coordinate component, not yet the agreed overload measure. Whether F_control uses a signed compressive projection, an absolute value, or another force combination still requires the experiment/interpretation step. Do not silently choose norm(F), abs(Fz), or a force sign just because m_fz exists.

Sensor TCP axes follow the actual tool, while n(s) above belongs to the nominal commanded attitude. If a later controller projects force into BASE or uses measured pose to account for tracking error, align pose and force timestamps and document which attitude performs the transformation. Do not combine components from differently oriented frames as if they were expressed in the same basis.

Changing the point at which a wrench is reported changes its moment reference; rotating the frame changes resolved components. Neither operation removes tool weight, inertia or spindle vibration. The user is awaiting mass/centre-of-mass/inertia data from Schunk. Keep gravity/dynamic compensation explicit and separate from coordinate transformation and filtering. A single static bias does not compensate changing gravity direction through an oriented path.

### 9.2 Configuration needed, without collecting all HTML

A saved response from http://<FTS-IP>/netftapi2.xml gives the active configuration by default. The user may supply it; do not fetch it from live hardware without authorization. Relevant fields are:

| Meaning | Net F/T XML fields |
| --- | --- |
| Active configuration/calibration | setcfgsel, cfgnam, cfgcalsel |
| Force/torque units and scales | scfgfu, scfgtu, cfgcpf, cfgcpt |
| Tool Transformation and its units | cfgtfx, scfgtdu, scfgtau |
| Existing low-pass filter selection | setuserfilter |
| RDT output rate and internal rate | comrdtrate, runrate |
| Bias/status context | setbias, runstat |

The existing C++ constants of 1,000,000 counts per unit and 7,000 Hz are assumptions, not readings of those settings. Counts per force and counts per torque are separate metadata. Convert units consistently; the proposed controller uses N for force and mm for displacement. If torque is used later, declare its units explicitly.

Do not ask for all the sensor's HTML files. If an export has already been provided in a later task, inspect and reuse it before requesting another. The one-time Tool Transformation approach is settled; the actual six values may remain external configuration until needed for validation. Never claim that the transformation has already been installed merely because the user described how they will set it.

ATI exposes built-in low-pass selections, including 73, 35 and 18 Hz, among others. Those are available settings, not recommendations already selected for this job. Inspect the active selection before adding software filtering.

### 9.3 Current receive path and proposed processing

FtsDevice parses the configured peer's RDT records and emits dataSampleHFReady before batching. publishState's 0.05-unit deadband affects the displayed values only. processBatch retains the latest sample of each approximately 16 ms batch for recording/display; it is not a control-rate low-pass filter. The recording cap is 7,500 samples. Do not use those decimated records to claim that high-frequency spindle noise has been characterized without accounting for their sampling limitations.

The existing FTS start command is 0x0002 (real-time RDT streaming), with the 0x1234 request header, an 8-byte request and 36-byte records using network byte order. It is not the ATI 0x0003 buffered-stream mode. Preserve the verified protocol or explicitly redesign both producer/consumer handling; do not infer sensor output filtering from the application's batch container.

Recommended control pipeline:

    RDT packet from configured peer
      -> framing, sequence, status and freshness validation
      -> configured counts/unit conversion
      -> explicit bias/gravity compensation policy, when available
      -> causal low-pass filtering
      -> selected force measure in the required frame
      -> latest valid processed sample on ControlIO
      -> retreat update on the RSI cycle

A configurable first-order software filter was suggested:

    filtered[k] = filtered[k-1] + alpha*(input[k] - filtered[k-1])
    alpha = 1 - exp(-2*pi*cutoffHz*sampleDt)

Initialize from the first valid sample instead of a fictitious zero-force transient. Reset/re-prime on a new stream or calibration change. Filter continuously through the leads even though force-driven motion changes occur only during Machining. Keep raw/scaled and processed values distinguishable in diagnostics. Consider the existing ATI filter and resulting total delay; a second filter is not automatically beneficial.

Use the sensor time base for sampleDt, not the GUI batch period or arbitrary intervals between callbacks draining queued packets. rdt_sequence counts output records; ft_sequence advances at the internal sensor rate and continues across stream requests. Handle wrap/reset/order correctly. The current timestamp expression rdt_sequence_difference/7000 is valid only when that output rate actually applies. Use verified rate/sequence semantics and retain a separate host monotonic receive timestamp for freshness.

Status faults, saturation, invalid configuration, out-of-order data and stale data must not disappear behind the filter. Keep them as validity/fault state. The existing 300 ms stream timeout is a panel/stream behavior, not an agreed force-control freshness limit. Define a control-specific age policy before enabling force control. A bound UDP socket is not a fresh force sample.

The user agreed that recording with the spindle rotating without cutting is useful. Record a suitable baseline and response data to choose force measure, threshold, cutoff and gains. Designing that experiment is distinct from permission to run it. No default numerical force threshold can be derived solely from the sensor model's rated range.

## 10. RSI encoding and controller configuration

### 10.1 Relative mode is cumulative

Use RSI_ON(#RELATIVE,#IPO_FAST) for the initial controller design. The PC must supply incremental corrections, not a sequence of absolute BASE poses. RSI_MOVECORR supplies the whole central motion through those corrections; it is not a KRL LIN nominal path with only a small force offset superposed.

The Ethernet XML only declares exchanged values. It does not define the complete RSI Visual signal graph, POSCORR reference frame, correction limits, stop action or completion feedback. No strict job-specific .src or .rsi graph currently exists; they must be designed and verified as a matched pair.

The user expressly allowed adding signals for diagnostics, debugging and control coordination. DEF_AIPos, DEF_RIst and DEF_MACur were supplied for screen output; RIst may later support correction/diagnostic logic. Preserve that intent and do not remove those fields just because the present callback ignores some of them. Ordinary matching packet extensions do not require asking again whether extensions are allowed.

For IPO_FAST, select the correction-coordinate orientation in POSCORR. The recommended first implementation uses BASE-oriented correction axes located at TCP. Do not treat this as a transform about the BASE origin. For IPO mode, the manual instead specifies the reference coordinate system through RSI_ON.

The proposed 12 ms alternative must be researched separately: the RSI 3.3 manual restricts IPO path correction to LIN/CIRC. Do not merely change dt from 0.004 to 0.012 and assume the same purely sensor-guided RSI_MOVECORR architecture remains supported. Keep 4 ms as the implemented target until a supported 12 ms method is established.

### 10.2 Orientation needs an explicit, verified adapter

Nominal orientation changes along this path. Six numbers called XYZABC are not a generic six-dimensional Euclidean vector, and relative Cartesian orientation is not safely inferred from the word RELATIVE.

Keep internal target orientations as rotation matrices/quaternions. The user has explicitly settled the software contract: each A/B/C correction adds directly to the corresponding starting TCP Euler angle. Use the project's BASE TCP Z-Y-X, degree-based Euler convention. This user-confirmed mapping is the implementation contract; hardware behavior still requires commissioning evidence.

Two operations that must not be confused are:

    Desired BASE-expressed rotation between target attitudes:
      delta_R = R_cmd[k] * transpose(R_cmd[k-1])

    Difference between two sets of absolute Euler coordinates:
      [A,B,C]_cmd[k] - [A,B,C]_cmd[k-1]

They are not generally equivalent. Nor is converting delta_R to ABC automatically correct for a controller that accumulates the components of a correction frame.

On 2026-10-07 the user explicitly confirmed "Add directly to the starting TCP angles." The starting-attitude mapping is settled; do not reopen it or implement a separate correction-frame application. The exact contract is TCP_ABC[k] = TCP_ABC_start + sum(delta_ABC[1..k]), component by component. Convert each target BASE TCP matrix to a continuous unwrapped absolute A/B/C representation, then transmit target_ABC - starting_ABC - previously_encoded_offset. The sample-zero starting coordinates are retained in the prepared encoding. Future execution must match the controller's starting Euler branch, allowing equivalent individual 360-degree windings; a matching rotation matrix alone cannot establish that the same increments apply to an alternate Euler family. No incremental rotation-matrix composition or selectable alternative is included.

The acceptance evidence must include reconstruction of the intended orientation using the same controller rule, a full revolution, branch crossings and small motions from a nonzero initial ABC attitude. Explicitly handle Euler singularities and winding; wrapping every orientation independently into [-180,180] can cause discontinuous corrections. Keep this adapter isolated so force logic does not depend on Euler details.

Use full internal precision. Serialize finite numbers with a C locale and enough precision for the accumulated path. If wire precision requires quantization, preserve/reconcile residual error in the encoded cumulative state. Do not independently round each ideal increment and forget what was actually encoded.

Milestone 4 implements nominal six-channel encoding in rsiposeencoder.h/.cpp using the direct-addition contract. It retains the initial XYZABC, computes increments against the accumulated serialized values, stores max_digits10 attribute strings and independently reconstructs every nominal target from those strings within the existing KR10 tolerances. A local stable atan2 decomposition unwraps the two equivalent Euler families against the preceding encoded attitude; at B=+/-90 degrees it preserves the coupled A/C value with the nearest gauge. Per-coordinate changes of 90 degrees or more are rejected as unresolved continuity, not accepted as controller limits. Prepared jobs retain sampled XYZ/ABC increment/offset bounds and physical rotation magnitudes/travel. These envelopes are numerical requirements, not installed monitoring configuration or hardware evidence.

The user subsequently clarified MaxTrans and MaxRotAngle as maximum allowable changes in X/Y/Z and Euler A/B/C respectively. The implemented software checks use component-wise absolute cumulative changes relative to the starting TCP: each XYZ component must stay within +/-MaxTrans and each unwrapped ABC component within +/-MaxRotAngle. These are neither Euclidean norms nor principal rotation angles. POSCORR uses its explicit cumulative lower/upper XYZ bounds and MaxRotAngle for each ABC component. The profile also specifies separate application per-cycle and cumulative XYZABC intervals. These mappings are user-confirmed software contracts; installed firmware ranges/settings and observed controller behavior remain commissioning facts.

rsicorrectionlimits.h/.cpp reads schema-1 rsi-correction-limits.json beside the executable, limited to 64 KiB. Copy resources/files/RSI_CorrectionLimits.example.json there and supply a profile identity and all numerical bounds; its null entries are intentionally invalid, not production defaults. Every six-value array is ordered X/Y/Z/A/B/C, using mm and degrees. Application intervals and POSCORR XYZ bounds must be finite, ordered and include zero; scalar maxima must be finite and nonnegative. Preparation snapshots the content revision and validates the actual serialized increments and their independent sums. It reports the first violating sample/channel/value/bounds and preserves nominal samples if configuration is unavailable or rejected. encodingReady is true only after numerical encoding and all nominal checks pass. GUI file/directory watches and direct before/after-worker reads invalidate changed profiles; future arming must recheck the profile against the installed controller. No settings are installed, auto-expanded or inferred from manual defaults, and force-retreat envelopes are deferred until retreat exists.

### 10.3 Existing Ethernet configuration

The user supplied this configuration. It is a reference to preserve while designing explicit extensions, not evidence of a complete configured controller:

```xml
<ROOT>
  <CONFIG>
    <IP_NUMBER>192.168.1.100</IP_NUMBER>
    <PORT>5555</PORT>
    <SENTYPE>ImFree</SENTYPE>
    <ONLYSEND>FALSE</ONLYSEND>
  </CONFIG>
  <SEND>
    <ELEMENTS>
      <ELEMENT TAG="DEF_AIPos" TYPE="DOUBLE" INDX="INTERNAL" />
      <ELEMENT TAG="DEF_RIst" TYPE="DOUBLE" INDX="INTERNAL" />
      <ELEMENT TAG="DEF_MACur" TYPE="DOUBLE" INDX="INTERNAL" />
    </ELEMENTS>
  </SEND>
  <RECEIVE>
    <ELEMENTS>
      <ELEMENT TAG="RKorr.X" TYPE="DOUBLE" INDX="1" HOLDON="1" />
      <ELEMENT TAG="RKorr.Y" TYPE="DOUBLE" INDX="2" HOLDON="1" />
      <ELEMENT TAG="RKorr.Z" TYPE="DOUBLE" INDX="3" HOLDON="1" />
      <ELEMENT TAG="RKorr.A" TYPE="DOUBLE" INDX="4" HOLDON="1" />
      <ELEMENT TAG="RKorr.B" TYPE="DOUBLE" INDX="5" HOLDON="1" />
      <ELEMENT TAG="RKorr.C" TYPE="DOUBLE" INDX="6" HOLDON="1" />
      <ELEMENT TAG="Stop" TYPE="BOOL" INDX="7" HOLDON="1" />
    </ELEMENTS>
  </RECEIVE>
</ROOT>
```

Treat the address and port as the supplied configuration, not addresses to probe automatically. The socket must bind the matching local endpoint and validate the configured controller peer. Preserve ImFree and case-sensitive element/attribute spelling across XML and C++.

HOLDON=1 retains the last valid ETHERNET output for late data. With relative corrections, retaining a nonzero increment can repeat motion. The user's observed setup stops immediately when its reply is missing, but that observation is not a general proof that no stale output can be reused. The manual describes a configured limit on late replies. A recommended change is reset-on-late behavior for motion increments (HOLDON=0), combined with an explicit controller timeout/abort policy; verify the actual context before adopting it. Stop/reason latching requires its own design and must not be accidentally cleared along with motion values.

The manual describes default individual correction limits of +/-5 mm or degrees and overall limits of +/-6 mm or degrees. A complete hole traversal and orientation evolution can exceed small-correction defaults even when every per-cycle increment is small. The offline profile now checks cumulative object/overall component bounds under the user-confirmed contract above; the matching graph must install those settings and respect the supported firmware ranges. Calculate the required nominal-plus-retreat envelope when force retreat is implemented. Do not set limits to arbitrary huge values or assume small increments make accumulated correction limits irrelevant.

The XML Stop field must actually be encoded in PC replies and wired to an action in the RSI graph. A desired reply shape, once that connection is implemented, is:

```xml
<Sen Type="ImFree">
  <RKorr X="0" Y="0" Z="0" A="0" B="0" C="0" />
  <Stop>0</Stop>
  <IPOC>123456</IPOC>
</Sen>
```

The IPOC shown is illustrative. Echo the exact request's IPOC, never an invented PC timestamp. Verify the BOOL representation against the configured controller. The existing encoder currently omits Stop.

### 10.4 Packet ordering and runtime obligations

Use a session state machine and enforce these invariants:

- Only a well-formed packet from the configured peer can reach the motion callback. Preserve address validation and the learned peer port; reset peer/session state on a deliberate reconnect.
- One new valid controller cycle advances the job once. Duplicate requests must not advance the path or retreat twice. If retransmitting a duplicate reply, reuse the cached reply rather than recomputing another increment.
- Stale/out-of-order requests must not advance motion. Define the relationship between IPOC values and cycle order from the controller, rather than assuming a difference of exactly one if IPOC uses another unit.
- Do not replay a backlog of motion increments after a gap. Treat unexpected gaps/session resets as a defined execution fault, with no automatic continuation from an uncertain position.
- A successful writeDatagram means the OS accepted a datagram, not that the robot applied it. Maintain that distinction in diagnostics and completion handling.
- While armed/waiting, return zero corrections without advancing the trajectory. After a final target, maintain zero increments while completing the controller handshake.
- On an application abort, latch the request and cease ordinary trajectory advancement. Do not close the socket before attempting the configured stop exchange while communication remains available. Lost communication requires the controller's own timeout response.
- Keep the callback bounded: no file access, CAD work, full IK compilation, synchronous UI calls, modal dialogs or large log writes. Both FTS and RSI callbacks share ControlIO; draining an unbounded FTS backlog must not starve RSI deadlines.

No host-side trick makes the chosen Windows network stack deterministic. Record deadline, gap and sample-age evidence and make failures explicit.

## 11. Start, normal completion and Stop are different events

Do not use one Boolean to mean both "nominal trajectory completed" and "abort requested". They have different subsequent KRL behavior.

**Start:** the job is prepared and armed on the PC; KRL has installed matching TOOL/BASE/job data and reached the clear lead-in start without blending. Establish a matched session/start handshake before consuming the first nonzero increment. Network traffic may begin after RSI_ON but before RSI_MOVECORR is ready; receiving an arbitrary first packet alone must not consume the trajectory prematurely. The exact ready/job/start signals are a controller integration detail still to be designed.

**Normal completion:** reach the exact clear lead-out target with the carried retreat removed, finish the planned velocity transition, send zero increments, and obtain a distinguishable controller-visible completion condition. Only then may KRL finish RSI_MOVECORR, switch RSI off and continue TransferOut and ReturnHome. Do not combine the last nonzero movement and a termination edge without verifying whether that edge can interrupt the final increment.

**Stop/abort:** request a controller motion stop, latch the abnormal result and prevent further automatic movement in this job. Stop is not a software replacement for the enabling switch, Start button or emergency-stop function; it is an application command delivered through RSI. It cannot guarantee instantaneous physical standstill or delivery if Ethernet has failed.

The recommended RSI graph has separate normal-completion and abort semantics, even if both ultimately use a STOP object configured to terminate RSI_MOVECORR. A possible extension is a separate Done value plus a latched result/reason accessible to KRL, along with ready/start feedback. The user allowed diagnostic/protocol extensions, but their exact element names, indices and signal mapping have not been finalized. Specify XML, RSI graph, C++ structs and KRL consumers together; do not write fictitious signal connections.

RSI's STOP object acts on a positive edge. ExitMoveCorr can terminate purely sensor-guided motion. Reset its input/state deliberately at a new session so a previous latched TRUE cannot suppress the next edge. Capture the terminal reason before destroying the context. A communication fault or ambiguous termination is not normal completion.

RSI_MOVECORR(#RSIBRAKE) avoids the explicit return-to-stop-signal-point behavior of #RSIBRAKERET, but it does not by itself prevent later KRL movement statements. KRL must branch on the terminal result and return from the job on abort. HALT alone is insufficient if pressing Start afterward would continue into a LIN or PTP return.

Stop concerns robot movement here. Automatic spindle shutdown policy has not been agreed. The spindle is operated through the existing PLC RUN MOT output; do not silently add, remove or invert PLC output writes while implementing RSI Stop.

## 12. KRL control-flow template

This is a **design skeleton, not a deployable .src program**. Symbolic positions and helper calls below are not existing KRL APIs. Their data, signal mappings, declarations and result checks must be supplied and reviewed for the installed controller. Only the named RSI functions are the documented RSI command interface.

```text
DEF HOLE_EDGE_JOB()
   DECL INT Ret
   DECL INT CleanupRet
   DECL INT ContextId
   DECL BOOL NormalEnd
   DECL BOOL CleanupOk
   ; Declare/populate validated HOME, PS_IN, PIN, PS_OUT.
   ; Use controller position/configuration types appropriate to each motion.

   NormalEnd = FALSE
   CleanupOk = FALSE
   ContextId = 0

   ; Initialize the robot program according to the existing cell conventions.
   ; Install validated $TOOL, $BASE, load data and motion settings.
   ; Complete file reading/job checks before the central RSI interval.
   ; Verify that PC and KRL refer to the same prepared job and cycle mode.
   IF NOT JOB_DATA_VALID() THEN
      RETURN
   ENDIF

   PTP HOME
   PTP PS_IN
   ; Set the validated transfer-in LIN speed/acceleration.
   LIN PIN
   ; Exact stop at PIN; establish readiness, without consuming path samples.

   Ret = RSI_CREATE("HoleEdge", ContextId, TRUE)
   IF Ret <> RSIOK THEN
      RETURN
   ENDIF

   Ret = RSI_ON(#RELATIVE, #IPO_FAST)
   IF Ret <> RSIOK THEN
      CleanupRet = RSI_DELETE(ContextId)
      ; Record/handle a cleanup failure; no movement follows this branch.
      RETURN
   ENDIF

   ; Context/PC handshake allows nonzero corrections only at the defined start.
   RSI_MOVECORR(#RSIBRAKE)

   ; Symbolic helper: read a controller-visible, latched terminal reason.
   ; FALSE includes Stop, fault, or missing/ambiguous completion.
   NormalEnd = READ_LATCHED_NORMAL_COMPLETION()

   Ret = RSI_OFF()
   CleanupOk = (Ret == RSIOK)
   CleanupRet = RSI_DELETE(ContextId)
   CleanupOk = CleanupOk AND (CleanupRet == RSIOK)

   IF (NOT NormalEnd) OR (NOT CleanupOk) THEN
      ; Record the terminal reason. No automatic retreat or HOME movement.
      RETURN
   ENDIF

   ; At the nominal clear lead-out end, r is zero and motion is settled.
   ; Set the validated transfer-out LIN speed/acceleration.
   LIN PS_OUT
   PTP HOME
END
```

The ordinary program flow above does not cover every controller exception or operator interruption. Complete cleanup/restart behavior for those paths as part of the KRL implementation. Preserve the result before calling RSI_OFF/RSI_DELETE; do not infer successful completion merely because RSI_MOVECORR returned. Ensure advance-run behavior cannot pre-plan a normal-return motion across an unresolved terminal decision. Review any CONTINUE usage rather than inserting it to hide synchronization problems.

### 12.1 Existing file mechanism to preserve later

The user has a functioning shared folder mounted as /RoboCrapJobs, backed by //172.31.1.47/RoboCrapJobs, and KRL helpers under R1/Program/FILE_UTILS. READ_FRAME uses krl_fscanf with six semicolon-separated reals:

    X;Y;Z;A;B;C

It constructs a FRAME and reports #DATA_OK through the existing KRL helper's Ok output. CLOSE_FILES uses krl_fclose_all. The C++ result-return style rules do not prohibit output arguments required by KRL APIs.

When file integration is requested, load/validate all auxiliary data before motion and close handles on every path. Check mount/open/read/close results, all six values, record count and job/calibration identity. Keep the points and PC central path from the same preparation snapshot; use an atomic completed-file publication rather than reading a partially written job. Do not perform shared-folder I/O in a 4 ms loop. Existing CWRITE file functions are synchronous and affect advance-run behavior.

The original example contains share credentials. They are deliberately not copied into this document; reuse the user's existing controller configuration rather than placing credentials into generated source or repository documentation.

## 13. Recommended software organization

These are responsibility boundaries, not mandatory names or permission to create a large framework. Reuse suitable existing helpers and add only what is needed for the current milestone.

**Prepared job:** an immutable numerical value containing source/path identity, compatibility/calibration identity, timing/cycle mode, initial/final poses, phase data, nominal samples or evaluator, auxiliary poses and validated execution settings. It can retain a shared immutable numerical motion snapshot. It must not retain scene/viewport QObjects for use on ControlIO.

**Preparation coordinator:** on the application side, captures a compatible saved path, prepares copied numerical data off the GUI thread when necessary, rejects stale results, and hands the complete job to the device through a queued typed boundary. Keep the current SceneMachining preview responsibilities intact; use main.cpp/application composition to connect layers without creating a Scene/Devices dependency cycle. QML selects/arms a job; it does not pass thousands of poses or perform control math.

**Nominal evaluator:** supplies the requested TCP pose, current phase, time/progress and normal for one controller cycle. It has no socket, QML or OCCT dependency. Offline work should leave the runtime evaluator bounded and inexpensive.

**Force processor:** owns verified scaling/frame metadata, validity, sample time, filtering and the selected force measure. It receives FTS samples on ControlIO and exposes a cohesive latest-sample value. Filter state and raw sample state are separate from the trajectory's accumulated retreat.

**Normal retreat state:** owns r, retreat velocity if used, phase-transition state and configured bounds. It takes nominal phase/pose plus a processed force sample and returns a correction/result value. It does not own UDP communication or alter the saved nominal path.

**RSI pose encoder:** turns consecutive complete commanded targets into controller-specific relative correction values, preserving encoded cumulative state, orientation conventions and quantization residuals. It is separate from XML serialization.

**Execution session in the device layer:** owns the immutable prepared job and mutable session state: start gate, cycle index, previous command, IPOC ordering/cache, force snapshot, retreat state, terminal reason and abort latch. Only its I/O thread mutates that state. It coordinates validation, evaluation, correction, encoding and replies.

**RsiProtocol:** remains responsible for XML/framing/peer validation and typed wire data, not scene access, kinematics, force gain selection or job ownership. It needs explicit Stop/completion/feedback fields once the controller contract is defined.

A useful proposed session lifecycle is Unprepared -> Prepared -> Armed -> WaitingForStart -> Executing -> Finishing -> Completed. AbortRequested/Faulted are distinct terminal paths. Exact enum names are optional; the behavioral distinction is not. Rearming creates a fresh execution state deliberately and does not automatically resume a partial path.

Publish concise value state to the GUI through the existing stateReady/DeviceRunner route. Report at least job identity, phase, cycle/progress, requested/effective timing, current retreat, force validity/age, terminal result and error. Avoid publishing every sensor sample through QML or blocking ControlIO with large log payloads.

## 14. Implementation sequence and observable acceptance

Implementation may begin with the settled numerical work while controller-specific details remain explicitly unresolved. Do not claim hardware readiness until those details have evidence. At each stage, update the implementation ExecPlan and inspect the diff. Do not add project tests or run builds without the authorization required by AGENTS.md.

### Stage A: Capture the current trajectory as a prepared nominal job

Connect preparation to a selected compatible SceneMachiningPath. Preserve the saved settings/model/TCP and extract only the three central phases. Retain auxiliary staging and clear-end poses as job data without implementing file publication yet. Show preparation success/failure and effective timing through existing application patterns.

Acceptance: the prepared first/last poses agree with the selected path's lead endpoints; HOME and transfers are excluded from RSI data; changing a GUI draft does not mutate a prepared job; calibration mismatch prevents arming; no device motion is started by Generate Path or Dry Run.

### Stage B: Implement nominal sampling and controller encoding as separate numerical steps

Use the configured cycle period, analytic phase-aware Cartesian targets and full internal precision. Nominal sampling retains the saved effective time map, explicit phase boundaries and each cycle's overlap with Machining; validation points are not additional controller cycles. Finish the documented POSCORR orientation adapter before enabling orientation commands. Remove reliance on the unrelated six-dimensional RsiPath distance calculation for this feature.

Acceptance: reconstruction through the chosen controller correction rule follows the prepared poses, reaches the final target without drift, handles the full turn and Euler branch cases, and produces bounded finite increments. Sampling is independent of viewport refresh rate. Joint-position limits remain enabled in preview; do not weaken IK constraints to make a problematic path appear valid.

### Stage C: Integrate protocol/session control and the matching RSI/KRL design

Define the exact start, normal-completion and abort handshake; extend C++ wire types, XML and RSI graph together. Serialize Stop, gate new cycles, handle duplicate/order/gap behavior, and preserve terminal reasons. Replace the demonstration action only within the authorized UI/API scope and update all string-dispatched call sites affected by changes.

Acceptance: a duplicate valid request does not consume another target; invalid peers/XML do not advance the job; armed waiting produces zero increments; missing/ambiguous terminal information cannot enter the KRL return path; application Stop cannot trigger preview-style HOME motion. A final target is not discarded by early termination. Validation starts with offline reasoning/captured data or approved existing local facilities; live controller checks need separate authorization.

### Stage D: Add force processing with correction disabled

Obtain the active Net F/T settings, implement verified scaling/time/validity and the proposed configurable filter, and expose the processed force for diagnosis. Preserve the current recording contract unless this stage explicitly requests its extension. Any high-rate diagnostic capture must have bounded storage and no synchronous receive-callback writes.

Acceptance: input units and output force units are known; a changed output rate does not silently corrupt time; malformed/stale/faulted data remains invalid; filter initialization is defined; UI batching does not become the force-control sampling clock. Nominal-only execution must be identifiable as such rather than pretending force feedback is active.

### Stage E: Add bounded one-direction retreat and the lead-out transition

Implement p_cmd = p_nom + r*n, monotone r during Machining, complete-target differencing, and a continuous carried displacement into LeadOut. Use explicit configurable parameters with no invented production settings. Define the force measure/sign, threshold, normal velocity/acceleration bounds, maximum retreat, freshness limit, sustained-overload response and clearance/blend criterion before activating this mode on equipment.

Acceptance scenarios to review using suitable offline inputs or authorized diagnostics:

- With correction disabled or no threshold exceedance, output matches the nominal job.
- Above-threshold force produces positive TCP-Z retreat and continued nominal progress.
- Below-threshold force retains the accumulated retreat during Machining.
- With constant r and changing orientation, the displaced target rotates with n; the increment includes the changing-normal term.
- The Machining-to-LeadOut pose is continuous, and the displacement becomes zero only through the specified clearance transition.
- A limit, invalid force state or explicit Stop cannot lead to automatic ReturnHome.
- Restarting a new job does not inherit old r, cached IPOC replies or a stale stop edge.

These are required behaviors to demonstrate, not an instruction to add a test suite. When a build is explicitly requested, follow the installed kit and AGENTS.md verification instructions; a successful backend build alone does not validate application QML/viewport integration or hardware behavior.

## 15. Remaining facts and decisions: do not invent them

No additional conceptual direction question is needed. The following table allocates the remaining work; it is **not a questionnaire to send back to the user**. Follow section 2.1 before asking for anything.

| Detail | Who resolves it and when | What can proceed first |
| --- | --- | --- |
| New RSI graph, BASE-oriented POSCORR and controller rotation/limit semantics | Implementer designs the graph and researches installed/documented behavior; verify before rotational hardware execution. No existing job-specific graph was promised. | Immutable job extraction, nominal sampling, translation/normal math and an isolated orientation adapter interface. |
| Start gate, job identity, Done versus Stop, terminal-reason mapping | Implementer designs one consistent XML/C++/RSI/KRL contract under the user's permission to extend signals. | Session state, data types and abort/completion separation; no need to ask the user to invent packet names. |
| Controller TOOL/BASE slots and values, load data, taught HOME and PTP configuration | Read available configuration; only unavailable controller-specific values require later user/equipment input before a deployable job. | Prepare numerical poses using the saved model/TCP; keep controller slot IDs configurable. |
| Actual Net F/T active settings | Read a supplied netftapi2.xml export; request that one export only when the dependent sensor stage needs it and it is still unavailable. | Typed configuration/scaling/filter code and nominal-only preparation. |
| Force measure/sign and bias/gravity policy | Implementer analyzes frame conventions and available recordings, then documents a proposed measure; verify physical response during authorized tuning. | Separate ForceProcessor interface and correction-disabled mode. |
| Threshold, cutoff, gain, retreat velocity/acceleration and displacement bounds | These are intentionally unset commissioning parameters. Design validated configuration, and determine values from evidence before activation. | Parameter validation, force/retreat equations and offline numerical preparation without claiming a tuned controller. |
| Freshness/gap limits and sustained-overload response | Implementer designs explicit bounded failure behavior; confirm numerical limits from communication/control evidence. | Sequence handling, monotonic timestamps, validity state and abort latching. |
| Lead-out clearance and smooth transition | Implementer evaluates the existing geometry and proposes a bounded blend; request only genuinely missing cutter/mount dimensions if required for clearance. | Preserve the boundary displacement and implement the transition representation; do not invent a clearance proof. |
| Mass/centre-of-mass/inertia data | Awaited from Schunk; do not repeatedly ask for data the user already said they do not have. | All nominal work and a clearly identified uncompensated processing path; no fabricated compensation. |
| Optional 12 ms method | Implementer researches only if that future alternative becomes relevant. | Complete the chosen 4 ms implementation without opening another mode-selection question. |

The current lack of RSI correction/retreat limits was explicitly stated by the user. It means values still have to be engineered for activation, not that the user forgot to answer and not that unlimited retreat is an approved setting. In the same way, unavailable tool inertia does not turn this task into a demand to obtain vendor data before any coding can start.

## 16. Source material and traceability

The primary source for current behavior is the code linked above. AGENTS.md is a useful map and binding work instruction, but implementation claims should be checked against calls, constructors, signal connections and CMake source ownership. Local documents can be stale: for example, joint-position checks are currently enabled through IgnorePreviewJointPositionLimits = false in src/3d/robot/model/kr10kinematicmodel.h. They are simulation checks, not an RSI force-displacement limit.

The following user-provided material was read. Its agreed decisions are incorporated above, so another model need not rely on the chat or have the images open to recover the sign convention:

- [clarifications.txt](C:/Users/vano8/Desktop/clarifications.txt): the user's original numbered answers. Later conversation decisions about +e_z^T and retaining/removing retreat supersede conflicting earlier wording.
- [chamfer_trajectory_math_model_circle_leads.md](C:/Users/vano8/Desktop/chamfer_trajectory_math_model_circle_leads.md): detailed nominal geometry/lead model. Use current source when checking implementation details.
- [EE.png](C:/Users/vano8/Desktop/EE.png): sensor, spindle/clamp and cylindrical burr arrangement.
- [axis.png](C:/Users/vano8/Desktop/axis.png): hole/local/TCP axes. Current implementation has e_z^T = -e_z^i; the final requested retreat is +e_z^T.

Manual references below are to the supplied local editions. Page numbers identify the relevant material, not proof that the user's installed configuration already implements it:

- [KUKA RobotSensorInterface 3.3, KSS 8.3 and 8.4](<D:/Qt/QtProjects/manuals/KUKA/KUKA RobotSensorInterface 3.3. KSS 8.3 and 8.4.pdf>): pp. 12 and 15 for cycles/deadlines; pp. 16-20 for correction coordinates, relative/absolute mode and correction method; p. 23 for correction limits; p. 25 for runtime requirements; pp. 40-42 for RSI_CREATE/ON/OFF/DELETE/MOVECORR; p. 50 for HOLDON and IPOC; p. 77 for STOP/ExitMoveCorr and POSCORR. Object-specific orientation semantics must be checked further in the installed documentation/context.
- [KR C4 KSS 8.3 Operating and Programming Instructions for SI](<D:/Qt/QtProjects/manuals/KUKA/KR C4. KUKA KSS 8.3. Operating and Programming Instructions for SI [ENG].pdf>): T1 operating behavior and the PTP/LIN programming sections; the reviewed edition discusses T1 speed monitoring around p. 31 and PTP/LIN around pp. 277-278. T1 speed limits do not select the machining feed or replace synchronization.
- [KUKA Programming CREAD-CWRITE](<D:/Qt/QtProjects/manuals/KUKA/KUKA. Programming CREAD-CWRITE.pdf>): CWRITE/advance-run behavior and section 6.16 file functions, including mount/open/close/fscanf, approximately pp. 34-44. File preparation remains outside cyclic RSI execution.
- [ATI Net F/T Manual](<D:/Qt/QtProjects/manuals/Schunk/ATI. Net FT. Manual.pdf>), document 9620-05-NET FT-23: B-45-47 for units, scales and Tool Transformation; B-50 for RDT rate; B-69 for filter selections; B-74-76 for netftapi2.xml; B-78-80 for real-time versus buffered RDT, sequences/status/scaling; filter response/delay figures near B-113-114.
- [ATI F/T Transducer Installation and Operation Manual](<D:/Qt/QtProjects/manuals/Schunk/ATI. FT Transducer. Installation and Operation Manual.pdf>): Delta-specific specifications and overload/load interpretation. Sensor calibrated ranges describe the transducer and are not machining-force thresholds.

## 17. Handoff completion status

This document defines the agreed strategy, verified current integration points, proposed implementation sequence and unresolved controller/tuning details. No C++, QML, CMake, robot program, RSI configuration, shared-folder job or live equipment setting was changed while preparing it. No build or hardware motion was performed.

Under the subsequent authorized Stage A and Milestones 3-4 / Stage B tasks, C++/QML/CMake implement PreparedChamferJob, SceneMachiningPreparation, queued RSI storage, the workspace Prepare action and nominal analytic Cartesian sampling at 4 ms. The immutable job retains validated samples, phase/junction poses, machining-time overlap and saved effective timing; the workspace reports sample count. Milestone 4 implements XYZABC serialization/reconstruction, sampled envelopes and explicit nominal correction-limit configuration/comparison under the user-confirmed direct TCP-angle addition and component-wise limit contracts. GUI/device encoding readiness and reason remain separate from sampling success. Engineering numerical profile values, matching installed controller settings/ranges and execution evidence remain pending. MACHINING_RSI_EXECPLAN.md records implementation and source-review evidence. No build, runtime or hardware verification has been performed. Commissioned execution and force retreat remain later work. Preserve the distinction between an offline prepared nominal job and commissioned robot execution. Update factual claims when implementation changes them; do not erase the final direction or Stop decisions when resolving lower-level details.

## 18. Complete answer record: do not re-ask these questions

This table accounts for every numbered answer in clarifications.txt. Later changes are identified explicitly. It records task facts, not permission to contact hardware.

| Original answer | Recorded fact or decision | Where it is applied |
| --- | --- | --- |
| 1 | No strict KRL program or job-specific .src/.rsi exists; design them. | Sections 10-12; do not request an existing context as a prerequisite. |
| 2 | Prefer #RELATIVE; begin at 4 ms. Consider 12 ms only later if necessary. | Sections 7 and 10, including the IPO-mode compatibility issue. |
| 3 | Current TCP trajectory is BASE-relative; BASE is the first implementation convention. H is geometry notation, not a required additional runtime frame. | Section 5.1. |
| 4 | No job-specific translational/rotational RSI limits have been configured. Existing preview joint limits are separate. | Sections 5.3, 10.3 and 15. |
| 5 | User originally compared Stop to the enabling switch. Later clarification settles an application motion abort with no automatic return. | Section 11; no claim of safety-function equivalence. |
| 6 | User observes an immediate controller error/stop when the matching IPOC response is missing. | Section 10.3 preserves the observation while distinguishing the manual's configurable late-packet behavior. |
| 7 | Additional signals are allowed. AIPos, RIst and MACur serve screen diagnostics; RIst may later support correction logic. | Sections 10.1 and 11. |
| 8 | KR10 R1420, KR C4, KSS 8.3.31, RSI 3.3. | Section 1; do not ask the robot/controller version again. |
| 9 | Nominal frame/path geometry is already described in the supplied math document and implemented in the project. Inspect both rather than asking the user to explain them again. | Sections 4-7 and 16. |
| 10 | TCP is defined on the burr surface. The app edits it relative to ER, then composes the flange-relative pose required for $TOOL. | Sections 5.1 and 9.1. |
| 11 | End-effector layout is supplied in EE.png; FTS is Schunk Delta IP68 SI-660-60. | Sections 2, 9 and 16. |
| 12 | Cutter is a cylindrical burr. | Sections 2 and 5.2. |
| 13 | Workpiece is steel; nominal chamfer is 0.5 x 45 degrees; roughness is not a current requirement. | Section 2. |
| 14 | The nominal pass is intended to produce the finished chamfer. | Sections 2 and 8; no automatic additional finishing pass is authorized. |
| 15 | Auxiliary motion concept is accepted; auxiliary approach/return origins may coincide. Their attitudes can differ. | Sections 5.2 and 6. The skeleton explicitly distinguishes PS_IN/PS_OUT staging poses from PIN, the clear lead-in endpoint. |
| 16 | Existing IK joint-position limits are in kr10.json and should be read from the project. | Section 5.3 lists the current values and source. |
| 17 | QML-selected trajectory feeds must govern RSI motion with the 4 ms time base. | Section 7; use the saved values and report any existing numerical time scale. |
| 18 | Sensor manuals are already supplied under D:/Qt/QtProjects/manuals/Schunk; the Delta model is known. | Sections 9 and 16; no need to request manuals again. |
| 19 | Configure ATI Tool Transformation to align the reported wrench frame with TCP. Later clarification: calculate once per TCP and save it through the sensor configuration interface. | Section 9.1; do not ask for a new transform every robot cycle. |
| 20 | Complete end-effector dynamics data has not arrived; Schunk centre-of-mass/inertia information is awaited. | Sections 9.1 and 15; preserve the known-unavailable status. |
| 21 | A spindle-rotating, non-cutting recording is a useful baseline. | Section 9.3; execution still requires hardware authorization. |
| 22 | Initially described the normal as e_z^i. **Superseded:** the final explicit instruction is retreat along positive e_z^T. | Section 8 is authoritative; do not reopen the sign choice. |
| 23 | Force-driven decisions apply only during Machining. Later accepted exception: retain displacement through Lead-out and remove it after clearance. | Sections 6 and 8.3; carrying displacement is not new lead-out force regulation. |
| 24 | Do not reduce tangential feed as force rises in the initial strategy. | Sections 1, 7 and 8. |
| 25 | Continue tangential motion while retreating; do not pause at a point and relieve force there. | Sections 1 and 8; supersedes the initial pause-and-relieve idea. |
| 26 | No retreat distance or relief-duration limits have yet been chosen. | Sections 8.2 and 15; values are unknown, not unlimited by design. |
| 27 | Same continuous-motion decision as answer 25. | Section 8. |
| 28 | Auxiliary points use the working shared folder; cyclic corrections use the existing PC RSI request/reply socket; T1 is the current scope. | Sections 6, 10.4 and 12.1; no new cyclic shared-file transport. |
| 29 | No recovery strategy yet, just stop. Existing T1 operator/controller stop mechanisms remain in place. | Section 11; no automated escape, return or resume after an abort. |
| 30 | Spindle runs from a PLC output already available in the panel, labelled RUN MOT in the current code. | Sections 4.2 and 11; preserve the existing mechanism. |
| 31 | Keep cyclic control in Windows Qt; devices share an Ethernet switch. The presumed C++ filter was checked and is not a filter on the RSI force path. | Sections 2, 9.3 and 10.4. |
| 32 | Import surface JSON -> intersect -> select edge -> configure TCP -> generate path -> prepare auxiliary points and central RSI motion. | Section 4.3; deferred file integration must not replace this workflow. |
| 33 | The principal control objective is a correctly formulated tool-retraction algorithm. | Section 8; retain the nominal/force/encoder separation. |

Additional decisions from the follow-up conversation are fully incorporated:

- Accumulated retreat is retained when force falls; with r >= 0 it can only increase during Machining. The equivalent signed d = -r can only decrease.
- Retraction direction is **positive TCP Z**. The final clarification supersedes earlier signs; there is no outstanding conceptual direction question.
- Carry the displacement into Lead-out, then smoothly remove it after adequate clearance.
- A one-time ATI TCP transformation is appropriate. This establishes the configuration method, not evidence that particular values are already active.
- A configurable first-order filter was proposed, but no cutoff or force-control tuning values were selected. One active Net F/T XML export is sufficient to inspect relevant settings; all HTML files are unnecessary.
- Stop means terminate robot movement for the job; there is no automatic return/recovery sequence. Controller stop wiring still needs implementation.
- Joint-position checks are deliberately enabled to avoid unreasonable animation/PTP constructions. Do not disable them to bypass preparation failures.
- Relative RSI A/B/C values add directly to the starting TCP A/B/C angles, explicitly confirmed by the user on 2026-10-07. This applies to nonzero starting attitudes. Incremental rotation-matrix composition and a separately applied accumulated rotation are excluded. The rotation rule is settled; correction-limit semantics and accepted settings remain separate unresolved facts.

If a future model believes another broad clarification is necessary, it should first identify the exact uncovered fact and why neither this record, current code nor the supplied manuals answers it. Research/design work and commissioning measurements must not be sent back as repeated strategy questions.
