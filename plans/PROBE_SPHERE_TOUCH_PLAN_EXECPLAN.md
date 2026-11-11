# Probe sphere calibration: calibration and verification touch plans

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`,
`Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is
maintained in accordance with `PLANS.md` at the repository root.

Three rules override `PLANS.md` for this plan. First, the implementing agent never runs a build,
CMake configure, QML lint, the application or any test; when a milestone needs a build or a run,
the agent stops, names exactly what the user must build or run, and continues from the user's
reported result. Second, no git staging or commits are made unless the user asks for them
(`AGENTS.md`). Third, no project tests or test harnesses are added (`AGENTS.md`). Every
`AGENTS.md` rule applies to every line written for this plan, in particular the "Mandatory C++
style" and "Mandatory code reduction" sections.

This plan replaces the touch geometry of the current code: a 37-touch grid with a different probe
orientation at every touch (`src/trajectories/probe/probetouchplan.cpp`). The Calibration submode,
the sphere file, the scene handling and Dry Run already exist and are kept.


## Purpose / Big Picture

The robot carries a WP-500 touch probe. Before the probe can measure workpieces, the centre of its
ruby ball must be known relative to the robot flange. It is found by touching a fixed 25 mm
calibration sphere many times and solving a least-squares problem; afterwards further touches of
the same sphere check the result. This plan creates the two touch plans that these steps need:
the calibration touch plan (the touches from which the ball centre will be computed) and the
verification touch plan (the touches that later measure the accuracy of the found centre). Both
plans contain every pose that the animation needs, and the existing animation is adapted so the
user can watch the whole session in the application.

Every geometric choice in the two plans is made for one goal: the smallest error of the ball
centre found by calibration. The reasons are stated in `Decision Log`; the essential ones are
repeated where the plan uses them.

After this plan the user can do the following. In Measuring mode, Calibration submode, "Generate
Path" computes a motion from HOME through 114 calibration touches and 44 verification touches back
to HOME. The probe keeps one of three fixed orientations within each block of touches. "Dry Run"
plays the motion; the status bar names the current block and touch, for example
"Calibration G2 pass 1, touch 5/19: approach". The console line of a successful Generate reports
the two touch counts and the clearances. No real robot moves; there is no RSI program, no KRL
program and no least-squares solution in this plan.


## Progress

- [x] (2026-11-11) Plan written; geometry, feasibility and identifiability checked numerically in a
  scratch script (see `Artifacts and Notes`).
- [x] (2026-11-11) Milestone 1: touch plan types and `createProbeTouchPlan` rewritten in
  `src/trajectories/include/trajectories/probetouchplan.h` and
  `src/trajectories/probe/probetouchplan.cpp`.
- [x] (2026-11-11) Milestone 1: `ProbeTouchMotion` builds the motion from the passes and stores the
  plan; `SceneCalibration` logs the new counts. The search for removed names finds nothing; compliance
  pass done (see `Artifacts and Notes`, "Milestone 1 implementation record").
- [x] (2026-11-11) Milestone 1: user built `robocrap` (objects of the three changed sources newer than
  the sources, `robocrap.exe` 04:49:31), ran Generate and Dry Run and reported that everything works;
  the console line itself was not sent.
- [x] (2026-11-11) Milestone 2: `docs/context/AGENT_CONTRACTS.md` (log line, Calibration Generate
  bullet), `docs/context/ARCHITECTURE.md` (§3.3 probe rows, §3.6 status example, §7 calibration
  clearance) and `docs/context/APP_DESCRIPTION.md` (Workflow step 1) describe the passes; no
  document mentions the 37-touch grid; `git diff --check` clean; CRLF working-copy endings kept.
  Compliance pass and C++ line counts are in "Milestone 1 implementation record"; Milestone 2
  changed no code.


## Surprises & Discoveries

- Observation: the two working orientations alone cannot determine the ball centre: one component
  of the centre is lost.
  Evidence: scratch Jacobian of the touches of groups G1 and G2 only has smallest singular value
  7.5e-16; with group G3 added it is 2.03 (see `Artifacts and Notes`).
- Observation: the strut of the sphere limits group G2 (stylus along +X) more than G1 and G3.
  Evidence: smallest ball clearance to the strut is 3.259 mm in G2 (touches at latitude −30°) and
  10.173 mm in G1 and G3; the required clearance is 3 mm.
- Observation: the stylus shank is a 20 mm long cylinder of radius 1.0 mm behind the ball; a touch
  whose approach makes more than 90° with the stylus drives the shank into the sphere.
  Evidence: `resources/cad/EE_WP-500.stp` has `SPHERICAL_SURFACE` radius 1.5 at x = −86.7 and
  `CYLINDRICAL_SURFACE` radius 1.0 starting at x = −66.7 on the same axis, then a cone to radius
  3.5 at x = −56.7 (30 mm from the ball centre, the stylus length) and the probe body of radius 21
  from x = −48.9.


## Decision Log

