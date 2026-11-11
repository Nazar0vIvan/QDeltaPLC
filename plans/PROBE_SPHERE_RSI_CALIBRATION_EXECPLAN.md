# Probe sphere calibration on the robot: RSI search, KRL program and touch recording

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`,
`Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is
maintained in accordance with `PLANS.md` at the repository root.

Four rules override `PLANS.md` for this plan. First, the implementing agent never runs a build,
CMake configure, QML lint, the application or any test; when a milestone needs a build or a run,
the agent stops, names exactly what the user must build or run, and continues from the user's
reported result. Second, the agent never moves the robot, never changes controller settings and
never starts a KRL program: every controller step is done by the user, and every step that moves
the robot needs the user's explicit authorization for that step (`AGENTS.md`, "Live PLC/FTS/RSI
connections, outputs, sensor bias or motion require explicit authorization"). Third, no git staging
or commits unless the user asks. Fourth, no project tests or test harnesses (`AGENTS.md`). Every
`AGENTS.md` rule applies to every line written for this plan.

This plan builds on `plans/PROBE_SPHERE_TOUCH_PLAN_EXECPLAN.md` (checked in): the calibration and
verification touch plans and their animation already exist; this plan does not change them.


## Purpose / Big Picture

Today the application can only animate the probe calibration: in Measuring mode, Calibration
submode, "Generate Path" builds a motion of 114 calibration and 44 verification touches of the
25 mm calibration sphere, and "Dry Run" plays it. Nothing runs on the robot and no touch is
recorded.

After this plan the user can run the same session on the real robot and obtain the measured
touches. In the Calibration submode a new toolbar action "RSI Sampling" turns the generated path
into a job file for the controller and into one search stream per touch for the PC's RSI partner.
The user starts the new KRL program `RSI_Probe_Calibration` on the controller in T1. The program
moves the robot exactly as the animation shows (PTP, LIN and CIRC moves executed by KRL), and every
approach to the sphere is a search executed under RSI: the PC streams a constant 1 mm/s motion along
the approach direction, and the controller stops the motion the moment the probe's signal
`$IN[8]` switches on. For every touch the PC writes the robot's actual joint angles in the cycle
in which the touch was detected (the RSI value `AIPos`) to a file
`records/rsi_touches_<id>.csv` beside the executable, next to a plan file that names every touch.
These files are the input of the later least-squares calculation of the ball centre, which is not
part of this plan.

The terms used: RSI (KUKA RobotSensorInterface) exchanges one XML packet with the PC every 4 ms
while a KRL program runs `RSI_MOVECORR()`; the PC answers with a position correction. KRL is the
robot's programming language; a program consists of an `.src` file (code) and a `.dat` file (data).
An RSI context is a signal diagram drawn in the controller tool "RSI Visual" and stored as three
files (`.rsi`, `.rsi.xml`, `.rsi.diagram`) on the controller; the repository has no such files and
they must not be hand-written, so this plan specifies the context as a table and a diagram that the
user draws.


## Progress

- [x] (2026-11-11) Plan written after reading `RsiDevice`, `SceneMachiningPreparation`,
  `chamferrsistream`, the KRL programs and contracts in `resources/files/rsi`, `ProbeTouchMotion`,
  `TimedMotion`, `ModeToolBar.qml`, `Main.qml` and `main.cpp`.
- [ ] Milestone 1: `resources/files/rsi/RSI_EthernetConfig.xml` sends `TouchIndex` and `Touch`;
  the user sets `Precision` and runs `RSI_Probe_Lin`; the capture shows the AIPos resolution and the
  new elements; result recorded here.
- [ ] Milestone 2: `RsiDevice` holds one stream per touch index, decodes `TouchIndex`, `Touch` and
  `AIPos`, and writes the touch file; chamfer and test LIN still work; user build and test LIN run
  recorded here.
- [ ] Milestone 3: calibration RSI Sampling: steps and streams from the calibration path,
  `C:/RoboCrapJobs/calibration_data.txt`, plan file, toolbar action; user build and Sampling result
  recorded here.
- [ ] Milestone 4: `resources/files/rsi/calibration/RSI_Probe_Calibration.src`, `.dat` and
  `RSI_Probe_Calibration_CONTRACT.md`; the user draws the RSI context, copies the files and runs a
  limited number of steps; result recorded here.
- [ ] Milestone 5: full session on the robot, touch file checked, documents in `docs/context`
  updated.


## Surprises & Discoveries

- Observation: the ETHERNET1 parameter `Precision` very probably sets the number of decimals of the
  values the controller sends, not of those it receives.
  Evidence: with `Precision` 1 the controller sends AIPos with one decimal (capture
  `records/rsi_packets_test_lin_20261030_110905_072.csv`: `A2="-89.5"`, METHOD document section 3.2),
  while `RSI_Probe_Lin` moves the robot with corrections of 0.004 mm per cycle, which a one-decimal
  rounding of received values would turn into zero; the user confirmed the motion and the stop on
  `$IN[8]` (2026-11-11). The RSI 3.3 manual does not mention `Precision`. Milestone 1 checks it.
