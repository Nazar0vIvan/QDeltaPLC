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
`$IN[8]` switches on. The PC's existing packet capture keeps every controller packet, including the
robot's actual joint angles (the RSI value `AIPos`) and the probe signal of that cycle. The script
`python/rsi_touches.py` extracts from the captures, for every touch, the joint angles of the cycle in
which the touch was detected into `records/rsi_touches_<id>.csv` beside the executable, next to a
plan file that names every touch. These files are the input of the later least-squares calculation
of the ball centre, which is not part of this plan.

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
- [x] (2026-11-11) Milestone 1, repository part: `resources/files/rsi/RSI_EthernetConfig.xml` sends
  `TouchIndex` (LONG, INDX 3) and `Touch` (BOOL, INDX 4), indentation normalised to spaces, CRLF
  endings kept; `RSI_Chamfer_Hole_CONTRACT.md` ("Signals") and `RSI_Probe_Check_CONTRACT.md`
  ("Controller files") state that these contexts leave In3 and In4 unconnected.
- [x] (2026-11-11) Milestone 1, controller part: the user copied the configuration, set `Precision` 6
  in the `RSI_Probe_Check` context and ran `RSI_Probe_Check` (capture
  `rsi_packets_test_lin_20261111_115130_773.csv`). Result: 6759 packets, all answered, Delay 0;
  AIPos and RIst with six decimals; 6757 increments of +0.004 mm and no PC Stop, so RSI_MOVECORR
  ended through `$IN[8]`; the unconnected inputs arrive as the texts `not linked:3` and
  `not linked:4`, not as 0 (handled by the decision on TouchIndex decoding). `RSI_Probe_Check_CONTRACT.md`
  records Precision 6 and the run; the chamfer contract notes the `not linked` texts.
- [x] (2026-11-11) Milestone 2, code: `RsiDevice` holds the stream table (`m_streams`,
  `m_diagnosticId`, `m_activeStream`), decodes `TouchIndex` (`not linked:…` as 0), ends a run whose
  touch index changes, and reads corrections with const `at()`; `correctionsReady` carries the
  table, `main.cpp` registers `QVector<QVector<V6d>>` instead of `QVector<V6d>`. Compliance pass and
  line counts in `Artifacts and Notes`, "Milestone 2 implementation record". Not built.
- [x] (2026-11-11) Milestone 2, user check: the user built `robocrap`. RSI Sampling of a machining
  path: "RSI Sampling: 3390 cycles, 13.3598 s movement", plan `rsi_plan_20261111_122853_273.csv`,
  then "RSI correction streams installed: 1", which `finishPreparation` emits only after
  `shared_data.txt` was written. "RSI test LIN": installed 0 (cleared) then 1. `RSI_Probe_Check` run,
  capture `rsi_packets_test_lin_20261111_123231_186.csv`: 3777 `rx` and 3777 `tx` rows, so every
  packet with `<TouchIndex>not linked:3</TouchIndex>` was decoded and answered; Delay 0; 3775
  corrections of +0.004 mm X, no PC Stop, so the run ended through `$IN[8]`; RIst X advanced
  15.01 mm (26.94 mm in Milestone 1 from the same start X 979.589 mm; the stop point depends on
  where the probe met the obstacle, which the capture does not show).
- [x] (2026-11-11) Milestone 3, code: `probeMotionSteps` published in `probetouchmotion.h` and used
  by the builder; `principalEulerDegrees` and `wireValues` moved to
  `src/trajectories/motion/rsicoordinates.h`; `sampleProbeCalibrationRsi` in
  `src/trajectories/probe/probersistream.cpp` builds the steps, the 158 search streams and
  `records/rsi_calibration_plan_<id>.csv`; `SceneMachiningPreparation::prepareCalibration` writes
  `C:/RoboCrapJobs/calibration_data.txt` and installs the table; "RSI Sampling" in the Calibration
  submode. Compliance pass and line counts in `Artifacts and Notes`, "Milestone 3 implementation
  record". Not built.
- [x] (2026-11-11) Milestone 3, user check: the user built `robocrap`, ran Generate Path and RSI
  Sampling in the Calibration submode, then RSI Sampling of a machining path. Console (16:18:25
  local): "RSI correction streams installed: 0", "RSI calibration sampling: 158 touches, 485 steps",
  "RSI correction streams installed: 159"; the machining Sampling at 16:20:08 then logged installed
  0 and 1 and saved `rsi_plan_20261111_132008_053.csv`. Files checked by the agent: `rsi_calibration_plan_20261111_131825_176.csv`
  has 158 rows with touch numbers 1–158 in order, 38 rows in each calibration group, 25 in
  verification G1 and 19 in verification G2, tilts 0°–90°, search steps 3 to 482 (with entry and
  descent before and retract, retreat and HOME after: 485 steps), S/T 2/2 and 2/10;
  `C:/RoboCrapJobs` holds `shared_data.txt` and no `calibration_data.txt`, as the final machining
  Sampling requires. The content of `calibration_data.txt` was not inspected, because that Sampling
  removed it.
- [x] (2026-11-11) Milestone 4, repository part: `resources/files/rsi/Calibration/` holds
  `RSI_Probe_Calibration.src`, `.dat` (CRLF, like the other KRL files) and
  `RSI_Probe_Calibration_CONTRACT.md`; `python/rsi_touches.py` builds the touch file from the
  captures. The folder is `Calibration/` to match the user's `Chamfer/`. The script was checked on a
  synthetic capture in the agent's scratchpad (one detected search with cycle 2 and its preceding
  AIPos, one missed search, `not linked` and Start = 0 packets ignored); nothing was compiled on the
  controller.