- Decision: touches are grouped into blocks with one fixed probe orientation (a "group"); the
  probe orientation changes only between blocks.
  Rationale: the probe will measure workpieces in two fixed orientations. The distance between ball
  centre and sphere centre at the moment of detection (the contact distance) contains the probe's
  pretravel, and the pretravel depends on the orientation of the probe relative to gravity. With
  fixed-orientation groups, the later least-squares solution can find one contact distance per
  attitude of the probe to gravity, each valid for the workpiece orientation it was measured in.
  With a different orientation at every touch (the old 37-touch grid) only one common contact
  distance is possible.
  Date/Author: 2026-11-11, user and agent.

- Decision: the baseline estimation model for these plans has two contact distances: ρ_down,
  shared by G1 and G3 (stylus down), and ρ_+X for G2 (stylus along +X); together with the ball
  centre in the flange frame (3 numbers) and the sphere centre in the base frame (3 numbers) that is
  8 unknowns. The effective ball radius for workpieces measured in an orientation is that
  orientation's contact distance minus the nominal sphere radius 12.5 mm.
  Rationale: in G1 and G3 gravity acts along the stylus in the probe frame and the approach
  directions in the probe frame are identical, so their pretravel is the same; G3 differs from G1
  only in the robot pose, whose errors a separate contact distance cannot compensate. Scratch
  computation for the 114 calibration touches (standard deviations as multiples of one touch's
  standard deviation, see `Artifacts and Notes`): one common contact distance gives 0.185 for it and
  0.355 for the ball-centre component along the stylus; two give 0.207 (down) and 0.262 (+X) with
  0.369; three give 0.236, 0.262, 0.236 with the same 0.369. Two are therefore better than three, and
  one cannot represent a pretravel difference between the two workpiece orientations. The sphere
  is a single unattested ball (theory document, section 1), so the deviation of its radius from
  12.5 mm passes unchanged into both effective radii; no touch plan can remove it.
  Date/Author: 2026-11-11, user and agent.

- Decision: three calibration groups: G1 stylus down (KUKA A 180°, B 0°, C 180°), G2 stylus along
  +X of the robot base frame (A 0°, B 90°, C 0°, the orientation at HOME), G3 stylus down with the
  flange turned 180° about the stylus (A 0°, B 0°, C 180°).
  Rationale: G1 and G2 are the two orientations in which workpieces are measured (user,
  2026-11-11). Two orientations leave the ball-centre component along the axis of their relative
  rotation undetermined; for G1 and G2 that axis is the flange y axis. A third orientation turned
  about another axis removes the gap. Turning about the vertical stylus keeps the probe's attitude to
  gravity identical to G1, so G3 adds no new pretravel condition. The sensitivity of the
  y component to a turn by angle φ is proportional to 2·sin(φ/2), largest at φ = 180°. Both G1 and
  G3 were already reached on this robot by manual jogging (`resources/json/calibration_sphere.json`,
  points XP1–XP10).
  Date/Author: 2026-11-11, user and agent.

- Decision: all three calibration groups use the same 19 approach directions expressed in the probe
  frame (the frame fixed to the probe, axes parallel to the flange axes).
  Rationale: the part of the pretravel that depends on the approach direction relative to the probe
  body moves fitted sphere centres by the same vector in the probe frame in every group when the
  direction sets are equal; the common ball-centre parameter then absorbs it consistently instead of
  producing disagreement between groups. G2 restricts which directions are reachable (strut), so the
  common set is the set reachable in G2.
  Date/Author: 2026-11-11, agent.

- Decision: directions are given by tilt τ, the angle between the stylus and the approach
  direction, and roll ψ, the angle about the stylus. In the probe frame the unit approach direction
  is (−sin τ·cos ψ, sin τ·sin ψ, cos τ); this is formula (2.2.4) of
  `docs/calibration/ROBOT_PROBE_SPHERE_CALIBRATION_THEORY_RU.md`. Calibration rings: τ = 0° (one
  direction), τ = 30° with ψ = 0°, 60°, …, 300° (six), and τ = 45°, 60°, 75°, 90° each with
  ψ = 60°, 180°, 300° (three each).
  Rationale: directions spread over τ separate the sphere centre along the stylus from the contact
  distance. Within a ring, N equally spaced rolls cancel the effect on the fitted centre of every
  periodic pretravel pattern of order m unless m − 1 or m + 1 is a multiple of N; a three-lobed
  kinematic probe has m = 3, cancelled for N = 3 and N = 6. In G2 a touch is reachable only when
  sin τ·cos ψ ≤ 0.5 (the touch point must not lie below latitude −30° because of the strut), which
  admits the full ring at τ = 30° and only the rolls 60°, 180°, 300° at τ ≥ 45°.
  Date/Author: 2026-11-11, agent.