- Observation: `RSI_OFF()` and `RSI_ON()` restart the whole RSI context (RSI 3.3 manual, section
  "RSI_ON() / RSI_OFF() response"), so a correction accumulated in one search does not carry into
  the next; the position-correction limits only need to cover one search.


## Decision Log

- Decision: KRL executes every motion of the session except the searches; each search is one
  `RSI_ON` / `RSI_MOVECORR` / `RSI_OFF` cycle.
  Rationale: the RSI STOP object ends `RSI_MOVECORR` at the touch (mode ExitMoveCorr), so one
  continuous RSI stream cannot cover several touches; KRL PTP, LIN and CIRC moves reproduce the
  animation's segments directly and need no 4 ms sampling. Only the search must be under RSI,
  because only RSI stops the motion in the same 4 ms cycle in which `$IN[8]` switches on (verified
  with `RSI_Probe_Lin`, 2026-11-11).
  Date/Author: 2026-11-11, user and agent.

- Decision: the PC selects the search direction from the touch number that KRL writes to
  `$SEN_PINT[3]` before each search; the controller sends it as `TouchIndex`. The PC holds one
  correction stream per touch number; number 0 is the single stream of `RSI_Chamfer_Hole` and
  `RSI_Probe_Lin`, which never set `$SEN_PINT[3]`.
  Rationale: every touch has its own approach direction; one generalised stream table serves all
  three programs without a mode switch in the device.
  Date/Author: 2026-11-11, agent.

- Decision: the controller also sends `Touch` (the value of `$IN[8]`, through the DIGIN object), and
  the PC records, for the first packet of a search in which `Touch` is 1, the IPOC, the number of
  cycles since the search started, the AIPos of that packet and the AIPos of the packet before it.
  Rationale: AIPos of the detection cycle gives the actual joint angles at detection; the previous
  cycle bounds the 4 ms interval in which the input switched; the cycle count gives the travelled
  search distance. The joint angles, not the controller's Cartesian position, are needed because
  the least-squares calculation uses the application's own forward kinematics (METHOD section 3.2).
  Date/Author: 2026-11-11, agent.

- Decision: one `RSI_EthernetConfig.xml` for all programs; the two new SEND elements are added to it,
  and contexts that do not connect them simply leave them unused.
  Rationale: user decision (2026-11-11). The RSI 3.3 manual does not say how a context behaves when
  the configuration has SEND elements whose ETHERNET inputs are not connected, so Milestone 1 runs
  `RSI_Probe_Lin` with the new file before anything depends on it.
  Date/Author: 2026-11-11, user.

- Decision: the search stream of a touch moves 5 mm along the approach direction −n at 1 mm/s
  (1250 cycles of 0.004 mm) followed by 50 zero cycles; KRL treats a search that ends without `$IN[8]`
  as a failed touch, retracts and returns HOME.
  Rationale: the search starts 3 mm outside the nominal contact; the contact position is uncertain
  by up to about 1 mm in the horizontal group G2 (orientation never touched manually, ball-centre
  component along the stylus unknown; conversation 2026-11-11), so 2 mm of travel beyond the nominal
  contact are allowed. The WP-500 stylus tolerates 3 mm axial and 20° lateral overtravel (manual), so
  a stop up to 2 mm late does not damage it. The 50 zero cycles match the existing streams.
  Date/Author: 2026-11-11, agent.

- Decision: the job for KRL is a separate file `C:/RoboCrapJobs/calibration_data.txt` (KRL path
  `/RoboCrapJobs/calibration_data.txt`), version 1, with a list of steps; `SceneMachiningPreparation`
  owns both job files and keeps at most one of them, together with the stream installed in the
  device.
  Rationale: a separate name prevents `RSI_Chamfer_Hole` from reading a calibration job and the
  reverse. One owner of "installed stream plus job file" keeps the existing rule that KRL never finds
  a file for a stream other than the installed one (the test LIN already follows this rule inside
  the same class).
  Date/Author: 2026-11-11, agent.

- Decision: KRL PTP moves of the session run at 20 % axis velocity and acceleration, LIN and CIRC at
  the Generate transfer feed and acceleration (default 10 mm/s and 20 mm/s², written to the job
  file as `$VEL.CP` and `$ACC.CP` in m/s and m/s²), with constant orientation.
  Rationale: the PTP moves turn the probe near the sphere; the animation does not check robot links
  against the stand, so a slow PTP gives the operator time to stop. LIN and CIRC use the same limits
  as the animation, so the session duration matches it.
  Date/Author: 2026-11-11, agent.

