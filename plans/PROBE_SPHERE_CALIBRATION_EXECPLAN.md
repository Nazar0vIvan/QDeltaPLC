# Probe sphere calibration, part 1: Calibration submode, touch plan and Dry Run animation

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`,
`Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is
maintained in accordance with `PLANS.md` at the repository root.

Three rules override `PLANS.md` for this plan. First, the implementing agent never runs a build,
CMake configure, QML lint, the application or any test, under any circumstances; when a
milestone needs a build or a run, the agent stops, asks the user to build or run exactly what the
milestone names, and continues from the user's reported result. Second, no git staging or commits
are made unless the user asks for them (`AGENTS.md`). Third, no project tests or test harnesses
are added (`AGENTS.md`). Every `AGENTS.md` rule applies to every line written for this plan.


## Purpose / Big Picture

The robot carries a WP-500 touch probe whose ruby ball centre must be known relative to the robot
flange before the probe can be used for surface scanning. The centre is found by touching a fixed
25 mm calibration sphere from many directions and probe orientations and solving the
least-squares problem described in `docs/ROBOT_PROBE_SPHERE_CALIBRATION.md`. This plan covers only
the first part: preparing and animating the touch motion inside the application. It does not move
the real robot and adds no RSI streaming, no KRL program and no least-squares solution for the
ball centre; those belong to later plans.

After this plan, a user can do the following, which is impossible today. In the Workflow panel,
under Measuring mode, a third submode button "Calibration" appears beside "Rough" and "Precise".
Clicking it reads the sphere file `<directory of robocrap.exe>/resources/json/calibration_sphere.json`,
fits the sphere centre to the ten points the user recorded by manual jogging, and shows the sphere
in the 3D scene. The toolbar then shows only "Measuring EE", "Generate Path", "Dry Run" and
"Stop". "Generate Path" computes a motion that starts at HOME, touches the sphere 27 times with
the probe in prescribed orientations and returns to HOME, and draws the path of the ruby ball
centre. "Dry Run" animates the robot along that motion; "Pause" and "Stop" behave as in the
existing chamfer Dry Run. Leaving the Calibration submode stops the animation at HOME and hides
the sphere and the path. Editing the sphere file and entering Calibration again uses the new file.


## Progress

No step has been started. Record every stopping point here with a UTC timestamp, splitting a
partially completed step into "completed" and "remaining".

- [ ] Milestone 1: shared PTP/LIN/joint-spline operations extracted from
  `src/trajectories/chamfer/chamfermotion.cpp`; chamfer behavior unchanged.
- [ ] Milestone 2: `ScenePlayback` owns Dry Run playback and the robot model copy; chamfer Dry Run
  unchanged.
- [ ] Milestone 3: measuring TCP (ruby ball centre) in `SceneEndEffectors` and the Measuring EE
  panel.
- [ ] Milestone 4: sphere fit, sphere drawing, sphere file deployment and the Calibration submode
  with its toolbar.
- [ ] Milestone 5: touch plan and touch motion numerics, Generate and path drawing in the
  Calibration submode.
- [ ] Milestone 6: Dry Run of the calibration motion.
- [ ] Milestone 7: documentation and contract updates, compliance pass, line counts.


## Surprises & Discoveries

- Observation: the touch table accepted by the user in conversation was announced as "25
  touches", but its rows define 27 touches.
  Evidence: group A has a tip and 4 side touches, groups B and C a tip and 2 side touches each,
  groups D–G a tip and 3 side touches each: 5 + 3 + 3 + 4 × 4 = 27.
- Observation: the user's ten jog readings are in `resources/calibration_sphere_jog_points.txt`
  as KRL `DECL E6POS` lines XP1–XP10 (XP1–XP5 with A ≈ 180°, B ≈ 0°, C ≈ 180°; XP6–XP10 with
  A ≈ 0°, B ≈ 0°, C = 180°).
  Evidence: a one-off calculation in the agent's scratch directory with the algebraic sphere fit
  of Milestone 4 (an estimate from jogged readings, not repository code) gave:

      all 10 points: centre (1054.1403, -331.9692, 454.2727) mm, radius 13.8342 mm, RMS 0.0694 mm
      XP1-XP5:       centre (1054.1971, -332.0422, 454.3040) mm, radius 13.8147 mm, RMS 0.0433 mm
      XP6-XP10:      centre (1054.0817, -331.8960, 454.2434) mm, radius 13.8512 mm, RMS 0.0165 mm

  The fitted radius is about 0.17 mm below the nominal 14 mm, consistent with readings taken
  after the triggering jog step (0–0.1 mm overshoot) plus probe pretravel. The two halves differ
  by (0.115, −0.146, 0.061) mm, which suggests a lateral error of TOOL_DATA[5] of about half the
  horizontal difference.
- Observation: `SceneObject::ScanPath` exists as a kind, but nothing creates it; the Precise
  submode's "Dry Scan" only toggles flags in `Main.qml`.
  Evidence: no call site creates a `ScanPath` object; `Main.qml` `onScanToggleRequested` sets
  `dryScanActive`/`dryScanRunning` only.
- Observation: the Measuring tool has no TCP axes in the viewport.
  Evidence: `RobotController::drawingPose` (`src/application/robot/robotcontroller.cpp`) returns
  before composing TCP axes unless the active tool is the spindle.
- Observation: `MachiningPanel.qml` reads the playback state from its `coordinator`
  (`App.Machining`): `playing`, `playbackActive`, `phase`, `elapsed`, `duration`.
  Evidence: `src/application/qml/Views/Workspace/MachiningPanel.qml` lines 108–110 at
  2026-10-24. Milestone 2 must give the panel the new playback object.


## Decision Log

- Decision: the sphere centre used for aiming is fitted from all ten jogged points together,
  with the radius left free; the fitted radius and RMS are displayed as checks only.
  Rationale: the ten points were taken with two probe orientations rotated 180° about the stylus;
  a lateral TCP error shifts the halves in opposite directions, so a joint fit places the centre
  between them.
  Date/Author: 2026-10-24, agent; touch set approved by the user.
- Decision: the contact distance d is the nominal `sphereDiameter / 2 + rubyBallDiameter / 2`
  = 12.5 + 1.5 = 14 mm, not the fitted radius.
  Rationale: the fitted radius contains the jogging step error.
  Date/Author: 2026-10-24, agent.
- Decision: the sphere file is `resources/json/calibration_sphere.json` in the repository. It is
  not added to `resources/json.qrc`. `cmake/DeployOcct.cmake` copies it to
  `<exe dir>/resources/json/calibration_sphere.json` only when that file does not exist yet. The
  application reads the deployed file on every entry into the Calibration submode.
  Rationale: the user chose `resources/json/`; embedded resources are read-only, and the later
  calibration plan writes refined sphere data into the deployed file, which a build must not
  overwrite; re-reading on entry makes a recalibration use the new file, as the user requires.
  Date/Author: 2026-10-24, user (location) and agent (deployment rule).