- Decision: the touch plans do not fix how the contact distance depends on tilt; they only provide
  rings at τ = 0°, 30°, 45°, 60°, 75°, 90° so that the later estimation can choose. The earlier idea
  (conversation, 2026-11-11) of one free contact distance per tilt band is not adopted here.
  Rationale: within one group the sphere centre component along the stylus and a contact distance
  shared by touches enter the residual through cos τ and 1; a free contact distance per band is
  separable from that centre component only through the spread of τ inside the band, which weakens
  the ball-centre component along the stylus, the worst-determined one already (0.37 against 0.20 and
  0.18 per unit touch noise). A pretravel difference between axial and lateral touches of the form
  a·cos τ is indistinguishable from a shift of the ball centre along the stylus by a; when a is the
  same in all groups, the common ball-centre parameter absorbs it, and a workpiece touched at τ = 0°
  (end face, stylus down) or τ = 90° (hole wall) then gets the correct offset from one contact
  distance per orientation.
  Date/Author: 2026-11-11, agent.

- Decision: the model is extended beyond the baseline only on evidence from the first real
  calibration: residuals of the calibration touches are examined per tilt ring and per roll in each
  group; the scatter between the two passes of the same direction gives the noise of a single touch
  (pure error, Р 50.1.040-2002, item 1.23); a term is added only when a residual pattern over τ or ψ
  exceeds that noise; among candidate models the one with the smallest 95th percentile of the
  absolute verification residual is kept. The touch plans support this without change: every touch
  records its group, pass, τ and ψ, every calibration direction is touched twice, and the
  verification touches take no part in the estimation.
  Rationale: the direction dependence of WP-500 pretravel is unknown (the manual gives none), so a
  richer model cannot be chosen in advance; judging candidates on touches not used to fit them
  prevents choosing a model only because more parameters reduce the calibration residuals. This is
  the selection rule of step 7 of the theory document (smallest criterion value). Building the
  estimation is outside this plan.
  Date/Author: 2026-11-11, user and agent.

- Decision: τ never exceeds 90°.
  Rationale: at τ = 90° the shank axis passes the sphere centre at the contact distance, 14 mm, so
  the shank surface stays 14 − 12.5 − 1.0 = 0.5 mm from the sphere; at larger τ it intersects the
  sphere. τ = 90° is needed because hole walls are touched with the stylus perpendicular to the
  approach.
  Date/Author: 2026-11-11, agent.

- Decision: each calibration group is touched in two passes, in the order G1, G2, G3, G3, G2, G1;
  the second pass of a group visits its directions in reverse order.
  Rationale: thermal drift of robot and probe changes the measured positions slowly with time. If it
  is linear in time, this mirrored order gives every group, and every direction within a group, the
  same mean time, so the drift does not create differences between the groups' fitted centres, from
  which the ball centre is computed. Two passes also double the number of touches, reduce the
  random error and give two touches of every calibration direction, from whose differences the noise
  of a single touch is estimated. There is no time limit (user, 2026-11-11).
  Date/Author: 2026-11-11, agent.

- Decision: verification touches are made only in G1 and G2, after all calibration touches, one pass
  each, on different directions: τ = 0° and rings τ = 15°, 45°, 75°, 90° with ψ = 30°, 90°, …, 330°,
  keeping only the touches reachable in the group (G1: 25 touches, G2: 19).
  Rationale: the accuracy criterion of the theory document (95th percentile of the absolute
  verification residual) requires touches not used by calibration, in the orientations of later
  measurements, spread over the reachable part of the sphere; rolls offset by 30° from the
  calibration rings test directions the calibration never saw. The theory document gives 25 touches
  per orientation as its example.
  Date/Author: 2026-11-11, agent.

- Decision: one search speed for all approaches, a plan parameter with default 1 mm/s; the approach
  starts 3 mm outside the contact position, as before.
  Rationale: the mean signal delay and the speed dependence of pretravel enter the contact distance
  only if calibration, verification and later measurements use the same speed. Measuring the delay
  with two speeds is a separate experiment and not part of these plans.
  Date/Author: 2026-11-11, user and agent.

- Decision (2026-11-11, later): the retract uses the transfer limits (10 mm/s, 20 mm/s², ramps)
  instead of the search feed; the approach keeps the whole 3 mm at the search feed, without a
  pre-approach.
  Rationale: nothing is measured during the retract, so its speed does not affect the result; a
  3 mm retract takes 2·√(3/20) = 0.77 s instead of 3 s, about 5.8 min less for 158 touches. A
  pre-approach at the transfer limits would save only 0.55 s per touch with a 2 mm search, which
  the first calibration needs because the contact position is uncertain by up to about 1 mm in G2
  (orientation never touched manually, ball-centre component along the stylus unknown); it is
  worth adding only after the first calibration has narrowed that uncertainty.
  Date/Author: 2026-11-11, user and agent.

- Decision: calibration and verification passes are executed in one motion, calibration first;
  consecutive passes with the same orientation continue with an ordinary transfer, passes with
  different orientations are joined through a retreat, a PTP motion to the next entry and a descent.
  Rationale: the theory document prescribes this order; one motion keeps the time between
  calibration and verification short and reuses the existing Calibration submode, which owns one
  path.
  Date/Author: 2026-11-11, agent.

- Decision: the plan keeps the existing motion pattern: PTP from HOME to an entry pose, LIN descent,
  per touch a CIRC transfer on the start shell (the sphere of radius contact distance + 3 mm),
  constant-speed LIN approach to the contact pose and LIN retract, LIN retreat to an exit pose and
  PTP to HOME.
  Rationale: the pattern is implemented and checked (`ProbeTouchMotion`). The touch order within a
  pass is set by the next decision.
  Date/Author: 2026-11-11, agent.