- Decision: the DAT holds `StepLimit` (0 means all steps); the first robot runs set it to a small
  number on the smartPAD.
  Rationale: the first runs check reachability, the first touches and the recording without
  running the whole 15-minute session.
  Date/Author: 2026-11-11, agent.


## Outcomes & Retrospective

Nothing implemented yet.


## Context and Orientation

Repository root: `D:\Qt\QtProjects\RoboCrap` (or `E:\...` on the other PC). The application is a
Qt 6.8 Quick program in C++17. Positions are millimetres in the robot base frame {B}, which equals
KUKA WORLD here because the KRL programs set `$BASE=$NULLFRAME` and the controller's `$ROBROOT` is
zero. Orientations are KUKA angles A, B, C in degrees, meaning the rotation Rz(A)·Ry(B)·Rx(C).

Network and signal path. The PC answers RSI on 172.31.1.47:5555 over a direct cable from the KR C4
(172.31.1.147). The WP-500 probe output goes through the Delta PLC (input X2.0 copied to Y1.7) to
EtherCAT input 8, read by KRL and RSI as `$IN[8]`; it is TRUE while the stylus is deflected.

Existing controller files in `resources/files/rsi`:

- `RSI_EthernetConfig.xml`: the ETHERNET configuration shared by all programs. The controller sends
  `DEF_RIst`, `DEF_AIPos`, `DEF_MACur`, `DEF_Delay`, `Start` (BOOL, ETHERNET input 1, from
  `$SEN_PINT[1]`) and `Result` (LONG, input 2, from `$SEN_PINT[2]`); it receives `RKorr.X..C`
  (DOUBLE, outputs 1–6), `Stop` (BOOL, output 7) and `Result` (LONG, output 8).
- `RSI_Chamfer_Hole.src/.dat` and `RSI_Chamfer_Hole_CONTRACT.md`: the machining run. Its contract
  defines the PC behavior that this plan keeps (section "PC behavior"): Start = 0 gives zero
  corrections and resets the stream index; Start = 1 plays one correction per packet; after the last
  correction the PC sends Result 1 until the controller echoes it, then Stop 1 until Start returns
  to 0; Start = 1 without a stream gives Stop 1 and Result 0.
- `FILE_UTILS/REMOUNT_SHARE.src`, `READ_POS.src`, `READ_FRAME.src`: KRL helpers. `REMOUNT_SHARE()`
  mounts the PC share `//192.168.2.100/RoboCrapJobs` as `/RoboCrapJobs`. `READ_POS(File, Ok)` reads
  one line `X;Y;Z;A;B;C;S;T` (S in 0–7, T in 0–63 required); `READ_FRAME(File, Ok)` reads
  `X;Y;Z;A;B;C`.
- `WP-500_test/RSI_Probe_Lin.src/.dat` and `RSI_Probe_Lin_CONTRACT.md`: the stop test. Its RSI
  context (drawn by the user): ETHERNET1 (config file above, Timeout 10, Precision 1); SEN_PINT1
  (index 1) → ETHERNET1.In1; SEN_PINT2 (index 2) → In2; ETHERNET1.Out1..6 → POSCORR1 (RefCorrSys
  World); DIGIN1 (input 8) → OR1.In2; ETHERNET1.Out7 → OR1.In1; OR1 → STOP1 (mode ExitMoveCorr);
  ETHERNET1.Out8 → MAP2SEN_PINT1 (index 2). The stop on `$IN[8]` works (user, 2026-11-11).

Existing PC code:

- `src/devices/include/devices/rsi/rsidevice.h`, `src/devices/rsi/rsidevice.cpp`: `RsiDevice`, the
  UDP partner on thread ControlIO. `decodeRequest` reads exactly one `IPOC`, `Start` (0/1) and
  `Result` from each `<Rob>` packet and ignores other elements. `advance(start, echoedResult)` is the
  state machine (Idle, Streaming, Finished, Stopping) over one `CorrectionStream` (increments and
  diagnostic id). `setCorrections(increments, diagnosticId)` installs a stream. Every accepted packet
  and reply is appended to an in-memory capture written as `records/rsi_packets_<id>.csv` by an
  asynchronous save (`saveTrace`, `writeTrace`). The receive path must not print to the console or
  touch files (contract).
- `src/application/scene/scenemachiningpreparation.h/.cpp`: `SceneMachiningPreparation` (QML
  singleton `App.MachiningPreparation`). `prepare()` samples the selected machining path on a worker
  (`sampleChamferRsiStream`), writes `C:/RoboCrapJobs/shared_data.txt` (`writeSharedData`) and emits
  `correctionsReady(increments, diagnosticId)`, which `src/application/main.cpp` connects (queued)
  to `RsiDevice::setCorrections`. `prepareTestLin()` installs the `RSI_Probe_Lin` stream and removes
  the shared file. `clearPrepared()` removes the file and emits empty corrections.