- Decision: the measuring TCP is the ruby centre from TOOL_DATA[5], X −0.048, Y −0.097,
  Z 127 mm, A = B = C = 0, so the axes of {P} equal the flange axes and the {P} z-axis points
  away from the flange along the stylus. It is editable with Apply in the Measuring EE panel and
  is not persisted, like the spindle TCP.
  Rationale: user-supplied CMM values; `docs/ARCHITECTURE.md` "Controller tool data".
  Date/Author: 2026-10-24, user.
- Decision: the motion is HOME, PTP to the first via pose, then for every touch LIN via→start,
  LIN start→contact at the approach feed, LIN contact→start, LIN start→via, PTP to the next via
  pose, and finally PTP to HOME.
  Rationale: two consecutive touches can share a via position and differ only in orientation; the
  existing straight-line LIN profile rejects a zero-length translation, a PTP does not.
  Date/Author: 2026-10-24, agent.
- Decision: every sampled point of the ToVia, Descend, Ascend and ReturnHome segments must keep the
  ball centre at least d + g − 0.0001 mm from the sphere centre.
  Rationale: the application has no collision model; this checks the only clearance that can be
  computed, the ball centre. It checks sampled points only (PTP samples at most 2° apart), not the
  continuous path or any other part of the probe or robot; Dry Run shows the rest visually.
  Date/Author: 2026-10-24, agent.
- Decision: all via poses lie in one horizontal plane z = c_z + d + g + 25 mm, directly above
  their start positions.
  Rationale: every start position lies at sphere latitude ≥ 10.5°, so every point of the vertical
  line above it is at least d + g from the sphere centre.
  Date/Author: 2026-10-24, agent.
- Decision: Generate fails with an error naming the touch and segment when any IK, joint-limit,
  speed, spline or clearance check fails; there is no retiming, reordering or skipping. The first
  IK is seeded from HOME only.
  Rationale: the same rule as chamfer Generate (`docs/AGENT_CONTRACTS.md`, Machining and playback).
  Date/Author: 2026-10-24, agent.
- Decision: reuse `ChamferRobotSetup`, `ChamferTimingParameters`, `ChamferJointPoint` and
  `ChamferTcpSolver` for the touch motion; their chamfer names remain.
  Rationale: `AGENTS.md` requires reuse before new types; renaming is outside this plan.
  Date/Author: 2026-10-24, agent.
- Decision: Dry Run playback and the robot model copy move from `SceneMachining` into one
  `ScenePlayback` shared by chamfer and calibration, holding a `std::variant` of the two immutable
  motion handles.
  Rationale: the timer, elapsed time, pause/resume, Stop to HOME and readiness loss are the same
  operation for both motions; a second copy in the calibration coordinator would violate the
  code-reduction rule.
  Date/Author: 2026-10-24, agent.
- Decision: `WorkspaceController` gets a fourth constructor collaborator, `ScenePlayback*`, and
  announces calibration activity through a new signal `calibrationActiveChanged(bool)` that
  `main.cpp` connects to `SceneCalibration::setActive`.
  Rationale: leaving Machining or Calibration must stop playback before the tool or input changes,
  which needs a direct call to the playback owner; the four collaborators are distinct (document,
  machining input, active tool, playback). A fifth collaborator for calibration would exceed the
  four-parameter limit, so calibration activation uses the signal.
  Date/Author: 2026-10-24, agent.
- Decision: Generate in the Calibration submode runs immediately with the fixed parameters of
  "The touch geometry" and opens no settings editor; Dry Run plays the current calibration path
  without requiring a selection.
  Rationale: the user listed only Measuring EE, Generate Path and Dry Run for this toolbar and
  accepted the touch set as it is; only one calibration path exists.
  Date/Author: 2026-10-24, user (toolbar) and agent (no editor).
- Decision: Stop is shown next to Dry Run in the Calibration toolbar.
  Rationale: Pause/resume and Stop to HOME are part of the existing Dry Run behavior that the user
  asked to reuse.
  Date/Author: 2026-10-24, agent.
- Decision: the sphere and calibration path are visible only while Measuring mode with the
  Calibration submode is active. A changed sphere file, a changed measuring TCP or a changed robot
  model removes the calibration path.
  Rationale: the user asked for the sphere to appear when switching to Calibration; a path
  computed for another sphere or TCP is invalid.
  Date/Author: 2026-10-24, user (appearance) and agent (removal).


## Outcomes & Retrospective

Not started.


## Context and Orientation

All paths are relative to the repository root `E:\Qt\QtProjects\RoboCrap`. Read `AGENTS.md`
first. Its mandatory rules apply to every change: no C++ lambdas, no project templates, at most
four function parameters with a recorded reason for three or four, results only through return
values, try/catch only around external-library calls, no project tests. Before each milestone
read the `docs/AGENT_CONTRACTS.md` sections it names; they are binding constraints, and
Milestone 7 updates the ones this plan changes.

The program is a Windows Qt Quick application (C++17, Qt ≥ 6.8, CMake ≥ 3.16, bundled Eigen) that
prepares motions for a KUKA KR 10 R1420 robot on a KR C4 controller.

Coordinate systems. {B} is the robot base system; it coincides with the KUKA WORLD system because
`$BASE` and `$ROBROOT` are null frames, and all scene positions are in {B}, in millimetres. {F} is
the robot flange system; `Kr10Kinematics::solveFK` (`src/kinematics/`) returns the flange as its
last transform. {P} is the probe TCP system: origin at the ruby ball centre, axes parallel to
{F}, z-axis pointing away from the flange along the stylus. A pose is written in KUKA form X, Y,
Z (mm) and A, B, C (degrees), with rotation R = Rz(A)·Ry(B)·Rx(C); `euler2rot` in
`src/geometry/include/geometry/utils.h` implements this, and `makeTransform(rotation, origin)`
builds a 4×4 transform matrix.

Probe and sphere. The ruby ball diameter is 3 mm (`SceneEndEffectors::rubyBallDiameter`). The
sphere has a 25 mm diameter and stands on a vertical pole on a table in front of the robot. At
contact the ball centre is d = 14 mm from the sphere centre c. The probe signal reaches the PLC
input X2.5; this plan does not use it.

