# Rebuild RSI Sampling and stream the nominal chamfer motion to the KR C4

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is maintained in accordance with `PLANS.md` in the repository root. `AGENTS.md` overrides `PLANS.md` where they conflict: no commits, branch changes or stashing without an explicit user request, no project tests or test harnesses, and no configure/build/lint unless the user explicitly asks for it.

This plan is the only RSI plan. The user deleted the earlier RSI execplan and strategy documents on 2026-10-17, so nothing outside this file is required to follow it.


## Purpose / Big Picture

After this change, a user can generate a chamfer path for one hole edge, press RSI Sampling, and then start the KRL program `RSI_Hole` on the KUKA KR C4 in T1 mode. The robot then moves HOME, to the staging point, along a straight line to the lead-in start, and through lead-in, machining and lead-out purely under PC corrections. It returns to the staging point and HOME only if the PC reports normal completion.

Two things exist after RSI Sampling that do not exist today. First, a correction stream: one six-value increment for each 4 ms controller cycle, held where the RSI socket reads it. Second, a fresh `C:\RoboCrapJobs\shared_data.txt` with the auxiliary KRL frames and transfer rates. Today RSI Sampling computes far more than it needs, refuses to export without a hand-written limits file, and nothing ever sends the result to the robot: the device still plays an unrelated hard-coded demonstration line.

The first hardware goal is a nominal run in T1 to confirm that the offsets are transmitted and applied correctly. Force correction is out of scope and the force/torque sensor is not connected.

How to see it working without the robot: generate a path from `resources/json/edge_cyl_plane/XC.json` and `XP.json` and press RSI Sampling. The status line shows the number of correction cycles, the central duration and the cumulative X/Y/Z/A/B/C correction range. `C:\RoboCrapJobs\shared_data.txt` then contains exactly five rows. With the robot, the operator observes the T1 run described in Validation and Acceptance. That run is a user action; this plan never operates hardware.


## Progress

- [x] (2026-10-17) Read AGENTS.md, PLANS.md, the subsystem contracts, the old RSI plans, the RSI_Hole controller files, the exported RSI context (`D:\RSI_Hole.rsi`, `.rsi.xml`, `.rsi.diagram`, `D:\RSI_EthernetConfig.xml`), the robot machine data in `resources/files/KR C4`, and the RSI 3.3 and KSS manuals.
- [x] (2026-10-17) Critically reviewed every existing RSI source file and wrote this plan. No source has been changed.
- [x] (2026-10-17) Milestone 1: shared checked TCP inverse kinematics and `$ACC_MA.CP` Cartesian bound. Source review and `git diff --check` done; not built (no build requested).
- [x] (2026-10-17) Milestone 2: new numerical correction stream `sampleChamferRsiStream`. Source review and whitespace checks done; not built (no build requested). Nothing calls it yet; Milestone 3 connects it.
- [x] (2026-10-17) Milestone 3: RSI Sampling coordinator, shared-data export and status UI. Source review and whitespace checks done; not built (no build requested). The device connection for `correctionsReady` is added in Milestone 4, where `RsiDevice::setCorrections` is created.
- [x] (2026-10-17) Milestone 4: RSI device streaming state machine and wire protocol. Source review and whitespace checks done; not built (no build requested); no local wire check was run.
- [x] (2026-10-17) Milestone 5: obsolete RSI code removed, KRL fixed, `RSI_Hole_CONTRACT.md`, `AGENTS.md`, `docs/AGENT_CONTRACTS.md` and `docs/ARCHITECTURE.md` updated. Source review and `git diff --check` done; not built.
- [ ] Milestone 5 remainder: copy `RSI_Hole.rsi`, `RSI_Hole.rsi.xml`, `RSI_Hole.rsi.diagram` and the exported `RSI_EthernetConfig.xml` from the user's export into `resources/files/rsi/`. Drive `D:` was not reachable on 2026-10-17, so the repository `RSI_EthernetConfig.xml` is the renamed former `RSI_Hole_EthernetConfig.xml`; its elements match the export.
- [ ] Milestone 6 (user-run): T1 nominal run on the robot. Before it, the user raises POSCORR1 MaxRotAngle, deploys the updated `RSI_Hole.src` and the context, and builds the application.
- [ ] Milestone 7 (prototype, gated on Milestone 6 observations): pause and resume when the T1 Start key is released.


## Surprises & Discoveries

- Observation: the existing RSI sampler does about four complete sample evaluations per 4 ms cycle. Each sample does one analytical evaluation, one checked IK and one extra FK. The checked IK (`Kr10Kinematics::solveIK`) runs two FKs internally, so each sample costs one IK and three FKs. The three extra samples per cycle come from interpolation checks at 1/4, 1/2 and 3/4 of the cycle.
  Evidence: `src/pathgeneration/chamfer/chamfercartesiansampler.cpp`, `validationTimes` (fractions 0.25/0.5/0.75 plus saved knots and timing switches) and `sampleAt` (`solveIK` plus `solveFK`); `src/kinematics/kr10kinematics.cpp:42-54` (`solveIK` = FK of the seed, angle-only IK, FK of the result).

- Observation: those interpolation checks only re-prove a bound that Generate already guarantees. Generate splits each central span into intervals of length $h_s$ so that $a_s h_s^2/8 \le \varepsilon$, with $\varepsilon = 0.01$ mm. Here $a_s$ (mm/s²) is the span's bound on TCP acceleration, tangential plus curvature (`makePreviewSchedule` in `src/trajectories/chamfer/chamfercentralvalidation.cpp`). The largest distance between a curve and its chord over a time step $\delta$ is at most $a_s\delta^2/8$. So if the RSI cycle $\delta = 0.004$ s is no longer than the shortest saved central interval $\min_s h_s$, every 4 ms chord is within 0.01 mm without any extra evaluation.
  Evidence: the formula and the 0.01 mm constant are in `makePreviewSchedule`. The saved central animation points are exactly those interval ends (`ChamferMotion::points()` between `boundaries()[2]` and `boundaries()[5]`).

- Observation: the old encoder stores six decimal strings per cycle and re-parses them three times (encoding, reconstruction, limit check). A double formatted with 17 significant digits (`max_digits10`) parses back to the same double, so the string round trip proves nothing a stored double does not.
  Evidence: `src/pathgeneration/chamfer/rsiposeencoder.cpp`, `AdditiveTcpPoseEncoder::encode`, `NominalEncodingCompiler::run`, `validateCorrectionLimits`.

- Observation: the PC-side correction limits (`rsi-correction-limits.json`) duplicated controller monitoring the user does not want. The installed context has POSCORR1 limits of ±1000 mm and MaxRotAngle 360, and no POSCORRMON. A full lead-in, machining turn and lead-out rotates the TCP by roughly $30° + 360° + 30° = 420°$ about the hole axis (default spans; hypothetical estimate). So MaxRotAngle 360 may clamp one cumulative A/B/C component.
  Evidence: `D:\RSI_Hole.rsi.xml`, POSCORR1 parameters. The user wants no POSCORR1 limits (LowerLim/UpperLim/MaxRotAngle) to restrict the motion and will raise MaxRotAngle to 3600 in RSI Visual.

- Observation: nothing reached the robot. `RsiDevice` stores the prepared job, but its receive callback only plays the hard-coded `generateTrajectory` demonstration. `RsiProtocol::encode` writes neither `Stop` nor `Result`. `RsiHoleProtocol` uses an older `<State Job Sample Result>` schema that does not match the installed `RSI_EthernetConfig.xml`.
  Evidence: `src/network/rsi/rsidevice.cpp` (`makeTxFrame`, `tickMotion`), `src/network/rsi/rsiprotocol.cpp` (`encode`), `src/network/rsi/rsiholeprotocol.cpp` (`readStatus`).

- Observation: `RSI_Hole.src` clears `$SEN_PINT[1]` before reading `$SEN_PINT[2]`. The graph object MAP2SEN_PINT1 writes the PC's Result into `$SEN_PINT[2]` every cycle while RSI is on. If the PC sees Start = 0 and returns Result = 0 before KRL executes the read, `Completed` becomes FALSE after a successful run. The window is short, but the order is wrong.
  Evidence: `resources/files/rsi/RSI_Hole.src:84-85` and the MAP2SEN_PINT1 → `$SEN_PINT[2]` wiring in `D:\RSI_Hole.rsi.xml`.

- Observation: `RSI_Hole.src` selects `$BASE=BASE_DATA[1]`, but the probe points are expressed in WORLD and the user intended `$BASE=$NULLFRAME`. The machine data confirms that `$ROBROOT` is the null frame, so WORLD, ROBROOT and the application's kinematic base coincide.
  Evidence: `resources/files/rsi/RSI_Hole.src:27`; `resources/files/KR C4/R1/Mada/$machine.dat:98`.

- Observation: the controller machine data has no fixed joint-acceleration limit. `$ADAP_ACC=#STEP2` selects the dynamic model, which derives axis accelerations from motor torque. Joint speed limits follow from `$VEL_AXIS_MA` (motor rpm) and `$RAT_MOT_AX` (gear ratio): 220, 210, 270, 381, 311, 492 °/s. These already equal the `ratedPayloadSpeedDegreesPerSecond` values in `resources/json/kr10.json`. The Cartesian path acceleration limit is `$ACC_MA.CP` = 10 m/s², and Cartesian acceleration monitoring is off (`$ACC_CAR_STOP=FALSE`).
  Evidence: `resources/files/KR C4/R1/Mada/$machine.dat:101-119,198-199,274`; `resources/files/KR C4/R1/Mada/$robcor.dat:9`.