- `src/trajectories/chamfer/chamferrsistream.cpp`: helpers `principalEulerDegrees(rotation)`
  (Z-Y-X angles on the principal branch), `wireValues` (round to six decimals), and the diagnostic id
  format `yyyyMMdd_HHmmss_zzz` (UTC); `writeSharedData` in `scenemachiningpreparation.cpp` shows how
  Status/Turn (S/T) are obtained: `Kr10Kinematics::configuration(joints, solveFK(joints))`.
- `src/trajectories/include/trajectories/probetouchplan.h`, `probetouchmotion.h` and their sources in
  `src/trajectories/probe/`: `createProbeTouchPlan` returns passes (purpose, group, repetition,
  entry, touches with start and contact poses, normal, tilt τ and roll ψ, exit);
  `ProbeTouchMotion::create(plan, robot)` turns them into steps (an internal `motionSteps` function
  with `ProbeSegment` ToEntry, Descend, Transfer, Approach, Retract, Retreat, ReturnHome) and appends
  each step as one phase of a `TimedMotion` (`src/trajectories/include/trajectories/timedmotion.h`):
  `phases()[i].last` is the index into `points()` of the end of step i, and `points()[k].joints` are
  the joint angles there. ToEntry and ReturnHome are PTP; Transfer is a CIRC on the start shell (the
  sphere of radius contact distance + 3 mm around the sphere centre); Approach is a constant 1 mm/s
  LIN from start to contact; Retract and the other LIN moves use the transfer limits.
- `src/application/scene/scenecalibration.h/.cpp`: `SceneCalibration` (`App.Calibration`) owns the
  calibration path; `path()` returns the `SceneTouchPath` whose `motion` is a
  `std::shared_ptr<const ProbeTouchMotion>`; it emits `availabilityChanged` when the path is
  inserted or removed.
- `src/application/qml/Views/Workspace/ModeToolBar.qml`: the "RSI Sampling" action is visible only
  outside Measuring mode (`visible: !root.measuring`); `src/application/qml/Main.qml` connects it to
  `App.MachiningPreparation.prepare()` and shows the preparation status in the status bar outside
  the Calibration submode.


## Plan of Work

Milestone 1 (controller prerequisites). Edit `resources/files/rsi/RSI_EthernetConfig.xml`: inside
`<SEND><ELEMENTS>` after `Result`, add

    <ELEMENT TAG="TouchIndex" TYPE="LONG" INDX="3" />
    <ELEMENT TAG="Touch" TYPE="BOOL" INDX="4" />

and normalise the indentation of the existing lines (currently mixed tabs). In
`resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md` and
`resources/files/rsi/WP-500_test/RSI_Probe_Lin_CONTRACT.md`, add one sentence each that the
controller now also sends `TouchIndex` and `Touch`, which these programs leave unconnected. Then the
user (with authorization for the test motion) copies the file to the controller, sets ETHERNET1
`Precision` to 4 in the `RSI_Probe_Lin` context in RSI Visual, and runs `RSI_Probe_Lin` until the
probe stops it. The agent decodes the resulting capture (command in `Concrete Steps`) and records:
the number of decimals of AIPos, the presence of `<TouchIndex>` and `<Touch>` in the packets, and
that the robot moved and stopped as before. If AIPos still has one decimal, the plan stops here and
the user and agent look for another source of joint angles; if the run fails with the new elements,
the decision on one shared configuration file is revisited.

Milestone 2 (RSI device). In `rsidevice.h/.cpp`:

Replace the member `CorrectionStream m_corrections` by a stream table: `QVector<QVector<V6d>>
m_streams`, `QString m_diagnosticId`, and the index of the active stream `qsizetype m_activeStream`.
Change the slot to `setCorrections(const QVector<QVector<V6d>>& streams, const QString&
diagnosticId)`; its body keeps the existing buffered-request handling and resets. Extend `Request`
and `decodeRequest`: optional `TouchIndex` (non-negative LONG, default 0), optional `Touch` (0/1,
default false), optional `AIPos` (attributes A1–A6 parsed as doubles; the element is optional and
its absence leaves a flag false). Duplicates of these elements make the packet invalid, as for the
existing fields. `Request` moves from the unnamed namespace of `rsidevice.cpp` into the private
section of `RsiDevice`, because the member `advance` now takes it. In `advance`, which gets the
request instead of two values (`advance(const Request& request)`, one parameter), the transition Idle → Streaming selects `m_streams[touchIndex]`
when that index exists and its stream is not empty, otherwise goes to Stopping with Result 0, exactly
as an empty stream does today. The rest of the state machine is unchanged; Result 1 and Stop follow
the end of the selected stream.