Manual points. In T1 with tool TOOL_DATA[5] and base World, the user jogged the probe onto the
sphere ten times: five times in orientation O1 (A = 180°, B = 0°, C = 180°, stylus down), five
times in orientation O2 (A = 0°, B = 0°, C = 180°, rotated 180° about the stylus). Each set is one
touch from above and four horizontal touches from +X, −X, +Y and −Y. At each trigger the smartPAD
pose was recorded; `resources/calibration_sphere_jog_points.txt` holds them.

The chamfer workflow, which this plan imitates. `SceneMachining`
(`src/application/scene/scenemachining.{h,cpp}`) owns chamfer Generate and Dry Run.
`SceneMachining::generate` copies numerical inputs and runs `ChamferMotion::create`
(`src/trajectories/chamfer/chamfermotion.cpp`) on a `std::async` worker polled every 30 ms by
`finishCalculation`, which inserts a scene object. Dry Run (`dryRun`, `presentPlayback`,
`advancePlayback`, `pause`, `stop`, `endPlayback`, `failPlayback`) evaluates
`ChamferMotion::evaluateJoints(seconds)` every 16 ms and calls
`RoboCrap3D::RobotController::applyPlayback(joints, flangeToTcp)`
(`src/application/robot/robotcontroller.cpp`), which runs forward kinematics and updates the
viewport. Stop presents time zero, which is HOME, then ends playback. `RobotController` passes the
robot model to `SceneMachining::setRobotModel` once and availability to `setRobotAvailable`.

Inside `ChamferMotion::create`, the private class `MotionCompiler` builds nodal animation points
(`ChamferJointPoint`: joints, FK-derived TCP, progress, time). A PTP segment ("point to point", a
synchronized joint motion) is sampled by `MotionCompiler::appendPtp` in joint steps of at most 2°
and timed by `HomeTimer::timePtp` with a quintic smoothstep. A LIN segment (a straight Cartesian
TCP motion) uses `TransferProfile` (trapezoidal speed: acceleration, cruise, deceleration),
`MotionCompiler::appendTransfer` (a fixed number of points per constant-acceleration span,
N = max(1, ceil(T·sqrt(a/(8·0.01 mm)))) with span duration T in s and acceleration a in mm/s²,
each solved by seeded IK and checked by FK) and `inspectTransferRates` (joint speed checks).
`ChamferMotion::prepareJointSpline` fits one clamped cubic joint spline per LIN range with zero
end velocities and checks each interval with `jointSplineIntervalError`.
`ChamferMotion::evaluateJoints` interpolates PTP ranges by smoothstep between their end joints and
LIN ranges by the spline. The touch motion needs exactly these operations, so Milestone 1 makes
them shared. `ChamferTcpSolver` (`src/trajectories/chamfer/chamfertcpsolver.h`) is the checked TCP
inverse kinematics: seeded IK, joint limits, one FK, flange and TCP residual checks against
0.0001 mm and 0.000001 rotation-matrix entries.

Scene and drawing. `SceneModel` (`src/application/scene/scenemodel.{h,cpp}`) owns `SceneObject`s
(`sceneobject.{h,cpp}`) with a kind, a classification and a `Shape`
(`std::variant<std::monostate, BoundedPlane, BoundedCylinder, SceneEdge>`). `SceneObject::drawing`
converts the shape into `RoboCrap3D::DrawingObject` parts defined in
`src/viewport/include/viewport/occcontroller.h` (`DrawingPlane`, `DrawingCylinder`,
`DrawingCurve`, a polyline `QList<V3d>`, a line list). `makeShape` in
`src/viewport/adapters/occsceneadapter.cpp` turns each part into an OCCT shape.
`SceneModel::appendObject` asks the viewport to accept the drawing before publishing the object.

Workspace and QML. `WorkspaceController` (`src/application/workspacecontroller.{h,cpp}`) owns the
mode (Measuring, Machining) and submode (Rough, Precise), and calls `SceneMachining::stop` when
leaving Machining. `src/application/qml/Views/Workspace/WorkflowPanel.qml` shows the mode and
submode buttons and declares a QML enum `Submode { Rough, Precise }` mirroring the C++ enum.
`ModeToolBar.qml` shows tool buttons by mode and submode. `Main.qml` binds the toolbar to
`App.Machining` and shows status text in its footer. `ScenePanel.qml` groups scene objects in a
`ListModel` named `groups`. `EndEffectorPanel.qml` is created twice in `Main.qml` (`measuring:
true` and `false`); its TCP rows are a `Repeater` of six `QxHField`/`QxTextInput` rows editing
`root.drafts`, followed by an Apply `QxButton`, all hidden when `measuring` is true. QML singletons
are registered in `src/application/backendqmltypes.h` with `QML_FOREIGN`, `create()` and
`claimQmlSingleton`; `src/application/main.cpp` constructs every application object on its stack.

Deployment. `cmake/DeployOcct.cmake` runs after linking `robocrap`: its first part generates a
configuration file with `file(GENERATE ...)` containing `set(...)` lines for source paths, and its
second part copies OCCT DLLs, OCCT resources and `resources/cad/kr10/*.brep` beside the
executable.

Line-count baseline (physical / nonblank lines, 2026-10-24): chamfermotion.cpp 710/669,
scenemachining.cpp 412/383, scenemachining.h 102/96, sceneobject.cpp 190/174, sceneobject.h 89/79,
scenemodel.cpp 218/192, scenemodel.h 58/50, scenegeometry.h 102/96, sceneendeffectors.cpp 122/104,
sceneendeffectors.h 59/48, workspacecontroller.cpp 111/95, workspacecontroller.h 66/57,
robotcontroller.cpp 182/160, robotcontroller.h 60/48, main.cpp 154/113, backendqmltypes.h 158/141,
occcontroller.h 164/141, occsceneadapter.cpp 265/248, Main.qml 455/396, ModeToolBar.qml 238/202,
WorkflowPanel.qml 135/114, ScenePanel.qml 303/255, EndEffectorPanel.qml 231/227,
MachiningPanel.qml (capture at Milestone 2 start), DeployOcct.cmake (capture at Milestone 4
start). Capture counts at the start and end of every milestone, including new files, and record
C++, QML and build inputs separately in `Progress`.


## The touch geometry

Inputs: the fitted sphere centre c (mm, {B}); d = 14 mm; approach gap g = 3 mm; overtravel
o = 1.5 mm (stored for the later RSI plan, unused by the animation); via clearance h = 25 mm;
tilt θ_t = 45°; side-touch angle α_s = 75°; approach feed v = 1 mm/s.

The {P} orientation of a touch is defined by tilt θ, tilt azimuth φ and roll ψ, in degrees:

    R_P(φ, θ, ψ) = Rz(φ) · Ry(θ) · Rx(180°) · Rz(ψ)