- Observation: `ETHERNET1` in the installed context has `Timeout=10`, `Precision=1`, an unconnected input In3 and 9 outputs, while the XML defines 2 inputs and 8 outputs. The local RSI 3.3 manual does not document these three parameters. "Precision" plausibly sets the decimal places of values the controller sends; if it also affected received values, the 0.02 mm increments would round away. This must be observed on the first run (Milestone 6), not assumed.
  Evidence: `D:\RSI_Hole.rsi.xml`, ETHERNET1 parameters; no match for "Precision" in the RSI 3.3 manual text.


- Observation: the user's build reported "use of 'auto ChamferProfiledKinematics::calculateIK…' before deduction of 'auto'" at `chamferpreparationdiagnostics.h:48`. It also reported a follow-on brace-conversion error at line 60.
  Evidence: `solveTcp` was defined above the `auto`-returning wrappers `calculateIK`, `solveFK` and `configuration`. A deduced return type cannot be used before its definition, even inside the class body. Fix (2026-10-17): `solveTcp` moved below those wrappers, unchanged.

## Decision Log

- Decision: Final review on the rebased base (2026-10-17).
  - The summary became one footer line, showing only the largest cumulative |X/Y/Z| and |A/B/C| offset instead of six ranges and the file path, because upstream's status bar elides a single line.
  - The coordinator's two scene lookups became one `pathById(id)`; the `MachiningPath` kind check was dropped, because upstream `SceneObject::geometry()` is non-null only for machining paths.
  - The sampler checks joint speed against `timing.jointSpeed` alone, because `ChamferMotion::create` sets it to the model's rated speeds.
  Rationale: remove redundant checks and lookups, and fit the reduced UI.
  Date/Author: 2026-10-17, Claude.

- Decision: The RSI work, first written on base `9a40e08`, was rebased onto `origin/main` `3e99496`. That base contains the user's code-reduction plan (`codex_plans/CODE_REDUCTION_EXECPLAN.md`), which removed Generate diagnostics, the `--profile-chamfer` profiler, `kr10.json` (now `RobotDefinition::kr10()`) and the scene geometry views. The adaptation:
  - `ChamferProfiledKinematics` (call counting and timing, kept upstream only for the old RSI sampler) is replaced by `ChamferTcpSolver` in `src/trajectories/chamfer/chamfertcpsolver.h`. It holds the borrowed robot setup, a plain `Kr10Kinematics` and ${}^{T}A_{F}$, and offers `kinematics()` and `solveTcp(tcp, seed)`. `chamferpreparationdiagnostics.h` is deleted.
  - `CentralValidator` and the RSI sampler use `ChamferTcpSolver`. `MotionCompiler` is exactly upstream's version (plain solver, own `m_tcpToFlange`).
  - `ChamferRsiStreamResult` has no `diagnostics` field. RSI Sampling logs one line, "RSI Sampling: N cycles, T s movement" or the error, matching upstream's Generate log line.
  - The profiler edit was dropped, because upstream deleted the profiler. `readProbeSamples` (whose last callers were the profiler and `Plane::fromJsonFile`) and `utils.cpp`'s `QFile` include were removed.
  - The RSI status moved into upstream's footer status bar (`Main.qml` `statusBar.message`): `error || summary || unavailableReason`, plus `Controller: <rsiState>` while the RSI socket is bound. The footer shows it on one elided line.
  - `CMakeLists.txt.user` takes upstream's version. The user's earlier local copy is in commit `03d66be`.
  Rationale: upstream's reduction removed diagnostics deliberately, so the RSI code must not reintroduce them, and the counting wrapper existed only for the old sampler.
  Date/Author: 2026-10-17, Claude, during the rebase.

- Decision: Rewrite RSI Sampling as one numerical operation, `sampleChamferRsiStream(const ChamferMotion&)`. It returns the per-cycle increments directly, with no stored poses, no decimal strings and no limit profile.
  Rationale: the stream is the only product the robot consumes. Poses, joint witnesses, envelopes and strings were intermediate data that each cost memory and re-validation.
  Date/Author: 2026-10-17, Claude.

- Decision: Per 4 ms cycle, do exactly one analytical evaluation through `ChamferCentralMotion::evaluate`, one seeded angle-only IK, one FK with flange and TCP checks, a joint position check, a joint speed check, a Cartesian acceleration check and the Euler continuity step. Remove the quarter-cycle interpolation evaluations and replace them with the zero-cost condition $\delta \le \min_s h_s$ (see Surprises).
  Rationale: per-cycle IK is the only way to prove joint limits, branch continuity and joint speed at the exact points the controller will execute, notably near the wrist singularity. Everything else is either already guaranteed by Generate or not a requirement.
  Date/Author: 2026-10-17, Claude.

- Decision: Keep `ChamferCentralMotion::evaluate` as the only source of targets. It turns time into traveled distance, recovers the curve parameter with the piecewise quintic Hermite inverse (`ChamferArcLength::progressAtDistance`), and evaluates the analytical pose.
  Rationale: the user requires the inverse quintic; it is the fast and accurate parameter lookup Generate already uses. No second parameter search, projection or interpolation is introduced.
  Date/Author: 2026-10-17, Claude, at the user's explicit instruction.

- Decision: No joint-acceleration rejection in RSI Sampling. The Cartesian acceleration bound for both Generate and RSI Sampling becomes `$ACC_MA.CP` = 10000 mm/s². Joint speed bounds remain the rated speeds in `kr10.json`, which equal the machine data. `jointAcceleration` stays only as the HOME PTP animation timing assumption.
  Rationale: the user asked that animation and sampling use controller values. `$ACC_MA.CP` is the maximum path acceleration KSS allows for programmed CP motions; the controller does not monitor it for RSI corrections (`$ACC_CAR_STOP=FALSE`, `$ACC_CAR_LIMIT` all zero). It is the only Cartesian acceleration value in the machine data. The controller has no fixed joint-acceleration number: the dynamic model derives it from torque, pose and load, and the `$DYN_DAT` layout is undocumented. So no fabricated joint value is enforced, matching the user's earlier request to keep joint-acceleration rejection disabled.
  Date/Author: 2026-10-17, Claude, from the user's answer "Both: for animation and sampling".

- Decision: `ChamferProfiledKinematics` is constructed from `const ChamferRobotSetup&` and owns the tool inverse ${}^{T}A_{F}$ (`tcpToFlange()`). `solveTcp` then takes two parameters (target, seed) instead of the three planned. The separate `m_tcpToFlange` copies in `CentralValidator`, `MotionCompiler` and the old `CartesianSampler` are removed.
  Rationale: AGENTS.md prefers zero to two parameters and forbids redundant state. Every solver user already holds a robot setup that outlives the solver, so borrowing it adds no lifetime risk. The solver already held a borrowed model reference, so copy-assignment behavior is unchanged.
  Date/Author: 2026-10-17, Claude, during Milestone 1.

- Decision: `sampleChamferRsiStream` does not re-validate the saved motion's boundaries, timing parameters or robot model.
  Rationale: `ChamferMotion` has a private constructor, and `ChamferMotion::create` is its only factory. That factory already validates boundaries, positive phase durations, `jointSpeed` and `cartesianAcceleration`. AGENTS.md forbids repeating a check that every path already made. The sampler checks only what is new: the optional `centralMotion()`, the 4 ms chord condition, per-cycle IK/limits/speed/acceleration, Euler continuity and the terminal winding.
  Date/Author: 2026-10-17, Claude, during Milestone 2.

- Decision: The five Euler helpers (`unwrapDegrees`, `unwrapEulerDegrees`, `principalEulerDegrees`, `rotationEntryError`, `continuousEulerDegrees`) are copied unchanged into `chamferrsistream.cpp`, while `rsiposeencoder.cpp` still holds the originals.
  Rationale: this is a parallel implementation during migration, as PLANS.md allows. The old encoder stays compiled only until Milestone 3 switches the coordinator, and Milestone 5 deletes it; the duplicate disappears with it.
  Date/Author: 2026-10-17, Claude, during Milestone 2.

- Decision: The coordinator exposes `summary` (one formatted, translated string) instead of separate `pathName`, `cycleCount`, `duration`, `minimumOffset`, `maximumOffset`, `sharedDataStatus` and `prepared` properties. The QML label is now `error || summary || unavailableReason || "Ready for RSI Sampling."`, and `unavailableReason` reports "Sampling RSI…" while the worker runs.
  Rationale: the only consumer is one status label, so seven properties plus QML formatting were a forwarding layer. The C++ string uses a single multi-argument `QString::arg`, so a `%` in a user-renamed path name stays literal.
  Date/Author: 2026-10-17, Claude, during Milestone 3.

- Decision: The coordinator observes only `SceneMachining::availabilityChanged`. It neither connects to `SceneModel::objectRemoved` nor stores a per-path `compatibilityChanged` connection. On each notification, a held stream is dropped unless its path still exists, is compatible and holds the same motion pointer (`isCurrent`). The same check rejects a finished worker result, so no generation counter is needed.
  Rationale: `SceneMachining` already emits `availabilityChanged` after selection changes (`setInputId`), after every object removal (`onObjectRemoved`, after the object has left the lookup table), and after compatibility refreshes (`refreshCompatibility`). Object IDs are never reused, so ID plus motion pointer identifies the sampled path exactly.
  Date/Author: 2026-10-17, Claude, during Milestone 3.