- Decision (supersedes the serpentine ring order of the first version): the touches of a pass are
  visited in the order of the shortest total CIRC transfer time that starts at τ = 0° and whose arcs
  keep the strut clearance in G2. Calibration: τ = 0°; (30°, 240°); the meridian ψ = 300° from 90°
  down to 30°; (30°, 0°); the meridian ψ = 60° from 30° up to 90°; (30°, 120°); the meridian
  ψ = 180° from 30° up to 90°. Verification: τ = 0°; the τ = 15° ring from ψ = 30° through 330°,
  270°, … to 90°; then the meridians ψ = 90° up, 150° down, 210° up, 270° down, 330° up, 30° down
  over τ = 45°, 75°, 90°.
  Rationale: the user asked to shorten the session without changing speeds (2026-11-11). Only the
  order changes; directions, groups, passes, reversal of second passes and speeds stay, so no
  accuracy decision is affected: the mirrored pass order cancels linear drift for any order, and
  the first calibration touch remains τ = 0°, so a reversed pass still ends where the next pass of
  the same orientation starts. Transfer time per arc is computed with the existing transfer feed
  10 mm/s and acceleration 20 mm/s² (trapezoidal profile on the start shell of radius 17 mm); an arc
  is admissible when 60 samples along it keep the ball ≥ 3 mm from the strut in G2. The calibration
  order is the exact optimum (Held–Karp search over all orders from τ = 0°); arc lengths depend only
  on the angles between directions, which are the same in every group, so it is optimal in G1, G2
  and G3. The verification order is the best of 200 randomised local searches; it gives the best
  value for G1 and, with the rolls 30° and 330° at τ ≥ 45° removed, for G2. Transfer time per pass
  (scratch): calibration 40.07 s → 22.27 s; verification G1 40.48 s → 31.33 s, G2 28.31 s → 22.65 s;
  in total about 122 s less (309 s → 188 s of transfers; approach and retract stay 948 s). The motion
  builder still checks every stored point.
  Date/Author: 2026-11-11, user and agent.


## Outcomes & Retrospective

Completed 2026-11-11. The Calibration submode now generates and animates the calibration touch plan
(six passes, 114 touches, groups G1, G2, G3 in mirrored order) followed by the verification touch
plan (two passes, 44 touches, G1 and G2); the user built the application and confirmed that Generate
and Dry Run work. Every touch carries its group, pass, tilt and roll, and the motion keeps the plan,
so the later real run and the least-squares estimation can use the nominal poses directly.

Remaining outside this plan: the RSI/KRL execution of the touches with angle registration in the
detection cycle, the least-squares estimation with two contact distances, the verification residual
and the 95th-percentile criterion with one contact distance per orientation in the theory document,
and the model-selection analysis of the first real calibration. Lesson: the strut, not the robot
reach, limits the horizontal group, and the common direction set had to be chosen from what that
group can reach.


## Context and Orientation

The application is a Windows Qt Quick program (C++17, Qt 6.8) in the repository root
`D:\Qt\QtProjects\RoboCrap` or `E:\Qt\QtProjects\RoboCrap`; paths below are relative to it. It
prepares and animates robot motions for a KUKA KR 10 R1420. Positions are in millimetres in the
robot base frame {B}; orientations of the tool are KUKA angles A, B, C in degrees, meaning the
rotation Rz(A)·Ry(B)·Rx(C) (`euler2rot` in `src/geometry/utils.cpp`). The flange frame {F} is
fixed to the robot's last link. The measuring tool centre point (TCP) is the ruby ball centre; its
default value is the controller tool TOOL_DATA[5] = (−0.048, −0.097, 127) mm with A = B = C = 0, so
the TCP orientation equals the flange orientation. All poses in the touch plan are TCP poses: 4×4
matrices (`M4d` in `src/geometry/mathtypes.h`, Eigen) whose rotation is the TCP orientation and whose
translation is the ball centre.

The calibration sphere has nominal radius r_s = 12.5 mm; the ruby ball radius is r_b = 1.5 mm, so at
contact the ball centre lies L = 14 mm from the sphere centre (the contact distance). The sphere sits
2 mm deep in the top of a vertical strut of radius 8.9 mm; the strut top is 10.5 mm below the sphere
centre. `ProbeTouchPlanParameters::strutClearance(ball)` in `src/trajectories/probe/probetouchplan.cpp`
returns the distance from the ball surface to the strut. Every ball position must keep at least
3 mm (`ProbeApproachGap`) to the strut; for touch points this means the latitude of the touch point
on the sphere must not be below about −30°. The sphere centre comes from the file
`<exe dir>/resources/json/calibration_sphere.json` (the fitted centre of ten manually jogged points,
about (1054.14, −331.97, 454.27) mm).