For θ = 0 the {P} z-axis is −e_z: the stylus points down. ψ is a rotation about the stylus. The
user's manual orientation O1 (A = 180°, B = 0°, C = 180°) equals R_P(0, 0, 180°), and O2
(A = 0°, B = 0°, C = 180°) equals R_P(0, 0, 0). For θ > 0 a tip contact lies at sphere longitude φ
and latitude 90° − θ.

The touch direction is given in {P} by the angle α from the stylus axis and the azimuth β around
it. The unit vector from the ball centre towards the sphere centre, in {P}, is

    u_P(α, β) = (sin α cos β, sin α sin β, cos α)

and the outward sphere normal at the intended contact, in {B}, is
n(φ, θ, ψ, α, β) = −R_P(φ, θ, ψ) · u_P(α, β). For each touch:

    contact position  p_contact = c + d · n
    start position    p_start   = c + (d + g) · n
    via position      p_via     = (p_start.x, p_start.y, c.z + d + g + h)

All three TCP poses of a touch have the rotation R_P of that touch. Approach moves the TCP from
p_start to p_contact, a translation of length g along −n; Retract moves it back.

The 27 touches, in execution order (tip means α = 0; side means α = α_s):

    Group A, θ = 0°,  φ = 0°,   ψ = 0°:    tip; side β = 0°, 90°, 180°, 270°
    Group B, θ = 0°,  φ = 0°,   ψ = 90°:   tip; side β = 0°, 180°
    Group C, θ = 0°,  φ = 0°,   ψ = 180°:  tip; side β = 0°, 180°
    Group D, θ = 45°, φ = 0°,   ψ = 0°:    tip; side β = 0°, 90°, 270°
    Group E, θ = 45°, φ = 90°,  ψ = 0°:    tip; side β = 0°, 90°, 270°
    Group F, θ = 45°, φ = 180°, ψ = 0°:    tip; side β = 0°, 90°, 270°
    Group G, θ = 45°, φ = 270°, ψ = 0°:    tip; side β = 0°, 90°, 270°