- [x] (2026-11-11) Milestone 4, controller part: the user drew the context, copied the files,
  generated the path (6176 points, 894.85 s, transfer clearance 3.0000 mm, strut clearance 3.2588 mm)
  and ran, after a separate RSI Sampling each, `StepLimit` = 10 (plan id `20261111_150029_843`, 3
  captures) and 58 (`20261111_152601_386`, 19 captures). `python/rsi_touches.py`: 3 and 19 touches,
  exit 0, no missed search. Every search was preceded by 3 Start = 0 packets; detection after
  781–846 cycles. The contract's "Verified on the controller" lists what these runs show; the
  measured values are in `Surprises & Discoveries`. The context has large POSCORR1 limits, so the
  POSCORR1 reset across `RSI_OFF`/`RSI_ON` was not tested; with those limits it does not matter
  (decision after Milestone 4). The user verified the renamed `RSI_Probe_Check` on the controller.
- [x] (2026-11-14) Milestone 5, run: full session in AUT EXT started from RoboCrap through the PLC
  (`plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md`, PTP 5 %, transfers 10 mm/s, override 100 %), plan id
  `20261114_170013_165`, PLC log `plc_io_20261114_195852.csv`: STARTED 20:00:41.535, finished with
  `PTP XHOME` at 20:17:22.962 (16 min 41 s). `python/rsi_touches.py`: 158 captures, 158 searches,
  158 touches, touch numbers 1–158 once each, detection 770–863 cycles; largest PC receive gap
  10.9 ms, largest reply latency 0.81 ms. An earlier attempt (`20261114_163247_675`) stopped at
  search 93 with RSITimeout after a 171.6 ms receive stall; the user then set display-off and sleep
  to Never, USB selective suspend to Disabled, cleared the adapter's "Allow the computer to turn off
  this device" and disabled its Energy-Efficient Ethernet (`docs/context/HARDWARE_SETUP.md`).
- [ ] Milestone 5, documents in `docs/context` updated.


## Surprises & Discoveries

- Observation: the ETHERNET1 parameter `Precision` very probably sets the number of decimals of the
  values the controller sends, not of those it receives.
  Evidence: with `Precision` 1 the controller sends AIPos with one decimal (capture
  `records/rsi_packets_test_lin_20261030_110905_072.csv`: `A2="-89.5"`, METHOD document section 3.2),
  while `RSI_Probe_Check` moves the robot with corrections of 0.004 mm per cycle, which a one-decimal
  rounding of received values would turn into zero; the user confirmed the motion and the stop on
  `$IN[8]` (2026-11-11). The RSI 3.3 manual does not mention `Precision`. Confirmed in Milestone 1:
  with `Precision` 6 AIPos and RIst have six decimals and the 0.004 mm corrections move the robot as
  before.
- Observation (Milestone 1): an ETHERNET input that the context leaves unconnected is sent as the
  text `not linked:<input number>`, for example `<TouchIndex>not linked:3</TouchIndex>`.
  Evidence: every packet of `rsi_packets_test_lin_20261111_115130_773.csv`.
- Observation (Milestone 1): the controller sent no Start = 0 packet between `RSI_ON` and
  `$SEN_PINT[1]=1`: the first packet of the run already has Start 1, and only the last 2 of 6759
  packets have Start 0. This confirms the review gap on run starts; KRL's `WAIT SEC 0.012` is
  therefore required, not only a precaution.
- Observation (Milestone 1): IPOC increases by exactly 4 from packet to packet, that is, it counts
  milliseconds of controller time (27032 over the 6759 packets).
- Observation: `RSI_OFF()` and `RSI_ON()` restart the whole RSI context (RSI 3.3 manual, section
  "RSI_ON() / RSI_OFF() response"). That the correction accumulated in POSCORR1 during one search is
  therefore reset before the next is an inference: the manual does not mention POSCORR in that
  section. Milestone 4 tests it (decision on the POSCORR limits).
- Observation (plan review, 2026-11-11): three gaps in the first version of this plan. The device
  starts a new run only after a Start = 0 packet (`advance` in `src/devices/rsi/rsidevice.cpp`), and
  the first version's `SEARCH` could pass from one search to the next without any such packet. The
  capture is saved after every 250 ms without packets, that is after every search, so a touch file
  saved with it would be overwritten or split into about 158 parts. The failure exit went PTP HOME
  from 3 mm above the sphere along a joint path the animation never checks. The decisions dated
  "review" below resolve them.
- Observation (Milestone 4 runs, 2026-11-11): the first measured touches behave as planned.
  Evidence (agent's scratch evaluation with `python/probe_sphere_calibration/kr10.py`, the
  application's nominal KR10 FK, and TOOL_DATA[5] as ball centre; estimates with the uncalibrated
  model, not measurements of accuracy): touches 1–3, measured in both runs 26 minutes apart, differ
  by 0.0052, 0.0045 and 0.0026 mm in ball-centre position; in the 4 ms cycle before detection the
  ball moved 0.0026–0.0051 mm, the 1 mm/s search feed; the detected ball centre lies 0.07–0.33 mm
  beyond the nominal contact along the approach and at most 0.013 mm beside the approach line. That
  distance varies smoothly with direction (largest at roll 300°, smallest at roll 180°), as an offset
  of the rough sphere centre from the ten jogged points would cause. A sphere fitted to the 19 ball
  centres of pass G1 has centre (1054.2699, −332.0397, 454.2133) mm, radius 13.8185 mm (nominal
  contact distance 14 mm), RMS 0.031 mm, maximum 0.080 mm; with one probe orientation this fit cannot
  separate the sphere centre from a TCP error, which is why the plan has three groups.


## Decision Log

- Decision: KRL executes every motion of the session except the searches; each search is one
  `RSI_ON` / `RSI_MOVECORR` / `RSI_OFF` cycle.
  Rationale: the RSI STOP object ends `RSI_MOVECORR` at the touch (mode ExitMoveCorr), so one
  continuous RSI stream cannot cover several touches; KRL PTP, LIN and CIRC moves reproduce the
  animation's segments directly and need no 4 ms sampling. Only the search must be under RSI,
  because only RSI stops the motion in the same 4 ms cycle in which `$IN[8]` switches on (verified
  with `RSI_Probe_Check`, 2026-11-11).
  Date/Author: 2026-11-11, user and agent.