- Decision: The wire codec lives in `src/devices/rsi/rsidevice.cpp`, in an unnamed namespace (`Request`, `decodeRequest`, `encodeReply`), instead of separate `rsiprotocol.h/.cpp` files. The sender check is the one-parameter member `RsiDevice::acceptsSender(const QNetworkDatagram&)`, replacing the four-parameter free function and the `replyForDatagram` project template. `RsiDevice::Reply` is a public nested struct so the codec can encode it.
  Rationale: the codec has a single user. AGENTS.md places single-file helpers in an unnamed namespace, forbids project templates, and prefers zero to two parameters. The configured address and learned port are device state.
  Date/Author: 2026-10-17, Claude, during Milestone 4.

- Decision: `decodeRequest` no longer validates RIst, AIPos or MACur numerically. It requires a well-formed `<Rob>` document with exactly one IPOC (digits), one Start (`0` or `1`) and one Result (integer), and skips other elements.
  Rationale: nothing reads the telemetry yet, and validating unused values would only add ways to drop a valid cycle. The well-formedness and trailing-garbage checks remain.
  Date/Author: 2026-10-17, Claude, during Milestone 4.

- Decision: `src/network/` was deleted in Milestone 4 rather than Milestone 5.
  Rationale: the moved device was its last consumer. Keeping the files would have required the Devices target to keep its Trajectories link and the legacy `src` include.
  Date/Author: 2026-10-17, Claude, during Milestone 4.

- Decision: Milestone 5 also removed `ChamferSamplingParameters` (`chamferpath.h`), `ChamferMotion::centralTimeScale` and the `SceneMachiningPath.centralTimeScale` QML property.
  Rationale: each existed only for the deleted RSI sampler/job ("compatibility: prescribed timing is never stretched"), and a repository search found no QML or C++ consumer.
  Date/Author: 2026-10-17, Claude, during Milestone 5.

- Decision: Historical plans (`codex_plans/ARCHITECTURE_EXECPLAN.md`, `ARCHITECTURE_REVIEW.md`) still mention `src/pathgeneration`. They are not edited.
  Rationale: they record past work, not current constraints. `docs/AGENT_CONTRACTS.md` and `docs/ARCHITECTURE.md` are the current descriptions.
  Date/Author: 2026-10-17, Claude, during Milestone 5.

- Decision: Delete the PC-side correction-limit profile (`rsicorrectionlimits.*`, `RSI_CorrectionLimits.example.json`, the file watcher and its readiness gate). RSI Sampling instead reports the cumulative min/max of each X/Y/Z/A/B/C correction so the user can compare it with POSCORR1 in RSI Visual.
  Rationale: the profile held exactly the POSCORR/POSCORRMON limits, and the user wants no POSCORR limits to restrict RSI motion. The controller still enforces its own POSCORR1 parameters, which the user sets in RSI Visual. This does not remove the joint speed and Cartesian acceleration checks on the prescribed motion.
  Date/Author: 2026-10-17, Claude.

- Decision: Encode each increment numerically as $\bar{\Delta}_k = q\big(\bar{c}(t_k) - \bar{c}(t_0) - \bar{S}_{k-1}\big)$, where $q$ rounds each component to $10^{-6}$ and $\bar{S}_{k-1}$ is the sum of all previously sent increments. Send it with six fixed decimal places.
  Rationale: fixed notation never produces exponent text such as `1e-05`, which the controller's XML parser may not accept (unverified). Reconciling against the sum of rounded values keeps the cumulative error at or below $5\cdot10^{-7}$ at every cycle, so no drift accumulates.
  Date/Author: 2026-10-17, Claude.

- Decision: Store the stream as an implicitly shared `QVector<V6d>` with 50 trailing zero increments. The GUI coordinator keeps one copy for status, and a queued signal hands the same shared data to `RsiDevice` on the ControlIO thread. That device is "where the RSI socket reads the offsets". Devices then depends on Geometry (for `V6d`) instead of Trajectories.
  Rationale: Qt's implicit sharing makes the hand-over a reference-count increment with no copy, and the data is never mutated. The 50 zeros let the robot settle before Result = 1, as the user agreed. Keeping them in the data keeps the device logic to a counter.
  Date/Author: 2026-10-17, Claude.

- Decision: A sampled stream belongs to its saved path, not to the current selection. It is invalidated only when that path is removed or regenerated, when the path becomes incompatible (calibration or robot model change), or at shutdown.
  Rationale: the user samples, then walks to the robot and may click other scene objects. The old coordinator cleared the job on any selection change, which would make the next robot run abort.
  Date/Author: 2026-10-17, Claude.

- Decision: Whenever the coordinator holds no valid stream (startup, invalidation, failed sampling, shutdown), it deletes `C:\RoboCrapJobs\shared_data.txt`.
  Rationale: `RSI_Hole.src` moves the robot to Ps and LeadStart from the file before RSI starts. A stale file would move the robot toward an old hole and then abort at RSI_MOVECORR. Without the file, KRL returns right after HOME. The user declined a job identifier; removing the file achieves the same safety without changing the format. Reversible: delete the removal calls if the user objects.
  Date/Author: 2026-10-17, Claude.

- Decision: Wire protocol. The PC replies to every accepted controller packet within the same callback.
  - Start = 0: zero corrections, Stop = 0, Result = 0, and the stream index returns to 0.
  - First packet with Start = 1: if a stream exists, start streaming; one received packet consumes one increment. If no stream exists, send Stop = 1 with Result = 0, so RSI_MOVECORR ends and KRL performs no return motion.
  - After the last increment: zeros with Result = 1. Once a packet echoes Result = 1: zeros with Result = 1 and Stop = 1, until Start returns to 0.
  Rationale: this is the contract already agreed in `RSI_Hole_CONTRACT.md`. The robot's 4 ms packet clock drives the stream, and returning to index 0 on Start = 0 replays the same stream on the next run, as requested.
  Date/Author: 2026-10-17, Claude.

- Decision: When several datagrams are queued at once, the PC answers only the newest one and advances the stream by one increment.
  Rationale: replies to older packets arrive late and are rejected by the controller, so increments sent in them would be lost for good (HOLDON = 0 resets the output). Answering only the newest packet delays the path by a cycle instead of skipping part of it. Gaps are logged. The user accepts that RSI stops the robot when replies are too late.
  Date/Author: 2026-10-17, Claude.

- Decision: A new stream or a clear received while the device is streaming, finished or stopping is held as pending and applied when Start returns to 0.
  Rationale: an executing run must keep its immutable data, while the next run must use the latest sampling result.
  Date/Author: 2026-10-17, Claude.

- Decision: Delete the old demonstration and frozen layers: `src/pathgeneration/**`, `src/scene/**`, `src/network/rsi/**`, `KukaPanel.qml`, `rsiholeprotocol`, `rsicorrectionlimits`, the outdated `RSI_request_example.xml`/`RSI_responce_example.xml`, and the geometry helpers only the demo used.
  Rationale: the user allowed deleting the demo. AGENTS.md requires removing obsolete paths in scope. The canonical module layout (`src/trajectories`, `src/devices`, `src/application`) can then own all RSI code without legacy include paths.
  Date/Author: 2026-10-17, Claude.

- Decision: Fix two KRL issues: use `$BASE=$NULLFRAME` and read `Completed` before clearing `$SEN_PINT[1]`.
  Rationale: see Surprises & Discoveries.
  Date/Author: 2026-10-17, Claude.

- Decision: Pause and resume on Start-key release is a separate prototype milestone (7), gated on observations from the first T1 run. During the first run the operator keeps Start pressed; if Start is released, the program is cancelled and restarted from HOME.
  Rationale: today nothing tells the PC that the robot stopped: `$SEN_PINT[1]` keeps its value. It is also unknown whether RSI keeps exchanging packets while the program is stopped, and whether POSCORR accumulates corrections during that time. If it does, the robot could jump on resume. These are controller facts to observe, not to guess.
  Date/Author: 2026-10-17, Claude.


## Outcomes & Retrospective

Milestone 1 (2026-10-17):
- `ChamferProfiledKinematics::solveTcp` and `chamferJointPositionError` now live in `src/trajectories/chamfer/chamferpreparationdiagnostics.h`. `CentralValidator::sample` calls `solveTcp` and keeps the same error texts and checks in the same order. `MotionCompiler::inverse` keeps its own flange-only check, as planned, but uses `m_solver.tcpToFlange()`.
- The Cartesian acceleration default is 10000 mm/s² in `ChamferTimingParameters` and `SceneMachiningSettings`.
- The old RSI sampler (`src/pathgeneration/chamfer/chamfercartesiansampler.cpp`) still enforces `jointAcceleration` until Milestone 2 replaces it.
- Not built; compilation and Generate behavior are unverified.

Line counts, physical/nonblank, before → after:

| File | Before | After |
| --- | --- | --- |
| `chamferpreparationdiagnostics.h` | 90/79 | 138/123 |
| `chamfercentralvalidation.cpp` | 517/482 | 491/457 |
| `trajectories/chamfermotion.h` | 90/79 | 92/81 |
| `chamfermotion.cpp` | 829/783 | 828/782 |
| `chamfercartesiansampler.cpp` | 411/384 | 409/382 |
| `scenegeometry.h` | 261/237 | 261/237 |

C++ total (Milestone 1) 2198/2044 → 2219/2062 (+21/+18). The growth is the result type and the two-line documentation of the shared operation in the header, against 26 lines removed from the validator. The smaller alternative, leaving the validator's copy in place, would make Milestone 2 add a second full copy of about 20 lines, so the net effect over Milestones 1–2 is a reduction.

