# Unified motion module: one PTP/LIN/CIRC/curve pipeline from motion segments to timed joint angles

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`,
`Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is
maintained in accordance with `PLANS.md` at the repository root.

Three rules override `PLANS.md` for this plan. First, the implementing agent never runs a build,
CMake configure, QML lint, the application or any test; when a milestone needs a build or a run,
the agent stops, asks the user to build or run exactly what the milestone names, and continues
from the user's reported result. Second, no git staging or commits are made unless the user asks
for them (`AGENTS.md`). Third, no project tests or test harnesses are added (`AGENTS.md`). Every
`AGENTS.md` rule applies to every line written for this plan; read its sections "Mandatory C++
style" and "Mandatory code reduction" before every milestone.


## Purpose / Big Picture

The application animates two robot motions: the chamfer machining motion (Machining mode,
Generate Path, Dry Run) and the probe calibration motion (Measuring mode, Calibration submode).
Both are sequences of KRL-like motion segments: PTP (a synchronized joint motion), LIN (a straight
Cartesian TCP motion, with a trapezoidal speed profile or at constant speed), CIRC (a circular-arc
TCP motion) and, for chamfer, a motion along a parametric curve (the lead-in arc, the machining
ellipse and the lead-out arc, always with an acceleration ramp, a cruise and a deceleration ramp).
For every segment the program fixes the times of the animation points before inverse kinematics
(IK, the calculation of joint angles for a TCP pose), solves IK for each point, checks joint
limits, speeds and accelerations, and stores timed joint angles that Dry Run plays.

Today this pipeline exists twice and in pieces. The chamfer motion (`src/trajectories/chamfer/
chamfermotion.cpp`) and the probe motion (`src/trajectories/probe/probetouchmotion.cpp`) each
chain their own segments, time their own PTPs, prepare their own joint splines and evaluate their
own playback; the curve has a third IK loop and its own rate checks
(`src/trajectories/chamfer/chamfercentralvalidation.cpp`). After this plan there is one module,
`src/trajectories/motion`, that takes a segment of kind PTP, LIN, CIRC or curve and returns timed
joint angles with one set of checks, and one container of timed joint angles that Dry Run plays
for any motion. A new motion (for example a later scanning path) is then built by listing its
segments, without writing another IK loop, timing code or playback evaluator.

The user-visible behavior must not change: after every milestone, chamfer Generate on the sample
edge prints exactly the baseline console line, calibration Generate prints exactly its baseline
line, and both Dry Runs look and behave as before. Only error texts of failing Generate runs
change, as recorded in the Decision Log.


## Progress

- [x] (2026-10-26) Milestone 0: baselines reported by the user before any edit:
  `Chamfer preparation: 375 points, 25.299220225 s movement` and `Probe calibration preparation:
  31 touches, 947 points, 246.191824177 s movement, minimum transfer clearance 3.0000 mm, minimum
  strut clearance 3.2588 mm`. The user confirmed the error-text and first-failure decision
  (Decision Log, fourth entry) the same day.