- Decision: the PC selects the search direction from the touch number that KRL writes to
  `$SEN_PINT[3]` before each search; the controller sends it as `TouchIndex`. The PC holds one
  correction stream per touch number; number 0 is the single stream of `RSI_Chamfer_Hole` and
  `RSI_Probe_Check`, whose contexts leave ETHERNET1.In3 unconnected; the controller then sends
  `not linked:3` (Milestone 1), which the device reads as 0 (decision on TouchIndex decoding).
  Rationale: every touch has its own approach direction; one generalised stream table serves all
  three programs without a mode switch in the device.
  Date/Author: 2026-11-11, agent; rationale corrected in review, 2026-11-11.

- Decision (review, 2026-11-11; supersedes recording touches in the device): the controller also
  sends `Touch` (the value of `$IN[8]`, through the DIGIN object). The device does not decode
  `Touch` or `AIPos` and writes no touch file. `python/rsi_touches.py` builds the touch file from the
  packet captures: per run, the first received packet with `Touch` 1 gives the IPOC, the number of
  controller cycles since the run's first packet ((IPOC − first IPOC) / 4), its AIPos and the AIPos
  of the preceding received packet.
  Rationale: the capture already records every accepted request as exact XML, including the queued
  requests the device does not answer, so it contains everything the device-side recording would
  have stored, with no new code in the receive path. The IPOC difference counts controller cycles
  even when the device skipped late packets or a packet was lost (IPOC counts milliseconds,
  Milestone 1). The captures are saved after every search (250 ms without packets) as `_partN` files
  of about 2600 rows each, far below the 64 MiB buffer, so no row is dropped. AIPos of the detection cycle gives the
  actual joint angles at detection; the previous packet bounds the 4 ms interval in which the input
  switched. The joint angles, not the controller's Cartesian position, are needed because the
  least-squares calculation uses the application's own forward kinematics (METHOD section 3.2). Not
  adopted: rewriting a device-side touch file at every save, because concurrent asynchronous saves of
  one file name can commit in the wrong order.
  Date/Author: 2026-11-11, agent; cycle count from IPOC after Milestone 1.

- Decision (Milestone 1, 2026-11-11): `decodeRequest` reads `TouchIndex` as follows: a missing element
  or a text starting with `not linked:` gives index 0; a non-negative decimal integer gives that
  index; any other text, or a second element, makes the packet invalid, as for the existing fields.
  Rationale: the chamfer and test contexts leave In3 unconnected, and the controller then sends
  `not linked:3` (Milestone 1); reading it as 0 keeps their single stream at index 0 without changing
  their contexts. Rejecting other texts keeps the strict decoding of the existing fields.
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 2, 2026-11-11): `RsiDevice::Request` is declared in the public section next to
  `Reply`, not in the private section; `RsiDevice::activeStreamSize()` is added; the published key
  `rsiCorrections` is the size of the active stream.
  Rationale: `decodeRequest` stays in the unnamed namespace of `rsidevice.cpp` and constructs the
  request, as `encodeReply` reads the public `Reply`; a private type would force the decoder into the
  class. The stream size bounded by the table is needed in three places of `advance` and in
  `publishState`, so one const member replaces four copies of the bounds check. No QML reads
  `rsiCorrections`; the active-stream size equals the former value for the single-stream programs.
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 4, 2026-11-11): the controller files are in `resources/files/rsi/Calibration/`
  (not `calibration/`); `python/rsi_touches.py` reads the captures in save order (first file, then `_part2`, `_part3`, …)
  instead of sorting by `pc_ns`, and a missed search is listed without failing the script; the
  contract states that the robot must be jogged clear before a restart and gives `StepLimit` = 10
  and 58 for the first runs.
  Rationale: the user moved the chamfer files to `Chamfer/`. `pc_ns` restarts at 0 when the
  application restarts, while the part numbers keep the save order. A missed search already ends
  the KRL program, and its absence from the touch file is the information. After a failure or
  `StepLimit` the robot stands 3 mm from the sphere, and the program begins with `PTP HOME`; the
  plan file gives touch 3's search at step 9 and touch 19's at step 57.
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 4 review, 2026-11-11): the search step records `SearchStart=$POS_ACT`, then
  ends the program if `$IN[8]` is TRUE, then calls `SEARCH`; a failed search and a `StepLimit` end
  after a search return with `LIN SearchStart`. `READ_JOB` no longer checks touch numbers or the LIN
  after a search; it resets kind and touch to 0 before reading each row. `SEARCH` returns
  `(RSI_OFF()==RSIOK) AND Touched`.
  Rationale: KRL executes value assignments and `IF` conditions in the advance run (KUKA Robot
  Programming 1, "Computer advance run"). System Variables 8.3 lists `$ADVANCE` as 1–5; the SI
  manual mentions `$ADVANCE = 0` only as preventing approximate positioning, and neither states
  whether logic after a motion then waits for its end. The `$IN[8]` check inside `SEARCH` could
  therefore be evaluated before the preceding CIRC ended.
  Reading `$POS_ACT` triggers an advance run stop (System Variables 8.3, `$POS_ACT`), so the check
  sees the probe at the search start. The recorded start is exactly the retract target, so the exit
  no longer depends on the job's next step, and the check of that step goes. A touch number without
  a stream makes the PC answer Stop, a missed search, so range checks add no safety; resetting the
  fields keeps a partial conversion from reusing the previous job's values. `SEARCH` no longer
  checks `$IN[8]` itself. Net: the SRC lost the retract-step loop and the touch checks (about 20
  lines) and gained the `$POS_ACT` record (5 lines).
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 3, 2026-11-11): `principalEulerDegrees` and `wireValues` move unchanged from the
  unnamed namespace of `chamferrsistream.cpp` into the internal header
  `src/trajectories/motion/rsicoordinates.h` (inline functions), used by both samplers.
  Rationale: the calibration sampler needs both unchanged, including the B = ±90° branch of
  `principalEulerDegrees` for group G2 (A 0, B 90, C 0); one definition instead of two copies.
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 3, 2026-11-11): a failure to write `rsi_calibration_plan_<id>.csv` fails the
  calibration Sampling, unlike the chamfer plan file, which is only diagnostic; the search length is
  `SearchDistance` = 5 mm at the plan's `searchFeed` (1250 cycles at 1 mm/s), not a fixed cycle count.
  Rationale: the plan file is the only record that maps a touch number to its pass, group, tilt,
  roll and nominal poses, so a touch file without it cannot be evaluated. Taking the feed from the
  plan keeps the robot's search speed equal to the animated Approach.
  Date/Author: 2026-11-11, agent.