Milestone 2 (2026-10-17):
- New: `src/trajectories/include/trajectories/chamferrsistream.h` (33/28) and `src/trajectories/chamfer/chamferrsistream.cpp` (241/216).
- `chamfercentralmotion.h` 69/57 → 70/58: the `path()` accessor.
- `src/trajectories/CMakeLists.txt` 37/36 → 39/38.
- The operation it replaces — old sampler, encoder, job, limits and their headers — is about 1208 physical lines. It stays in the tree until Milestones 3 and 5 remove its last caller and its files.
- Per cycle the new code does one analytical evaluation, one angle-only IK with its three-joint prefix, one FK and a few vector operations. The old sampler did about four evaluations, four checked IKs and twelve FKs. These are source-derived counts, not measurements.
- Not built. Compilation and numerical results are unverified.

Milestone 3 (2026-10-17):
- `src/application/scene/scenemachiningpreparation.h/.cpp` replace `src/scene/scenemachiningpreparation.*` and `src/scene/chamfershareddata.*`; the `src/scene` directory is gone. The class name and the `App.MachiningPreparation` QML name are unchanged.
- RSI Sampling now samples through `sampleChamferRsiStream`, writes the five-row file only for an accepted result, and deletes the file whenever no stream is held: at startup, at a new request, on invalidation and at shutdown.
- `main.cpp` no longer registers `PreparedChamferJobPtr` and no longer connects `preparedJobReady` or `FtsDevice::dataSampleHFReady → RsiDevice::setForce`. `chamferprofile.cpp` calls the new sampler. `src/application/CMakeLists.txt` dropped the `src/scene` include.
- The old device and the old trajectory files still compile but are no longer reached from the application; Milestones 4 and 5 remove them.
- Not built; compilation and runtime behavior are unverified.

Line counts, physical/nonblank, before → after:

| File | Before | After |
| --- | --- | --- |
| Coordinator and shared-data files | 427/391 (four files) | 280/250 (two files) |
| `main.cpp` | 184/139 | 180/136 |
| `src/application/CMakeLists.txt` | 135/130 | 132/127 |
| `chamferprofile.cpp` | 154/148 | 145/139 |
| `Main.qml` | 514/455 | 501/442 |

Net C++ and CMake −163/−150, QML −13/−13.

Milestone 4 (2026-10-17):
- New: `src/devices/include/devices/rsi/rsidevice.h` and `src/devices/rsi/rsidevice.cpp`. Deleted: `src/network/` (the old device, `RsiProtocol` and `RsiHoleProtocol`).
- `RoboCrapDevices` links `RoboCrapGeometry` instead of `RoboCrapTrajectories` and has no legacy `src` include. `robocrap` dropped its legacy `src` include.
- `main.cpp` registers `QVector<V6d>` and connects `correctionsReady` to `RsiDevice::setCorrections` with a queued connection. The `Main.qml` status label adds `Controller: <rsiState>` while the RSI socket is bound.
- `KukaPanel.qml` still exists and names the removed invokables `generateTrajectory`/`startStreaming`/`stopStreaming`. It is never instantiated; Milestone 5 deletes it.
- Not built; no local controller stand-in was run.

Line counts, physical/nonblank, before → after:

| File | Before | After |
| --- | --- | --- |
| RSI device and protocol (`src/network/rsi`, 6 files → 2 files) | 920/755 | 314/280 |
| `src/devices/CMakeLists.txt` | 27/27 | 21/21 |
| `main.cpp` | 180/136 | 184/140 |
| `src/application/CMakeLists.txt` | 132/127 | 130/125 |
| `Main.qml` | 501/442 | 505/446 |

Net C++ and CMake −610/−479, QML +4/+4.

Milestone 5 (2026-10-17):
- Deleted `src/pathgeneration/` (old job, sampler, encoder, limits and demo), `KukaPanel.qml`, `RSI_request_example.xml` and `RSI_responce_example.xml`: 1622/1397 lines.
- Removed the demo-only geometry helpers `Plane::fromJsonFile`, `Cylinder::setSurfacePose`/`surfacePose`, `Pose::offsetPose` (with its helper) and `readJsonPoints`, plus `ChamferSamplingParameters` and `centralTimeScale`.
- `RSI_Hole.src` uses `$BASE=$NULLFRAME` and reads `Completed` before clearing `$SEN_PINT[1]`. `RSI_Hole_EthernetConfig.xml` is renamed to the installed name `RSI_EthernetConfig.xml`.
- The exported `.rsi` files are not yet copied (see Progress).
- Not built.

Line counts, physical/nonblank, before → after:

| File | Before | After |
| --- | --- | --- |
| `plane.cpp` | 83/64 | 71/53 |
| `plane.h` | 23/17 | 21/15 |
| `pose.cpp` | 158/114 | 132/95 |
| `pose.h` | 41/30 | 39/29 |
| `cylinder.cpp` | 191/142 | 170/126 |
| `cylinder.h` | 35/23 | 31/20 |
| `utils.cpp` | 357/279 | 347/270 |
| `utils.h` | 49/39 | 48/38 |
| `chamferpath.h` | 105/90 | 99/85 |
| `scenegeometry.h` | 261/237 | 259/235 |
| `src/trajectories/CMakeLists.txt` | 39/38 | 17/17 |
| `src/application/CMakeLists.txt` | 130/125 | 129/124 |
| `RSI_Hole.src` | 107/95 | 108/96 |
| `RSI_Hole_CONTRACT.md` | 127/105 | 90/68 |

Whole plan so far: the RSI C++ that was replaced or deleted totalled 2904/2493 lines, against 868/774 new lines (sampler, coordinator, device). The demo, geometry and CMake removals come on top of that.


## Context and Orientation

Work from `E:\Qt\QtProjects\RoboCrap`. The application is a Windows Qt Quick program (C++17, Qt ≥ 6.8, CMake ≥ 3.16, bundled Eigen). It prepares and previews edge chamfering on a KUKA KR10 R1420 with a KR C4 controller (KSS 8.3, RobotSensorInterface 3.3). Read `AGENTS.md` before every code change. Its C++ rules apply to everything here: no lambdas, no project templates, no new exception types or throws, results through return values only, at most four parameters, and no project tests.

Terms used throughout:
- TCP: tool center point.
- Transform matrix ${}^{j}A_i$: maps coordinates from frame $i$ to frame $j$. ${}^{B}A_{T}(t)$ is the TCP pose in BASE at time $t$. Its translation column is the TCP position $\bar{p}(t)$ in mm; its rotation columns are the TCP axes.
- BASE: the robot's world frame. With `$BASE=$NULLFRAME` and `$ROBROOT` null, KUKA's BASE, WORLD and the application's kinematic base are the same frame.
- XYZABC: position in mm and Z-Y-X Euler angles in degrees, so the rotation is $R(A,B,C) = R_z(A)\,R_y(B)\,R_x(C)$ (`euler2rot` in `src/geometry/utils.cpp`).
- RSI: the controller option that lets a PC change the robot position every 4 ms.
  - Each cycle the controller sends a `<Rob>` XML datagram over UDP, carrying a cycle timestamp called IPOC. The PC must answer with a `<Sen>` datagram echoing that IPOC within the same cycle; late answers are rejected.
  - In relative mode with `RSI_MOVECORR`, the robot moves only by the corrections the PC sends. The controller adds each received correction to the running total.
  - POSCORR1 is the RSI object that applies the X/Y/Z/A/B/C corrections with BASE orientation at the TCP. The user confirmed that A/B/C corrections add directly to the starting TCP A/B/C angles.
- Central motion: the three phases executed through RSI (lead-in, machining, lead-out). HOME, staging and the straight transfers are executed by KRL itself.

How a path comes to exist. The user imports plane and cylinder probe files, intersects them into an ellipse edge, and presses Generate. `SceneMachining::generate` (`src/application/scene/scenemachining.cpp`) builds a `ChamferPath` and runs `ChamferMotion::create` on a worker thread (`src/trajectories/chamfer/chamfermotion.cpp`). The resulting immutable `ChamferMotion` is stored in a saved path object, `SceneMachiningPath` (`src/application/scene/scenegeometry.h`), as `std::shared_ptr<const ChamferMotion>`.

A `ChamferMotion` contains:
- `points()`: timed animation points with joints and FK-derived TCP.
- `boundaries()`: eight indices delimiting the seven phases Approach, TransferIn, LeadIn, Machining, LeadOut, TransferOut and ReturnHome.
- `robot()`: the kinematic model, `flangeToTcp` (${}^{F}A_{T}$, flange to TCP) and the timing parameters.
- `centralMotion()`: a `ChamferCentralMotion`.

`ChamferCentralMotion::evaluate(seconds)` (`src/trajectories/chamfer/chamfercentralmotion.cpp`) takes seconds from the lead-in start. It computes the traveled distance $s(t)$ from the prescribed speed profile, recovers the curve parameter $u(s(t))$ with the inverse quintic of `ChamferArcLength::progressAtDistance` (`src/trajectories/chamfer/chamferarclength.cpp`), and evaluates the analytical pose ${}^{B}A_{T}(u(s(t)))$ with `ChamferPath::evaluatePhase`. Its `phaseTimes()` are the four central times: lead-in start (0), machining start, lead-out start and the end $T$.

How Generate validates the central motion. `ChamferCentralPreparation` and `CentralValidator` (`src/trajectories/chamfer/chamfercentralvalidation.cpp`) evaluate targets on a schedule chosen before IK, keeping each chord within 0.01 mm (see Surprises). At each target they run seeded IK, a joint limit check, FK and flange/TCP checks, and joint speed and XYZ acceleration checks. The same IK-plus-checks sequence also appears, without the TCP check, in `MotionCompiler::inverse` in `chamfermotion.cpp` for the straight transfers.