Touch recording: while Streaming, the first request of the run with `Touch` = 1 appends one row to a
new in-memory buffer `m_touches` (`touch_index,ipoc,cycle,A1..A6,prev_A1..prev_A6`), where `cycle`
is the stream index before processing the request and `prev_*` is the AIPos of the previous accepted
request of the same run (empty fields if there was none). A flag prevents more rows until Start
returns to 0. The previous AIPos is kept in a member updated on every accepted request. Rows are
written by the same asynchronous save that writes the capture, as `records/rsi_touches_<id>.csv`
with a header line, and only when the buffer is not empty. `writeTrace` gets the header as a third
parameter so both files share it. `publishState` adds `rsiTouches` (number of recorded rows of the
current id). No console output and no file access are added to the receive path.

In `scenemachiningpreparation.h/.cpp` change the signal to `correctionsReady(const
QVector<QVector<V6d>>& streams, const QString& diagnosticId)`; `finishPreparation` and
`prepareTestLin` emit `{increments}` (one stream at index 0); `clearPrepared` emits an empty table.
In `src/application/main.cpp` register `QVector<QVector<V6d>>` with `qRegisterMetaType` next to the
existing `QVector<V6d>` registration so the queued connection works.

Milestone 3 (calibration RSI Sampling on the PC). Publish the motion steps: move the struct
`MotionStep` and the function `motionSteps` from the unnamed namespace of
`src/trajectories/probe/probetouchmotion.cpp` to the header as `struct ProbeMotionStep {
ProbeMotionPhase phase; M4d target; }` and `QVector<ProbeMotionStep> probeMotionSteps(const
ProbeTouchPlan& plan)`; the builder uses the published function, so there is one definition of the
step order.

Create `src/trajectories/include/trajectories/probersistream.h` and
`src/trajectories/probe/probersistream.cpp` (add the source to `src/trajectories/CMakeLists.txt`
next to `probetouchmotion.cpp`). `sampleProbeCalibrationRsi(const ProbeTouchMotion& motion)`
returns `ProbeCalibrationRsiResult { std::optional<ProbeCalibrationRsi> job; QString error; }`.
`ProbeCalibrationRsi` holds the diagnostic id (same UTC format as the chamfer sampling), the tool
coordinates (flange-to-TCP XYZ and principal Z-Y-X ABC, six decimals), the steps for KRL and the
stream table. It walks `probeMotionSteps(motion.plan())` together with `motion.timed().phases()`
(their counts must be equal, else an error) and, for step i with end point
`points()[phases()[i].last]`:

- ToEntry → kind 1 (PTP) with target pose and S/T from the end point's joints;
- Descend, Retreat, Retract → kind 2 (LIN) with target pose and S/T;
- Transfer → kind 3 (CIRC) with target pose, S/T and an auxiliary point: the start-shell point
  in the direction of the normalised sum of the two arc end directions (from the sphere centre),
  with the target's orientation; the arcs of the plan are shorter than 180°, so the sum is not zero
  (check it, else an error);
- Approach → kind 4 (SEARCH) with the next global touch number t (1, 2, … over all passes) and the
  stream `streams[t]`: target positions p_j = j·0.004·(−n) for j = 1 … 1250 relative to the start,
  increments `wireValues(p_j) − wireValues(p_{j−1})` in X, Y, Z (A, B, C zero), then 50 zero
  increments, so the rounded increments add up exactly to the rounded 5 mm offset;
- ReturnHome → kind 5 (HOME), no pose.

Every exported pose must reproduce the saved TCP of its end point within 0.0001 mm and 0.000001 in
the rotation entries after six-decimal rounding (as `writeSharedData` checks), else an error. The
number of steps must not exceed 600, the array size in the KRL DAT. The function also writes
`records/rsi_calibration_plan_<id>.csv` beside the executable on the worker: one row per touch number
with the 1-based number of its search step in the job file, purpose, group, repetition, touch number
within the pass, τ, ψ, normal, start pose, contact pose (XYZ and principal ABC, nine decimals) and the
S/T of the search start.

In `SceneMachiningPreparation`: the constructor gets `SceneCalibration* calibration` and loses the
unused `QObject* parent` (main constructs it on the stack), so it keeps three parameters. Add
`Q_PROPERTY(bool canPrepareCalibration …)` with `calibrationUnavailableReason()` (shutting down,
preparing, Calibration calculating, no calibration path), `Q_INVOKABLE void prepareCalibration()`
running `sampleProbeCalibrationRsi` on a worker with a copy of the path's motion handle, and in its
completion writing `C:/RoboCrapJobs/calibration_data.txt` (format in `Artifacts and Notes`) with
`QSaveFile`, then emitting the stream table and the id. The held state gets a third kind next to the
sampled chamfer stream and the test LIN; `clearPrepared` removes both job files; a new calibration
preparation removes the chamfer file and the reverse. Connect `SceneCalibration::availabilityChanged`
so that a removed or regenerated calibration path drops the held calibration stream and file.
`summary()` for a held calibration job: "RSI calibration: <touches> touches · <steps> steps".