Expected contact positions on the sphere (latitude, longitude in degrees, computed once in the
agent's scratch directory): A tip (90, –); A side β = 0°, 90°, 180°, 270°: (15, 180), (15, 90),
(15, 0), (15, 270); B tip (90, –), B side β = 0°, 180°: (15, 90), (15, 270); C tip (90, –),
C side β = 0°, 180°: (15, 0), (15, 180); for D–G with azimuth φ: tip (45, φ), side β = 0°:
(60, φ + 180), side β = 90°: (10.5, φ + 79.3), side β = 270°: (10.5, φ − 79.3), longitudes
modulo 360. β = 180° is excluded for tilted groups because it gives latitude −30°, below the
equator, towards the pole.


## Plan of Work

### Milestone 1: shared PTP/LIN/joint-spline operations (no behavior change)

Scope: make the PTP, LIN and joint-spline operations of `chamfermotion.cpp` reusable without
copying them. At the end, `src/trajectories/motion/jointsegments.h` and `jointsegments.cpp`
(internal to `RoboCrapTrajectories`, not under `include/`) contain the operations, and chamfer
Generate produces exactly the same points, duration and error texts as before. Read the contract
sections "Machining and playback" and "Geometry and scene" first.

Move from the unnamed namespace of `chamfermotion.cpp`, with unchanged bodies, numerical order,
tolerances and error texts: `rigid`, `withinLimits`, `quadraticUnitRoots`,
`jointSplineIntervalError`, `validRobot`, `smoothProgress`, `smoothVelocity`, `inverseProgress`
and the constant `TransferPositionTolerance`. `MaximumTransferFeed` moves too, because the touch
motion applies the same `$VEL_MA.CP` bound. Then create:

`struct LinearProfile`, the former `TransferProfile` with the same members and its `duration`,
`progress` and `target` methods, and `std::optional<LinearProfile> linearProfile(const M4d& first,
const M4d& last, double feed, double acceleration)` containing the body of `transferProfile` after
the two poses are known (four parameters: the two end poses and the two speed-profile limits of
one LIN). The chamfer `transferProfile(path, side, timing)` keeps only the evaluation of its staging
and lead poses and calls `linearProfile(first, last, timing.transferFeed,
timing.transferAcceleration)`.

`struct LinearSchedule { QVector<double> localTimes; double startTime; QString error; }` and
`LinearSchedule linearSchedule(const LinearProfile& profile, double acceleration, double
startTime)`, containing the counting loop of `appendTransfer` (the per-span interval counts with
the 200000-point limit and the local time of every point after the first). Three parameters: the
profile, the acceleration used for counting, and the absolute start time that the solver needs for
its monotonic-time check.

`class TransferSolver`, owning `Kr10Kinematics` and the inverse tool transform and borrowing the
worker-owned `ChamferRobotSetup` (the state `MotionCompiler` holds today in `m_solver` and
`m_tcpToFlange`). Its methods are the former `MotionCompiler::inverse` (`inverse(tcp, seed)`),
`forward(joints, progress)`, `kinematics()`, `samplePtp(start, end)` returning the points of the
former `appendPtp` loop, and `LinearSamples solveLinear(const LinearProfile&, const
LinearSchedule&, const Kr10Kinematics::IkSolution& seed, const QString& label)` returning
`struct LinearSamples { QVector<ChamferJointPoint> points; Kr10Kinematics::IkSolution last;
QString error; }`. `solveLinear` contains the IK loop of `appendTransfer`; `label` is the text the
current code inserts with `centralPhaseName(m_phase)`, so the produced errors are character for
character identical, including the unprefixed "Transfer timestamps cannot be resolved.". Four
parameters: profile, schedule, seed and error label are distinct inputs of one LIN solve.

`QString inspectLinearRates(const QVector<ChamferJointPoint>& points, qsizetype first, const V6d&
jointSpeed, const QString& label)`, the former `inspectTransferRates`, with the same label rule.

`double ptpDuration(const V6d& delta, const ChamferTimingParameters& timing)`, the duration
calculation of `HomeTimer::timePtp`.

`struct JointSplineResult { QVector<std::array<V6d, 4>> coefficients; QString error; }` and
`JointSplineResult clampedJointSpline(const QVector<ChamferJointPoint>& points, qsizetype first,
qsizetype last, const ChamferRobotSetup& robot)`, the body of `ChamferMotion::prepareJointSpline`
up to the interval checks; `prepareJointSpline` copies the coefficients into `m_jointSpline`.

`MotionCompiler::appendTransfer` becomes: build the profile, check its start against the
preceding TCP, call `linearSchedule`, keep the existing 400000-point check between
`linearSchedule` and `solveLinear` (this preserves the first failure), append the solved points,
then call `inspectLinearRates`. Add both new files to `src/trajectories/CMakeLists.txt`.

Acceptance: chamfer Generate on the sample edge prints the same console line before and after.
Before editing, ask the user to run the current application and report the line, using:
Measuring → Rough → Plane from JSON `resources/json/edge_cyl_plane/XP.json` → Cylinder from JSON
`XC.json` → select both → Intersect Selected → Machining → select the edge → Generate Path →
Generate with default settings. The line has the form

    Chamfer preparation: <points> points, <seconds> s movement

After the milestone, ask the user to build `robocrap` and repeat; the numbers must be identical.

### Milestone 2: ScenePlayback (no behavior change)

Scope: one owner of Dry Run playback and of the robot model copy. At the end
`src/application/scene/sceneplayback.{h,cpp}` exist, `SceneMachining` holds no timer, clock,
elapsed time or robot-controller pointer, and chamfer Dry Run, Pause, Stop and readiness loss
behave as before. Read the contract sections "Machining and playback", "Build and modules" and
"QML" first.

Interface (QML singleton `Playback`, registered in `backendqmltypes.h` like `Machining`):

    class ScenePlayback : public QObject
      Q_PROPERTY(bool active READ active NOTIFY playbackChanged)
      Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
      Q_PROPERTY(double elapsed READ elapsed NOTIFY playbackChanged)
      Q_PROPERTY(double duration READ duration NOTIFY playbackChanged)
      Q_PROPERTY(QString phase READ phase NOTIFY playbackChanged)
      explicit ScenePlayback(SceneModel* scene, QObject* parent = nullptr)
      using Motion = std::variant<std::shared_ptr<const ChamferMotion>>   (Milestone 6 adds the
                                                                            ProbeTouchMotion handle)
      bool start(Motion motion, quint32 ownerId)
      Q_INVOKABLE void pause()
      Q_INVOKABLE void stop()
      void end()
      quint32 ownerId() const
      const std::optional<RoboCrap3D::Kr10KinematicModel>& robotModel() const
      bool robotAvailable() const
      void setRobotModel(const RoboCrap3D::Kr10KinematicModel& model)
      void setRobotAvailable(bool available)
      void setRobotController(RoboCrap3D::RobotController* controller)
    signals: playbackChanged(), failed(quint32 ownerId, const QString& error),
             robotModelChanged(), robotAvailabilityChanged()

`start` receives the bodies of `SceneMachining::dryRun` after its path check: when paused with
the same owner it resumes from the elapsed time, otherwise it stores the motion and owner,
presents time zero and starts the clock and the 16 ms precise timer. `pause`, `stop`, `end`
(former `endPlayback`), `presentPlayback`, `advancePlayback` and the failure path move unchanged;
a failure calls `end()` and emits `failed(ownerId, error)` instead of setting a coordinator error.
`presentPlayback` uses `std::get_if` on the variant, calls `RobotController::applyPlayback(joints,
motion->robot().flangeToTcp)` and sets the existing chamfer phase texts. The constructor connects
`SceneModel::objectRemoved` (ending playback whose owner was removed) and
`QCoreApplication::aboutToQuit` (ending playback). `setRobotAvailable(false)` ends playback.

`SceneMachining(SceneModel*, SceneEndEffectors*, ScenePlayback*)` (three collaborators: document,
tool calibration, playback) keeps Generate, input selection, compatibility, `canPlay` and a
`dryRun()` that checks the selected path and calls `ScenePlayback::start`. It loses the playback
properties, `pause`, `stop`, `setRobotModel`, `setRobotAvailable` and `setRobotController`. Its
former `setRobotModel` work (joint speeds into settings, then `calibrationChanged`) runs from
`ScenePlayback::robotModelChanged`; `refreshCompatibility` runs from `robotAvailabilityChanged`;
`availabilityChanged` is emitted on `playbackChanged`; `failed` with a machining owner sets the
machining error. `RobotController(OccController*, SceneEndEffectors*, ScenePlayback*)` calls the
playback object where it called `SceneMachining`. `WorkspaceController(SceneModel*,
SceneMachining*, SceneEndEffectors*, ScenePlayback*)` stops playback through `ScenePlayback::stop`
when leaving Machining, before the tool and input change, exactly where it calls
`SceneMachining::stop` today. `main.cpp` constructs `ScenePlayback` after `SceneModel` and before
`SceneMachining`, sets its singleton instance and calls `setRobotController` where it now calls
`machining.setRobotController`.

QML: in `Main.qml`, `ModeToolBar.playing`, `playbackActive`, `onPauseRequested` and
`onStopRequested` use `App.Playback`; `playbackAvailable` and `onDryRunRequested` stay on
`App.Machining`; `generationAvailable` uses `!App.Playback.active`. `MachiningPanel.qml` gets a
required property for the playback object and reads `playing`, `active`, `phase`, `elapsed` and
`duration` from it; update its creator in `Main.qml`.

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` (hot reload OFF) and to report
errors and warnings; then, with the chamfer path from Milestone 1, Dry Run plays, Pause holds the
pose, Dry Run resumes, Stop returns to HOME, playback ends at HOME, the Machining panel shows the
same status line as before, and switching to Measuring during playback returns to HOME.

### Milestone 3: measuring TCP

Scope: the application knows the ruby ball centre in {F}. Read "Geometry and scene" and "QML".

In `SceneEndEffectors` add `Q_PROPERTY(QList<double> measuringTcp READ measuringTcp NOTIFY
measuringTcpChanged)`, `const std::optional<M4d>& measuringTcpFrame() const`, `Q_INVOKABLE bool
applyMeasuringTcp(const QList<double>& pose)` and the signal `measuringTcpChanged()`. The
constructor sets (−0.048, −0.097, 127, 0, 0, 0) and its transform through the existing
`poseTransform`; `applyMeasuringTcp` rejects anything but six finite values, does nothing when the
value is unchanged, otherwise stores list and frame and then emits.

In `EndEffectorPanel.qml` add `signal measuringTcpRequested(var tcpValues)`. Show the existing
TCP caption, six TCP rows and the Apply button also when `measuring` is true; the caption reads
"Ruby ball centre relative to flange" when measuring and "TCP pose relative to ER" otherwise; the
"TCP trihedron" row stays spindle-only; Apply emits `measuringTcpRequested(drafts as numbers)` when
measuring and `posesRequested(...)` otherwise; `validDraft` checks only `drafts` when measuring.
In `Main.qml` the measuring panel binds `tcp: App.EndEffectors.measuringTcp` and handles
`onMeasuringTcpRequested: values => App.EndEffectors.applyMeasuringTcp(values)`. The panel already
scrolls its content, so the extra rows do not change its size.

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report; in Measuring mode
"Measuring EE" shows −0.0480, −0.0970, 127.0000, 0.0000, 0.0000, 0.0000; an edited value stays
after Apply, Close and reopening.

### Milestone 4: sphere file, sphere object and Calibration submode

Scope: entering the Calibration submode shows the fitted sphere. Read "Build and modules",
"Viewport and CAD", "Geometry and scene" and "QML".

Sphere fit. Add `src/geometry/include/geometry/spherefit.h` and `src/geometry/spherefit.cpp` to
`src/geometry/CMakeLists.txt` with `struct SphereFit { V3d centre; double radius; double rms; }`
and `std::optional<SphereFit> fitSphere(const QVector<V3d>& points)`. The fit is the algebraic
linear least-squares fit: for each point p, ‖p‖² = 2·pᵀc + k is linear in the unknowns c (three
numbers) and k = r² − ‖c‖². Subtract the mean point from all points first, solve the N×4 system
with Eigen `JacobiSVD`, and reject fewer than four points, nonfinite input, a smallest singular
value below 1e-9 times the largest (coplanar points) and r² ≤ 0. Return c plus the mean,
r = sqrt(k + ‖c‖²) and rms = sqrt(mean((‖p − c‖ − r)²)).

Sphere drawing. Add `struct DrawingSphere { V3d centre = V3d::Zero(); double radius = 0.0; }` to
`occcontroller.h` and to the `DrawingPart::geometry` variant, and a branch in `makeShape`
(`occsceneadapter.cpp`) building `BRepBuilderAPI_MakeFace(gp_Sphere(gp_Ax3(toOccPoint(centre),
gp_Dir(0.0, 0.0, 1.0)), radius))` and returning the face when done. `BRepBuilderAPI` is in the linked
`TKTopAlgo`; add `#include <gp_Sphere.hxx>`.

Scene value. In `scenegeometry.h` add `struct SceneSphere { V3d centre = V3d::Zero(); double
radius = 0.0; double fittedContactRadius = 0.0; double rms = 0.0; QList<V3d> roughPoints; }`;
add it to `SceneObject::Shape` with a `sphere()` lookup, and add the kinds `Sphere` and
`CalibrationPath` to `SceneObject::Kind`. `SceneObject::drawing` returns one `DrawingSphere` part;
`drawingDiagnostics` returns the rough points as markers; `properties` returns rows "Centre",
"Diameter", "Fitted contact radius", "Fit RMS" and "Points". Add `SceneObject*
SceneModel::addSphere(SceneSphere sphere, const QString& name)` like `addBoundedPlane`. In
`ScenePanel.qml` add a "Calibration" group for kinds `Sphere` and `CalibrationPath`, and return
`qrc:/pics/calibration.svg` and `qrc:/pics/path.svg` from `objectIcon`.

Sphere file. Create `resources/json/calibration_sphere.json` from the ten lines of
`resources/calibration_sphere_jog_points.txt`, one `roughPoints` entry per `DECL E6POS` line with
its X, Y, Z, A, B, C, S and T values (E1–E6 omitted):

    {
      "version": 1,
      "sphereDiameter": 25.0,
      "roughPoints": [
        {"X": 1053.17981, "Y": -332.410950, "Z": 468.076385,
         "A": -179.999786, "B": 0.00114061497, "C": 179.999619, "S": 2, "T": 10},
        ...
      ]
    }

A valid file has `version` 1, a finite positive `sphereDiameter` and at least four entries with
finite X, Y and Z; this plan uses only X, Y and Z, and ignores unknown keys. In
`cmake/DeployOcct.cmake`, add to the generated configuration the line
`set(calibration_source [==[${PROJECT_SOURCE_DIR}/resources/json/calibration_sphere.json]==])`,
and at the end of the script copy it only when absent:

    set(calibration_target "${ROBOCRAP_DEPLOY_DEST}/resources/json/calibration_sphere.json")
    if(EXISTS "${calibration_source}" AND NOT EXISTS "${calibration_target}")
      file(MAKE_DIRECTORY "${ROBOCRAP_DEPLOY_DEST}/resources/json")
      file(COPY "${calibration_source}" DESTINATION "${ROBOCRAP_DEPLOY_DEST}/resources/json")
    endif()

Coordinator. Add `src/application/scene/scenecalibration.{h,cpp}`: `class SceneCalibration`,
QML singleton `Calibration`, `SceneCalibration(SceneModel*, SceneEndEffectors*, ScenePlayback*)`
(three collaborators: document, measuring TCP, playback and robot model), properties `error`,
`unavailableReason`, `canGenerate`, `canPlay`, `calculating` with their signals, the slot
`setActive(bool)`, and `Q_INVOKABLE generate()` and `dryRun()` (implemented in Milestones 5 and
6). `setActive(true)` reads `QCoreApplication::applicationDirPath() +
"/resources/json/calibration_sphere.json"` with `QFile` and `QJsonDocument` on the GUI thread,
validates it, calls `fitSphere`, and builds `SceneSphere` with radius `sphereDiameter / 2` at the
fitted centre. If no sphere object exists, or its centre, radius or points differ from the new
value, it inserts the new object, then removes the old sphere and any calibration path. It then
makes both visible. `setActive(false)` calls `ScenePlayback::stop` if the playback owner is the
calibration path, then hides the sphere and the path. An unreadable or invalid file removes
nothing and sets `error` to the path and the failed condition, for example "Calibration sphere
file <path>: roughPoints needs at least 4 points with finite X, Y, Z."

Submode. Add `Calibration` to `WorkspaceController::Submode` and to the QML enum in
`WorkflowPanel.qml`; accept it in `setSubmode`; add a third submode button "Calibration" with a
new icon `resources/pics/calibration.svg` registered in `resources/pics.qrc`. `WorkspaceController`
emits `calibrationActiveChanged(bool)` whenever "mode is Measuring and submode is Calibration"
changes, after stopping playback when it becomes false; `main.cpp` connects it to
`SceneCalibration::setActive`. In `ModeToolBar.qml` add `readonly property bool calibration` and
the inputs and signals the calibration buttons need; in the Calibration submode show only
"Measuring EE", "Generate Path", "Dry Run"/"Pause" and "Stop", and hide "Intersect Selected", the
JSON imports, "Scan" and "Dry Scan". `Main.qml` binds those buttons to `App.Calibration` and
`App.Playback`, its footer label reads "Mode: Measuring / Calibration", and its status message
shows `App.Calibration.error`, else `App.Calibration.unavailableReason`, in the existing label.
Inspect the toolbar, Workflow panel and viewport sizes when switching submodes and when messages
change (`AGENTS.md`, Mandatory interface stability).

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report. Then, in the
running application, clicking Calibration shows a 25 mm sphere; selecting it shows Centre
[1054.1403; -331.9692; 454.2727], Diameter 25.0000, Fitted contact radius 13.8342, Fit RMS 0.0694
and Points 10 (the values from `Surprises & Discoveries`); the points overlay shows the ten
readings on a shell about 14 mm from the centre; clicking Rough hides the sphere; changing one
coordinate in the deployed file and clicking Calibration again moves the sphere; renaming the file
shows the path error in the status bar without moving any control.

### Milestone 5: touch plan, touch motion, Generate and path drawing

Scope: Generate Path in the Calibration submode computes the 27-touch motion and draws it. Read
"Machining and playback" and "Geometry and scene".

Add to `RoboCrapTrajectories` the published headers `include/trajectories/probetouchplan.h` and
`include/trajectories/probetouchmotion.h` with sources `probe/probetouchplan.cpp` and
`probe/probetouchmotion.cpp`, all listed in `src/trajectories/CMakeLists.txt`.

`probetouchplan.h` defines `struct ProbeTouchPlanParameters { V3d sphereCentre = V3d::Zero();
double contactDistance = 14.0; double approachGap = 3.0; double overtravel = 1.5; double
viaClearance = 25.0; double tiltDegrees = 45.0; double sideAngleDegrees = 75.0; double approachFeed
= 1.0; }`, `struct ProbeTouch { M4d via; M4d start; M4d contact; V3d normal; int group; }`,
`struct ProbeTouchPlan { ProbeTouchPlanParameters parameters; QVector<ProbeTouch> touches; }` and
`struct ProbeTouchPlanResult { std::optional<ProbeTouchPlan> plan; QString error; }` returned by
`ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters)`, which
rejects nonfinite values and nonpositive distances, angles outside (0°, 90°] and a nonpositive
feed, and builds the 27 touches of "The touch geometry" in its order.

`probetouchmotion.h` defines `enum class ProbeSegment { ToVia, Descend, Approach, Retract, Ascend,
ReturnHome }`, `struct ProbeMotionPhase { int touch; ProbeSegment segment; }`, and `class
ProbeTouchMotion` with `static ProbeTouchMotionResult create(const ProbeTouchPlan& plan, const
ChamferRobotSetup& robot)`, `const QVector<ChamferJointPoint>& points() const`, `const
ChamferRobotSetup& robot() const`, `double duration() const`, `double minimumTransferClearance()
const` and `std::optional<std::pair<V6d, ProbeMotionPhase>> evaluateJoints(double seconds)
const`; `struct ProbeTouchMotionResult { std::optional<ProbeTouchMotion> motion; QString error; }`.
The motion stores its points, one entry per segment (first and last point index, kind PTP or
LIN, `ProbeMotionPhase`) and one coefficient array per LIN segment.

`create` validates the robot with `validRobot`, copies the rated joint speeds from the model as
`ChamferMotion::create` does, rejects `transferFeed` above `MaximumTransferFeed` or
`transferAcceleration` above `cartesianAcceleration` with the same error text as chamfer, and
builds the segments in order with a `TransferSolver`: the HOME point; ToVia as PTP to the first
via (target joints from `ChamferTcpSolver::solveTcp` seeded with the HOME configuration, which
`TransferSolver::kinematics().configuration(qHome, solveFK(qHome))` provides); for each touch,
Descend (LIN via→start at `transferFeed`), Approach (LIN start→contact at `approachFeed`), Retract
(LIN contact→start at `transferFeed`) and Ascend (LIN start→via at `transferFeed`), all LINs with
`transferAcceleration`, each LIN sampled with `linearSchedule`, `solveLinear` and
`inspectLinearRates`; between touches ToVia as PTP to the next via (target from `solveTcp` seeded
with the current solution); finally ReturnHome as PTP to `qHome`. PTP segments are sampled with
`TransferSolver::samplePtp` and timed with `ptpDuration` and `inverseProgress` as
`HomeTimer::timePtp` does; each LIN segment gets its own `clampedJointSpline`. Every point of
ToVia, Descend, Ascend and ReturnHome must satisfy ‖p_tcp − c‖ ≥ d + g − 0.0001 mm; the smallest
such distance is stored as `minimumTransferClearance`. Every error starts with
"Touch <n>/27, <segment>: " (or "Return HOME: ") followed by the underlying message. The total
point count is limited to 400000 as in chamfer. `evaluateJoints` returns the first joints at 0
and the last at `duration()`, smoothstep interpolation between PTP end joints and spline values on
LIN segments, and checks joint limits like `ChamferMotion::evaluateJoints`.

In `SceneCalibration::generate`: refuse while calculating, while `ScenePlayback::active()`, without
a sphere, without a robot model or availability; build `ProbeTouchPlanParameters` from the sphere
centre, `contactDistance` = sphere radius + `rubyBallDiameter / 2`, and defaults; copy the model,
`measuringTcpFrame` and default `ChamferTimingParameters` into `ChamferRobotSetup`; record the
current revision; and run plan and motion creation on a `std::async` worker polled every 30 ms.
The revision increases on sphere replacement, `measuringTcpChanged` and
`ScenePlayback::robotModelChanged`, and each increase removes the calibration path. On the GUI
thread a stale revision is rejected with "Sphere, probe TCP or robot changed. Generate again.";
an accepted result is inserted as a `CalibrationPath` object whose shape is
`SceneTouchPath { QList<QList<V3d>> polylines; }` (new value in `scenegeometry.h`, one polyline of
saved TCP positions per segment, drawn as `QList<V3d>` parts), while `SceneCalibration` keeps the
`std::shared_ptr<const ProbeTouchMotion>` for that object id; an older path is removed after the
new one is inserted, and removing the path object drops the motion. One console line is logged:

    Probe calibration preparation: 27 touches, <points> points, <seconds> s movement, minimum transfer clearance <mm> mm

or `Probe calibration preparation failed: <error>`. `Main.qml` shows "Calculating trajectory…"
with the busy indicator while `App.Calibration.calculating` is true.

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report; then, in
Calibration, Generate Path either prints the success line with a minimum transfer clearance of at
least 17.0000 mm and draws the path around the sphere, with approach segments ending on the 14 mm
contact shell at the latitudes listed in "The touch geometry", or prints a failure naming touch
and segment. Record a failure, the sphere position and the user's decision in this plan before
changing any parameter.

### Milestone 6: Dry Run of the calibration motion

Scope: animate the generated motion. Add `std::shared_ptr<const ProbeTouchMotion>` as the second
alternative of `ScenePlayback::Motion`; `presentPlayback` evaluates it with
`motion->robot().flangeToTcp` and sets the phase to "Touch <n>/27: <segment>" with the segment
texts "to via", "descend", "approach", "retract", "ascend", or to "Return HOME".
`SceneCalibration::canPlay` is true when a calibration path exists, the robot is available and
nothing is calculating; `dryRun` calls `ScenePlayback::start(motion, pathId)`; `failed` with the
calibration path as owner sets `SceneCalibration::error`.

Acceptance: ask the user to build `robocrap` and report, then to run: Calibration → Generate Path
→ Dry Run. The robot with the measuring tool moves from HOME through the 27 touches, the ball
stops on the sphere surface in every approach, and the motion ends at HOME; Pause, resume and Stop
behave as in chamfer Dry Run; clicking Rough during playback returns to HOME; Apply of a different
measuring TCP removes the path and disables Dry Run.

### Milestone 7: documentation, contracts and compliance

Update `docs/AGENT_CONTRACTS.md`: in "Machining and playback", playback and the robot model copy
now belong to `ScenePlayback`, `RobotController` publishes the model to it once, and the
calibration Generate, touch motion and its checks are added; in "Geometry and scene", the sphere
fit and sphere object; in "Viewport and CAD", the sphere drawing part; in "Build and modules", the
copy-if-absent sphere file; in "QML", the Calibration toolbar. Update `docs/ARCHITECTURE.md`
(sections 3.2, 3.3, 3.5, 3.6, 4 and 5) and `docs/APP_DESCRIPTION.md` (a Calibration workflow
step). Then perform the `AGENTS.md` compliance pass on every changed declaration, body and caller
(parameter counts and recorded reasons, result channels, no lambdas or templates, ownership,
repetition), fix findings within scope, and record the findings and the line counts against the
baseline in `Progress` and `Outcomes & Retrospective`.


## Concrete Steps

Working directory: `E:\Qt\QtProjects\RoboCrap`. For every milestone, in this order:

1. Re-read the mandatory sections of `AGENTS.md` and the contract sections the milestone names.
2. Capture line counts of every file the milestone touches, for example in Git Bash:

       wc -l src/trajectories/chamfer/chamfermotion.cpp
       grep -cv '^\s*$' src/trajectories/chamfer/chamfermotion.cpp

3. Make the edits described for the milestone.
4. Run `git diff --check` and `git status --short`; expect no whitespace errors and only the files
   the milestone names, plus new files listed as untracked. Read untracked files in full.
5. Stop and ask the user to build exactly the named targets (and, where stated, run the named
   scenario), and to report compiler errors, lint warnings and the named console lines. Do not run
   any build, lint, application or test yourself. Continue only after the user's report, and
   record it in `Progress`; fix reported failures introduced by the milestone before moving on.
6. Update `Progress`, `Surprises & Discoveries` and `Decision Log`.


## Validation and Acceptance

The plan is complete when the user reports, from a disconnected runtime (no robot, PLC or RSI
connection): the chamfer console line and Dry Run behavior unchanged (Milestones 1–2); the
measuring TCP editable (Milestone 3); the sphere shown with the fit values listed in Milestone 4;
a successful Generate with a minimum transfer clearance of at least 17 mm (Milestone 5); a Dry
Run of all 27 touches that ends at HOME (Milestone 6); and no layout movement of the toolbar,
panels or viewport when switching submodes, showing errors or calculating. Build and lint success
alone prove neither visual correctness nor that the real robot can perform the motion; real robot
motion is not part of this plan.


## Idempotence and Recovery

All steps are additive source edits. Re-entering the Calibration submode re-reads the file and
replaces the sphere only when its content changed. The deployment step never overwrites an
existing deployed sphere file; to deploy a changed repository file, the user deletes the deployed
copy before building. If a milestone fails halfway, the previous milestone's acceptance still
describes a working state; any git command to revert edits requires the user's request.


## Artifacts and Notes

The motion duration is movement time, not computation time. Hypothetical estimate for the default
values: per touch about 3 s descent, 3 s approach at 1 mm/s, 0.4 s retract, 3 s ascent and 1–3 s
PTP, roughly 11–13 s per touch and 5–6 minutes for 27 touches.


## Interfaces and Dependencies

No new external dependency. New published headers: `geometry/spherefit.h`,
`trajectories/probetouchplan.h`, `trajectories/probetouchmotion.h`. New internal Trajectories
files: `motion/jointsegments.{h,cpp}`. New application files: `scene/sceneplayback.{h,cpp}`,
`scene/scenecalibration.{h,cpp}`; new resources: `resources/json/calibration_sphere.json`,
`resources/pics/calibration.svg`. New QML singletons: `Playback`, `Calibration`. Changed
signatures: `RobotController(OccController*, SceneEndEffectors*, ScenePlayback*)`;
`SceneMachining(SceneModel*, SceneEndEffectors*, ScenePlayback*)`;
`WorkspaceController(SceneModel*, SceneMachining*, SceneEndEffectors*, ScenePlayback*)`;
`DrawingPart::geometry` gains `DrawingSphere`; `SceneObject::Kind` gains `Sphere` and
`CalibrationPath`; `SceneObject::Shape` gains `SceneSphere` and `SceneTouchPath`;
`WorkspaceController::Submode` gains `Calibration` and the signal
`calibrationActiveChanged(bool)`; `SceneEndEffectors` gains `measuringTcp`,
`measuringTcpFrame`, `applyMeasuringTcp` and `measuringTcpChanged`; `EndEffectorPanel.qml` gains
`measuringTcpRequested`; `MachiningPanel.qml` gains a required playback property. Every three- or
four-parameter signature above records its reason where it is introduced in this plan.


## Revision notes

- 2026-10-24: `roughPoints` entries became full smartPAD poses (X, Y, Z, A, B, C, S, T) because the
  user's readings contain them; the fit of those readings was added to `Surprises & Discoveries`.
- 2026-10-24: the sphere file moved from `<exe dir>/calibration/` to the repository file
  `resources/json/calibration_sphere.json`, deployed beside the executable only when absent (user
  decision).
- 2026-10-24: full rewrite for self-containment. Added the override that the agent never builds,
  lints, runs or tests and asks the user instead (user instruction); converted the Decision Log and
  Surprises to the `PLANS.md` formats; gave exact interfaces for Milestones 1–6, the deployment
  mechanism and the `EndEffectorPanel` changes; made `WorkspaceController` take `ScenePlayback*` so
  stopping playback stays a direct call; merged the former numerics milestone into Generate so each
  milestone has an observable acceptance; added the O1/O2 to ψ mapping, the expected contact
  positions and the expected fit values from the user's points as acceptance data.