What exists today for RSI (all to be replaced):
- `src/pathgeneration/chamfer/chamferjob.*`: `PreparedChamferJob` re-evaluates all eight saved boundaries, calls the sampler and the encoder, and keeps samples, boundary poses and the encoding.
- `src/pathgeneration/chamfer/chamfercartesiansampler.*`: the expensive sampler described in Surprises.
- `src/pathgeneration/chamfer/rsiposeencoder.*`: Euler unwrapping (worth keeping), string serialization, reconstruction, envelopes and limit checks.
- `src/pathgeneration/chamfer/rsicorrectionlimits.*`: the JSON limit profile.
- `src/pathgeneration/rsi/*`: the old demonstration (`RsiPath`, `offsetjsonwriter`).
- `src/scene/scenemachiningpreparation.*`: the GUI coordinator exposed to QML as `App.MachiningPreparation`; it is compiled into the application target from this legacy path.
- `src/scene/chamfershareddata.*`: the `shared_data.txt` payload and writer.
- `src/network/rsi/rsidevice.*`: the UDP device on the ControlIO thread, still running the demo.
- `src/network/rsi/rsiprotocol.*`: XML decode/encode. It contains a project template, `replyForDatagram`, which AGENTS.md forbids.
- `src/network/rsi/rsiholeprotocol.*`: an unused, mismatched schema.
- `src/application/qml/Views/Dashboard/KukaPanel/KukaPanel.qml`: listed in CMake but never instantiated.

The CMake targets reach these legacy paths through special include directories: `frozen_rsi_sources` in `src/trajectories/CMakeLists.txt`, the PRIVATE `${PROJECT_SOURCE_DIR}/src` include in `src/devices/CMakeLists.txt`, and `${PROJECT_SOURCE_DIR}/src` plus `src/scene` in `src/application/CMakeLists.txt`.

Device threading, which must be preserved:
- `DeviceHub` (`src/application/devices/devicehub.cpp`) moves each device to an I/O thread; RSI and FTS share ControlIO.
- QML talks to devices only through `App.Hub.device(key)`, a `DeviceRunner` with queued `invoke` and a `data` property map fed by the device's `stateReady(QVariantHash)` signal.
- Typed data reaches a device through a queued connection made in `src/application/main.cpp`.
- Sockets live and die on the device thread, in `startDevice`/`stopDevice`.