QML: in `ModeToolBar.qml` the "RSI Sampling" action becomes visible in the Calibration submode too
(`visible: !root.measuring || root.calibration`). In `Main.qml`, `preparationAvailable` and
`onPreparationRequested` choose `App.MachiningPreparation.canPrepareCalibration` /
`prepareCalibration()` while `root.calibrationActive`, and the status bar in the Calibration submode
shows the preparation error or summary and the controller state (`rsiState`, `rsiTouches`) when the
calibration has no error and no playback runs. The button appears only after a submode change, which
is a deliberate navigation, so the layout rule of `AGENTS.md` is kept.

Milestone 4 (controller program and contract). Create in `resources/files/rsi/calibration`:

`RSI_Probe_Calibration.dat` declares `StepLimit` (INT, 0), `StepCount` (INT), arrays `StepKind[600]`,
`StepTouch[600]` (INT), `StepTarget[600]`, `StepAux[600]` (POS), `ToolFrame` (FRAME),
`TransferVelocity`, `TransferAcceleration` (REAL).

`RSI_Probe_Calibration.src`: `INI`; `$ADVANCE=0`; clear `$SEN_PINT[1..3]`; `RSI_RESET()`; `PTP HOME`;
`READ_JOB()` (local function like the chamfer program's, reading the file in `Artifacts and Notes`
with `REMOUNT_SHARE`, `READ_FRAME`, `READ_POS` and `krl_fscanf`, checking version 1, step count 1–600
and every row); `$BASE=$NULLFRAME`, `$TOOL=ToolFrame`, `$IPO_MODE=#BASE`, `$ORI_TYPE=#CONSTANT`,
`$VEL.CP=TransferVelocity`, `$ACC.CP=TransferAcceleration`, `$VEL_AXIS[1..6]=20`,
`$ACC_AXIS[1..6]=20`; `RSI_CREATE("RSI_Probe_Calibration.rsi")`; then a loop over the steps up to
`StepLimit` (when non-zero) or `StepCount` with a `SWITCH StepKind[i]`: 1 `PTP StepTarget[i]`,
2 `LIN StepTarget[i]`, 3 `CIRC StepAux[i], StepTarget[i]`, 4 `SEARCH(StepTouch[i])`, 5 `PTP HOME`.
`SEARCH` is a local function: it returns FALSE without moving if `$IN[8]` is already TRUE (the STOP
object needs a rising edge); sets `$SEN_PINT[3]=Touch`; `RSI_ON(#RELATIVE)`; `$SEN_PINT[1]=1`;
`RSI_MOVECORR()`; `Touched=$IN[8]`; `$SEN_PINT[1]=0`; `RSI_OFF()`; returns `Touched` (FALSE also on
an RSI call failure). When `SEARCH` returns FALSE the loop executes the next step if it is the
retract LIN, then `PTP HOME`, and ends the program. When `StepLimit` ends the loop early, the program
retracts in the same way if the last step was a search and goes `PTP HOME`.

`RSI_Probe_Calibration_CONTRACT.md` in the same folder, written like the two existing contracts:
controller files (the SRC, DAT, the user-drawn context files, the shared Ethernet configuration,
FILE_UTILS), frames, the RSI context (table and the Mermaid diagram in `Artifacts and Notes`),
signals (including `$SEN_PINT[3]`, `TouchIndex`, `Touch`), PC behavior (reference to the chamfer
contract's state machine plus stream selection and touch recording), the job file format, the touch
and plan files, the KRL flow including failure handling, and a "Not yet verified" list.

The user draws the context `RSI_Probe_Calibration` in RSI Visual starting from a copy of
`RSI_Probe_Lin`: add SEN_PINT3 (index 3) → ETHERNET1.In3, connect DIGIN1.Out1 also to ETHERNET1.In4,
set ETHERNET1 `Precision` to the value found in Milestone 1, set POSCORR1 limits to ±10 mm in X, Y, Z
and a small rotation limit (for example 1°). The user copies SRC, DAT and the context to the
controller, generates a path in the application, runs RSI Sampling, and with authorization runs the
program with `StepLimit` set to cover the first pass's entry and three touches (the step numbers are
printed in the plan file), then the whole first pass.