A touch, as animated: the ball waits at the start pose 3 mm outside the contact pose on the outward
sphere normal n through the touch point, moves at constant search speed to the contact pose (approach)
and back (retract). Between touches the ball moves along a circular arc (CIRC) on the start shell,
the sphere of radius L + 3 = 17 mm around the sphere centre. If the outward normal is n and the
sphere centre c, the contact pose has translation c + L·n and the start pose c + (L + 3)·n.

Approach direction and tilt and roll. The approach direction is −n. Expressed in the probe frame
(axes parallel to the flange axes; z along the stylus, pointing away from the flange), a direction
with tilt τ and roll ψ is

    e(τ, ψ) = (−sin τ·cos ψ, sin τ·sin ψ, cos τ)

τ = 0 means approach along the stylus (the ball is pushed axially), τ = 90° means approach
perpendicular to the stylus. If R is the 3×3 rotation of the probe (the TCP rotation), the outward
normal of the touch point is n = −R·e(τ, ψ). Within one group R is fixed, so the touch points of a
group are determined by its list of (τ, ψ).

The groups. G1: A 180, B 0, C 180, rotation diag(−1, 1, −1), stylus pointing down (−Z of {B}).
G2: A 0, B 90, C 0, rotation Ry(90°), stylus pointing along +X of {B}. G3: A 0, B 0, C 180, rotation
diag(1, −1, −1), stylus down with the flange turned 180° about the stylus relative to G1. For G1 and
G3 every direction with τ ≤ 90° has n_z ≥ 0 (touch points on the upper half of the sphere). For G2,
n_z = −sin τ·cos ψ, so the strut condition becomes sin τ·cos ψ ≤ 0.5.

A pass is one execution of a group's direction list; the calibration plan is six passes (G1, G2,
G3, G3 reversed, G2 reversed, G1 reversed), the verification plan two passes (G1, G2). The
verification direction list is filtered per group by the strut condition, so G2 has fewer touches.

Current code that this plan changes:

`src/trajectories/include/trajectories/probetouchplan.h` and
`src/trajectories/probe/probetouchplan.cpp` define `ProbeTouchPlanParameters`, `ProbeTouch`
(start pose, contact pose, normal), `ProbeTouchPlan` (one entry pose, 37 touches, one exit pose),
the constant `ProbeTouchCount = 37`, `ProbeApproachGap = 3.0` and `ProbeApproachFeed = 1.0`, and
`createProbeTouchPlan(parameters)`, which builds the 37 touches from a table of latitude, longitude,
tilt and roll with a different probe rotation per touch. The general approach of that file is kept:
a constant table in an unnamed namespace, poses from sphere centre plus distance times normal, strut
check per touch, entry pose 60 mm out along the first normal and 80 mm higher, exit pose 42 mm out
along the last normal. The touch table and its rotations are replaced.

`src/trajectories/include/trajectories/probetouchmotion.h` and
`src/trajectories/probe/probetouchmotion.cpp` define `ProbeTouchMotion::create(plan, robot)`. It
turns the plan into a list of steps (PTP to entry, LIN descend, for each touch CIRC transfer, LIN
approach at `ProbeApproachFeed`, LIN retract, then LIN retreat and PTP to HOME) and appends them with
`MotionBuilder` (`src/trajectories/motion/motionbuilder.h`). After each step it checks every stored
point: ball-to-strut clearance at least 3 mm, and outside approach and retract the ball at least
3 mm outside the contact shell. Phase names come from `ProbeMotionPhase::text()`, for example
"Touch 5/37: approach"; these names are shown by Dry Run in the status bar.

`src/application/scene/scenecalibration.cpp` builds the parameters in `SceneCalibration::generate()`,
runs `calibrationMotion` on a worker thread, and in `finishCalculation()` logs
"Probe calibration preparation: 37 touches, …" using `ProbeTouchCount`.