The controller side lives in `resources/files/rsi/`. `RSI_Hole.src` and `RSI_Hole.dat` are deployed and compile on the controller but have never run. The sequence is:
1. `INI`, then `PTP HOME`.
2. Open `/RoboCrapJobs/shared_data.txt` (the PC's `C:\RoboCrapJobs\shared_data.txt` through a share) and read three frames with `READ_FRAME`: Ps, LeadStart, LeadEnd. Then read `$VEL.CP` (m/s) and `$ACC.CP` (m/s²) with `krl_fscanf`.
3. `RSI_CREATE("RSI_Hole.rsi")`, `PTP Ps`, `LIN LeadStart`.
4. `$SEN_PINT[2]=0`, `RSI_ON(#RELATIVE,#IPO_FAST)`, `$SEN_PINT[1]=1`, `RSI_MOVECORR(#RSIBRAKE)`.
5. After MOVECORR returns: `$SEN_PINT[1]=0`, `Completed=($SEN_PINT[2]==1)`, `RSI_OFF`, `RSI_DELETE`.
6. Only if completed: `LIN` to Ps with the LeadEnd attitude, then `PTP HOME`.

The installed context, exported by the user, is in `D:\` and must be copied into `resources/files/rsi/` in Milestone 5:
- SEN_PINT1 (reads `$SEN_PINT[1]`) feeds ETHERNET1 In1 "Start"; SEN_PINT2 feeds In2 "Result".
- ETHERNET1 Out1..6 feed POSCORR1 CorrX..CorrC (RefCorrSys Base).
- Out7 "Stop" feeds STOP1 (Mode ExitMoveCorr).
- Out8 "Result" feeds MAP2SEN_PINT1, which writes `$SEN_PINT[2]`.
- ETHERNET1 uses `RSI_EthernetConfig.xml`: PC 192.168.1.100:5555, type ImFree.
  - SEND: `DEF_RIst`, `DEF_AIPos`, `DEF_MACur`, `Start` BOOL, `Result` LONG.
  - RECEIVE: `RKorr.X..C` DOUBLE with HOLDON 0, `Stop` BOOL with HOLDON 1, `Result` LONG with HOLDON 1.

A `<Rob>` request therefore looks like this; IPOC values are illustrative:

    <Rob TYPE="KUKA">
      <RIst X="..." Y="..." Z="..." A="..." B="..." C="..." />
      <AIPos A1="..." ... A6="..." />
      <MACur A1="..." ... A6="..." />
      <Start>1</Start>
      <Result>0</Result>
      <IPOC>123456</IPOC>
    </Rob>

and the PC reply like this:

    <Sen Type="ImFree">
      <RKorr X="0.020000" Y="-0.000312" Z="0.001000" A="0.000000" B="0.000000" C="0.012000" />
      <Stop>0</Stop>
      <Result>0</Result>
      <IPOC>123456</IPOC>
    </Sen>

The robot machine data (`resources/files/KR C4/R1/Mada/$machine.dat` and `$robcor.dat`) supplies the limits listed in Surprises & Discoveries.


## Plan of Work

The work goes bottom-up: numerical helpers, the stream, the GUI coordinator, the device, then deletion and documentation. Each milestone leaves a compilable tree and can be reviewed alone. Before each milestone, re-read AGENTS.md "Mandatory C++ style" and "Mandatory code reduction", record the file and nonblank-line counts of the touched files, and afterwards record the new counts in Outcomes & Retrospective.

Example of the quantities used below, hypothetical values for illustration only. A hole of radius about 6 mm machined at the default 5 mm/s with default leads gives a central duration of about $T \approx 11$ s. That makes $K = \lceil T/\delta \rceil = 2750$ motion increments plus 50 settle zeros, 2800 replies in total. The robot moves for $K\delta = 11.0$ s, not the computation time. Sampling computation should then be a few tens of milliseconds, an estimate from operation counts that Milestone 2 replaces with a measurement if the user requests a build.


### Milestone 1: shared checked TCP inverse kinematics and the `$ACC_MA.CP` Cartesian bound

Goal: one implementation of "solve seeded IK for a TCP target, check joint limits, run FK, check flange and TCP", used by central validation now and by the RSI sampler in Milestone 2. Also, the Cartesian acceleration bound becomes the controller value.

Add a member to `ChamferProfiledKinematics` in `src/trajectories/chamfer/chamferpreparationdiagnostics.h`:

    struct ChamferTcpIkResult
    {
      std::optional<RoboCrap3D::Kr10Kinematics::IkSolution> solution; // configuration from the same FK
      M4d actualTcp = M4d::Identity();                                 // FK flange * flangeToTcp
      QString error;
    };
    explicit ChamferProfiledKinematics(const ChamferRobotSetup& robot); // borrows; owns tcpToFlange
    ChamferTcpIkResult solveTcp(const M4d& tcp, const RoboCrap3D::Kr10Kinematics::IkSolution& seed) const;

Requirement record: central validation and the RSI sampler need the identical checked operation, and duplicating it a third time is what AGENTS.md forbids. The solver borrows the robot setup (limits and `flangeToTcp`) and stores ${}^{T}A_{F}$ once, so `solveTcp` needs only the target and the seed defining the branch.

The body is the current `CentralValidator::sample` IK section: `calculateIK(tcp * tcpToFlange, seed)`, the joint limit check (the former `positionError`, now the inline header function `chamferJointPositionError`), `solveFK`, `Kr10Kinematics::matchesFlange`, the TCP position and rotation checks against `Kr10PosEps`/`Kr10RotEps`, and `configuration(q, transforms)`. Keep the existing error texts. Then make `CentralValidator::sample` call `solveTcp` and keep only its target evaluation and sample assembly. Leave `MotionCompiler::inverse` in `chamfermotion.cpp` unchanged: it has no TCP check and no error text, so switching it would change which transfers are accepted.

Change the default `ChamferTimingParameters::cartesianAcceleration` in `src/trajectories/include/trajectories/chamfermotion.h` from 20 to 10000. Change its comment to "mm/s^2, `$ACC_MA.CP` of the installed KR C4 (`resources/files/KR C4/R1/Mada/$machine.dat`)". Change `SceneMachiningSettings::acceleration` in `src/application/scene/scenegeometry.h` the same way; it is not shown in the QML editor. Update the profiler's report key only if its meaning changes; it stays "cartesianAccelerationBound".

Result: Generate behaves identically except that the XYZ acceleration check uses 10000 mm/s². Proof: source review of the diff; when the user requests a build, Generate on the XC/XP edge succeeds as before.


### Milestone 2: the numerical correction stream

Goal: a single worker-side function producing everything the robot and the KRL file need.

Create `src/trajectories/include/trajectories/chamferrsistream.h` and `src/trajectories/chamfer/chamferrsistream.cpp`, and add both to `src/trajectories/CMakeLists.txt`. Add to `ChamferCentralMotion` (`chamfercentralmotion.h`) the accessor `const ChamferPath& path() const { return m_path; }`. Requirement record: the staging point $P_s$ must be evaluated analytically from the same path that defines the targets. `ChamferMotion` exposes only the path parameters, and rebuilding a path on the worker would duplicate validated construction.

Public interface:

    struct ChamferRsiStream
    {
      V6d initialCoordinates = V6d::Zero(); // lead-in start XYZ mm, ABC deg (principal Euler branch)
      V6d finalCoordinates = V6d::Zero();   // initial + sum of increments; terminal Euler branch
      V3d stagingPoint = V3d::Zero();       // P_s, mm
      QVector<V6d> increments;              // one per controller cycle after Start = 1, then 50 zeros
      int motionCycles = 0;                 // increments that carry motion (excludes settle zeros)
      double duration = 0.0;                // central duration T, s
      V6d minimumOffset = V6d::Zero();      // cumulative correction range per X/Y/Z/A/B/C
      V6d maximumOffset = V6d::Zero();
    };
    struct ChamferRsiStreamResult
    {
      std::optional<ChamferRsiStream> stream;
      QString error;
      QJsonObject diagnostics;
    };
    // Copied-input worker operation; no scene, device or file access.
    ChamferRsiStreamResult sampleChamferRsiStream(const ChamferMotion& motion);

Requirement record: the device needs the increments, KRL needs the three frames (derived from initial, final and staging), and the user needs the cumulative range to set POSCORR1. No existing type holds these. `ChamferCartesianSample` and `PreparedChamferJob` are deleted because they hold poses that nothing consumes.

The operation, in order:
1. Require `motion.centralMotion()`. Let $T$ = `phaseTimes()[3]` and $\delta = 0.004$ s.
2. Compute $h_{\min}$, the smallest time difference between consecutive saved points from `boundaries()[2]` to `boundaries()[5]` of `motion.points()`. If $h_{\min} < \delta$, fail with "Machining speed is too high for the 4 ms RSI chord tolerance (shortest Generate interval %1 ms)."
3. Let $K = \lceil T/\delta \rceil$, then decrement $K$ if $(K-1)\delta \ge T$, exactly as the old sampler handles roundoff. Fail if $K$ exceeds 200000.
4. Times are $t_k = k\delta$ for $k < K$ and $t_K = T$; the final increment covers the remaining fraction of the last cycle.
5. Seed: `q0 = points[boundaries[2]].joints` and `seed = configuration(q0, solveFK(q0))`.
6. For $k = 0..K$:
   - `pose = centralMotion()->evaluate(t_k)`; fail if empty.
   - `ik = solver.solveTcp(pose->tcp, seed, motion.robot())`; fail with its error and the time.
   - For $k > 0$: joint speed $|\bar{q}_k - \bar{q}_{k-1}|/\delta \le$ `model.joints[i].ratedPayloadSpeedDegreesPerSecond` for each joint.
   - Cartesian acceleration: compute the cycle velocity $\bar{v}_k = (\bar{p}(t_k) - \bar{p}(t_{k-1}))/\delta$ for $k \ge 1$, with $\bar{v}_0 = \bar{0}$ (start at rest). For $k \ge 1$, check $|\bar{v}_k - \bar{v}_{k-1}|/\delta \le$ `timing.cartesianAcceleration`.
   - Euler continuity: $k = 0$ uses `principalEulerDegrees`, and later cycles use `continuousEulerDegrees` against the previous absolute angles. Both are moved unchanged from `rsiposeencoder.cpp`, keeping the ≥ 90° jump guard and the rotation reconstruction check.
   - For $k > 0$, append $\bar{\Delta}_k = q(\bar{c}(t_k) - \bar{c}(t_0) - \bar{S}_{k-1})$, where $q(x)$ = `std::round(x * 1e6) / 1e6` per component. Update $\bar{S}_k$ and the min/max offsets.
   - Set `seed = *ik.solution`.
7. After the loop:
   - Check the terminal stop $|\bar{v}_K|/\delta \le$ `cartesianAcceleration`, because the next cycle is zero.
   - Check that the final joints match `points[boundaries[5]].joints` within 1e-6 degrees (saved winding).
   - Append 50 `V6d::Zero()` increments.
   - Fill `initialCoordinates` $= \bar{c}(t_0)$, `finalCoordinates` $= \bar{c}(t_0) + \bar{S}_K$, and `stagingPoint = centralMotion()->path().stagingPoint()`.
8. Diagnostics: `{"rsiSamplingMs", "motionCycles", "settleCycles", "shortestGenerateIntervalMs"}` merged with `solver.diagnostics()`.

Notes for the implementer:
- $\bar{c}(t_k)$ is the six-vector of TCP position and continuous Euler angles at $t_k$.
- Rotation per cycle needs no separate check; the Euler continuity guard rejects any unresolved branch.
- The TCP speed needs no check: the chord is never longer than the arc, so the cycle speed never exceeds the machining feed.
- Use the C++17 idioms from AGENTS.md and named helper functions or a small worker class in an unnamed namespace, as the old sampler did. No lambdas.

Result: the function exists and compiles. When the user requests a build, the profiler (Milestone 3 updates `chamferprofile.cpp`) reports `motionCycles` ≈ $T/\delta$ and the sampling time.


### Milestone 3: RSI Sampling coordinator, export and status

Goal: pressing RSI Sampling produces the stream, gives it to the device, writes the file and shows a concise status.

Move the coordinator to `src/application/scene/scenemachiningpreparation.h/.cpp`, keeping the class name `SceneMachiningPreparation` and the QML singleton name `App.MachiningPreparation` so `ModeToolBar` and `Main.qml` keep their bindings. Rewrite it around one member:

    struct PreparedRsiStream
    {
      quint32 pathId = 0;
      QString pathName;
      std::shared_ptr<const ChamferMotion> motion; // identity check only
      ChamferRsiStream stream;
    };
    std::optional<PreparedRsiStream> m_prepared;

Requirement record: the stream must survive selection changes (Decision Log), so the coordinator, not the selection, owns which path it belongs to. The motion pointer detects regeneration. No existing owner holds this association.

Behavior:
- `prepare()` requires a selected saved path that is `compatible()`, and no sampling already running. Drop the old "finish generating" and "stop Dry Run" gates: sampling reads an immutable motion and does not conflict with either.
- It copies the path's `shared_ptr<const ChamferMotion>` and runs `std::async(std::launch::async, &sampleChamferRsiStream, ...)`. Since the function takes a reference, use a named unnamed-namespace wrapper that takes the shared pointer by value, so the worker owns its input. Collect the result with the existing 30 ms `QTimer` polling pattern and log the diagnostics with `qInfo().noquote()`.
- On collection, reject the result unless `isCurrent(m_pending)` holds: the pending path still exists, is compatible and holds the same motion pointer.
- On success:
  1. Build the five-row payload on the GUI thread with the unnamed-namespace helper `sharedDataPayload(const ChamferRsiStream&, const ChamferTimingParameters&)`.
  2. Write it atomically with `QSaveFile` (direct-write fallback disabled, `C:/RoboCrapJobs` created if missing).
  3. Store `m_prepared` and emit `correctionsReady(stream.increments)`.
  4. Emit `preparedChanged`.
  If the write fails, keep no stream, emit an empty `correctionsReady`, and show the write error: the robot cannot run without the file, and the device must not hold a stream whose frames KRL cannot read.
- The five rows, using `QString::number(value, 'f', 6)`, `;` separators and CRLF line ends:
  - Row 1: `stagingPoint` XYZ with `initialCoordinates` ABC.
  - Row 2: `initialCoordinates`.
  - Row 3: `finalCoordinates`.
  - Row 4: `transferFeed / 1000`.
  - Row 5: `transferAcceleration / 1000`.
- Invalidation clears `m_prepared`, emits an empty `correctionsReady`, deletes `C:/RoboCrapJobs/shared_data.txt` (ignoring a missing file) and emits `preparedChanged`. It happens when `SceneMachining::availabilityChanged` arrives and `isCurrent(*m_prepared)` fails, at a new request, at construction and at shutdown.
- QML properties: `canPrepare`, `unavailableReason`, `preparing`, `error` and `summary` (see Decision Log). Remove the others (`jobId`, `controllerDuration`, `cyclePeriod`, `phaseTimes`, `correctionCount`, `centralTimeScale`, `sampleCount`, `encodingReady`, `encodingUnavailableReason`, `sharedDataExported`).

Delete `src/scene/chamfershareddata.*` and the old `src/scene/scenemachiningpreparation.*`. Update `src/application/CMakeLists.txt` source paths, and update `src/application/backendqmltypes.h` to include the new header path. In `src/application/main.cpp`:
- Connect `SceneMachiningPreparation::correctionsReady` to `RsiDevice::setCorrections` with `Qt::QueuedConnection`.
- Register `QVector<V6d>` with `qRegisterMetaType<QVector<V6d>>("QVector<V6d>")` and remove the `PreparedChamferJobPtr` registration.
- Remove the `FtsDevice::dataSampleHFReady → RsiDevice::setForce` connection: force is out of scope and `setForce` stored a value nothing read.

In `src/application/qml/Main.qml`, rewrite the RSI status label text. Inspect the label and its layout first, per the QML contract:
- While sampling: "Sampling RSI…".
- On error: the error.
- When prepared: the C++ `summary`, one footer line: `RSI: <path name> · <cycles> cycles · <duration> s · offset up to <mm> mm, <deg>°`. The two offsets are the largest cumulative |X/Y/Z| and |A/B/C| correction, the values POSCORR1 must cover. Milestone 4 appends `Controller: <App.Hub.device("rsi").data.rsiState ?? "idle">`.
- Otherwise: the unavailable reason, or "Ready for RSI Sampling."

Update `src/application/chamferprofile.cpp` so `previewOnly=false` calls `sampleChamferRsiStream(*motion)` and reports its diagnostics, error and `motionCycles` instead of `PreparedChamferJob`.

Result: in a disconnected application, RSI Sampling fills the status line and writes the file; changing the selection does not clear it; deleting or regenerating the path clears it and removes the file.


### Milestone 4: RSI device streaming and wire protocol

Goal: the device answers the controller as described in the Decision Log.

Move the device to the canonical Devices layout: `src/devices/include/devices/rsi/rsidevice.h` and `src/devices/rsi/rsidevice.cpp`. The wire codec lives in that cpp's unnamed namespace (see Decision Log); the protocol sketch below describes its behavior. Update `src/devices/CMakeLists.txt`: remove the `../network/rsi` sources and the PRIVATE `${PROJECT_SOURCE_DIR}/src` include, and link `RoboCrapGeometry` publicly instead of `RoboCrapTrajectories`. Then check that FTS and PLC sources compile without Trajectories; they include only Qt and their own headers. Update `main.cpp` to include `devices/rsi/rsidevice.h`.

Protocol (`rsiprotocol.h`, namespace `RsiProtocol`, no templates):

    struct Request { quint64 ipoc = 0; bool start = false; qint64 result = 0; };
    struct Reply { quint64 ipoc = 0; V6d correction = V6d::Zero(); bool stop = false; qint64 result = 0; };
    std::optional<Request> decode(const QByteArray& datagram);
    QByteArray encode(const Reply& reply);
    bool acceptsSender(const QHostAddress& configured, quint16 learnedPort,
                       const QHostAddress& sender, quint16 senderPort);

Requirement record: the installed XML exchanges Start and Result in requests and Stop and Result in replies, and the old types carry neither. `acceptsSender` keeps four parameters because it compares two distinct endpoints, each an address and a port.

Decoding rules:
- Keep the existing well-formedness, IPOC digit, RIst/AIPos/MACur six-value checks and trailing-garbage rejection.
- Additionally require exactly one `<Start>` whose text is `0` or `1`, and exactly one `<Result>` whose text is a decimal integer.
- On failure, the device logs the first 200 bytes once per connection, so a different BOOL spelling from the controller is visible immediately. The robot is still standing, because Start is reported after `RSI_ON` and before `RSI_MOVECORR`.

Encoding writes `<Sen Type="ImFree">`, `<RKorr X Y Z A B C>` with `QString::number(value, 'f', 6)`, then `<Stop>0|1</Stop>`, `<Result>n</Result>` and `<IPOC>`, using `QXmlStreamWriter` as today.

Device (`RsiDevice : AbstractDevice`):
- Keep the invokables `connect(QVariantMap)` and `disconnect()`, used by `Network.qml`.
- Add the public slot `setCorrections(const QVector<V6d>& increments)`.
- Remove `generateTrajectory`, `startStreaming`, `stopStreaming`, `setForce`, `setPreparedJob`, the cooldown timer and the demo offset state.

State on ControlIO:

    enum class RunState { Idle, Streaming, Finished, Stopping };
    QVector<V6d> m_corrections, m_pending;
    bool m_hasPending = false;
    qsizetype m_index = 0;
    RunState m_state = RunState::Idle;
    qint64 m_result = 0;
    quint64 m_lastIpoc = 0;

`onReadyRead` drains all pending datagrams. For each, it checks the sender, decodes, and keeps the last valid request; it counts the extra valid requests and logs "answered newest of N queued requests" when N > 1. It then calls `Reply reply(const Request&)` once and writes the encoded reply to the learned peer.

`reply` implements the state machine:
- Start = 0: apply a pending stream, go to Idle, set index and result to 0, reply zeros with Stop 0 and Result 0.
- Idle with Start = 1:
  - If `m_corrections` is empty: go to Stopping with result 0 (abort) and log "no sampled stream".
  - Otherwise go to Streaming.
- Streaming: reply `m_corrections[m_index++]` with Result 0; when the index reaches the end, go to Finished.
- Finished: zeros with Result 1; when `request.result == 1`, go to Stopping with result 1.
- Stopping: zeros with Stop 1 and the latched result.

`setCorrections` stores into `m_pending` and sets `m_hasPending` when the state is not Idle; otherwise it replaces `m_corrections` directly. It publishes only on state changes, through `stateReady`, with the keys `rsiState` (`"idle"`, `"streaming"`, `"finished"`, `"stopping"`) and `rsiCorrections` (size of the active stream). It never publishes per cycle. Log each state transition once with `qInfo().noquote()` and the existing `[INFO]`/`[ERROR]` labels.

Result: the device can be exercised against a local stand-in (see Validation) and produces the documented packet sequence.


### Milestone 5: removal, controller files and documentation

Delete:
- `src/pathgeneration/` (all files). `src/network/` was already deleted in Milestone 4.
- `src/application/qml/Views/Dashboard/KukaPanel/KukaPanel.qml` and its line in `src/application/CMakeLists.txt`.
- `resources/files/rsi/RSI_request_example.xml` and `RSI_responce_example.xml`. (`RSI_CorrectionLimits.example.json` was already deleted by the user.)

Then:
- Remove `frozen_rsi_sources` and its `set_source_files_properties` from `src/trajectories/CMakeLists.txt`.
- Remove the `${PROJECT_SOURCE_DIR}/src` and `${PROJECT_SOURCE_DIR}/src/scene` include lines from `src/application/CMakeLists.txt`, after grep confirms no remaining include needs them.
- Search for callers of `Plane::fromJsonFile`, `Cylinder::setSurfacePose`, `Cylinder::surfacePose` (with `m_surfacePose`), `Pose::offsetPose` and `readJsonPoints`, and remove each that no longer has a caller outside its own definition.

Controller files in `resources/files/rsi/`:
- Copy `D:\RSI_Hole.rsi`, `D:\RSI_Hole.rsi.xml` and `D:\RSI_Hole.rsi.diagram` in unchanged.
- Replace `RSI_Hole_EthernetConfig.xml` with the installed name `RSI_EthernetConfig.xml`; its content is the same as `D:\RSI_EthernetConfig.xml`.
- After the user raises MaxRotAngle to 3600 in RSI Visual, the user re-exports and the three context files are copied again. Do not hand-edit the native `.rsi*` files.

In `RSI_Hole.src`, change `$BASE=BASE_DATA[1]` to `$BASE=$NULLFRAME`, and swap lines 84 and 85 so `Completed=($SEN_PINT[2]==1)` runs before `$SEN_PINT[1]=0`. `RSI_Hole_DIAGRAM.md` is already deleted; the native diagram replaces it. Rewrite `RSI_Hole_CONTRACT.md` to the final contract: signals, the PC state machine, the five-row file, invalidation and file removal, and the known unknowns (Precision/Timeout, pause behavior).

Documentation:
- `docs/AGENT_CONTRACTS.md`: in "Build and modules", Devices links Geometry, and no frozen RSI or legacy paths remain. In "Machining and playback", RSI Sampling is in scope; summarize the stream, ownership, wire and invalidation contracts, and remove the limits-file and frozen-path statements. Also update the acceleration statement: the Cartesian bound is `$ACC_MA.CP` (the programmed CP ceiling, not RSI-monitored), with no joint-acceleration rejection.
- `docs/ARCHITECTURE.md`: replace the RSI rows and §5/§7 RSI statements.
- `AGENTS.md`: change the scope bullet "Do not change or run RSI preparation unless the user explicitly reopens it" to state that nominal RSI Sampling and streaming are in scope and force correction is not. Live hardware still needs explicit authorization.
Result: `git grep -n "pathgeneration\|network/rsi\|PreparedChamferJob\|rsicorrectionlimits\|RsiHoleProtocol\|KukaPanel"` returns only documentation history lines.


### Milestone 6 (user-run): first nominal T1 run

This milestone is executed by the user on the robot; this plan authorizes no hardware action.

Preconditions:
- MaxRotAngle is raised and re-exported.
- `RSI_Hole.src` with the two fixes and the three context files are on the controller.
- The application is connected to the RSI endpoint in the Network panel (local 192.168.1.100:5555, peer 192.168.1.200).
- A path is generated and sampled, and the status shows the stream and "Exported C:/RoboCrapJobs/shared_data.txt".

During the run the operator keeps Start pressed; on any doubt, emergency stop. Releasing Start is not supported yet: if it happens, cancel the program and restart from HOME.

Observe:
1. After `RSI_ON` the status shows "Controller: idle".
2. It changes to "streaming" when MOVECORR starts, then "finished" and "stopping".
3. KRL performs the return LIN and PTP HOME.
4. The robot traces lead-in, the full hole edge and lead-out at the machining feed.

Record in Surprises & Discoveries:
- Whether decoding of `<Start>` succeeded (no "[ERROR] … decode" lines).
- Any "queued requests" log lines.
- Whether the motion stalled or was too short. If the robot does not move at all, suspect `Precision` truncation.
- The final TCP pose on the smartPAD compared with row 3 of `shared_data.txt`.
- The `$ACC_CAR_MAX` and `$POS_ACT` readings, if the user takes them.


### Milestone 7 (prototype): pause and resume on Start release

Proceed only after Milestone 6, and only with the user's answers to two observations from a deliberate, operator-controlled T1 trial:
1. Do controller packets keep arriving while the program is stopped? The device log shows IPOC lines or state while stopped.
2. After pressing Start again, does the robot continue smoothly or jump?

Proposed mechanism, to be confirmed with the user before implementation. It needs controller changes the user owns:
- The submit interpreter (`sps.sub`, which keeps running while the robot program is stopped) writes `$SEN_PINT[3]` = 1 while `$PRO_STATE1 == #P_ACTIVE`, otherwise 0.
- A new SEN_PINT3 object feeds the free ETHERNET1 In3.
- A new SEND element `<ELEMENT TAG="Run" TYPE="BOOL" INDX="3" />` carries it to the PC.
- The PC treats Run = 0 during Streaming as "hold": zero corrections, index unchanged.

Known residual risk: increments sent in the one or two cycles before Run drops may already be accumulated, and are executed on resume. This milestone must record what is observed and either adopt or discard the mechanism, with the reason, in the Decision Log.


## Concrete Steps

All commands run from `E:\Qt\QtProjects\RoboCrap` in Git Bash unless stated otherwise. Inspect before and after every milestone:

    git status --short
    git diff --stat
    git diff --check

For each new untracked text file, check whitespace without staging:

    git diff --no-index --check -- /dev/null <path>

Record line counts at milestone start and finish (physical, then nonblank) for every touched file:

    wc -l <files>
    grep -cv '^\s*$' <file>

Search for leftovers after Milestone 5:

    git grep -n -E "pathgeneration|network/rsi|PreparedChamferJob|rsicorrectionlimits|RsiHoleProtocol|KukaPanel|setPreparedJob|generateTrajectory"

Builds, lint and the profiler run only when the user explicitly requests them (AGENTS.md). In that case, find the existing build directory and check its cache first:

    grep -E "CMAKE_HOME_DIRECTORY|CMAKE_CXX_COMPILER|Qt6_DIR|USE_HOT_RELOAD|FAST_QML_BUILD" <buildDir>/CMakeCache.txt
    cmake --build <buildDir> --target robocrap --parallel
    cmake --build <buildDir> --target robocrap_qmllint

For an optional measurement, write a profiler input such as `codex_plans/rsi_sampling_profile_input.json`. It contains `"plane": "../resources/json/edge_cyl_plane/XP.json"`, `"cylinder": "../resources/json/edge_cyl_plane/XC.json"`, `"robot": "../resources/json/kr10.json"`, `"previewOnly": false`, `"tcpInCollet"` with the six applied Machining EE values, and `"settings"` with the editor defaults (`flipAxis` false, `chamferSize` 0.5, `chamferAngle` 45, `stagingDistance` 10, both clearances 5, both spans 30, `machiningFeed` 5, `accelerationDistanceFraction` 0.3, `transferFeed` 10, `transferAcceleration` 20, `auxiliaryScale` 1.0; the profiler passes it straight into `ChamferTimingParameters::auxiliaryScale`, whose range is (0, 1], not the editor's percent). Run:

    <buildDir>/robocrap.exe --profile-chamfer codex_plans/rsi_sampling_profile_input.json <scratch>/rsi_profile.json

Expect a report containing `"motionCycles"` near $T/0.004$ and `"rsiSamplingMs"`. Record both numbers here.


## Validation and Acceptance

Source acceptance (always): the diff review in AGENTS.md "Mandatory change procedure" finds no lambda, template, exception, output parameter or parameter-count violation in changed declarations. Every new helper, type or field has its requirement record in this plan. `git diff --check` is clean, and the Milestone 5 grep finds no leftover references.

Behavioral acceptance in a disconnected application (when the user runs a build):
1. Import XP.json as a plane and XC.json as a cylinder, intersect, select the edge, apply Machining EE, and Generate.
2. Select the new path and press RSI Sampling. The footer shows `RSI: <name> · N cycles · T s · offset up to … mm, …°`, with N = `motionCycles` + 50.
3. `C:\RoboCrapJobs\shared_data.txt` has five CRLF rows. Rows 1 and 2 have identical ABC; row 1 XYZ equals $P_s$ and row 2 equals the lead-in start. Row 4 is 0.010000 and row 5 is 0.020000 for the default 10 mm/s and 20 mm/s².
4. Select another object: the status still shows the stream.
5. Delete the path, or change the TCP calibration: the status returns to the unavailable reason, and the file is gone.

Wire acceptance without the robot (optional, user-requested): a throwaway script in the session scratch directory, never added to the repository, can play the controller role. Configure the RSI device in the Network panel with local 127.0.0.1:5555 and peer 127.0.0.1, then send from 127.0.0.1:
- `<Rob TYPE="KUKA"><Start>0</Start><Result>0</Result><IPOC>1</IPOC></Rob>` — expect a reply with zero RKorr, Stop 0, Result 0, IPOC 1.
- The same with Start 1 and increasing IPOC — expect the stream's increments in order, then zero corrections with Result 1.
- With Result 1 — expect Stop 1.
- With Start 0 — expect Stop 0 and Result 0, and the next Start 1 restarts from the first increment.
- Without a sampled stream, Start 1 — expect Stop 1 with Result 0.

Robot acceptance (user-run, Milestone 6): the motion and status sequence described there, KRL completing the return moves, and no RSI error message on the smartPAD.


## Idempotence and Recovery

Every code milestone is additive first, then subtractive. If a milestone stops halfway, the old coordinator and device still exist until Milestone 5 deletes them, so the tree can be inspected in either state. RSI Sampling can be repeated at any time; each run replaces the stream and rewrites the file atomically. A failed sampling leaves no stream and no file. Deleting `shared_data.txt` is safe because RSI Sampling recreates it. Nothing here touches git history; the user decides when to commit.


## Artifacts and Notes

Baseline at plan time (2026-10-17), physical/nonblank lines of the RSI C++ that this plan replaces or deletes: `src/pathgeneration/**`, `src/scene/**` and `src/network/rsi/**` total 2904/2493. Largest files: `chamfercartesiansampler.cpp` 411/384, `rsidevice.cpp` 361/270, `rsiposeencoder.cpp` 307/282, `rsiholeprotocol.cpp` 271/252, `rsipath.cpp` 249/177, `scenemachiningpreparation.cpp` 208/191, `rsicorrectionlimits.cpp` 163/149. `KukaPanel.qml` is 58/42.

The target is a total below 1000 physical lines for the new stream, coordinator, device and protocol files together. This is an estimate to check against the finished counts, not a constraint that overrides required behavior.

Controller facts used (from `resources/files/KR C4`): `$ROBROOT` null; `$ACC_MA` = {CP 10 m/s², ORI1 1000 °/s², ORI2 1000 °/s²}; `$VEL_MA.CP` 3 m/s; `$VEL_CP_T1` 0.229 m/s; `$ACC_CAR_STOP` FALSE; `$ADAP_ACC` #STEP2; joint speeds 220/210/270/381/311/492 °/s.


## Interfaces and Dependencies

At the end of the plan these declarations exist; anything not listed here is deleted.

- `src/trajectories/include/trajectories/chamferrsistream.h`: `ChamferRsiStream`, `ChamferRsiStreamResult` (stream and error, no diagnostics), `sampleChamferRsiStream(const ChamferMotion&)`.
- `src/trajectories/include/trajectories/chamfercentralmotion.h`: `const ChamferPath& ChamferCentralMotion::path() const`.
- `src/trajectories/chamfer/chamfertcpsolver.h`: `chamferJointPositionError(const V6d&, const Kr10KinematicModel&)`, `ChamferTcpIkResult`, `ChamferTcpSolver(const ChamferRobotSetup&)`, `kinematics() const`, `solveTcp(const M4d&, const Kr10Kinematics::IkSolution&) const`.
- `src/application/scene/scenemachiningpreparation.h`: `SceneMachiningPreparation` with the QML properties `canPrepare`, `unavailableReason`, `preparing`, `error` and `summary`, plus `Q_INVOKABLE void prepare()`, `void shutdown()` and the signal `correctionsReady(const QVector<V6d>&)`.
- `src/devices/include/devices/rsi/rsidevice.h`: `RsiDevice` with the public `struct Reply`, `Q_INVOKABLE connect(const QVariantMap&)`, `Q_INVOKABLE disconnect()` and `public slots: setCorrections(const QVector<V6d>&)`. The wire codec is private to `src/devices/rsi/rsidevice.cpp`.
- Dependencies: Trajectories → Kinematics → Geometry, unchanged. Devices → Geometry and Qt6::Network, no longer Trajectories. Application → all, unchanged. No new external libraries, URIs or minimum versions.


Revision note (2026-10-17, Claude): initial plan, written after the user reopened RSI for a nominal T1 run. It records the user's answers: no RSI limits, override irrelevant, WORLD base, TOOL_DATA[6] equals the application TCP, 50 settle cycles, controller limits for animation and sampling, and a mandatory inverse quintic. It also records the critical review of the old RSI code that motivates the rewrite.

Revision note (2026-10-17, Claude): "No limits" means no POSCORR1 limits only, as the user clarified. Corrected the description of `$ACC_MA.CP`: it is the KSS ceiling for programmed CP path acceleration, not a limit the controller monitors during RSI. Recorded that joint acceleration limits cannot be derived from the supplied machine data.

Revision note (2026-10-17, Claude): Milestones 2–4 implemented. Their deviations from the original text (two-parameter `solveTcp`, `summary` property, single availability connection, codec in the device cpp, `src/network` removed early) are recorded in the Decision Log.

Revision note (2026-10-17, Claude): rebased onto `origin/main` `3e99496` and adapted to the code-reduction base (see the first Decision Log entry). The profiler-based measurement steps in Concrete Steps no longer apply, because upstream removed `--profile-chamfer`.