- [x] (2026-10-26) Milestone 1 implemented (completed; the user's build and checks remaining):
  `trajectories/timedmotion.h` (the three moved shared types, `MotionInterpolation`,
  `MotionSegment`, `MotionPhaseRange`, `TimedMotionData`, `TimedMotion`) and
  `motion/timedmotion.cpp` (`TimedMotion::evaluateJoints`); `ChamferMotion` and `ProbeTouchMotion`
  store a `TimedMotion` and expose `timed()`; both `evaluateJoints`, `ProbeMotionSegment`,
  `ProbeTouchMotionData`, `ProbeTouchMotion::segments()`, `ChamferMotion::preparePlaybackSplines`,
  the `PlaybackMotion` variant and `chamferPhaseText` are removed; `ScenePlayback::start` takes
  `std::shared_ptr<const TimedMotion>`, created by `SceneMachining::dryRun` and
  `SceneCalibration::dryRun` with the aliasing constructor; the probe path drawing chooses colours
  from `MotionSegment::interpolation`.
  Line counts, physical/nonblank. C++ before: chamfermotion.h 78/68, chamfermotion.cpp 336/312,
  probetouchmotion.h 67/56, probetouchmotion.cpp 242/220, jointsegments.h 118/101,
  sceneplayback.h 80/72, sceneplayback.cpp 154/136, sceneobject.cpp 255/234, scenemachining.cpp
  325/299, scenecalibration.cpp 273/244, total 1928/1742. After: 38/32, 315/290, 54/44, 211/190,
  118/101, 76/69, 125/111, 255/234, 327/301, 275/246, plus the new timedmotion.h 92/81 and
  timedmotion.cpp 49/46, total 1935/1745 (+7/+3). Build inputs: src/trajectories/CMakeLists.txt
  23/23 to 25/25. The 35 lines of the three moved types appear in both counts; without them the
  container and its single evaluator (about 100 lines) replace the two evaluators, the playback
  variant and the spline preparation member (about 95 lines). The reduction of this plan comes in
  Milestones 2 and 3.
  Compliance pass: no lambdas, templates, exceptions or output parameters; results return through
  `std::optional` values and the internal `PlaybackResult`. Signatures: `ChamferMotion`'s private
  constructor went from four parameters to three (path, timed motion, boundaries: the timed motion
  now carries the robot and the points); `TimedMotion(robot, data)`,
  `ProbeTouchMotion(timed, clearances)`, `playbackMotion(data, robot)` and
  `TouchMotionBuilder::clearanceError(phase, first)` have two; `evaluateJoints(seconds)` one.
- [x] (2026-10-26) Milestone 1 acceptance: the user built the application and reported
  `Chamfer preparation: 375 points, 25.299220225 s movement` and `Probe calibration preparation:
  31 touches, 947 points, 246.191824177 s movement, minimum transfer clearance 3.0000 mm, minimum
  strut clearance 3.2588 mm`, identical to Milestone 0, and all Dry Run checks as passed.
- [x] (2026-10-26) Milestone 2 implemented (completed; the user's build and checks remaining):
  `ChamferTcpSolver` moved unchanged to `src/trajectories/motion/tcpsolver.h` (its includes now
  name `jointsegments.h` and `trajectories/timedmotion.h`; the four includers updated;
  `chamfer/chamfertcpsolver.h` deleted); new `motion/motionbuilder.{h,cpp}` with `PathSchedule`,
  `PathTarget`, `TimedPath`, `LinearPath` and `MotionBuilder` (`appendPtp`, `appendPath` with the
  start-pose check, per-point `solveTcp` and joint-speed check, the grouped rate inspection
  `rateIssue` moved from the curve validator, spline, all-or-nothing commit); the probe
  `TouchMotionBuilder` keeps only the step list, the wrist-unwind seed, the profile choice and
  `clearanceError`.
  Line counts, physical/nonblank. Before: chamfertcpsolver.h 192/177, probetouchmotion.cpp 211/190,
  total 403/367. After: tcpsolver.h 192/177, motionbuilder.h 82/72, motionbuilder.cpp 194/178,
  probetouchmotion.cpp 164/145, total 632/572 (+229/+205). Build inputs:
  src/trajectories/CMakeLists.txt 25/25 to 27/27. The growth is temporary by design: chamfer still
  uses `solveLinear`, `linearRateError`, `samplePtp`, `CentralValidator` and `HomeTimer`, which
  Milestone 3 deletes together with the chamfer appending code; the probe motion already lost 47
  lines.
  Compliance pass: no lambdas, templates, exceptions or output parameters; `TimedPath` is an
  abstract class with two implementations planned (`LinearPath` now, `CentralPath` in
  Milestone 3). Three-parameter functions, both internal to motionbuilder.cpp:
  `accelerationError(before, after, timing)` (the two compared secants and the bound; a member
  would publish `Rates` in the header) and `atTime(label, message, seconds)` (the three parts of
  one error text; joining label and message at five callers would repeat the separator). All
  other new signatures have at most two parameters.
- [x] (2026-10-26) Milestone 2 acceptance: the user built the application and reported
  `Chamfer preparation: 375 points, 25.299220225 s movement` and `Probe calibration preparation:
  31 touches, 947 points, 246.191824177 s movement, minimum transfer clearance 3.0000 mm, minimum
  strut clearance 3.2588 mm`, identical to Milestone 0, and the calibration Dry Run as passed.
- [x] (2026-10-26) Milestone 3 implemented (completed; the user's build and checks remaining):
  `CentralPath` (schedule from the curvature spans with the phases "Lead-in", "Machining",
  "Lead-out"; targets from the cached `evaluateTarget` with progress 1 at phase ends) in
  `chamfercentralvalidation.{h,cpp}`; `ChamferCentralPreparation::prepare` is public and
  `validate`, `CentralValidator`, `SampleResult`, `Rates`, `ChamferCentralJointSample`,
  `ChamferCentralValidationData` and `ChamferCentralValidationResult` are removed;
  `MotionCompiler::compile` is one `MotionBuilder` sequence (PTP, LIN, curve, LIN, PTP) held in
  `std::optional<MotionBuilder> m_motion`, with the boundaries read from the builder's seven
  phases; `HomeTimer`, `appendPtp`, `appendTransfer`, `playbackMotion`, `PlaybackResult`,
  `centralPhaseName` and `ChamferMotionPhase` are removed; the PTP sampling loop moved from
  `ChamferTcpSolver::samplePtp` into `MotionBuilder::appendPtp`, and `samplePtp`, `solveLinear`,
  `linearRateError` and `LinearSamples` are removed from `tcpsolver.h`.
  Line counts, physical/nonblank. Before: chamfermotion.cpp 315/290, chamfercentralvalidation.cpp
  347/319, chamfercentralvalidation.h 63/54, tcpsolver.h 192/177, motionbuilder.h 82/72,
  motionbuilder.cpp 194/178, total 1193/1090. After: 160/142, 230/212, 53/46, 101/90, 82/72,
  201/185, total 827/747 (−366/−343).
  Compliance pass: no lambdas, templates, exceptions or output parameters. `CentralPath` borrows
  the worker-local preparation, which outlives the compile call; `MotionCompiler` owns the
  candidate builder (`m_motion`), so `appendTransfer(side, label)` and `appendCentral()` operate on
  their own object's state with at most two parameters. The removed "Missing prescribed central
  trajectory." check cannot fail any more: a successful compile has prepared the central motion.
  The curve's former joint-continuity check ("Lead-in does not retain the transfer's joint
  configuration.") is replaced by the builder's start-pose check, because the curve now starts
  from the transfer's stored last point and seed.
- [x] (2026-10-26) Milestone 3 acceptance: the user built the application and reported
  `Probe calibration preparation: 31 touches, 947 points, 246.191824177 s movement, minimum
  transfer clearance 3.0000 mm, minimum strut clearance 3.2588 mm` and `Chamfer preparation:
  375 points, 25.299220225 s movement`, identical to Milestone 0, and both Dry Runs as passed.
  RSI Sampling was not checked (no RSI socket available at the time); its inputs are unchanged in
  meaning, and it remains to be run once when the socket is available.
- [x] (2026-10-26) Milestone 4 (completed): `docs/AGENT_CONTRACTS.md` "Machining and playback"
  (motion module files, `MotionBuilder` and its check order, the error format and point-order first
  failure, `TimedMotion` playback through the aliasing handle, `ChamferTcpSolver` in
  `motion/tcpsolver.h`, the path rate inspection replacing the central and LIN wording) and
  `docs/ARCHITECTURE.md` (3.2 `ScenePlayback`, 3.3 point schedule, whole chamfer motion, new
  motion-module row, shared numerics and probe motion rows, 4 shared immutable data, 5.2 and 5.5
  Dry Run traces).
  Final compliance pass over every file the plan changed: no lambdas, project templates, throw/catch
  or output parameters (text search plus the per-milestone reviews); no four-parameter signature;
  the three-parameter functions are recorded with their reasons (`ChamferMotion`'s private
  constructor, `accelerationError`, `atTime`, and the unchanged `transferProfile`).
  Line counts against the Milestone 0 baseline, physical/nonblank, for the 15 baseline files plus
  the new timedmotion.h, timedmotion.cpp, motionbuilder.h and motionbuilder.cpp (tcpsolver.h
  counted in place of chamfertcpsolver.h): 3136/2851 before, 3006/2716 after (−130/−135). Build
  inputs: src/trajectories/CMakeLists.txt 23/23 to 27/27 (two new sources, two new headers, one
  moved header). The reduction is smaller than the 150–250 lines estimated in `Artifacts and
  Notes`: the 35 lines of moved shared types count once, the single evaluator and builder are about
  330 lines, and they replace about 460 lines of duplicated PTP timing, LIN and curve sampling,
  rate checks, spline preparation, evaluators and the playback variant.
  RSI Sampling still needs one run on the sample edge when the RSI socket is available.
- [x] (2026-10-26) Leftover review of every touched file (user request): removed `smoothVelocity`
  (its only caller was `HomeTimer`), the `profile` and `startTime` members of `LinearSchedule` and
  the `startTime` parameter of `linearSchedule` (only the deleted `solveLinear` read them), the
  unused `<cmath>` in chamfermotion.cpp and probetouchmotion.cpp, `<utility>` in chamfermotion.h
  (its `std::pair` user `evaluateJoints` is gone) and the `chamfermotion.h` include of
  chamfercentralvalidation.h (its `ChamferRobotSetup` user `validate` is gone); corrected two stale
  comments (jointsegments.h purpose line, `jointSpeed` resolved by `ratedRobotSetup`). Line counts
  of the eight edited files 1162/1045 to 1150/1034 (−12/−11); the plan's files now total
  2994/2705 against the 3136/2851 baseline (−142/−146). Not built yet.


## Surprises & Discoveries

None yet.


## Decision Log

- Decision: the module is `src/trajectories/motion` inside `RoboCrapTrajectories`, not a new CMake
  target. Its only published header is `src/trajectories/include/trajectories/timedmotion.h`
  (the stored motion and the shared robot/timing/point types); the builder and the path interface
  stay internal headers under `src/trajectories/motion`.
  Rationale: the chamfer, probe and RSI code already live in this library and link Kinematics;
  a separate target would add CMake ownership and link edges without a consumer outside the
  library. The application needs only the stored motion, for playback.
  Date/Author: 2026-10-26, agent.
- Decision: `ChamferTimingParameters`, `ChamferRobotSetup` and `ChamferJointPoint` move unchanged
  (same names, members and defaults) from `trajectories/chamfermotion.h` into
  `trajectories/timedmotion.h`; `chamfermotion.h` includes the new header. They are not renamed.
  Rationale: the module must not include a chamfer header, so the shared types move to the module.
  Renaming them would touch every application consumer without reducing code; `AGENTS.md` keeps
  public names unless the task requires a change.
  Date/Author: 2026-10-26, agent.
- Decision: a segment's Cartesian path is an abstract class `TimedPath` with two virtual functions
  (`schedule()` and `target(index, seconds)`), implemented by `LinearPath` (LIN and CIRC, in the
  module) and `CentralPath` (the chamfer curve, in `src/trajectories/chamfer`).
  Rationale: the builder must not depend on chamfer code, and `AGENTS.md` forbids templates and
  lambdas; a `std::variant` of path types inside the builder would make the module include the
  chamfer curve. Two current implementations justify the interface.
  Date/Author: 2026-10-26, agent.
- Decision: one IK and check sequence for every Cartesian point: `ChamferTcpSolver::solveTcp`
  (seeded IK, joint limits, flange and TCP residuals against 0.0001 mm and 0.000001
  rotation-matrix entries), the joint-speed check of each new interval immediately after its IK,
  and after the segment the grouped rate inspection of the current curve code (grouped joint speeds,
  XYZ acceleration against `cartesianAcceleration`, the stop at the segment end). LIN and CIRC
  therefore also get the XYZ acceleration check, and their first failure is reported in point
  order. `solveLinear`, `linearRateError`, `samplePtp` and the curve's `CentralValidator` loops are
  removed.
  Rationale: the two existing sequences check the same quantities with different orders and texts;
  keeping both would keep the duplication this plan removes. Successful motions do not change:
  every chamfer and probe LIN in use has XYZ accelerations of at most about 26 mm/s² against the
  10000 mm/s² bound, and the same IK, limits and residual tolerances apply. The visible changes are
  the texts of failing Generate runs, which now read "<phase label>: <message> (t=<s> s)", and,
  when one LIN has both an IK failure and an earlier speed violation, which of the two is reported.
  `AGENTS.md` requires preserving the first failure; this plan changes it deliberately, and the
  user must confirm the change before Milestone 2 (see the question at the end of Milestone 0).
  The user confirmed all three parts on 2026-10-26: the error format, the point-order first
  failure and the XYZ acceleration check for LIN and CIRC.
  Date/Author: 2026-10-26, agent; confirmed by the user 2026-10-26.
- Decision: point times are summed in segment order (segment start time plus local time). The
  chamfer motion today adds the HOME PTP duration afterwards (`HomeTimer`), so stored times may
  differ by rounding in the last bits; acceptance compares the console line (12 significant
  digits), not bitwise times.
  Rationale: timing each segment when it is appended removes `HomeTimer`; floating-point addition
  in a different order cannot change 12 significant digits of a 25 s motion.
  Date/Author: 2026-10-26, agent.
- Decision: `ScenePlayback` plays a `std::shared_ptr<const TimedMotion>` created with the
  `std::shared_ptr` aliasing constructor from the saved motion's own handle, and reads the phase
  label from the `TimedMotion`. The `PlaybackMotion` variant, `chamferPhaseText`,
  `ChamferMotion::evaluateJoints` and `ProbeTouchMotion::evaluateJoints` are removed.
  Rationale: the aliasing constructor keeps the owning `ChamferMotion` or `ProbeTouchMotion` alive
  while pointing at its stored `TimedMotion`, so playback needs one type and one code path. The
  project has no translation files (no `.ts`, no `QTranslator`), so moving the chamfer phase texts
  from `ScenePlayback::tr` into plain strings changes nothing visible.
  Date/Author: 2026-10-26, agent.
- Decision: Milestone 1 details. (1) `ChamferMotionPhase` moved from `chamfermotion.h` into the
  unnamed namespace of `chamfermotion.cpp`, its only remaining user (error labels of
  `MotionCompiler`). (2) The probe motion's two minimum clearances form `ProbeClearances
  {transfer, strut}`, so `ProbeTouchMotion`'s constructor takes the timed motion and the
  clearances. (3) The chamfer playback data are built by the free function
  `playbackMotion(compiled points, robot)` with the former spline-range checks and error texts;
  the central-motion check keeps its place before the splines. (4) `TimedMotion::evaluateJoints`
  assigns a stored point to the ending phase also inside a segment, so the chamfer phase shown
  exactly at the Lead-in/Machining and Machining/Lead-out knots is the ending one.
  Rationale: (1) removes a public type without a public use; (2) keeps two parameters by grouping
  one concept; (3) moves the former member without changing its checks; (4) is the rule of the
  plan, visible only for one instant.
  Date/Author: 2026-10-26, agent.
- Decision: no generic arc-length curve class is added; the chamfer curve remains
  `ChamferCentralMotion` with its arc-length maps and curvature-based schedule, wrapped by
  `CentralPath`.
  Rationale: it is the only curve with a current requirement; a later curve implements `TimedPath`
  the same way.
  Date/Author: 2026-10-26, agent.


## Outcomes & Retrospective

Plan outcome (2026-10-26): one module turns PTP, LIN, CIRC and parametric-curve segments into timed
joint angles. `MotionBuilder` appends every segment with one IK and check sequence,
`TimedMotion` stores the result and evaluates it for Dry Run, and the chamfer and probe motions
only list their segments (`MotionCompiler::compile`, `TouchMotionBuilder`). Both Generate console
lines stayed identical to the Milestone 0 baselines after Milestones 1, 2 and 3, and both Dry Runs
behaved as before. The C++ shrank by 130 physical lines while adding the module. Remaining: one RSI
Sampling run on the sample edge when the socket is available. Lesson: keeping each milestone's
console lines identical made every step verifiable without tests; the deliberate changes (error
texts, point-order first failure, XYZ check on LIN/CIRC) were confirmed by the user before they
were written.


## Context and Orientation

All paths are relative to the repository root, `E:\Qt\QtProjects\RoboCrap` or
`D:\Qt\QtProjects\RoboCrap` depending on the computer. The program is a Windows Qt Quick
application (C++17, Qt ≥ 6.8, CMake ≥ 3.16, bundled Eigen) for a KUKA KR 10 R1420 robot. Read
`AGENTS.md` first; then the sections "Machining and playback" and "Geometry and scene" of
`docs/AGENT_CONTRACTS.md`, which are binding.

Terms. A TCP pose is a 4×4 transform matrix (`M4d`) of the tool centre point in the robot base
system {B}. The flange-to-TCP transform matrix is the tool. IK is `Kr10Kinematics::calculateIK`
(`src/kinematics/`), seeded with the preceding solution (`Kr10Kinematics::IkSolution`: six joint
angles in degrees, a Status bit field selecting shoulder/elbow/wrist branch, a Turn field). FK
(forward kinematics) is `Kr10Kinematics::solveFK`. An animation point is a `ChamferJointPoint`
(joints, FK TCP, a phase-local progress, time in seconds from HOME). A joint spline is a clamped
cubic spline in joint space through the points of one segment, with zero joint velocity at both
segment ends (`clampedJointSpline` in `src/trajectories/motion/jointsegments.cpp`); Dry Run
evaluates it between stored points. Smoothstep is s(u) = 10u³ − 15u⁴ + 6u⁵, used for PTP.

Shared code today. `src/trajectories/motion/jointsegments.{h,cpp}` (internal) holds
`ratedRobotSetup` (model, timing and $VEL_MA.CP/$ACC_MA.CP checks; rated joint speeds copied from
the model), `rigid`, `withinLimits`, `smoothProgress`, `smoothVelocity`, `inverseProgress`,
`ptpDuration`, `LinearLimits`, `LinearProfile` (a timed LIN, or a CIRC when its `angle` is nonzero;
`target(t)` gives the TCP pose; trapezoidal, triangular or constant speed when the acceleration is
0), `linearProfile`, `circularProfile`, `LinearSchedule`, `linearSchedule` (point times fixed
before IK: N = max(1, ceil(T·√(a / (8 · 0.01 mm)))) intervals per constant-acceleration stage of
duration T, plus the arc's normal acceleration) and `clampedJointSpline`.
`src/trajectories/chamfer/chamfertcpsolver.h` holds `ChamferTcpSolver`: `solveTcp(tcp, seed)`
(checked IK), `forward(joints, progress)`, `inverse(tcp, seed)`, `samplePtp(start, end)` (PTP
points in joint steps of at most 2°), `solveLinear(schedule, seed, label)` (IK of every LIN point,
TCP residual check, then `linearRateError`: joint speeds of every interval), plus the free functions
`chamferJointPositionError` and `chamferJointSpeedError`.

The chamfer motion. `ChamferMotion::create(path, robot)` in
`src/trajectories/chamfer/chamfermotion.cpp` calls `ratedRobotSetup`, then tries start
configurations (`MotionCompiler::starts`: IK of the staging pose P_s from several seeds) and for
each one `MotionCompiler::compile`: the HOME point, PTP HOME→P_s (`appendPtp`, untimed), LIN
P_s→lead-in start (`appendTransfer`: `transferProfile`, start-pose check, `linearSchedule`,
`solveLinear`), the central curve (`ChamferCentralPreparation::validate`, whose `CentralValidator`
in `chamfercentralvalidation.cpp` solves `solveTcp` at every scheduled time, checks each interval's
joint speeds while appending and then `inspectRates`: grouped joint speeds, XYZ acceleration of the
analytical targets against `cartesianAcceleration`, and the final stop), LIN lead-out end→P_s and
PTP P_s→HOME. `HomeTimer` then times both PTPs with `ptpDuration` and `inverseProgress` and shifts
all later times. `preparePlaybackSplines` fits three splines: LIN in, the whole central curve (its
three phases share one spline), LIN out. `m_boundaries` (8 point indices) separates the 7 phases
`ChamferMotionPhase` {Approach (HOME→P_s), TransferIn, LeadIn, Machining, LeadOut, TransferOut,
ReturnHome}. Consumers of the chamfer motion outside its file: `ScenePlayback` (`evaluateJoints`,
`robot()`), `SceneObject::drawing` (`points()`, `boundaries()`), `SceneMachiningPreparation`
(`points()`, `boundaries()` 1, 2, 5, 6, `robot()`), `SceneMachiningSettings::fromMotion`
(`robot().timing`, `parameters()`), `SceneMachiningPath::centralDuration` (`centralMotion()`), and
`sampleChamferRsiStream` in `chamferrsistream.cpp` (`centralMotion()`, `points()`, `boundaries()`
2 and 5, `robot()`, and its own `ChamferTcpSolver`). These public accessors stay.

The curve. `ChamferCentralMotion` (`chamfercentralmotion.{h,cpp}`) evaluates the analytical TCP
pose at a time: the lead-in accelerates over `accelerationDistanceFraction` of its length to
`machiningFeed`, the ellipse runs at `machiningFeed`, the lead-out decelerates over the same
fraction of its length; distance becomes curve progress through quintic Hermite arc-length maps.
`ChamferCentralPreparation` builds it once per Generate worker, builds the point schedule from
analytical curvature bounds (`makePreviewSchedule`: spans with interval counts, 0.01 mm chord
criterion) and caches the evaluated targets by point index (`evaluateTarget(index, seconds)`, which
accepts only the next uncached index, so candidates reuse targets).

The probe motion. `ProbeTouchMotion::create(plan, robot)` in
`src/trajectories/probe/probetouchmotion.cpp` lists its steps (`motionSteps`: PTP to entry, LIN
descend, per touch CIRC transfer, LIN Approach and Retract at constant 1 mm/s, PTP wrist unwind
after T21, LIN retreat, PTP HOME) and appends each with its own `TouchMotionBuilder` (`appendPtp`,
`appendLinear`, spline per Descend/Transfer/Retreat, `clearanceError` for the 3 mm sphere and
strut clearances). It stores `ProbeTouchMotionData` (points, `ProbeMotionSegment` list, splines,
minimum clearances) and evaluates playback in its own `evaluateJoints`: smoothstep on PTP, the
splines, joint-linear interpolation on Approach and Retract. `ProbeMotionPhase::text()` gives
"Entry: descend", "Touch 5/31: approach" and similar.

Playback. `ScenePlayback` (`src/application/scene/sceneplayback.{h,cpp}`) holds
`PlaybackMotion = std::variant<std::monostate, std::shared_ptr<const ChamferMotion>,
std::shared_ptr<const ProbeTouchMotion>>`, and `present(seconds)` evaluates the active alternative,
copies joints, tool transform and phase text and calls `RobotController::applyPlayback`.
`SceneMachining::dryRun` and `SceneCalibration::dryRun` call `ScenePlayback::start(motion,
pathId)`. `SceneObject::drawing` draws the probe path from `ProbeTouchMotion::segments()`.

Baseline line counts (physical/nonblank, 2026-10-26): jointsegments.h 118/101, jointsegments.cpp
336/313, chamfertcpsolver.h 192/177, chamfermotion.cpp 336/312, chamfermotion.h 78/68,
chamfercentralvalidation.cpp 347/319, chamfercentralvalidation.h 63/54, probetouchmotion.cpp
242/220, probetouchmotion.h 67/56, chamferrsistream.cpp 270/246, sceneplayback.h 80/72,
sceneplayback.cpp 154/136, sceneobject.cpp 255/234, scenemachining.cpp and scenecalibration.cpp
(capture at Milestone 1 start), src/trajectories/CMakeLists.txt 23/23. Capture counts at the start
and end of every milestone, including new and deleted files, and record C++ and build inputs
separately in `Progress`.


## Plan of Work

### Milestone 0: baselines

Before any edit, ask the user to run the current application and report two console lines. The
chamfer line comes from: Measuring → Rough → Plane from JSON `resources/json/edge_cyl_plane/XP.json`
→ Cylinder from JSON `XC.json` → select both → Intersect Selected → Machining → select the edge →
Generate Path → Generate with default settings:

    Chamfer preparation: <points> points, <seconds> s movement

(the 2026-10-25 value was `375 points, 25.299220225 s movement`). The calibration line comes from
Measuring → Calibration → Generate Path:

    Probe calibration preparation: 31 touches, <points> points, <seconds> s movement, minimum transfer clearance <mm> mm, minimum strut clearance <mm> mm

Record both lines in `Progress`. In the same message, ask the user to confirm the error-text and
first-failure decision of the Decision Log (fourth entry); record the answer there.

### Milestone 1: TimedMotion and one playback evaluator (no Generate change)

Scope: one stored-motion type that Dry Run plays for both motions. At the end,
`src/trajectories/include/trajectories/timedmotion.h` and `src/trajectories/motion/timedmotion.cpp`
exist; `ChamferMotion` and `ProbeTouchMotion` each store a `TimedMotion`; `ScenePlayback` holds a
`std::shared_ptr<const TimedMotion>`; Generate code is unchanged except where it now fills the
`TimedMotion`.

Move `ChamferTimingParameters`, `ChamferRobotSetup` and `ChamferJointPoint` unchanged from
`chamfermotion.h` into `timedmotion.h`; `chamfermotion.h` includes `trajectories/timedmotion.h`.
In `timedmotion.h` define:

    enum class MotionInterpolation { Smoothstep, Spline, JointLinear };

    // Inclusive point range played with one interpolation; the first point is the preceding
    // segment's last point.
    struct MotionSegment
    {
      qsizetype first = 0;
      qsizetype last = 0;
      MotionInterpolation interpolation = MotionInterpolation::Smoothstep;
      qsizetype spline = -1; // First coefficient interval of a Spline segment.
    };

    // Inclusive point range shown under one label; ranges cover all points in order.
    struct MotionPhaseRange
    {
      qsizetype first = 0;
      qsizetype last = 0;
      QString label;
    };

    struct TimedMotionData
    {
      QVector<ChamferJointPoint> points;
      QVector<MotionSegment> segments;
      QVector<MotionPhaseRange> phases;
      QVector<std::array<V6d, 4>> splines;
    };

    class TimedMotion
    {
    public:
      TimedMotion(const ChamferRobotSetup& robot, TimedMotionData data);
      const ChamferRobotSetup& robot() const;
      const QVector<ChamferJointPoint>& points() const;
      const QVector<MotionSegment>& segments() const;
      const QVector<MotionPhaseRange>& phases() const;
      double duration() const; // Last point time; 0 without points.
      // Joints and phase index at absolute seconds; finite times outside clamp to the ends.
      std::optional<std::pair<V6d, qsizetype>> evaluateJoints(double seconds) const;
    };

`TimedMotion::evaluateJoints` merges the two current evaluators: reject a nonfinite time or an
empty motion; clamp to [0, duration]; return the first joints at 0 and the last at the duration;
find the segment whose last point time is the first at or after the time (a junction belongs to
the ending segment, as in the probe evaluator; the chamfer evaluator assigns it to the following
phase, which changes only the label shown at that exact instant, because both segments return the
stored junction joints there); Smoothstep: first joints plus
smoothProgress(u) times the joint difference, with u the time fraction of the segment; otherwise
binary-search the interval [a, b] of stored points containing the time, return a's or b's joints
exactly at their times, else the cubic of that interval (Spline) or a + fraction · (b − a)
(JointLinear); check `withinLimits`; the phase index is the phase range containing the found
interval's later point (a point shared by two ranges belongs to the ending range, which matches
the current chamfer rule that a phase boundary point belongs to the ending phase). Put the body in
`src/trajectories/motion/timedmotion.cpp` and list both new files in
`src/trajectories/CMakeLists.txt`.

`ChamferMotion` replaces `m_robot`, `m_points` and `m_jointSpline` by `TimedMotion m_timed` and
keeps `m_boundaries`, `m_parameters` and `m_centralMotion`; `points()`, `robot()` and `duration()`
delegate; `const TimedMotion& timed() const` is added; `evaluateJoints` is removed. The constructor
builds the `TimedMotionData` from the compiled points: segments {boundaries 0–1 Smoothstep, 1–2
Spline, 2–5 Spline, 5–6 Spline, 6–7 Smoothstep}; phases {0–1 "HOME to P_s", 1–2 "P_s to lead-in",
2–3 "Lead-in", 3–4 "Machining", 4–5 "Lead-out", 5–6 "Lead-out to P_s", 6–7 "Return HOME"} (the
texts of today's `ScenePlayback` switch); the three splines from `preparePlaybackSplines`, with
segment `spline` offsets 0, the interval count of the first spline range, and the sum of the first
two.

`ProbeTouchMotion` replaces `ProbeTouchMotionData` by `TimedMotion m_timed` plus its two minimum
clearances; `TouchMotionBuilder` fills `TimedMotionData` (segment interpolation: Smoothstep for
ToEntry/Unwind/ReturnHome, JointLinear for Approach/Retract, Spline otherwise; one phase per
segment labelled `ProbeMotionPhase::text()`). `ProbeMotionSegment`, `ProbeTouchMotionData`,
`segments()` and `evaluateJoints` are removed from `probetouchmotion.h`; `timed()` is added.
`SceneObject::drawing` reads `touchPath()->motion->timed().segments()` and decides the colour from
the segment interpolation (JointLinear green 3 px, Smoothstep purple, Spline blue) instead of
`ProbeSegment`.

`ScenePlayback`: `using PlaybackMotion` and the variant go; `start(std::shared_ptr<const
TimedMotion> motion, quint32 ownerId)` and the member `std::shared_ptr<const TimedMotion>
m_motion`; `active()` is `bool(m_motion)`; `start` takes `m_duration = m_motion->duration()`;
`present` evaluates `m_motion->evaluateJoints(seconds)`, copies the joints, `robot().flangeToTcp`
and `phases()[index].label`, then calls `applyPlayback` as now; `chamferPhaseText` is deleted.
`SceneMachining::dryRun` passes `std::shared_ptr<const TimedMotion>(motion, &motion->timed())`
for its `std::shared_ptr<const ChamferMotion> motion`; `SceneCalibration::dryRun` does the same
for the probe motion.

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report; both Generate lines
equal Milestone 0; chamfer Dry Run shows the same phase texts in the Machining panel and ends at
HOME; calibration Dry Run shows "Touch 5/31: approach" style texts in the status bar, plays at
constant speed during Approach/Retract and ends at HOME; Pause, resume and Stop work in both.

### Milestone 2: MotionBuilder, TimedPath, PTP/LIN/CIRC; the probe motion uses them

Scope: one builder that appends PTP, LIN and CIRC segments with one IK and check sequence. At the
end `src/trajectories/motion/motionbuilder.{h,cpp}` exist and `TouchMotionBuilder` keeps only the
probe step list, the wrist-unwind seed and the clearance checks.

Move `ChamferTcpSolver`, `ChamferTcpIkResult`, `chamferJointPositionError` and
`chamferJointSpeedError` unchanged from `src/trajectories/chamfer/chamfertcpsolver.h` to
`src/trajectories/motion/tcpsolver.h` (the file name follows the module; the class keeps its
name), and update the four includes (chamfercentralvalidation.cpp, chamfermotion.cpp,
chamferrsistream.cpp, probetouchmotion.cpp). In `motionbuilder.h` define:

    // Point times of a Cartesian path, fixed before IK, and its phase labels.
    struct PathSchedule
    {
      QVector<double> times; // Local seconds of every point after the start, increasing.
      QVector<std::pair<qsizetype, QString>> phases; // (index of the phase's last point, label).
      QString error;
    };

    // Prescribed TCP pose of one scheduled point and the progress stored with it
    // (LIN/CIRC: travelled fraction; curve: geometric phase progress, 1 at a phase end).
    struct PathTarget
    {
      M4d tcp = M4d::Identity();
      double progress = 0.0;
    };

    // A timed Cartesian TCP path of one segment.
    class TimedPath
    {
    public:
      virtual ~TimedPath() = default;
      virtual PathSchedule schedule() const = 0;
      // Target of point index at local seconds; index 0 is the start (local 0).
      virtual std::optional<PathTarget> target(qsizetype index, double seconds) const = 0;
    };

    // LIN or CIRC from a LinearProfile under one phase label.
    class LinearPath final : public TimedPath
    {
    public:
      LinearPath(const LinearProfile& profile, const QString& label);
      PathSchedule schedule() const override; // linearSchedule(profile, 0.0)
      // {profile.target(seconds), profile.progress(seconds)}; index is not used.
      std::optional<PathTarget> target(qsizetype index, double seconds) const override;
    };

    class MotionBuilder
    {
    public:
      // Starts at HOME with the HOME configuration as seed; borrows the worker-owned setup.
      explicit MotionBuilder(const ChamferRobotSetup& robot);
      const ChamferTcpSolver& solver() const;
      const Kr10Kinematics::IkSolution& seed() const; // Solution of the last point.
      const QVector<ChamferJointPoint>& points() const;
      Kr10Kinematics::IkSolution home() const;
      // Smoothstep PTP from the last point to end (end becomes the seed), one phase label.
      QString appendPtp(const Kr10Kinematics::IkSolution& end, const QString& label);
      // Cartesian path from the last point; Spline or JointLinear interpolation.
      QString appendPath(const TimedPath& path, MotionInterpolation interpolation);
      TimedMotion finish() const; // Valid after the last successful append.
    };

`appendPtp` contains today's probe `appendPtp` timing: `solver.samplePtp` loop moves inside it
(sample in joint steps of at most 2°, time = start + ptpDuration(delta) ·
inverseProgress(progress), strictly increasing times, the 400000-point limit), appends one
Smoothstep segment and one phase. `appendPath` does, in order: `path.schedule()` (error returned
as is); the 400000-point limit; `path.target(0, 0.0)` must match the last point's FK TCP within
0.0001 mm and 0.000001 rotation entries ("<label>: path start does not match the preceding TCP");
for each scheduled time: `solveTcp(target.tcp, seed)`, the FK TCP as the point TCP, the target's
progress, time = segment start + local, strictly increasing, and the joint speed of
the new interval against `timing.jointSpeed` (`chamferJointSpeedError`); then the grouped rate
inspection moved from `CentralValidator::inspectRates` (same 1e-4 s grouping, grouped joint speeds,
XYZ acceleration of the targets against `cartesianAcceleration`, the stop at the end, using the
segment's local times and end); then, for Spline, `clampedJointSpline` of the segment's points;
then the segment, its phases (from `schedule.phases`) and points are committed. Every error is
"<label>: <message> (t=<local seconds> s)", where label is the phase containing the failing point.
Nothing is committed on failure. `finish` returns `TimedMotion(robot, data)`.

The probe motion then becomes: `ratedRobotSetup`; a `MotionBuilder`; for each step of
`motionSteps`: ToEntry `appendPtp(solveTcp(entry, home).solution)`, Unwind
`appendPtp(solveTcp(start, otherWrist(seed)).solution)` (a failed `solveTcp` returns its error with
the step label), ReturnHome `appendPtp(home())`, Approach and Retract
`appendPath(LinearPath(*linearProfile(last TCP, target, {1.0, 0.0}), label), JointLinear)`,
Transfer `appendPath(LinearPath(*circularProfile(...), label), Spline)`, Descend and Retreat with
`{transferFeed, transferAcceleration}` and Spline, each profile checked first and rejected with
"<label>: Invalid or degenerate LIN or CIRC profile." as today; after each step the existing
`clearanceError` on the new points; `finish`. Delete `TouchMotionBuilder::appendPtp`,
`appendLinear`, the spline code and the probe `evaluateJoints` remnants. `solveLinear`,
`linearRateError` and `samplePtp` stay in `ChamferTcpSolver` until Milestone 3 removes their last
caller.

Acceptance: ask the user to build `robocrap` and report; the calibration line equals Milestone 0;
calibration Dry Run behaves as in Milestone 1; the chamfer line is unchanged (chamfer is not yet
migrated).

### Milestone 3: the chamfer motion and its curve use MotionBuilder

Scope: the chamfer motion is built by the same builder; `HomeTimer`, `appendTransfer`,
`MotionCompiler::appendPtp`, `preparePlaybackSplines`, `CentralValidator`, `solveLinear`,
`linearRateError` and `samplePtp` are removed.

Add `CentralPath final : public TimedPath` to `chamfercentralvalidation.{h,cpp}`, constructed
from the `ChamferCentralPreparation&`: `schedule()` calls `prepare()` and converts its spans into
times (the existing span loop: interval index / intervals of each span, the span end exactly) and
three phases ("Lead-in", "Machining", "Lead-out") ending at the points at the phase times;
`target(index, seconds)` returns the tcp and progress of `evaluateTarget(index, seconds)`, with
progress 1.0 at the last point of each phase as `MotionCompiler::compile` sets it today, and keeps
the target cache.
The XYZ acceleration of the grouped inspection uses these targets exactly as `CentralValidator`
does today. Keep the initial-seed joint-limit check of `validate` in `MotionCompiler::compile`.

`MotionCompiler::compile(start)` becomes: a fresh `MotionBuilder`; `appendPtp(start, "HOME to
P_s")`; record boundary 1; `appendPath(LinearPath(transferProfile(LeadIn), "P_s to lead-in"),
Spline)`; boundary 2; `appendPath(CentralPath(...), Spline)`; boundaries 3, 4, 5 from the builder's
phase ranges; `appendPath(LinearPath(transferProfile(LeadOut), "Lead-out to P_s"), Spline)`;
boundary 6; `appendPtp(home(), "Return HOME")`; boundary 7; the existing final rigid/limit check;
`finish()`. `ChamferMotion::create` keeps its candidate loop and error rules (the last
candidate's error is returned) and stores `finish()`, boundaries and the central motion. Then
delete the now unused code listed in Scope and in `chamfertcpsolver` (moved file `tcpsolver.h`).

Acceptance: ask the user to build `robocrap` and report; the chamfer line equals Milestone 0 and
the calibration line too; chamfer Dry Run shows the same phases and ends at HOME; RSI Sampling is
not part of this acceptance (it needs the RSI socket), but its inputs (`centralMotion()`,
`points()`, `boundaries()` 1, 2, 5, 6) are unchanged in meaning. If the user can bind the RSI
socket without a controller connection, RSI Sampling of the sample edge should report the same
summary as before the plan.

### Milestone 4: documentation, contracts and compliance

Update `docs/AGENT_CONTRACTS.md` ("Machining and playback": one builder and one IK/check sequence
for all Cartesian segments, the new error format and first-failure order, `TimedMotion` playback;
"Geometry and scene" if it names moved types) and `docs/ARCHITECTURE.md` (3.3 path generation,
3.2 ScenePlayback, 5.2 and 5.5 traces). Perform the `AGENTS.md` compliance pass on every changed
declaration, body and caller and record findings and before/after line counts in `Progress` and
`Outcomes & Retrospective`.


## Concrete Steps

Working directory: the repository root. For every milestone: re-read the mandatory sections of
`AGENTS.md` and the named contract sections; capture line counts of every touched file, for
example in Git Bash:

    wc -l src/trajectories/chamfer/chamfermotion.cpp
    grep -cv '^\s*$' src/trajectories/chamfer/chamfermotion.cpp

make the edits; run `git diff --check` and `git status --short`; stop and ask the user to build
exactly the named targets and run the named scenario; record the reported result in `Progress`
and fix reported failures before the next milestone.


## Validation and Acceptance

The plan is complete when, from a disconnected runtime, the user reports: the chamfer and
calibration console lines equal to the Milestone 0 baselines after Milestones 1, 2 and 3; both
Dry Runs with unchanged phases, speeds, Pause, resume, Stop and end at HOME; and the line counts
in `Progress` show the removed duplicate code. Build success alone does not prove that the motions
are unchanged; the console lines and Dry Runs do.


## Idempotence and Recovery

All steps are source edits; each milestone leaves a buildable state that reproduces the baselines.
If a milestone's console line differs from the baseline, stop, record the difference in
`Surprises & Discoveries`, and find the changed arithmetic before continuing; reverting edits with
git requires the user's request.


## Artifacts and Notes

Measured result: −130/−135 physical/nonblank lines over the plan's files (Progress, Milestone 4).
Expected removals as estimated before the work: in `chamfermotion.cpp`
`HomeTimer`, `appendPtp`, `appendTransfer`, `preparePlaybackSplines` and `evaluateJoints` (about
120 lines); in `probetouchmotion.cpp` `appendPtp`, `appendLinear`, the spline code and
`evaluateJoints` (about 90); in `chamfertcpsolver.h` `samplePtp`, `solveLinear`, `linearRateError`
(about 95, partly moved into the builder); in `chamfercentralvalidation.cpp` the `CentralValidator`
loops (about 110, partly moved); in `sceneplayback.cpp` the variant branches and `chamferPhaseText`
(about 25). Added: `timedmotion.{h,cpp}` and `motionbuilder.{h,cpp}`.


## Interfaces and Dependencies

No new external dependency and no new CMake target. New published header
`trajectories/timedmotion.h` (`ChamferTimingParameters`, `ChamferRobotSetup`, `ChamferJointPoint`,
`MotionInterpolation`, `MotionSegment`, `MotionPhaseRange`, `TimedMotionData`, `TimedMotion`); new
internal files `motion/timedmotion.cpp`, `motion/motionbuilder.{h,cpp}` (`PathSchedule`,
`PathTarget`, `TimedPath`, `LinearPath`, `MotionBuilder`), `motion/tcpsolver.h` (moved from
`chamfer/chamfertcpsolver.h`). Changed public APIs: `ChamferMotion` loses
`evaluateJoints` and gains `timed()`, and `ChamferMotionPhase` leaves its header; `ProbeTouchMotion`
loses `segments()`, `evaluateJoints`, `ProbeMotionSegment` and `ProbeTouchMotionData` and gains
`timed()` and `ProbeClearances`; `ScenePlayback::start` takes
`std::shared_ptr<const TimedMotion>`. Every signature has at most two parameters; a third requires
recorded reduction attempts in the Decision Log.


## Revision notes

- 2026-10-26: first version, written after the probe calibration plan was completed, at the user's
  request for a separate module that turns PTP, LIN, CIRC and curve segments into joint angles for
  animation.
- 2026-10-26: Milestones 1–4 implemented and Milestones 1–3 accepted by the user; Progress records
  each milestone's counts and compliance pass, the Decision Log the user's confirmation of the error
  format and the Milestone 1 details, and Outcomes & Retrospective the plan outcome.