- Decision (Milestone 3, 2026-11-11): `SceneCalibration::path()` becomes public; the file replacement
  of `writeSharedData` becomes `replaceJobFile(path, contents)`, shared with `writeCalibrationData`;
  the two completion slots `finishPreparation` and `finishCalibration` stay separate. In the
  Calibration submode the status bar shows, after the calibration error and the reason Generate is
  unavailable, the preparation error, summary or `calibrationUnavailableReason` and the controller
  state, as in Machining mode.
  Rationale: the preparation needs read access to the saved motion, and friendship is not allowed.
  Both job files need the same atomic replacement and error texts ("Cannot write shared_data.txt:
  …" is unchanged). The completion slots differ in result type, pending input, writer and summary
  data; merging them would need a template or a generic dispatcher, both excluded by `AGENTS.md`.
  Date/Author: 2026-11-11, agent.

- Decision (review, 2026-11-11): a new run starts for every search by two independent means. KRL waits
  `WAIT SEC 0.012` (three 4 ms cycles) between `RSI_ON` and `$SEN_PINT[1]=1`, so the controller sends
  Start = 0 packets that reset the device. The device also ends the current run (as on Start = 0)
  when a Start = 1 request carries a touch index other than the run's active stream index.
  Rationale: without either means, a search stopped by the touch at stream index about 750 leaves the
  device Streaming; if the next search starts within one cycle, the device continues the old stream,
  moving the probe up to 500 × 0.004 = 2 mm along the previous touch's direction. The KRL wait also
  covers a restarted program whose first search has the same touch number as the aborted one; the
  device rule makes a stream impossible to play for another touch even if the wait were too short.
  Touch numbers are unique within a job and the chamfer and test streams always use 0, so the device
  rule never ends a valid run. Milestone 1 showed that without the wait the controller sends no
  Start = 0 packet at all between `RSI_ON` and `$SEN_PINT[1]=1`, so the wait is required, not a
  precaution.
  Date/Author: 2026-11-11, agent; Milestone 1 evidence added 2026-11-11.

- Decision (review, 2026-11-11): after a failed search, and when `StepLimit` ends the loop, the program
  executes the retract LIN if the last executed step was a search and then ends without a further
  motion; the operator moves the robot away in T1. The normal end of the job is its HOME step,
  written as `PTP XHOME` (not the inline form `PTP HOME Vel=100 % DEFAULT`, which sets 100 % axis
  velocity), so it keeps the 20 % of the session.
  Rationale: a PTP HOME from the start shell turns the probe next to the sphere along a joint path
  that the animation never computed; ARCHITECTURE §7 states that robot links are not checked against
  the stand at all. The retract only reverses the search along the approach line.
  Date/Author: 2026-11-11, agent.

- Decision (after Milestone 4, 2026-11-11; supersedes the ±6 mm decision below): POSCORR1 keeps the
  large limits the user set; whether POSCORR1 starts every search from zero is not tested.
  Rationale: user decision. In `#RELATIVE` mode POSCORR1 applies each cycle's increment and compares
  only the summed correction with its limits, so with limits far above the summed search travel a
  sum kept across `RSI_OFF`/`RSI_ON` changes no motion. The limits then bound nothing: one search
  moves at most the 5 mm of its stream, and only the PC's installed streams can move the robot.
  Date/Author: 2026-11-11, user and agent.

- Decision (review, 2026-11-11; superseded above): POSCORR1 limits are ±6 mm in X, Y and Z (one 5 mm
  search plus 1 mm), and Milestone 4 runs the whole first pass.
  Rationale: the limit covers one search; if the accumulated correction were not reset by
  `RSI_OFF`/`RSI_ON`, the world Z components of the first G1 touches (3.0, 2.6, 0 and 0.8 mm for
  τ = 0°, 30°, 90°, 75°) would exceed 6 mm at touch 4, which would then end without touch. A failed
  search is safe at 1 mm/s. With ±10 mm it would appear only at touch 7 (adding 1.5, 2.1 and 2.6 mm
  for τ = 60°, 45°, 30°), and the three-touch run could not show it with either limit.
  Date/Author: 2026-11-11, agent.

- Decision (review, 2026-11-11): Milestone 1 sets `Precision` 6 and accepts AIPos with at least four
  decimals.
  Rationale: one step of the fourth decimal, 0.0001°, moves a point at the sphere, about 1105 mm from
  axis A1, by 0.0019 mm; the standard deviation of the rounding is 0.0019/√12 ≈ 0.0006 mm per joint,
  comparable to the WP-500 repeatability of 0.001 mm (estimate, not a measurement). Six decimals make
  the rounding negligible if the controller sends them.
  Date/Author: 2026-11-11, agent.

- Decision: one `RSI_EthernetConfig.xml` for all programs; the two new SEND elements are added to it,
  and contexts that do not connect them simply leave them unused.
  Rationale: user decision (2026-11-11). The RSI 3.3 manual does not say how a context behaves when
  the configuration has SEND elements whose ETHERNET inputs are not connected, so Milestone 1 runs
  `RSI_Probe_Check` with the new file before anything depends on it.
  Date/Author: 2026-11-11, user.

- Decision: the search stream of a touch moves 5 mm along the approach direction −n at 1 mm/s
  (1250 cycles of 0.004 mm) followed by 50 zero cycles; KRL treats a search that ends without `$IN[8]`
  as a failed touch, retracts and ends (review decision on the failure exit).
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

- Decision: KRL PTP moves of the session, including its final HOME step, run at 20 % axis velocity
  and acceleration, LIN and CIRC at
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
  (DOUBLE, outputs 1–6), `Stop` (BOOL, output 7) and `Result` (LONG, output 8). Since Milestone 1
  it also sends `TouchIndex` (LONG, input 3) and `Touch` (BOOL, input 4); a context that leaves an
  input unconnected sends `not linked:<input number>` in its element.
- `Chamfer/RSI_Chamfer_Hole.src/.dat` and `Chamfer/RSI_Chamfer_Hole_CONTRACT.md`: the machining run. Its contract
  defines the PC behavior that this plan keeps (section "PC behavior"): Start = 0 gives zero
  corrections and resets the stream index; Start = 1 plays one correction per packet; after the last
  correction the PC sends Result 1 until the controller echoes it, then Stop 1 until Start returns
  to 0; Start = 1 without a stream gives Stop 1 and Result 0.
- `FILE_UTILS/REMOUNT_SHARE.src`, `READ_POS.src`, `READ_FRAME.src`: KRL helpers. `REMOUNT_SHARE()`
  mounts the PC share `//192.168.2.100/RoboCrapJobs` as `/RoboCrapJobs`. `READ_POS(File, Ok)` reads
  one line `X;Y;Z;A;B;C;S;T` (S in 0–7, T in 0–63 required); `READ_FRAME(File, Ok)` reads
  `X;Y;Z;A;B;C`.
- `ProbeCheck/RSI_Probe_Check.src/.dat` and `RSI_Probe_Check_CONTRACT.md`: the stop test. Its RSI
  context (drawn by the user): ETHERNET1 (config file above, Timeout 10, Precision 6 since
  Milestone 1; the chamfer context keeps 1); SEN_PINT1
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
  to `RsiDevice::setCorrections`. `prepareTestLin()` installs the `RSI_Probe_Check` stream and removes
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
`resources/files/rsi/Chamfer/RSI_Chamfer_Hole_CONTRACT.md` and
`resources/files/rsi/ProbeCheck/RSI_Probe_Check_CONTRACT.md`, add one sentence each that the
controller now also sends `TouchIndex` and `Touch`, which these programs leave unconnected, and in
the ETHERNET1 row of `RSI_Probe_Check_CONTRACT.md` replace `Precision 1` by the value the run below
establishes. Then the user (with authorization for the test motion) copies the file to the
controller, sets ETHERNET1 `Precision` to 6 in the `RSI_Probe_Check` context in RSI Visual, and runs
`RSI_Probe_Check` until the probe stops it. The agent decodes the resulting capture (command in
`Concrete Steps`) and records: the number of decimals of AIPos, the presence of `<TouchIndex>` and
`<Touch>` in the packets, the value of `TouchIndex` (the chamfer and test streams are stream 0, so
the device must read it as 0) and that the robot moved and stopped as before. If RSI Visual or the
run rejects `Precision` 6, the user repeats with 4. If AIPos has fewer than four decimals, the plan
stops here and the user and agent look for another source of joint angles; if the run fails with the
new elements, the decision on one shared configuration file is revisited. Outcome (2026-11-11):
Precision 6 accepted, six decimals, the run unchanged, `TouchIndex` sent as `not linked:3`, which the
decision on TouchIndex decoding maps to 0.

Milestone 2 (RSI device). In `rsidevice.h/.cpp`:

Replace the member `CorrectionStream m_corrections` by a stream table: `QVector<QVector<V6d>>
m_streams`, `QString m_diagnosticId`, and the index of the active stream `qsizetype m_activeStream`.
Change the slot to `setCorrections(const QVector<QVector<V6d>>& streams, const QString&
diagnosticId)`; its body keeps the existing buffered-request handling and resets. Extend `Request`
and `decodeRequest` by one optional field, `TouchIndex`: missing or `not linked:…` gives 0, a
non-negative decimal integer gives the index, any other text or a duplicate makes the packet invalid
(decision on TouchIndex decoding). `Touch` and `AIPos` are not decoded (review
decision on touch recording). `Request` moves from the unnamed namespace of `rsidevice.cpp` into the
public section of `RsiDevice` next to `Reply`, because the member `advance` now takes it (decision of
Milestone 2). In `advance`, which gets
the request instead of two values (`advance(const Request& request)`, one parameter), a Start = 1
request whose touch index differs from `m_activeStream` while the state is not Idle first calls
`resetRun()`, as a Start = 0 request would (review decision on run starts; `m_activeStream` already
holds the run's index, so no member is added). The transition Idle → Streaming then sets
`m_activeStream` to the touch index and selects `m_streams[touchIndex]` when that index exists and
its stream is not empty, otherwise goes to Stopping with Result 0, exactly as an empty stream does
today. The rest of the state machine is unchanged; Result 1 and Stop follow the end of the selected
stream. The capture, `writeTrace` and `publishState` are unchanged. No console output and no file
access are added to the receive path.

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
  with the target's orientation and the target's S/T (KRL CIRC ignores S/T, but `READ_POS` rejects
  S outside 0–7 and T outside 0–63); the arcs of the plan are shorter than 180°, so the sum is not
  zero (check it, else an error);
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
shows the preparation error or summary and the controller state (`rsiState`) when the calibration
has no error and no playback runs. The button appears only after a submode change, which
is a deliberate navigation, so the layout rule of `AGENTS.md` is kept.

Milestone 4 (controller program and contract). Create in `resources/files/rsi/Calibration`:

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
2 `LIN StepTarget[i]`, 3 `CIRC StepAux[i], StepTarget[i]`, 4 `SEARCH(StepTouch[i])`, 5 `PTP XHOME`
(plain form, keeping the 20 % axis velocity).
Step kind 4 records `SearchStart=$POS_ACT` (an advance run stop), ends the program if `$IN[8]` is
already TRUE (the STOP object needs a rising edge) and calls `SEARCH`, a local function: it sets
`$SEN_PINT[3]=Touch`; `RSI_ON(#RELATIVE)`; `WAIT SEC 0.012` (its Start = 0 packets reset the device;
required, because Milestone 1 showed none without it); `$SEN_PINT[1]=1`; `RSI_MOVECORR()`;
`Touched=$IN[8]`; `$SEN_PINT[1]=0`; returns `(RSI_OFF()==RSIOK) AND Touched` (FALSE also when
`RSI_ON` fails). When `SEARCH` returns FALSE the program moves `LIN SearchStart` and ends. When
`StepLimit` ends the loop right after a search, the program moves `LIN SearchStart` and ends. Neither
exit moves PTP; the operator moves the robot away in T1 (review decision on the failure exit and
Milestone 4 review decision).

`RSI_Probe_Calibration_CONTRACT.md` in the same folder, written like the two existing contracts:
controller files (the SRC, DAT, the user-drawn context files, the shared Ethernet configuration,
FILE_UTILS), frames, the RSI context (table and the Mermaid diagram in `Artifacts and Notes`),
signals (including `$SEN_PINT[3]`, `TouchIndex`, `Touch`), PC behavior (reference to the chamfer
contract's state machine plus stream selection and the run end on a changed touch index), the job
file format, the packet captures as the record of the touches, the touch and plan files, the KRL flow
including failure handling, and a "Not yet verified" list.

`python/rsi_touches.py <records directory> <id>`: reads `rsi_packets_<id>.csv` and its `_partN`
files in save order (decision of Milestone 4), takes the received (`rx`) rows in file order, decodes
the Base64 XML and splits it into runs:
maximal sequences of packets with `Start` 1 and the same numeric `TouchIndex` greater than 0. For each run it
writes one row of the touch file (format in `Artifacts and Notes`) from the first packet with `Touch`
1 and the packet before it; it reports runs without `Touch` 1 and touch numbers that occur in more
than one run, and exits with an error if a file has a `# dropped_rows` value above 0. It writes
`rsi_touches_<id>.csv` into the same directory and prints the row count. Run it with `python -I`.

The user draws the context `RSI_Probe_Calibration` in RSI Visual starting from a copy of
`RSI_Probe_Check`: add SEN_PINT3 (index 3) → ETHERNET1.In3, connect DIGIN1.Out1 also to ETHERNET1.In4,
set ETHERNET1 `Precision` to the value found in Milestone 1 and keep large POSCORR1 limits (decision
after Milestone 4). The user copies SRC, DAT and the context to the controller, generates a path in
the application, runs RSI Sampling, and with authorization runs the program with `StepLimit` set to
cover the first pass's entry and three touches (the step numbers are printed in the plan file), then
the whole first pass.

Milestone 5 (full session and documents). With authorization the user runs the whole session. The
agent runs `python/rsi_touches.py` on the session's captures and checks the touch file (command in
`Concrete Steps`): 158 rows, touch numbers 1–158 once each, cycle values near 750 (3 mm at 0.004 mm
per cycle) within ±250, AIPos with the decimals of Milestone 1. Then update
`docs/context/AGENT_CONTRACTS.md` (Devices and protocols: stream table, TouchIndex decoding, run end
on a changed touch index, captures as the touch record; Machining and playback, RSI: calibration
sampling and job file, and the sentence "No RSI, KRL or real touch exists for calibration"; QML: the
Calibration toolbar list gains RSI Sampling), `docs/context/ARCHITECTURE.md` (RsiDevice,
SceneMachiningPreparation, the new trajectories file, the controller files, the script) and
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

Expected after Milestone 1: AIPos attributes with six decimals (for example `A2="-89.512345"`), at
least four, and elements `<TouchIndex>` and `<Touch>`. Observed: six decimals and
`<TouchIndex>not linked:3</TouchIndex><Touch>not linked:4</Touch>` (decision on TouchIndex decoding).

Building and checking the touch file (Milestones 4 and 5; `<records>` is the `records` directory
beside the executable, `<id>` the plan id of the RSI Sampling):

    python -I python/rsi_touches.py <records> <id>
    python -I -c "import csv,sys; r=[x for x in csv.reader(open(sys.argv[1])) if x and x[0][0]!='#'][1:]; t=[int(x[0]) for x in r]; c=[int(x[2]) for x in r]; print(len(r), sorted(t)==list(range(1,159)), min(c), max(c))" <rsi_touches_id.csv>

Expected: `158 True` and cycle values between about 500 and 1000.

Searching for remaining uses of the changed device API after Milestone 2:

    grep -rn "setCorrections\|correctionsReady" src

Every match must use the stream-table signature.


## Validation and Acceptance

Milestone 1: the user reports that `RSI_Probe_Check` with the new `RSI_EthernetConfig.xml` and
`Precision` 6 (or 4) moved and stopped as before; the decoded capture shows AIPos with at least four
decimals, the two new elements and `TouchIndex` 0 or `not linked:3` (met, 2026-11-11).

Milestone 2: after building `robocrap`, "RSI test LIN" on the Network page and a run of
`RSI_Probe_Check` (authorized) behave as before; the capture is written as before and has as many `tx`
as `rx` rows, so every packet with `<TouchIndex>not linked:3</TouchIndex>` was decoded as touch
index 0 and answered. RSI Sampling of a
machining path still writes `shared_data.txt`. The device's console line on installation reads
"RSI correction streams installed: 1" for the chamfer and test streams.

Milestone 3: in the Calibration submode, after "Generate Path", the toolbar shows "RSI Sampling";
pressing it prints "RSI calibration sampling: 158 touches, <M> steps" and the status bar shows
"RSI calibration: 158 touches · <M> steps"; `C:/RoboCrapJobs/calibration_data.txt` and
`records/rsi_calibration_plan_<id>.csv` exist, `shared_data.txt` does not; leaving Calibration and
sampling a machining path removes `calibration_data.txt`. No robot moves in this milestone.

Milestone 4: the program compiles on the controller (selecting it shows no error); with
`StepLimit` covering three touches the robot reaches the first entry, performs three searches that
stop on the touch, retracts and ends there; `python/rsi_touches.py` writes
`records/rsi_touches_<id>.csv` with three rows, touch numbers 1–3. The whole first pass then ends
with 19 rows and no failed search (met, 2026-11-11).

Milestone 5: the full session ends at HOME with no failed touch; the touch file passes the check in
`Concrete Steps`.


## Idempotence and Recovery

All repository edits are repeatable. RSI Sampling can be pressed again at any time; it replaces the
held stream and job file. If a robot run stops (failed touch, RSI error, operator stop), the robot
stays near the sphere: the operator moves it away in T1 and restarts the program from the beginning
with a fresh selection (T1 program reset); the PC needs no reset, because the Start = 0 packets of
every search's `WAIT SEC 0.012` reset the device and each search starts from its own stream. If a
touch fails because the contact is farther than 2 mm beyond the nominal position, record it in
`Surprises & Discoveries` and decide with the user whether to lengthen the search or correct the
sphere file; do not change the stream length silently.


## Artifacts and Notes

RSI context `RSI_Probe_Calibration` (to be drawn in RSI Visual; parameter names as in the
`RSI_Probe_Check` contract, which asks to confirm them in RSI Visual's object properties):

    flowchart LR
      SP1["SEN_PINT1<br/>Index 1<br/>$SEN_PINT[1] = Start"] -->|In1| ETH
      SP2["SEN_PINT2<br/>Index 2<br/>$SEN_PINT[2] = Result"] -->|In2| ETH
      SP3["SEN_PINT3<br/>Index 3<br/>$SEN_PINT[3] = touch number"] -->|In3| ETH
      DI["DIGIN1<br/>input 8, 1 bit<br/>$IN[8] = WP-500 touch"] -->|In4 Touch| ETH
      DI -->|In2| OR1
      ETH["ETHERNET1<br/>RSI_EthernetConfig.xml<br/>Timeout 10, Precision from Milestone 1"]
      ETH -->|"Out1..Out6 RKorr X/Y/Z/A/B/C"| PC1["POSCORR1<br/>RefCorrSys World<br/>large X/Y/Z and rotation limits"]
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
      kind 3 CIRC: two rows, auxiliary point then target, each X;Y;Z;A;B;C;S;T (both with the target's S/T)
      kind 4 SEARCH: no further row; touch = global touch number 1..158
      kind 5 HOME: no further row
    touch is 0 for every kind except 4.

Touch file `records/rsi_touches_<id>.csv`, written by `python/rsi_touches.py` from the packet
captures:

    # plan_id,<id>
    # cycle: (IPOC - first IPOC of the run) / 4; AIPos deg; prev_*: preceding packet
    touch_index,ipoc,cycle,A1,A2,A3,A4,A5,A6,prev_A1,prev_A2,prev_A3,prev_A4,prev_A5,prev_A6

Milestone 2 implementation record (2026-11-11).

Reuse before adding: `isDigits` validates the numeric `TouchIndex`; `resetRun()` ends a run for a
changed touch index exactly as for Start = 0; the capture, `saveTrace`, `writeTrace` and the
Idle/Streaming/Finished/Stopping transitions are unchanged. `CorrectionStream` was removed: its two
fields became `m_streams` and `m_diagnosticId`, with `m_activeStream` as the only new state (the
run's touch index, required to detect a changed index). Added: `Request::touchIndex`, the
`TouchIndex` branch of `decodeRequest` and `activeStreamSize()` (decision of Milestone 2).

Compliance pass (declarations, bodies, callers): no lambdas, templates, exceptions or output
parameters; `advance` has one parameter, `setCorrections` and `correctionsReady` two;
`decodeRequest` still returns `std::optional`; every caller of `setCorrections`/`correctionsReady`
(`main.cpp:96`, `scenemachiningpreparation.cpp:188, 213, 232`) uses the table signature. Finding,
fixed: the stream read `m_streams[m_activeStream][m_index++]` used non-const `operator[]`, which
detaches the implicitly shared table on the first reply and copies it inside the reply path (the
former single stream had the same pattern); it now reads with const `at()`.

Line counts (physical/non-blank) before and after: rsidevice.cpp 353/327 → 366/340, rsidevice.h
88/73 → 95/80, scenemachiningpreparation.cpp 255/232 → 255/232, scenemachiningpreparation.h
69/61 → 69/61, main.cpp 161/119 → 161/119; non-blank total 812 → 832. The growth is the TouchIndex
decoding (7 lines), the run end on a changed index and the stream selection (4), the size helper
with its declaration (5) and the `touchIndex` field with comments; removing `CorrectionStream`
offsets its three new members.

Milestone 3 implementation record (2026-11-11).

Reuse before adding: the step order is the builder's own function, now published as
`probeMotionSteps` (no second definition); poses and S/T come from the saved points and
`Kr10Kinematics::configuration`, as in `writeSharedData`; rounding and angles use the moved
`principalEulerDegrees` and `wireValues`; arcs use the plan's sphere centre and start shell; the
device, `correctionsReady` and the capture are unchanged from Milestone 2. Added and why reuse fails:
`CalibrationJobSampler` (worker-local state: motion, kinematics, job, plan rows; same pattern as the
chamfer `RsiStreamSampler`), `wirePose` (the six-decimal round trip of one pose, needed for the tool,
every target and every auxiliary point), `searchStream`, `csvPose`, `arcMidpoint`; in the
application `PreparedCalibration` (identity and summary counts of the held job), `poseRow`,
`writeCalibrationData`, `replaceJobFile` (extracted, see decision), `prepareCalibration`,
`finishCalibration`, `isCurrentCalibration`, `calibrationUnavailableReason` and
`onCalibrationAvailabilityChanged`.

Compliance pass (declarations, bodies, callers): no lambdas, templates, exceptions or output
parameters; the largest parameter count is three (`SceneMachiningPreparation` constructor, as
planned); results return through `std::optional`/result structs and error strings; the worker gets a
copied `shared_ptr` and returns through its future; every caller of the constructor (`main.cpp:93`)
and of `probeMotionSteps` (builder, sampler) is updated; no remaining use of `motionSteps` or
`MotionStep`. Not checked by the pass: the 2 mm of search beyond the nominal contact is not IK-checked
(the animation checks only the 3 mm Approach); a missed touch ends the search there.

Line counts (physical/non-blank) before and after: probetouchmotion.cpp 171/153 → 163/146,
probetouchmotion.h 58/48 → 71/59, chamferrsistream.cpp 270/246 → 252/230,
scenemachiningpreparation.cpp 255/232 → 374/342, scenemachiningpreparation.h 69/61 → 94/85,
scenecalibration.h 68/62 → 69/63, main.cpp 161/119 → 161/119, new rsicoordinates.h 27/22,
probersistream.h 41/34, probersistream.cpp 196/177; C++ non-blank 921 → 1277. QML: Main.qml
478/419 → 489/430, ModeToolBar.qml 239/203 → 239/203. Build input: src/trajectories/CMakeLists.txt
27 → 30. The growth is the new capability (job steps, search streams, plan file, job file, held
calibration state and its availability); the smaller alternative of exporting the animation's
4 ms joint samples instead of KRL steps was rejected in the first decision of this plan.


## Interfaces and Dependencies

No new library. `RoboCrapDevices` keeps QtNetwork and Geometry; `RoboCrapTrajectories` gets one new
source; `python/rsi_touches.py` uses only the Python standard library (`base64`, `csv`, `glob`,
`xml.etree.ElementTree`). At the end of Milestone 2:

    // rsidevice.h
    public:
      struct Request { quint64 ipoc = 0; bool start = false; qint64 result = 0; qsizetype touchIndex = 0; };
    public slots:
      void setCorrections(const QVector<QVector<V6d>>& streams, const QString& diagnosticId);
    private:
      Reply advance(const Request& request);
      qsizetype activeStreamSize() const;
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


Revision note (2026-11-11, review): resolved the plan-review findings recorded in `Surprises &
Discoveries`. Every search now starts a new device run (KRL `WAIT SEC 0.012` after `RSI_ON`, and the
device ends a run whose touch index changes), so no stream continues into another touch. The touch
file is built from the packet captures by `python/rsi_touches.py` instead of being recorded in the
device, which removes the touch buffer, the `Touch`/`AIPos` decoding, the `writeTrace` change and
`rsiTouches` from Milestone 2 and counts controller packets instead of device replies. A failed
search or `StepLimit` now retracts and ends without a PTP; the final HOME is `PTP XHOME` at 20 %.
POSCORR1 limits are ±6 mm so that the first pass reveals a correction kept across
`RSI_OFF`/`RSI_ON`. Milestone 1 tries `Precision` 6 and checks `TouchIndex` 0. The CIRC auxiliary
row carries the target's S/T. The TouchIndex rationale and the `RSI_Probe_Check` contract's Precision
entry were corrected. Nothing is implemented yet; the milestone order is unchanged.

Revision note (2026-11-11, Milestone 1): recorded the controller run with the extended configuration
and Precision 6. Unconnected ETHERNET inputs arrive as `not linked:<n>`, so a new decision defines
how `decodeRequest` reads `TouchIndex` and Milestone 2 follows it. The run showed no Start = 0 packet
before the first Start = 1, which turns the KRL wait from a precaution into a requirement. IPOC counts
milliseconds, so the touch script takes the cycle count from IPOC. Paths follow the move of the
chamfer files to `resources/files/rsi/Chamfer/`. The same three findings are carried into the
TouchIndex stream decision, the run-start decision, Context (configuration elements, Precision 6),
the Milestone 1 and Milestone 4 work descriptions and the Milestone 2 acceptance check.

Revision note (2026-11-11, Milestone 4): the controller files were written in
`resources/files/rsi/Calibration/` instead of `calibration/`, following the user's `Chamfer/`
folder; the touch script reads the captures in save order instead of by `pc_ns`. Both are recorded
in the Milestone 4 decision.

Revision note (2026-11-11, Milestone 4 review): `RSI_Probe_Calibration.src` re-checked against the
KRL manuals; the `$IN[8]` check moved behind a `$POS_ACT` advance run stop, the failure and
`StepLimit` exits return to the recorded search start, and `READ_JOB` lost the checks this made
unnecessary (Milestone 4 review decision). The contract was updated accordingly; the DAT is unchanged.

Revision note (2026-11-11, after Milestone 4): the user renamed `WP-500_test/RSI_Probe_Lin.*` to
`ProbeCheck/RSI_Probe_Check.*`; this plan, the contracts, `docs/context`, the calibration method
document and the code comments now use the new name and folder, including the records of earlier
runs (the application keeps the "RSI test LIN" button and the `test_lin_` capture prefix). The
renamed SRC and DAT still declared `DEF RSI_Probe_Lin()` and `DEFDAT RSI_Probe_Lin`; both now match
the file name. The Milestone 4 runs did not use the ±6 mm POSCORR1 limits, so the POSCORR1 reset is
recorded as not yet verified.

Revision note (2026-11-11, POSCORR limits): the user keeps large POSCORR1 limits instead of ±6 mm;
the ±6 mm decision is superseded, the POSCORR1 reset is no longer a check (new decision after
Milestone 4), and the contract table, work text, acceptance text and diagram follow. The user
verified the renamed `RSI_Probe_Check` on the controller.