Documents that describe the old 37-touch plan: `docs/context/AGENT_CONTRACTS.md` (section "Machining
and playback", bullet "Calibration Generate builds createProbeTouchPlan …"),
`docs/context/ARCHITECTURE.md` (§3.3 rows "Probe touch plan" and "Probe touch motion", §3.6 status
example "Touch 5/37", §7 "Calibration clearance") and `docs/context/APP_DESCRIPTION.md` (Workflow
step 1).


## Plan of Work

Milestone 1 rewrites the touch plan and adapts its two consumers, because removing the old plan
members breaks `probetouchmotion.cpp` (same target `RoboCrapTrajectories`) and
`scenecalibration.cpp` (target `robocrap`); only together do they build and produce an observable
result. At its end Generate and Dry Run show the new session. In `probetouchplan.h`, remove `ProbeTouchCount` and
`ProbeApproachFeed`; keep `ProbeApproachGap`. Add `double searchFeed = 1.0;` as the last member of
`ProbeTouchPlanParameters` (last, so the existing aggregate initialisation in `SceneCalibration`
keeps working). Add `enum class ProbeTouchPurpose { Calibration, Verification };`. Extend
`ProbeTouch` with `double tilt` and `double roll` in degrees (τ and ψ of the touch). Add
`ProbeTouchPass` with purpose, group number (1–3), repetition number (1 or 2), entry pose, touches
and exit pose, and a `label()` method returning, for example, "Calibration G2 pass 1" or
"Verification G1 pass 1". Replace the members of `ProbeTouchPlan` by the parameters and
`QVector<ProbeTouchPass> passes`, with `touchCount(ProbeTouchPurpose)` summing the touches of passes
of that purpose. `createProbeTouchPlan` keeps its signature and returns the calibration passes
followed by the verification passes.

In `probetouchplan.cpp`, replace the old table and `touchRotation` by four constant tables in the
unnamed namespace: the three group orientations (A, B, C), the 19 calibration directions in visit
order, the 25 verification directions in visit order, and the pass sequence (purpose, group,
reversed). Visit orders are those of the decision on the shortest transfer order in `Decision
Log`. A helper computes the approach direction e(τ, ψ) as given in `Context and
Orientation`, a helper builds a `ProbeTouch` from the parameters, the group rotation and a table row
(normal n = −R·e, contact c + L·n, start c + (L + 3)·n), and a helper builds one pass: it takes the
rows in table order or reversed, computes each touch, rejects a calibration touch whose strut
clearance at start or contact is below `ProbeApproachGap` with an error naming the pass and the
touch (calibration directions must be reachable in every group by design), skips such a verification
touch, and sets entry and exit poses with the existing constants. Keep `strutClearance` and the
parameter validation unchanged, and also reject a `searchFeed` that is not finite and positive.
The function then loops over the pass table. Every helper takes at most two parameters except the
touch builder, which needs parameters, rotation and row (three).

Still in Milestone 1, adapt the motion. In `probetouchmotion.cpp`, `motionSteps(plan)` iterates over the
passes. For the first pass it emits PTP to its entry and the descent to the first start. For a later
pass whose group equals the previous pass's group, it emits a transfer to the first start unless the
first start equals the previous pass's last start (then nothing, because a zero-length CIRC is
rejected as degenerate). For a later pass of another group it emits a retreat to the previous pass's
exit, PTP to the new entry and the descent. Then, per touch, the transfer (not before the first touch
of a pass), approach and retract, and at the end the retreat to the last exit and PTP to HOME. Add
the pass index to `ProbeMotionPhase` and let `text` take the plan to produce "<pass label>, touch
t/n: <segment>" for touch segments, "<pass label>: <segment>" for entry, descent and retreat, and
"Exit: return HOME". The approach uses `plan.parameters.searchFeed` instead of
`ProbeApproachFeed`; the retract uses the transfer limits with spline interpolation (later decision). Store the plan in `ProbeTouchMotion` (constructor takes the timed motion, the
plan and the clearances) and expose `const ProbeTouchPlan& plan() const`, because the log needs the
counts and the later real run needs the nominal touch poses. The clearance checks stay as they are.

In `scenecalibration.cpp`, replace `ProbeTouchCount` in the log line by
`plan().touchCount(ProbeTouchPurpose::Calibration)` and `…Verification` and change the text to
"Probe calibration preparation: %1 calibration and %2 verification touches, %3 points, %4 s movement,
minimum transfer clearance %5 mm, minimum strut clearance %6 mm".

Milestone 2 updates `docs/context` (only the places listed in `Context and Orientation`, describing
the groups, passes, counts and phase names), performs the `AGENTS.md` compliance pass on all changed
declarations, bodies and callers, and records line counts.


## Concrete Steps

Work in the repository root. Before editing, record line counts (physical and non-blank) in Git Bash:

    for f in src/trajectories/include/trajectories/probetouchplan.h src/trajectories/probe/probetouchplan.cpp \
             src/trajectories/include/trajectories/probetouchmotion.h src/trajectories/probe/probetouchmotion.cpp \
             src/application/scene/scenecalibration.cpp; do echo "$f $(wc -l < $f) $(grep -c . $f)"; done

Values at plan writing (2026-11-11): probetouchplan.h 52/43, probetouchplan.cpp 105/93,
probetouchmotion.h 54/44, probetouchmotion.cpp 152/134, scenecalibration.cpp 300/270.

Milestone 1: edit the two plan files, the two motion files and `scenecalibration.cpp`, then search
for remaining uses of removed names; the search must find nothing:

    grep -rn "ProbeTouchCount\|ProbeApproachFeed\|plan.entry\|plan.exit\|plan.touches" src

Ask the user to build `robocrap` in their existing build directory, start it, choose Measuring mode,
Calibration submode, press "Generate Path", then "Dry Run", and report the console line and what the
3D view shows.

Milestone 2: edit the three documents, run `git diff --check`, inspect `git status` and the full diff,
and record final line counts with the command above.


## Validation and Acceptance

After Milestone 1 the user observes:

1. "Generate Path" succeeds and the console shows a line of the form

        Probe calibration preparation: 114 calibration and 44 verification touches, <N> points,
        <T> s movement, minimum transfer clearance 3.0000 mm, minimum strut clearance 3.2xxx mm

   The transfer clearance may differ from 3.0000 mm in the fourth decimal (stored poses are
   reconstructed with 0.0001 mm accuracy).

   The strut clearance is expected near 3.259 mm (G2 touches at latitude −30°). The movement time is
   at least 158 × 6 s = 948 s for approaches and retracts alone; with transfers about 25 minutes.