Milestone 5 (full session and documents). With authorization the user runs the whole session. The
agent checks the touch file (command in `Concrete Steps`): 158 rows, touch numbers 1–158 once each,
cycle values near 750 (3 mm at 0.004 mm per cycle) within ±250, AIPos with the decimals of
Milestone 1. Then update `docs/context/AGENT_CONTRACTS.md` (Devices and protocols: stream table,
TouchIndex/Touch/AIPos decoding, touch file; Machining and playback, RSI: calibration sampling and
job file; QML: RSI Sampling in the Calibration submode), `docs/context/ARCHITECTURE.md` (RsiDevice,
SceneMachiningPreparation, the new trajectories file, the controller files) and
`docs/context/APP_DESCRIPTION.md` (Workflow step 1 and the run on the controller).


## Concrete Steps

Line counts before each code milestone (Git Bash, repository root):

    for f in src/devices/rsi/rsidevice.cpp src/devices/include/devices/rsi/rsidevice.h \
             src/application/scene/scenemachiningpreparation.cpp src/application/scene/scenemachiningpreparation.h \
             src/application/main.cpp src/trajectories/probe/probetouchmotion.cpp \
             src/trajectories/include/trajectories/probetouchmotion.h \
             src/application/qml/Views/Workspace/ModeToolBar.qml src/application/qml/Main.qml; do
      echo "$f $(wc -l < $f) $(grep -c '[^[:space:]]' $f)"; done

Decoding a packet capture (Milestone 1; the user reports the file name in `records/` beside the
executable). The sixth column is the packet XML in Base64; print the AIPos and the new elements of
the first received rows:

    python -I -c "import base64,csv,sys,itertools; rows=(x for x in csv.reader(open(sys.argv[1])) if len(x)>5 and x[1]=='rx'); [print(base64.b64decode(x[5]).decode()[:400]) for x in itertools.islice(rows,3)]" <capture.csv>

Expected after Milestone 1: AIPos attributes with four decimals (for example `A2="-89.5123"`) and
elements `<TouchIndex>0</TouchIndex><Touch>0</Touch>`.

Checking the touch file (Milestone 5):

    python -I -c "import csv,sys; r=[x for x in csv.reader(open(sys.argv[1])) if x and x[0][0]!='#'][1:]; t=[int(x[0]) for x in r]; c=[int(x[2]) for x in r]; print(len(r), sorted(t)==list(range(1,159)), min(c), max(c))" <rsi_touches_id.csv>

Expected: `158 True` and cycle values between about 500 and 1000.

Searching for remaining uses of the changed device API after Milestone 2:

    grep -rn "setCorrections\|correctionsReady" src

Every match must use the stream-table signature.


## Validation and Acceptance

Milestone 1: the user reports that `RSI_Probe_Lin` with the new `RSI_EthernetConfig.xml` and
`Precision` 4 moved and stopped as before; the decoded capture shows AIPos with four decimals and the
two new elements.

Milestone 2: after building `robocrap`, "RSI test LIN" on the Network page and a run of
`RSI_Probe_Lin` (authorized) behave as before; the capture is written as before; no
`rsi_touches_*.csv` appears, because `RSI_Probe_Lin` does not connect `Touch`. RSI Sampling of a
machining path still writes `shared_data.txt`. The device's console line on installation reads
"RSI correction streams installed: 1" for the chamfer and test streams.

Milestone 3: in the Calibration submode, after "Generate Path", the toolbar shows "RSI Sampling";
pressing it prints "RSI calibration sampling: 158 touches, <M> steps" and the status bar shows
"RSI calibration: 158 touches · <M> steps"; `C:/RoboCrapJobs/calibration_data.txt` and
`records/rsi_calibration_plan_<id>.csv` exist, `shared_data.txt` does not; leaving Calibration and
sampling a machining path removes `calibration_data.txt`. No robot moves in this milestone.

Milestone 4: the program compiles on the controller (selecting it shows no error); with
`StepLimit` covering three touches the robot reaches the first entry, performs three searches that
stop on the touch, retracts and returns HOME; `records/rsi_touches_<id>.csv` has three rows with touch
numbers 1–3.

Milestone 5: the full session ends at HOME with no failed touch; the touch file passes the check in
`Concrete Steps`.


## Idempotence and Recovery

All repository edits are repeatable. RSI Sampling can be pressed again at any time; it replaces the
held stream and job file. If a robot run stops (failed touch, RSI error, operator stop), the program
must be restarted from the beginning with a fresh selection (T1 program reset); the PC needs no reset,
because Start = 0 resets the device and each search starts from its own stream. If a touch fails
because the contact is farther than 2 mm beyond the nominal position, record it in `Surprises &
Discoveries` and decide with the user whether to lengthen the search or correct the sphere file; do
not change the stream length silently.


## Artifacts and Notes