2. During "Dry Run" the status bar shows "Calibration G1 pass 1: to entry", then "… : descend",
   then "Calibration G1 pass 1, touch 1/19: approach";
   the probe orientation stays constant within each pass; the passes appear in the order G1, G2, G3,
   G3, G2, G1 (calibration) and G1, G2 (verification); between G1 and G2 the robot leaves the sphere,
   turns the probe from vertical to horizontal and returns; between G2 and G3 it turns the probe back
   to vertical with the flange turned 180°; between the two G3 passes and between the last
   calibration pass and the first verification pass there is no reorientation. The motion ends at
   HOME. No link visibly passes through the table or the stand.

3. A failure message names the pass and touch, for example "Calibration G2 pass 1, touch 12/19:
   transfer: …". Inverse-kinematics failures are possible in G2 because the stylus along +X near the
   sphere requires a small wrist angle A5 (estimated 20°–25°, not singular); report them in
   `Surprises & Discoveries` with the message before changing anything.

Build success alone does not prove the plan; acceptance requires the user's Generate and Dry Run
report.


## Idempotence and Recovery

All edits are source edits; repeating a step rewrites the same text. If Generate fails at an entry,
descent or retreat (clearance or inverse kinematics), change only `EntryDistance`, `EntryRise` or
`ExitDistance` in `probetouchplan.cpp`, record the change and the reason in `Decision Log`, and ask for
a new run. Do not change the direction tables or the group orientations to fix a motion failure
without recording the effect on accuracy in `Decision Log`; they carry the accuracy decisions.


## Artifacts and Notes

Scratch check of the plan geometry (Python, NumPy; run in the scratchpad, not added to the
repository). It builds the touch normals from the group rotations and direction tables, the strut
clearance at contact and start, and the Jacobian of the residuals
r_i = ‖ p_F,i + R_k·p − c ‖ − ρ with respect to the ball centre p in {F}, the sphere centre c in {B}
and the contact distances ρ (one common, one per attitude to gravity, or one per group k). Output:

    calibration directions 19
     group 1 min strut clearance 10.173
     group 2 min strut clearance 3.259
     group 3 min strut clearance 10.173
     verification group 1 25
     verification group 2 19
    per-group rho rows 114 min sv 2.028 sigma p_r (x,y,z)_F per unit touch noise [0.204 0.184 0.369]
    common rho rows 114 min sv 2.34 sigma p_r (x,y,z)_F per unit touch noise [0.194 0.184 0.355]
    groups 1,2 only min sv 7.543214290033645e-16
    ---- rho precision
    one sigma p_r [0.194 0.184 0.355] sigma rho [0.185]
    two sigma p_r [0.204 0.184 0.369] sigma rho [0.207 0.262]
    three sigma p_r [0.204 0.184 0.369] sigma rho [0.236 0.262 0.236]

Reading: with 114 calibration touches and the baseline model (two contact distances: G1 and G3
together, G2 alone), the standard deviation of the found ball-centre components x, y, z of {F} is
0.20, 0.18 and 0.37 times the standard deviation of a single touch, and of the two contact distances
0.21 and 0.26 times. A third contact distance for G3 improves nothing and loosens the stylus-down one.
Without G3 the centre is not determined.

Milestone 1 implementation record (2026-11-11).

Reuse before adding code: group rotations use the existing `euler2rot(A, B, C)`
(`src/geometry/utils.cpp`, which also zeroes rounding residue), poses use `makeTransform`, arcs and
straight segments use the existing `linearProfile`/`circularProfile` and `MotionBuilder`, and the
strut test keeps `ProbeTouchPlanParameters::strutClearance`. The old `TouchRow` table,
`sphereNormal`, `touchRotation`, `ProbeTouchCount` and `ProbeApproachFeed` were removed.

Added elements and why reuse fails: the tables `Groups`, `CalibrationDirections`,
`VerificationDirections` and `Passes` (with their row types `GroupOrientation`, `Direction`,
`PassRow`) hold the accuracy decisions as data, which no existing table holds. `passDirections`
returns one pass's direction list forward or reversed because the two direction tables have
different sizes. `makeTouch` (three parameters: plan parameters, group rotation, direction; the
rotation is per pass and the direction per touch, so neither belongs to the parameters object)
replaces `sphereNormal`/`touchRotation` with formula e(τ, ψ). `ProbeTouchPass::label`,
`ProbeTouchPlan::touchCount`, `ProbeMotionPhase::pass` and `ProbeTouchMotion::plan` serve the phase
texts and the log line; storing the plan in the motion also keeps the nominal touch poses for the
later real run. `searchFeed` replaces the constant `ProbeApproachFeed` (decision on one search speed
as a parameter).

Compliance pass (declarations, bodies, callers): no lambdas, no project templates, at most three
parameters, results through return values, helpers in unnamed namespaces, no output references.
`ProbeTouchMotion::create` keeps its own check for empty passes, as the old code checked for an empty
touch list, because it is a public entry point that accepts any plan.

Line counts (physical/non-blank) before and after: probetouchplan.h 52/43 → 69/56,
probetouchplan.cpp 105/93 → 158/139, probetouchmotion.h 54/44 → 58/48, probetouchmotion.cpp
152/134 → 169/151, scenecalibration.cpp 300/270 → 303/273; non-blank total 584 → 667. The growth is
the four data tables (about 30 lines; generating the rings in loops would save a few lines but hide
the visit order and the G2 roll subsets, which are decisions), the pass type with label and
count, and the pass transitions in `motionSteps`.

Core of the script, for repetition:

    G = {1: Rz(180)@Rx(180), 2: Ry(90), 3: Rx(180)}
    e(t,p) = [-sin t cos p, sin t sin p, cos t];  n = -G[k] @ e(t,p)
    row = [n @ G[k], -n, -1 in the column of group k]


## Interfaces and Dependencies

No new library or target. `RoboCrapTrajectories` (src/trajectories) keeps its dependencies. At the
end of Milestone 1, `src/trajectories/include/trajectories/probetouchplan.h` declares the following
(existing includes `geometry/mathtypes.h`, `<QString>`, `<QVector>`, `<optional>` stay):

    constexpr double ProbeApproachGap = 3.0; // mm from the start shell to the contact shell.

    enum class ProbeTouchPurpose { Calibration, Verification };

    struct ProbeTouchPlanParameters
    {
      V3d sphereCentre = V3d::Zero();
      double sphereRadius = 12.5;
      double ballRadius = 1.5;
      double strutRadius = 8.9;
      double strutRecess = 2.0;
      double searchFeed = 1.0; // mm/s, constant during every approach; retract uses transfer limits.
      double contactDistance() const { return sphereRadius + ballRadius; }
      double strutClearance(const V3d& ball) const;
    };

    struct ProbeTouch
    {
      M4d start = M4d::Identity();
      M4d contact = M4d::Identity();
      V3d normal = V3d::Zero(); // Outward sphere normal at the contact.
      double tilt = 0.0; // deg, between stylus and approach direction.
      double roll = 0.0; // deg, about the stylus.
    };

    struct ProbeTouchPass
    {
      ProbeTouchPurpose purpose = ProbeTouchPurpose::Calibration;
      int group = 1; // G1 stylus down, G2 stylus +X, G3 stylus down turned 180 deg.
      int repetition = 1;
      M4d entry = M4d::Identity();
      QVector<ProbeTouch> touches;
      M4d exit = M4d::Identity();
      QString label() const; // "Calibration G2 pass 1"
    };

    struct ProbeTouchPlan
    {
      ProbeTouchPlanParameters parameters;
      QVector<ProbeTouchPass> passes; // Calibration passes first, then verification passes.
      int touchCount(ProbeTouchPurpose purpose) const;
    };

    struct ProbeTouchPlanResult
    {
      std::optional<ProbeTouchPlan> plan;
      QString error;
    };

    ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters);

At the end of Milestone 1, `probetouchmotion.h` also keeps `ProbeTouchMotion::create(const ProbeTouchPlan&,
const ChamferRobotSetup&)`, adds `const ProbeTouchPlan& plan() const`, and `ProbeMotionPhase` gains
`int pass` with `QString text(const ProbeTouchPlan& plan) const`. Code follows `AGENTS.md`: no
lambdas, no project templates, at most four parameters, results through return values, helpers in
the unnamed namespace.

Dependency for later work, outside this plan: the least-squares estimation and the verification
residual of the theory document currently use one contact distance; with these plans the baseline
model uses two (stylus down: G1 and G3; stylus along +X: G2), and the verification residual of a
touch subtracts the contact distance of its orientation. Model extensions follow the selection rule
in `Decision Log`. That change belongs to the estimation plan and to the theory document.


Revision note (2026-11-11): contact-distance strategy fixed after discussion with the user. The
baseline model has two contact distances (per attitude to gravity) instead of one per group; model
extensions are chosen from the first real calibration's residuals by the smallest verification
percentile; the two passes are named as the repeat measurements for the noise estimate; the scratch
results for one, two and three contact distances were added. The touch tables, groups, order and
counts are unchanged.

Revision note (2026-11-11, second): touch order within passes replaced by the shortest-transfer
order (new decision in `Decision Log`, `Plan of Work` updated); about 122 s less transfer time with
unchanged directions, passes and speeds. The user must rebuild `robocrap` and repeat Generate and
Dry Run, because the pass transitions now start and end at other touches.

Revision note (2026-11-11, third): the retract runs at the transfer limits instead of the search
feed (new decision in `Decision Log`); about 5.8 min less. Code: `probetouchmotion.cpp` (only the
Approach keeps the search feed and joint-linear playback), comments in `probetouchplan.h` and
`sceneobject.cpp`; documents: `AGENT_CONTRACTS.md`, `APP_DESCRIPTION.md`. The user must rebuild
`robocrap` and repeat Generate and Dry Run.