RSI context `RSI_Probe_Calibration` (to be drawn in RSI Visual; parameter names as in the
`RSI_Probe_Lin` contract, which asks to confirm them in RSI Visual's object properties):

    flowchart LR
      SP1["SEN_PINT1<br/>Index 1<br/>$SEN_PINT[1] = Start"] -->|In1| ETH
      SP2["SEN_PINT2<br/>Index 2<br/>$SEN_PINT[2] = Result"] -->|In2| ETH
      SP3["SEN_PINT3<br/>Index 3<br/>$SEN_PINT[3] = touch number"] -->|In3| ETH
      DI["DIGIN1<br/>input 8, 1 bit<br/>$IN[8] = WP-500 touch"] -->|In4 Touch| ETH
      DI -->|In2| OR1
      ETH["ETHERNET1<br/>RSI_EthernetConfig.xml<br/>Timeout 10, Precision from Milestone 1"]
      ETH -->|"Out1..Out6 RKorr X/Y/Z/A/B/C"| PC1["POSCORR1<br/>RefCorrSys World<br/>X/Y/Z ±10 mm, rotation small"]
      ETH -->|"Out7 Stop"| OR1["OR1<br/>2 inputs"]
      OR1 -->|Out1| ST["STOP1<br/>Mode ExitMoveCorr"]
      ETH -->|"Out8 Result"| MP["MAP2SEN_PINT1<br/>Index 2"]

Job file `C:/RoboCrapJobs/calibration_data.txt`, version 1 (CRLF rows, `;` separators, six-decimal
floats, XYZ mm, ABC principal Z-Y-X degrees):

    1                                   version
    X;Y;Z;A;B;C                         ToolFrame, flange to TCP
    0.010000                            $VEL.CP, m/s
    0.020000                            $ACC.CP, m/s^2
    493                                 step count M, 1..600 (example value)
    then M steps, each a header row "kind;touch" followed by:
      kind 1 PTP, 2 LIN: one row X;Y;Z;A;B;C;S;T (target)
      kind 3 CIRC: two rows, auxiliary point then target, each X;Y;Z;A;B;C;S;T
      kind 4 SEARCH: no further row; touch = global touch number 1..158
      kind 5 HOME: no further row
    touch is 0 for every kind except 4.

Touch file `records/rsi_touches_<id>.csv`:

    # plan_id,<id>
    # cycle: search stream index before the detection packet; AIPos deg; prev_*: preceding packet
    touch_index,ipoc,cycle,A1,A2,A3,A4,A5,A6,prev_A1,prev_A2,prev_A3,prev_A4,prev_A5,prev_A6


## Interfaces and Dependencies

No new library. `RoboCrapDevices` keeps QtNetwork and Geometry; `RoboCrapTrajectories` gets one new
source. At the end of Milestone 2:

    // rsidevice.h
    public slots:
      void setCorrections(const QVector<QVector<V6d>>& streams, const QString& diagnosticId);
    // scenemachiningpreparation.h
    signals:
      void correctionsReady(const QVector<QVector<V6d>>& streams, const QString& diagnosticId);

At the end of Milestone 3:

    // probetouchmotion.h
    struct ProbeMotionStep { ProbeMotionPhase phase; M4d target = M4d::Identity(); };
    QVector<ProbeMotionStep> probeMotionSteps(const ProbeTouchPlan& plan);

    // probersistream.h
    enum class ProbeRsiStepKind { Ptp = 1, Lin = 2, Circ = 3, Search = 4, Home = 5 };
    struct ProbeRsiStep
    {
      ProbeRsiStepKind kind = ProbeRsiStepKind::Ptp;
      int touch = 0; // Global touch number of a Search, 0 otherwise.
      V6d target = V6d::Zero(); // Six-decimal XYZ mm, principal ABC deg.
      V6d auxiliary = V6d::Zero(); // CIRC only.
      int status = 0;
      int turn = 0;
    };
    struct ProbeCalibrationRsi
    {
      QString diagnosticId;
      V6d toolCoordinates = V6d::Zero();
      QVector<ProbeRsiStep> steps;
      QVector<QVector<V6d>> streams; // Index = global touch number; index 0 empty.
      int touchCount = 0;
    };
    struct ProbeCalibrationRsiResult
    {
      std::optional<ProbeCalibrationRsi> job;
      QString error;
    };
    ProbeCalibrationRsiResult sampleProbeCalibrationRsi(const ProbeTouchMotion& motion);

    // scenemachiningpreparation.h
    SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining, SceneCalibration* calibration);
    Q_PROPERTY(bool canPrepareCalibration READ canPrepareCalibration NOTIFY availabilityChanged)
    Q_INVOKABLE void prepareCalibration();

Helpers from `chamferrsistream.cpp` that the new file needs (`principalEulerDegrees`,
`wireValues`) are moved to a shared place only if both files need them unchanged; record the choice
and the reason in `Decision Log` (`AGENTS.md`: reuse before adding).
