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
under Measuring mode, a submode button "Calibration" appears first, before "Rough" and "Precise".
Clicking it reads the sphere file `<directory of robocrap.exe>/resources/json/calibration_sphere.json`,
fits the sphere centre to the ten points the user recorded by manual jogging, and shows the sphere
in the 3D scene. The toolbar then shows only "Measuring EE", "Generate Path", "Dry Run" and
"Stop". "Generate Path" computes a motion that starts at HOME, touches the sphere 31 times with
the probe in prescribed orientations and returns to HOME, and draws the path of the ruby ball
centre. "Dry Run" animates the robot along that motion; "Pause" and "Stop" behave as in the
existing chamfer Dry Run. While the Calibration submode is active, the 3D scene shows only the
robot with the measuring tool (MEE), the sphere and the calibration path; every other scene object
is hidden and is not listed in the Scene panel. The sphere and the path are shown on every entry;
in the Scene and Properties panels the user can hide and show both the sphere and the path;
neither can be deleted, and the touch geometry of the path is fixed in code.
Leaving the Calibration submode stops the animation at HOME, hides the sphere and the path, and
restores the visibility the other objects had before entry. Editing the sphere file and entering
Calibration again uses the new file.


## Progress

Record every stopping point here with a UTC timestamp, splitting a partially completed step into
"completed" and "remaining".

- [x] (2026-10-25) Milestone 1 baseline reported by the user before any edit, sample edge with
  default settings: `Chamfer preparation: 375 points, 25.299220225 s movement`.
- [x] (2026-10-25) Milestone 1: shared PTP/LIN/joint-spline operations extracted from
  `src/trajectories/chamfer/chamfermotion.cpp`; chamfer behavior unchanged. The user built
  `robocrap` and repeated Generate on the sample edge: `Chamfer preparation: 375 points,
  25.299220225 s movement`, identical to the baseline; no build errors were reported.
  Line counts, physical/nonblank. C++ before: chamfermotion.cpp 708/667,
  include/trajectories/chamfermotion.h 79/69, chamfer/chamfertcpsolver.h 77/68, total 864/804.
  C++ after: chamfermotion.cpp 358/333, chamfermotion.h 78/68, chamfertcpsolver.h 192/177,
  motion/jointsegments.h 100/84, motion/jointsegments.cpp 267/250, total 995/912 (+131/+108).
  Build inputs: src/trajectories/CMakeLists.txt 17/17 to 19/19. The growth is the price of
  sharing: the moved operations need header declarations, the result structs `LinearLimits`,
  `LinearSchedule`, `LinearSamples` and `JointSplineResult`, includes and comments. The smaller
  alternative, leaving the operations in chamfermotion.cpp, would force Milestone 5 to copy about
  300 lines of PTP, LIN and spline code.
- [x] (2026-10-25) Milestone 2: `ScenePlayback` owns Dry Run playback and the robot model copy;
  chamfer Dry Run unchanged. The user built `robocrap`, ran the lint and the Dry Run scenario
  (play, pause, resume, Stop to HOME, end at HOME, panel status, Measuring during playback) and
  reported everything as working.
  Line counts, physical/nonblank. C++ before: scenemachining.cpp 412/383, scenemachining.h 101/95,
  robotcontroller.cpp 182/160, robotcontroller.h 59/47, main.cpp 154/113, backendqmltypes.h
  158/141, total 1066/939. C++ after: scenemachining.cpp 325/299, scenemachining.h 73/68,
  sceneplayback.cpp 133/119, sceneplayback.h 74/67, robotcontroller.cpp 182/160,
  robotcontroller.h 59/47, main.cpp 156/115, backendqmltypes.h 171/153, total 1173/1028
  (+107/+89). QML: Main.qml 455/396 to 456/397, MachiningPanel.qml 196/194 to 197/195. Build
  inputs: src/application/CMakeLists.txt 127/122 to 129/124. `SceneMachining` lost 115 lines; the
  growth is the new class's header, signal wiring and QML registration. The smaller alternative,
  keeping the player inside `SceneMachining`, would force Milestone 6 to copy about 120 lines of
  timer, pause, Stop and cleanup code into the calibration coordinator. Contract and architecture
  text for the moved playback is updated in Milestone 7.
- [x] (2026-10-25) Milestone 3: measuring TCP (ruby ball centre) in `SceneEndEffectors` and the
  Measuring EE panel. The user built `robocrap`, ran the lint and the panel check and reported
  everything as working. Line counts, physical/nonblank. C++:
  sceneendeffectors.cpp 122/104 to 134/115, sceneendeffectors.h 59/48 to 69/58 (+22/+21). QML:
  EndEffectorPanel.qml 231/227 to 232/228, Main.qml 456/397 to 457/398. The growth is the new
  property, frame, signal and `applyMeasuringTcp`; `applyMeasuringTcp` repeats the eight-line
  pattern of the private `setSpindleTcp` because sharing it would need member and signal
  pointers, which is more code. Compliance pass: one new one-parameter invokable returning its
  result; no lambdas, templates or output parameters.
- [x] (2026-10-26) Milestone 4: sphere fit, sphere drawing, sphere file deployment and the
  Calibration submode with its toolbar. `resources/json/calibration_sphere.json` was created as
  preparation at the user's request; after the review rounds recorded in the Decision Log (button
  order, sphere icon, sphere visibility, Geometry layout with the ball-center and compensated
  spheres, panel width, seam, transparent highlight) the user built the application and accepted
  the representation; on 2026-10-26 the user also reported the remaining checks as passed (other
  objects hidden on entry and restored on leaving, Delete without effect, a deployed-file edit
  moving the sphere on re-entry, the missing-file error in the status bar).
  The fit formula reproduces
  the expected values on the ten readings in a scratch check outside the repository: center
  (1054.1403, −331.9692, 454.2727) mm, radius 13.8342 mm, RMS 0.0694 mm, singular values 55.56,
  54.82, 32.82 and 3.16 (ratio 0.057, well above the 1e-9 rejection).
  Line counts, physical/nonblank. C++ before (occcontroller.h, occsceneadapter.cpp,
  scenegeometry.h, sceneobject.h, sceneobject.cpp, scenemodel.h, scenemodel.cpp,
  backendqmltypes.h, main.cpp, workspacecontroller.h, workspacecontroller.cpp): 1585/1395; after,
  with the new spherefit.h 16/12, spherefit.cpp 52/43, scenecalibration.h 37/31 and
  scenecalibration.cpp 136/119: 1908/1675 (+323/+280). QML (WorkflowPanel, ModeToolBar,
  ScenePanel, PropertiesPanel, Main): 1446/1241 to 1481/1275 (+35/+34). Build inputs:
  src/geometry/CMakeLists.txt 18/18 to 20/20, src/application/CMakeLists.txt 129/124 to 131/126,
  cmake/DeployOcct.cmake 74/71 to 82/78, resources/pics.qrc 27/27 to 28/28. Assets: new
  resources/pics/calibration.svg 5/5. The growth is the required new behavior (sphere file
  parsing with its error texts, the fit, the drawing part, the scene value, the submode and the
  visibility record); the smaller alternative of fitting inside `SceneCalibration` was rejected
  because numerical fitting belongs to `RoboCrapGeometry` (`docs/AGENT_CONTRACTS.md`, Geometry
  and scene). Compliance pass: every new or changed signature has at most two parameters, results
  return through values (`SphereFileResult`, `std::optional<SphereFit>`), no lambdas, templates or
  output parameters; `SceneModel::addSphere` uses `SceneObject`'s private `setShape` method like
  `addBoundedPlane`.
- [x] (2026-10-26) Milestone 5, steps 1–4 of the user's list (completed): the constant-speed
  branch of `linearProfile`, `probetouchplan.{h,cpp}`, `probetouchmotion.{h,cpp}` and the strut
  keys of the sphere file reader. The user built `robocrap` without errors (2026-10-26).
  Line counts, physical/nonblank. C++ before: jointsegments.h 100/84, jointsegments.cpp 267/250,
  chamfermotion.cpp 358/333, scenecalibration.cpp 141/124, scenegeometry.h 119/111, total
  985/902. C++ after: jointsegments.h 109/92, jointsegments.cpp 308/287, chamfermotion.cpp
  336/312, scenecalibration.cpp 154/136, scenegeometry.h 122/114 (total 1029/941, +44/+39), plus
  the new probetouchplan.h 53/44, probetouchplan.cpp 112/99, probetouchmotion.h 66/55 and
  probetouchmotion.cpp 241/219 (472/417). Build inputs: src/trajectories/CMakeLists.txt 19/19 to
  23/23. The growth is the required touch plan (touch table, rotations, transfer waypoints, strut
  clearance) and motion (segment sequence, PTP/LIN construction with the existing solver, the two
  clearance checks, playback evaluation). Reductions: chamfer's robot validation became the shared
  `ratedRobotSetup` (chamfermotion.cpp −22 lines) instead of a copy in the probe motion;
  `validRobot` and `validTiming` became internal to jointsegments.cpp; the motion sequence is one
  list of steps instead of a hand-written chain of calls; the sphere file reader checks its three
  sizes with one helper. Considered and rejected: sharing the interval search and cubic
  evaluation of `ChamferMotion::evaluateJoints` (about 6 lines saved for two new declarations
  with a four-parameter search).
  Compliance pass: no lambdas, templates, exceptions or output parameters; results return
  through `ProbeTouchPlanResult`, `ProbeTouchMotionResult`, `RobotSetupResult` and the internal
  `BuildResult`; the builder owns its sequential state (points, segments, splines, minima, IK
  seed) and borrows the plan and the worker-owned robot setup like `MotionCompiler`. Every new
  signature has at most two parameters except the internal `transferPoses(from, to, centre)`:
  the centre cannot be derived from the two touches without the contact distance, and making it
  a member of `ProbeTouchPlanParameters` would publish an internal helper in the public header.
- [x] (2026-10-26) Milestone 5, steps 5–6 of the user's list (completed): `SceneCalibration`
  Generate (worker, revision, calibration path object, console line) and the QML Generate button
  with the busy indicator and status text.
  Line counts, physical/nonblank. C++ before: scenegeometry.h 122/114, sceneobject.h 98/88,
  sceneobject.cpp 235/215, scenemodel.h 59/51, scenemodel.cpp 239/211, scenecalibration.h 39/33,
  scenecalibration.cpp 154/136, main.cpp 161/119, probetouchmotion.h 66/55, total 1173/1022.
  After: 129/120, 100/90, 255/234, 60/52, 240/212, 61/55, 240/216, 161/119, 67/56, total
  1313/1154 (+140/+132). QML: Main.qml 468/409 to 473/414. The growth is the required Generate
  state (playback collaborator, path id, revision, timer, future), its availability, the worker
  entry, the result handling with the console line, and the path drawing. Reductions: the motion
  lives in the path's `SceneTouchPath` shape, so no id-to-motion map and no separate polyline value
  exist; `SceneModel::addSphere` became `addCalibrationObject(shape, name)` for both calibration
  kinds instead of a second insertion function; the machining path drawing and the calibration
  path drawing share `tcpPolyline`; `calculating` is the validity of the future, so no flag and
  no second notification signal exist.
  Compliance pass: no lambdas, templates, exceptions or output parameters; the worker receives
  copies and returns through the future. Signatures: `SceneCalibration(scene, effectors,
  playback)` has three collaborators, each a different owner (scene objects, measuring TCP and
  ruby ball, robot model and playback state) with no common owner to merge; `tcpPolyline` takes
  one point range (`QVector::mid`), like `clampedJointSpline`; every other new signature has at
  most two parameters.
- [x] (2026-10-26) Milestone 5 acceptance: the user built and ran Calibration → Generate Path:
  `Probe calibration preparation: 31 touches, 887 points, 248.446856148 s movement, minimum
  transfer clearance 2.5404 mm, minimum strut clearance 3.2588 mm`. Points, movement time and
  strut clearance equal the scratch estimates. The transfer clearance is checked on stored points
  only: on a λ = −10° chord between longitudes 30° apart (29.53°, 8.666 mm long) the stored
  points nearest the middle are the ends of the 2.5 mm ramps, 2.5404 mm outside the contact shell,
  while the unstored chord middle is 2.4385 mm outside it.
- [x] (2026-10-26) CIRC transfers (user request after the first Dry Run): every transfer between
  consecutive start points is one arc on the start shell instead of 1–3 chords. Recorded in "The
  touch geometry", Milestone 5 and the Decision Log before the code edit. The user built it and
  reported all checks as passed (2026-10-26).
  Line counts, physical/nonblank. C++ before: jointsegments.h 109/92, jointsegments.cpp 308/287,
  probetouchplan.h 53/44, probetouchplan.cpp 112/99, probetouchmotion.cpp 241/219, total 823/741;
  after: 118/101, 336/313, 52/43, 92/80, 242/220, total 840/757 (+17/+16). The arc fields, the
  arc branch of `target`, `circularProfile`, the extracted `timedProfile` and the normal term of
  `linearSchedule` replace `transferPoses`, `ProbeTouch::transfer`, `ProbeMaximumChordDegrees` and
  the chord clearance bound. Compliance: `circularProfile(chord, centre)` and `timedProfile(profile)`
  have two and one parameters; no lambdas, templates or exceptions; chamfer LIN arithmetic is
  unchanged because its arc angle is 0 and the normal term is exactly 0.
- [x] (2026-10-26) Milestone 6 implemented (completed): `ScenePlayback` plays a
  `PlaybackMotion` variant of the chamfer and the probe motion, and `SceneCalibration` has
  `canPlay`, `dryRun`, playback ownership, failure reporting and Stop when leaving the submode;
  `Main.qml` binds Dry Run and shows the playback status in the status bar.
  Line counts, physical/nonblank. C++ before: sceneplayback.h 74/67, sceneplayback.cpp 133/119,
  scenecalibration.h 61/55, scenecalibration.cpp 240/216, total 508/457; after: 80/72, 154/136,
  68/62, 273/244, total 575/514 (+67/+57). QML: Main.qml 473/414 to 478/419. The growth is the
  variant alias, the second evaluation branch in `present`, the duration branch in `start`, and
  the calibration Dry Run members (availability, start or resume, ownership, failure). The chamfer
  phase texts moved unchanged into `chamferPhaseText`; `present` now copies the joints, the
  flange-to-TCP transform and the phase text before `applyPlayback` instead of holding a second
  owner of the motion, so the motion may still be released during that call.
  Compliance pass: no lambdas, templates, exceptions or output parameters; `std::variant` and the
  `PlaybackMotion` alias use the standard template without a project template; `start` keeps two
  parameters, `onPlaybackFailed` has two, the other new members none.
- [x] (2026-10-26) Milestone 6 acceptance: the user built `robocrap` and `robocrap_qmllint`, ran
  the Dry Run scenario of Milestone 6 with the CIRC transfers and reported all checks as passed.
- [x] (2026-10-26) Milestone 7 (completed, before the user's Milestone 6 checks at the user's
  request "let's finish calibration plan"): `docs/AGENT_CONTRACTS.md` (Build and modules:
  singletons, construction order, copy-if-absent sphere file; Viewport and CAD: sphere parts,
  transparent highlight, model published to `ScenePlayback`; Geometry and scene: measuring TCP,
  sphere and calibration path objects, `fitSphere`, sphere file, Calibration visibility;
  Machining and playback: `ScenePlayback`, constant-speed LIN, CIRC, splines, `ratedRobotSetup`,
  calibration path invalidation, console line, calibration Generate; QML: Calibration toolbar,
  status and Scene panel), `docs/ARCHITECTURE.md` (3.1 `spherefit`, 3.2 `ScenePlayback` and
  `SceneCalibration`, 3.3 shared numerics and probe plan/motion, 3.5 sphere parts, 3.6 status bar,
  submodes, Calibration tools and panels, resources, 4 threads and ownership, 5.1, 5.2 Dry Run
  through `ScenePlayback`, 5.4 shutdown order, new 5.5 calibration trace, 6 workers, 7 open
  calibration clearance question) and `docs/APP_DESCRIPTION.md` (CIRC term, measuring TCP, the
  Calibration workflow step). Documentation lines: AGENT_CONTRACTS.md 361 to 401,
  ARCHITECTURE.md 410 to 426, APP_DESCRIPTION.md 82 to 97.
  Compliance pass over every C++ file changed by Milestones 1–7 (jointsegments, chamfertcpsolver,
  chamfermotion, probetouchplan, probetouchmotion, spherefit, sceneplayback, scenecalibration,
  sceneobject, scenemodel, scenegeometry, sceneendeffectors, workspacecontroller, robotcontroller,
  main): no lambdas, project templates, throw/catch or output parameters (text search plus the
  per-milestone reviews recorded above); no signature with four parameters; the three-parameter
  signatures are recorded with their reasons (Milestone 1 reductions, `SceneCalibration`,
  `setSphereDisplay`, `linearProfile`, `solveLinear`). No findings required a fix.


## Surprises & Discoveries

- Observation: the touch table accepted by the user in conversation was announced as "25
  touches", but its rows define 27 touches.
  Evidence: group A has a tip and 4 side touches, groups B and C a tip and 2 side touches each,
  groups D–G a tip and 3 side touches each: 5 + 3 + 3 + 4 × 4 = 27. (The 27-touch table was
  replaced by the 31-touch ring scan on 2026-10-26.)
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
- Observation: the movement-time estimate in `Artifacts and Notes` understates two segments. With
  the default `ChamferTimingParameters` (`transferFeed` 10 mm/s, `transferAcceleration`
  20 mm/s²) the 3 mm Retract never reaches 10 mm/s, because accelerating to 10 mm/s and braking
  again needs 2 · 10² / (2 · 20) = 5 mm; its speed profile is a triangle with duration
  2 · sqrt(1.5 mm / 20 mm/s²) ≈ 0.77 s, not 0.4 s. The Descent of a θ = 0° side touch is
  25 + 17 · (1 − sin 15°) ≈ 37.6 mm and takes 0.5 + 32.6 / 10 + 0.5 ≈ 4.3 s, not 3 s.
  Evidence: hand calculation from the defaults in
  `src/trajectories/include/trajectories/chamfermotion.h`, 2026-10-25; the user asked to be
  reminded of it when Milestone 5 starts. (Superseded on 2026-10-26: the ring scan has no Descent,
  Retract runs at a constant 1 mm/s for 3 s, and `Artifacts and Notes` holds the new estimate.)
- Observation: one wrist configuration cannot cover the touches T1–T31, and the in-place wrist
  unwind after T21 swings the probe far below the sphere.
  Evidence: scratch run of the deterministic sequence (2026-10-26): without the unwind, the
  transfer from T21 to T22 fails at A4 = 183.9° halfway (limit 185°); with it, the probe reaches
  211 mm below c.z during the unwind, against at most 84.5 mm in every other segment; the swing
  depth grows when the unwind is moved radially outward (220 mm at 42 mm from c).


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
- Decision (superseded on 2026-10-26 by the ring scan): the motion is HOME, PTP to the first via pose, then for every touch LIN via→start,
  LIN start→contact at the approach feed, LIN contact→start, LIN start→via, PTP to the next via
  pose, and finally PTP to HOME.
  Rationale: two consecutive touches can share a via position and differ only in orientation; the
  existing straight-line LIN profile rejects a zero-length translation, a PTP does not.
  Date/Author: 2026-10-24, agent.
- Decision (threshold superseded on 2026-10-26 by the ring scan: 2.4206 mm outside the contact
  shell): every sampled point of the ToVia, Descend, Ascend and ReturnHome segments must keep the
  ball centre at least d + g − 0.0001 mm from the sphere centre.
  Rationale: the application has no collision model; this checks the only clearance that can be
  computed, the ball centre. It checks sampled points only (PTP samples at most 2° apart), not the
  continuous path or any other part of the probe or robot; Dry Run shows the rest visually.
  Date/Author: 2026-10-24, agent.
- Decision (superseded on 2026-10-26 by the ring scan, which has no via poses): all via poses lie in one horizontal plane z = c_z + d + g + 25 mm, directly above
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
- Decision: `WorkspaceController` announces calibration activity through a new signal
  `calibrationActiveChanged(bool)` that `main.cpp` connects to `SceneCalibration::setActive`. The
  2026-10-24 version also gave `WorkspaceController` a fourth constructor parameter
  `ScenePlayback*`; that part was superseded on 2026-10-25 (Milestone 2 deviations): it keeps
  `(SceneModel*, SceneMachining*, SceneEndEffectors*)`.
  Rationale: leaving Machining or Calibration must stop playback before the tool or input changes.
  `SceneMachining::stop()` stops machining-owned playback and `SceneCalibration::setActive(false)`
  stops calibration-owned playback, so `WorkspaceController` needs no playback collaborator; a
  fourth collaborator for calibration would need a recorded justification under `AGENTS.md`, so
  calibration activation uses the signal.
  Date/Author: 2026-10-24, agent; revised 2026-10-25.
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
- Decision: in the Calibration submode the scene shows only the robot with the MEE, the sphere and
  the calibration path. Entering the submode shows the sphere and the path. (Revised the same day:
  the user can hide and show both the sphere and the path; the first version kept the sphere
  always visible.)
  Rationale: user requirement.
  Date/Author: 2026-10-25, user; revised 2026-10-25, user.
- Decision: `SceneCalibration::setActive(true)` records the ids of the visible objects that are
  neither the sphere nor the calibration path and hides them through
  `SceneModel::setObjectVisible`; an object added while the submode is active is hidden and
  recorded the same way. `setActive(false)` hides the sphere and the path and shows the recorded
  objects that still exist, then clears the record. The Scene panel lists only the "Calibration"
  group in the Calibration submode and lists that group in no other submode or mode. Entering and
  leaving the Calibration submode clears the selection.
  Rationale: restoring the recorded visibility returns the other submodes to the state the user
  left, including objects the user had hidden; the coordinator that owns calibration activity
  owns the record. Hidden objects cannot be selected or deleted, because the viewport does not
  pick hidden parts and the panel does not list them; clearing the selection removes a selection
  made before entry.
  Date/Author: 2026-10-25, agent.
- Decision: the user cannot delete the sphere or the calibration path; in the Calibration submode
  `WorkspaceController::deleteSelected` removes nothing. The user can only hide and show the path.
  The touch geometry is fixed in code (the touch table of "The touch geometry" and the defaults of
  `ProbeTouchPlanParameters`), with no editor and no file input; only the sphere centre comes from
  the sphere file. `SceneCalibration` itself still replaces the path when the sphere, the
  measuring TCP or the robot model changes, because a path computed for other inputs is invalid.
  Rationale: user requirement ("the calibration path must be set once and hardcoded").
  Date/Author: 2026-10-25, user (agent: replacement on input change kept).
- Decision: no `TransferSolver` class; `ChamferTcpSolver` gains `forward`, `inverse`, `samplePtp`
  and `solveLinear`.
  Rationale: `ChamferTcpSolver` already owns the same state (borrowed `ChamferRobotSetup`,
  `Kr10Kinematics`, inverse tool transform); a second class with identical members violates the
  `AGENTS.md` reuse rule, and Milestone 5 needs only one solver object. `inverse` keeps the
  former `MotionCompiler::inverse` checks, so LIN transfers still do not use `solveTcp`.
  Date/Author: 2026-10-25, agent.
- Decision: parameter reductions. `linearProfile(first, last, limits)`: feed and acceleration form
  one `LinearLimits` value (the LIN's speed-profile limits), stored in the profile.
  `linearSchedule(profile, startTime)`: the acceleration comes from `profile.limits`, the same
  value as before, so the interval counts are unchanged. `solveLinear(schedule, seed, label)`:
  `LinearSchedule` carries its profile, and the former `inspectTransferRates` runs at the end of
  `solveLinear` as the private `linearRateError(points, label)`, which reads the joint speeds from
  the solver's robot setup; its anchor point is the seed's angles at `schedule.startTime`, equal
  to the preceding point because the seed is that point's solution. `clampedJointSpline(points,
  robot)`: the caller passes only the points of one range (`QVector::mid`, a copy of a few hundred
  points), and `preparePlaybackSplines` appends the consecutive ranges, which removes
  `ChamferMotion::prepareJointSpline`. Remaining three-parameter signatures: `linearProfile` (two
  end poses and the limits), `solveLinear` (schedule, seed, error label), and the moved, unchanged
  `jointSplineIntervalError` and `quadraticUnitRoots`.
  Rationale: `AGENTS.md` permits four parameters only after reduction attempts fail; every
  reduction above keeps the arithmetic order and error texts.
  Date/Author: 2026-10-25, agent.
- Decision: Milestone 2 deviations, accepted by the user. (1) `ScenePlayback` has `start(motion,
  ownerId)` for a fresh start and `resume()`; `SceneMachining::dryRun` resumes whenever playback
  is paused, as today, instead of the planned "resume only for the same owner". (2)
  `WorkspaceController` keeps its three constructor parameters and its `SceneMachining::stop()`
  call, now meaning "stop the playback if a machining path owns it". (3) The unused `QObject*
  parent` parameter of `SceneMachining` is dropped, so it has three parameters with
  `ScenePlayback*`. (4) `robotSetupChanged` ends only machining-owned playback. (5) The playback
  motion is a plain `std::shared_ptr<const ChamferMotion>` until Milestone 6 needs a second motion
  type; then it becomes a `std::variant`.
  Rationale: (1) keeps the current behavior when the selection changes while paused; (2) and (3)
  avoid four-parameter constructors as `AGENTS.md` requires; (4) keeps a spindle TCP change from
  stopping a calibration Dry Run; (5) adds no abstraction before it is needed.
  Date/Author: 2026-10-25, agent; accepted by the user.
- Decision: Milestone 4 details, accepted by the user. (1) Until Milestones 5 and 6,
  `SceneCalibration::canGenerate` and `canPlay` are false and `unavailableReason` reports only
  sphere-file problems; no temporary "not implemented" text is shown. (2) The sphere object is
  named "Calibration sphere". (3) The width of the three-button submode row in the Workflow panel
  is checked visually by the user; shorter button text is the fallback. (4) The new icon
  `resources/pics/calibration.svg` is a single-colour 20 × 20 stroke icon like the existing tinted
  icons. (5) The Properties panel's label list
  gains the new sphere rows so the label column width does not depend on the selection, and the
  sphere shows the Points overlay control but not the Normals control, because only the rough
  points have a diagnostic drawing.
  Rationale: (1)–(4) as proposed in the Milestone 4 explanation; (5) keeps the Properties panel
  stable for the new rows. A first version also disabled the sphere's Visible check box and
  Scene panel eye button; the user then asked to keep sphere visibility switchable.
  Date/Author: 2026-10-25, agent; accepted by the user.
- Decision: Milestone 4 implementation choices. `SceneCalibration` starts with only
  `SceneModel*`, `error` and `setActive`, because the playback, measuring TCP and Generate/Dry Run
  members have no use before Milestones 5 and 6. Its new fields are required state: the active
  flag (entering and leaving run once each), the sphere object id (replacement and visibility),
  the list of ids hidden on entry (restoring the user's visibility), and the error text; no
  existing object owns calibration activity. `WorkspaceController` gains `m_calibrationActive` so
  that `calibrationActiveChanged` and the selection clear fire only on transitions, whichever of
  mode or submode caused them. Code identifiers use `center`, the spelling of the existing code;
  the user-visible row names follow the user's list ("Fitted center" and so on).
  Rationale: `AGENTS.md` requires recording the requirement of each new field and rejects members
  without a current use.
  Date/Author: 2026-10-25, agent.
- Decision: sphere parts are drawn without face boundaries and get no selection outline:
  `OccPartProps` and `OccPart` gain `faceBoundary` (default true, so every other part is
  unchanged), `OccSceneAdapter::createPresentation` sets it false for a `DrawingSphere` part,
  `OccPart` passes it to `Prs3d_Drawer::SetFaceBoundaryDraw`, and `OccScene::selectParts` builds
  the dark outline only for parts that draw face boundaries.
  Rationale: the face of `gp_Sphere` built by `BRepBuilderAPI_MakeFace` is bounded only by its
  seam meridian and two degenerate pole edges, so the black boundary and the selection outline
  showed a seam line (user report with screenshot, 2026-10-25). The outline exists to keep face
  boundaries visible under the highlight color, so a part without boundaries needs none.
  Date/Author: 2026-10-25, user (request) and agent (mechanism).
- Decision: the calibration sphere object shows two spheres at the fitted center, the ball-center
  sphere (radius of the fit through the ruby ball centers) and the compensated sphere (that radius
  minus the 1.5 mm ruby ball radius), each with its own visibility check box in the Geometry
  section; the nominal sphere is no longer drawn, only its radius and diameter are listed.
  Mechanism: `DrawingPart::visible` (a hidden part is not installed by `OccSceneAdapter`) and
  `DrawingStyle::transparency` (passed to `AIS_Shape::SetTransparency`), and
  `SceneModel::setSphereDisplay`, which reuses the replacement behavior of
  `OccController::insertObject` for an installed id. `SceneCalibration` now takes
  `SceneEndEffectors*` for the ruby ball diameter and rejects a ball-center radius that does not
  exceed the ball radius. `setSphereDisplay` has three parameters, the object and the two flags of
  one display operation, like the existing `setObjectDiagnosticOverlays`.
  Rationale: the fitted 13.8342 mm is the ball-center distance, not the sphere radius, which
  confused the comparison with the nominal 12.5 mm; the user chose the names and the layout.
  Both spheres are shown by default because entering the submode shows everything, and the outer
  sphere is translucent so the inner one is not hidden.
  Date/Author: 2026-10-25, user (names, layout) and agent (defaults, mechanism).
- Decision: a transparent part keeps its transparency when highlighted. `OccScene::displayPart`
  gives a part with `IsTransparent()` its own copies of the viewport's Selected and LocalDynamic
  highlight styles with the part's transparency (`SetHilightAttributes`,
  `SetDynamicHilightAttributes`); opaque parts keep the shared styles of
  `OccViewer::configureHighlightStyles`. No new state: the transparency is read from the AIS
  shape. occscene.cpp 361/296 to 374/308 physical/nonblank lines.
  Rationale: the shared styles draw the highlight opaque in `Graphic3d_ZLayerId_Top`, so a
  selected ball-center sphere became an opaque amber ball that hid the compensated sphere (user
  report, 2026-10-26). The user chose the common CAD rule (one highlight color, the object's own
  transparency) over an amber wireframe, which on a sphere shows only iso-lines.
  Date/Author: 2026-10-26, user (choice) and agent (mechanism).
- Decision: the touch motion keeps the ruby ball at least g = 3 mm from the strut that holds the
  sphere, modeled as a vertical cylinder of radius 8.9 mm on the axis through the fitted center,
  topped 10.5 mm below that center; the strut dimensions are the sphere file keys `strutDiameter`
  and `strutRecess`. The 27-touch table is unchanged: its smallest strut clearance is 11.56 mm.
  (Revised on 2026-10-26: the strut clearance is the exact distance to the strut solid instead of
  the larger of the height and side distances, which underestimated it near the strut edge; the
  ring scan's smallest strut clearance is 3.26 mm.)
  Rationale: the user's drawing (2026-10-26) shows the ball recessed 2 mm into a 17.8 mm strut, so
  part of the lower hemisphere cannot be touched and a transfer could hit the strut; the existing
  sphere clearance check does not see it. The dimensions belong to the fixture, like the sphere
  diameter, so they live in the editable sphere file. The strut is assumed coaxial and vertical
  and to extend downward at the same diameter; the drawing shows only its top 5 mm.
  Date/Author: 2026-10-26, user (geometry) and agent (model, rule).
- Decision: the touch set is the 31-touch ring scan of "The touch geometry": touches on rings at
  latitudes 90°, 50°, 20°, −10° and −30°, transfers along chords of at most 30° on the 17 mm start
  shell, entry from 60 mm above the pole, exit 42 mm out along the last normal, and one in-place
  wrist unwind after T21. It replaces the 27 via-point touches, and Milestone 5 builds it.
  Rationale: the user rejected the via-point motion (2026-10-26): it did not use the lower half
  of the sphere and lifted to a via plane 25 mm above the start shell and back for every touch.
  The rings reach down to −30°, the lowest latitude that keeps g from the strut; the omitted
  positions are unreachable or near the A5 singularity (scratch reach checks). The shell
  transfers keep the ball at least 2.42 mm outside the contact shell, against 3 mm before. The
  rotation rule R_P(μ, 90° − λ − τ, 30°) gives every touch the same roll relative to its
  meridian, and the scratch sequence with this rule has no joint jumps; the wrist unwind is needed
  because A4 would leave its limit. The deterministic sequence was checked in scratch
  (2026-10-26).
  Date/Author: 2026-10-26, user (requirement, rings accepted) and agent (layout, checks).
- Decision: Approach and Retract run at a constant 1 mm/s without ramps; every other LIN keeps the
  10 mm/s, 20 mm/s² trapezoid. `linearProfile` accepts acceleration 0 as constant speed, so the
  existing profile, schedule and solver functions build both kinds.
  Rationale: the user (2026-10-26) asked for the same law in both directions and found a ramp
  pointless for 3 mm; with the trapezoid the 3 mm Retract never reached its feed. Reusing
  `linearProfile` with acceleration 0 was the user's proposal and costs one branch instead of a
  second profile type.
  Date/Author: 2026-10-26, user.
- Decision: Dry Run interpolates Approach and Retract joint-linearly between their two stored
  points; Descend, Transfer and Retreat keep one clamped cubic joint spline each; PTPs keep the
  smoothstep. Dry Run evaluates only these stored values.
  Rationale: the clamped spline has zero joint speed at both ends of a range, so with only the
  start and the contact stored it would play an Approach as 0 → 1.5 → 0 mm/s. Joint-linear
  interpolation needs no stored coefficients and keeps the TCP within 0.0022 mm of the line at
  0.9961–1.0039 mm/s (scratch check of all 31 Approaches). The user requires that Dry Run only
  animates what Generate computed (2026-10-26).
  Date/Author: 2026-10-26, user (requirement) and agent (mechanism).
- Decision: overtravel and the real stop on contact belong to the later RSI plan, whose agreed
  motion is recorded in `Artifacts and Notes`; `ProbeTouchPlanParameters` keeps no overtravel.
  Rationale: the animation does not use it, and `AGENTS.md` rejects fields without a current use.
  Date/Author: 2026-10-26, user (RSI motion) and agent (no field).
- Decision: Milestone 5 implementation details. (1) Chamfer's robot validation (`validRobot`,
  rated joint speeds, `validTiming`, the $VEL_MA.CP/$ACC_MA.CP check, same error texts) moved
  into `RobotSetupResult ratedRobotSetup(const ChamferRobotSetup&)` in `motion/jointsegments`,
  used by `ChamferMotion::create` and `ProbeTouchMotion::create`. (2) Errors read
  "<phase text>: <message>" with the phase texts of Milestone 6 ("Touch 22/31: transfer: …",
  "Entry: descend: …"), produced by `ProbeMotionPhase::text()`, which Milestone 6 reuses for the
  playback phase; `solveLinear` already prefixes most of its errors with that text, so the
  builder adds it only when missing. (3) `probetouchplan.h` publishes the constants both files
  need (`ProbeTouchCount`, `ProbeApproachGap`, `ProbeApproachFeed`, `ProbeMaximumChordDegrees`)
  and `ProbeTouchPlanParameters::contactDistance()` and `strutClearance(ball)`; entry height,
  exit distance, roll, the unwind touch and the touch table stay in `probetouchplan.cpp`.
  (4) `probetouchmotion.h` publishes `ProbeMotionSegment` and `ProbeTouchMotionData`, the stored
  motion that the internal builder returns to the private constructor. (5) The sphere file reader
  reports a missing or invalid size as "sphereDiameter, strutDiameter and strutRecess must be
  finite positive numbers." and a recess of at least the diameter as "strutRecess must be smaller
  than sphereDiameter."; `SceneSphere` gains `strutRadius` and `strutRecess`.
  Rationale: (1) removes a ten-line copy with identical failure semantics; (2) gives one text per
  phase for errors and playback; (3)–(4) keep shared values in one place and the builder outside
  the public class; (5) is shorter than three separate messages and names every key.
  Date/Author: 2026-10-26, agent.
- Decision: Milestone 5 Generate details. (1) The calibration path's shape is `SceneTouchPath {
  std::shared_ptr<const ProbeTouchMotion> motion; }` instead of polylines with the motion kept in
  `SceneCalibration`; `SceneObject::drawing` builds one polyline per `ProbeMotionSegment`
  (Approach/Retract green 3 px, PTP purple, other LINs blue, the machining path colors), and
  Milestone 6 plays `touchPath()->motion`. (2) `SceneModel::addSphere` became
  `addCalibrationObject(SceneObject::Shape, name)`, whose kind is Sphere for a `SceneSphere` and
  CalibrationPath otherwise. (3) `calculating` is `m_calculation.valid()`; `canGenerate`,
  `unavailableReason` and `calculating` share `availabilityChanged`. (4) A result that arrives
  after leaving the Calibration submode is inserted hidden. (5) In the Calibration submode the
  status bar shows `error`, else `unavailableReason`; Generate Path is available when
  `canGenerate` is true and calls `generate()` without the chamfer editor.
  Rationale: (1) removing the path object releases its motion without a second record, and the
  drawing needs no stored copy of the positions; (2)–(3) avoid a second insertion function, a
  flag and a signal; (4) keeps the other submodes free of calibration objects.
  Date/Author: 2026-10-26, agent.
- Decision: Milestone 6 details. (1) The playback motion is `using PlaybackMotion =
  std::variant<std::monostate, std::shared_ptr<const ChamferMotion>, std::shared_ptr<const
  ProbeTouchMotion>>`; monostate means inactive, replacing the null pointer. (2) The probe phase
  text is `ProbeMotionPhase::text()` ("Entry: to entry", "Touch 5/31: approach", "Exit: return
  HOME"), the same text as the Generate errors. (3) In the Calibration submode the Machining
  panel is not shown, so the status bar shows "<Playing|Paused> · <phase> · <elapsed> /
  <duration> s" while playback is active and no calibration error is set, in the existing label.
  (4) A measuring TCP, sphere or robot model change during calibration playback removes the path,
  and `ScenePlayback` ends playback of a removed owner without returning to HOME, as it does for
  a removed machining path.
  Rationale: (1)–(2) add no type or text table; (3) gives the playback status a place in the
  Calibration layout without a new panel or layout change; (4) keeps the existing rule.
  Date/Author: 2026-10-26, agent.
- Decision: every transfer between consecutive start points is one CIRC on the start shell;
  `LinearProfile` gains the arc (`circularProfile`) instead of a separate profile type, and the
  transfer waypoints (`ProbeTouch::transfer`, `transferPoses`, `ProbeMaximumChordDegrees`) are
  removed. The transfer clearance rule becomes g − 0.0001 mm.
  Rationale: the user saw the 2- and 3-chord transfers stop at their waypoints and asked for a
  smoother, planar transfer (2026-10-26). A chord between two start points always comes closer
  to the sphere than g (2.44 mm in the middle of a 29.53° chord); the arc keeps 3 mm, has no
  intermediate stop and lies in one plane. A general SPLINE was considered and rejected for now:
  the robot stops at every start point for the touches anyway, and a spline needs construction,
  arc-length reparametrization and orientation interpolation without a current requirement. The
  rerun of the scratch sequence with arcs (2026-10-26) keeps the IK, |A5| ≥ 48.3°, the joint
  speeds and the probe clearances of the chord version. A unified PTP/LIN/CIRC/curve motion
  module is a separate later plan (user, 2026-10-26).
  Date/Author: 2026-10-26, user (request) and agent (mechanism).
- Decision: `ScenePlayback` has two notification signals: `stateChanged` for start, resume, pause
  and end (properties `active`, `playing`, `duration`) and `playbackChanged` for every presented
  time (`elapsed`, `phase`). `SceneMachining::availabilityChanged` follows `stateChanged` only.
  Rationale: `SceneMachining` emitted `availabilityChanged` only on those state changes, and that
  signal also redraws the draft axis; forwarding the 16 ms `playbackChanged` would redraw it and
  re-evaluate the toolbar bindings every frame.
  Date/Author: 2026-10-25, agent.
- Decision: `samplePtp` computes all PTP points before the caller checks the 400000-point limit,
  and failed LIN or spline preparation no longer leaves partially appended points or
  coefficients.
  Rationale: the PTP point count is at most 350 because both ends are within joint limits, and a
  failed candidate is discarded as a whole; the first failure and every successful result are
  unchanged.
  Date/Author: 2026-10-25, agent.


## Outcomes & Retrospective

Plan outcome (2026-10-26): Calibration submode, sphere fit and drawing, measuring TCP, the 31-touch
ring-scan touch plan and motion (CIRC transfers, constant-speed touches, one wrist unwind) and its
Dry Run are implemented; chamfer behavior is unchanged (Milestone 1 baseline). C++ grew by about
1340 physical lines over the plan (Milestones 1–6 and CIRC as recorded in Progress), QML by about
50. All milestone checks passed (user, 2026-10-26). Open: the table and
strut-base height for the wrist unwind swing (about 211 mm below the sphere centre); the real
touch, RSI streaming and ball-centre least-squares solution belong to later plans, and a unified
PTP/LIN/CIRC/curve motion module to its own plan (user, 2026-10-26).

Milestone 1 (2026-10-25): the PTP, LIN and joint-spline operations are shared through
`src/trajectories/motion/jointsegments` and `ChamferTcpSolver`, and chamfer Generate reproduces
the baseline exactly (375 points, 25.299220225 s). Reusing `ChamferTcpSolver` instead of adding a
`TransferSolver` removed a duplicate class; the C++ source grew by 131 physical lines for headers
and result structs, which Milestone 5 reuses instead of copying the code.


## Context and Orientation

All paths are relative to the repository root, which is `E:\Qt\QtProjects\RoboCrap` or
`D:\Qt\QtProjects\RoboCrap` depending on the computer the user works at. Read `AGENTS.md`
first. Its mandatory rules apply to every change: no C++ lambdas, no project templates, at most
four function parameters, where four is chosen only after attempts to reduce the count to three
or fewer have failed and those attempts are recorded in this plan, results only through return
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

Line-count baseline (physical / nonblank lines, 2026-10-24; files changed by the 2026-10-25
comment removal recounted that day): chamfermotion.cpp 708/667,
scenemachining.cpp 412/383, scenemachining.h 101/95, sceneobject.cpp 189/173, sceneobject.h 89/79,
scenemodel.cpp 217/191, scenemodel.h 56/48, scenegeometry.h 102/96, sceneendeffectors.cpp 122/104,
sceneendeffectors.h 59/48, workspacecontroller.cpp 111/95, workspacecontroller.h 65/56,
robotcontroller.cpp 182/160, robotcontroller.h 59/47, main.cpp 154/113, backendqmltypes.h 158/141,
occcontroller.h 164/141, occsceneadapter.cpp 265/248, Main.qml 455/396, ModeToolBar.qml 238/202,
WorkflowPanel.qml 135/114, ScenePanel.qml 303/255, EndEffectorPanel.qml 231/227,
MachiningPanel.qml (capture at Milestone 2 start), DeployOcct.cmake (capture at Milestone 4
start). Capture counts at the start and end of every milestone, including new files, and record
C++, QML and build inputs separately in `Progress`.


## The touch geometry

Fixed inputs (constants in code unless noted): the fitted sphere centre c (mm, {B}, from the
sphere file); sphere radius r_s = 12.5 mm and ruby ball radius r_b = 1.5 mm, so the contact
distance is d = r_s + r_b = 14 mm; approach gap g = 3 mm, so every start position lies on the
start shell of radius d + g = 17 mm around c; approach feed v = 1 mm/s; entry height h_e = 60 mm;
exit distance h_x = 42 mm; roll ψ = 30°; and the strut
that holds the sphere (sphere file keys `strutDiameter` 17.8 mm and `strutRecess` 2 mm): a
vertical cylinder of radius R_s = 8.9 mm on the axis through c, whose top face lies e = 2 mm above
the sphere's lowest point, at z_top = c.z − r_s + e = c.z − 10.5 mm, assumed to extend downward
at the same diameter (user drawing, 2026-10-26).

Directions. Latitude λ and longitude μ, in degrees, give the outward unit sphere normal in {B}:

    n(λ, μ) = (cos λ · cos μ, cos λ · sin μ, sin λ)

μ = 0° points along the {B} x-axis and μ = 90° along the {B} y-axis. Seen from c, the {B} origin
lies at μ = 162.5° (c = (1054.1, −332.0, 454.3) mm), so longitudes near 162.5° are the near side
of the sphere and longitudes near 342.5° the far side.

The {P} orientation of a touch is defined by tilt θ, tilt azimuth φ and roll ψ, in degrees:

    R_P(φ, θ, ψ) = Rz(φ) · Ry(θ) · Rx(180°) · Rz(ψ)

The stylus direction, the {P} z-axis, is R_P(φ, θ, ψ) · e_z = (−sin θ · cos φ, −sin θ · sin φ,
−cos θ): for θ = 0 the stylus points down, and ψ is a rotation about the stylus. The user's manual
orientation O1 (A = 180°, B = 0°, C = 180°) equals R_P(0, 0, 180°), and O2 (A = 0°, B = 0°,
C = 180°) equals R_P(0, 0, 0).

A touch is given by its contact latitude λ, longitude μ and stylus tilt τ (degrees). Its rotation
is R_P(μ, 90° − λ − τ, 30°). For τ = 0 the stylus points along −n(λ, μ), towards c, so the
contact lies on the stylus axis. For τ > 0 the stylus is turned by τ in the meridian plane of μ,
from −n(λ, μ) towards straight down, so the contact lies τ off the stylus axis. The pole touch
uses μ = 90°, the longitude of the touch after it; because R_P(φ, 0, ψ) = R_P(0, 0, ψ − φ), this
keeps the roll unchanged along the meridian to that touch. For each touch:

    contact position  p_contact = c + d · n(λ, μ)
    start position    p_start   = c + (d + g) · n(λ, μ)

Both TCP poses of a touch have the rotation of that touch. Approach moves the TCP from p_start to
p_contact, a translation of length g along −n; Retract moves it back.

The 31 touches, in execution order, ring by ring from the top (τ = 0 unless stated):

    T1         λ = 90°:   μ = 90° (pole)
    T2–T7      λ = 50°:   μ = 90°, 45°, 0°, 315°, 270°, 225°
    T8–T17     λ = 20°:   μ = 210°, 240°, 270°, 300°, 330°, 0°, 30°, 60°, 90°, 120°;
                          τ = 30° at μ = 300°, 330°, 0°, 30° (T11–T14)
    T18–T25    λ = −10°:  μ = 60°, 90°, 120°, 150°, 180°, 210°, 240°, 270°
    T26–T31    λ = −30°:  μ = 240°, 210°, 180°, 150°, 120°, 90°

θ = 90° − λ − τ is therefore 0° for T1, 40° for T2–T7 and T11–T14, 70° for T8–T10 and T15–T17,
100° for T18–T25 and 120° for T26–T31.

Surface regions without touches (scratch checks, below): the near-side upper positions λ = 50° at
μ = 135° and 180° and λ = 20° at μ = 150° and 180° have |A5| of only 20°–31°, close to the A5 = 0
wrist singularity, whereas every kept pose has |A5| ≥ 48°; the far side μ = 300°–30° has no IK
solution within the joint limits at λ = 20° with τ = 0 (hence τ = 30° there) and none at
λ = −10°; latitudes below −30° bring the contact closer to the strut than g.

Strut clearance of a ball centre p (mm): the distance from p to the strut solid minus r_b. With
ρ = √((p.x − c.x)² + (p.y − c.y)²) and h = p.z − z_top it is h − r_b when ρ ≤ R_s, ρ − R_s − r_b
when h ≤ 0, and √((ρ − R_s)² + h²) − r_b otherwise. Every touch's p_start and p_contact must have
a strut clearance of at least g. The smallest one is 3.26 mm, at the λ = −30° contacts
(ρ = 12.12 mm, h = 3.5 mm); at λ = −40° it would be 0.86 mm.

The motion, in order:

- Entry: PTP from HOME to the entry pose E with position c + h_e · e_z and the rotation of T1,
  then a LIN of 43 mm straight down from E to p_start of T1.
- Every touch: Approach, then Retract.
- Transfer from touch i to touch i + 1: one CIRC, the great-circle arc of the start shell from
  p_start of touch i to p_start of touch i + 1. γ_i = arccos(n_i · n_(i+1)) is the arc angle and
  a_i = (n_i × n_(i+1)) / ‖n_i × n_(i+1)‖ the arc axis. At the travelled fraction u ∈ [0, 1] the
  TCP position is c + (d + g) · (cos(u · γ_i) · n_i + sin(u · γ_i) · (a_i × n_i)) and the rotation
  slerp(R_i, R_(i+1), u), where R_i is the rotation of touch i. Every transfer point lies exactly
  d + g = 17 mm from c, g = 3 mm outside the contact shell, in the plane through c, p_start of
  touch i and p_start of touch i + 1. The KRL analogue is `CIRC` with the auxiliary point
  c + (d + g) · (n_i + n_(i+1)) / ‖n_i + n_(i+1)‖; the controller's own CIRC orientation control
  (`$ORI_TYPE`, `$CIRC_TYPE`) differs from the slerp. The largest arc is 66.2° (T17 to T18).
- Wrist unwind after the Retract of T21 (λ = −10°, μ = 150°): a PTP at p_start of T21 from the
  current joints q to the other wrist configuration of the same TCP pose: A4 − 180° if A4 > 0,
  else A4 + 180°; A5 replaced by −A5; A6 + 180° if A6 < 0, else A6 − 180°. Continuing to T22 with
  the same wrist configuration would drive A4 past its +185° limit (scratch: A4 = 183.9° halfway
  along the transfer from T21 to T22).
- Exit: after the Retract of T31, a LIN of 25 mm from p_start of T31 outward along n_31 to the exit
  position c + h_x · n_31 with the rotation of T31, then a PTP to HOME.

Motion law: Approach and Retract run at the constant speed v = 1 mm/s without acceleration or
deceleration ramps, 3 s each. Every other LIN and every CIRC uses the trapezoidal profile with
`transferFeed` 10 mm/s and `transferAcceleration` 20 mm/s² along its path length. PTP segments use the chamfer HOME PTP timing
(`ptpDuration` and the quintic smoothstep).

Scratch results (computed once on 2026-10-26 in the agent's scratch directory with the KR10 model
of `robotdefinition.cpp`, TOOL_DATA[5] and damped least-squares IK seeded from the preceding pose;
estimates, not repository code; the entry IK starts on the positive wrist branch, A5 > 0, as the
project IK does from HOME): every touch pose and every LIN pose sampled 0.25 mm apart lies within
the joint limits; |A5| ≥ 48.3° throughout; joint speeds stay below 14 % of the rated speeds at
10 mm/s; on the CIRC transfers the ball centre stays 3.0 mm outside the contact shell; the stylus
stem and probe body stay at least 2.28 mm from the sphere, and the whole probe at least 3.26 mm
from the strut; during the wrist unwind the probe stays at least 5.5 mm from sphere and strut; the
movement takes 246.2 s with about 947 stored points (2026-10-26 rerun with CIRC transfers; the
chord transfers gave 248.4 s and 887 points). Open: during the wrist unwind the probe
swings down to 211 mm below c.z (elsewhere at most 84.5 mm below c.z); the height of the table and
of the strut base is unknown, and robot links other than the probe are not checked. Retreating
radially before the unwind does not help: at 42, 80 and 120 mm from c the swing reaches 220, 231
and 242 mm below c.z.


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

`struct LinearLimits { double feed; double acceleration; }`, the $VEL.CP/$ACC.CP-style limits of
one LIN, and `struct LinearProfile`, the former `TransferProfile` with the same members plus
`LinearLimits limits` and its `duration`, `progress` and `target` methods.
`std::optional<LinearProfile> linearProfile(const M4d& first, const M4d& last, const LinearLimits&
limits)` contains the body of `transferProfile` after the two poses are known. The chamfer
`transferProfile(path, side, timing)` keeps only the evaluation of its staging and lead poses and
calls `linearProfile(first, last, {timing.transferFeed, timing.transferAcceleration})`.

`struct LinearSchedule { LinearProfile profile; QVector<double> localTimes; double startTime;
QString error; }` and `LinearSchedule linearSchedule(const LinearProfile& profile, double
startTime)`, containing the counting loop of `appendTransfer` (the per-span interval counts with
the 200000-point limit, using `profile.limits.acceleration`, and the local time of every point
after the first). `startTime` is the motion time of the preceding point, which the solver needs
for its monotonic-time check.

No new solver class. `ChamferTcpSolver` (`src/trajectories/chamfer/chamfertcpsolver.h`) already
owns exactly the state `MotionCompiler` held in `m_solver` and `m_tcpToFlange` (a
`Kr10Kinematics`, the inverse tool transform and the borrowed worker-owned `ChamferRobotSetup`),
so it gains the members: `forward(joints, progress)` (FK TCP point), `inverse(tcp, seed)` (the
former `MotionCompiler::inverse`: seeded IK, `withinLimits`, one flange FK check; not `solveTcp`),
`samplePtp(start, end)` returning the points of the former `appendPtp` loop, and
`LinearSamples solveLinear(const LinearSchedule& schedule, const Kr10Kinematics::IkSolution&
seed, const QString& label)` returning `struct LinearSamples { QVector<ChamferJointPoint> points;
Kr10Kinematics::IkSolution last; QString error; }`. `solveLinear` contains the IK loop of
`appendTransfer` followed by the former `inspectTransferRates`, now the private
`linearRateError(points, label)`. The rate check is anchored at the seed's angles and
`schedule.startTime`; the seed is always the solution of the preceding point, whose angles equal
that point's angles. `label` is the text the current code inserts with `centralPhaseName(m_phase)`,
so the produced errors are character for character identical, including the unprefixed
"Transfer timestamps cannot be resolved.".

`double ptpDuration(const V6d& delta, const ChamferTimingParameters& timing)`, the duration
calculation of `HomeTimer::timePtp`.

`struct JointSplineResult { QVector<std::array<V6d, 4>> coefficients; QString error; }` and
`JointSplineResult clampedJointSpline(const QVector<ChamferJointPoint>& points, const
ChamferRobotSetup& robot)`, the body of the former `ChamferMotion::prepareJointSpline` for the
points of one range, including the interval checks. `ChamferMotion::preparePlaybackSplines`
passes each range as `m_points.mid(first, last - first + 1)` and appends the coefficients, so
`prepareJointSpline` is removed.

`MotionCompiler::appendTransfer` becomes: build the profile, check its start against the
preceding TCP, call `linearSchedule`, keep the existing 400000-point check between
`linearSchedule` and `solveLinear` (this preserves the first failure), then append the solved
points. Add both new files to `src/trajectories/CMakeLists.txt`.

Parameter counts. The four-parameter signatures of the first version of this plan were reduced
(`Decision Log`, 2026-10-25): `linearProfile` to three by grouping feed and acceleration,
`linearSchedule` to two because the profile stores its acceleration, `solveLinear` to three
because the schedule carries its profile and the solver owns the joint speeds, the rate check to
two as a private member, and `clampedJointSpline` to two by passing only the points of the range.

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
      Q_PROPERTY(bool active READ active NOTIFY stateChanged)
      Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
      Q_PROPERTY(double elapsed READ elapsed NOTIFY playbackChanged)
      Q_PROPERTY(double duration READ duration NOTIFY stateChanged)
      Q_PROPERTY(QString phase READ phase NOTIFY playbackChanged)
      explicit ScenePlayback(SceneModel* scene)
      quint32 ownerId() const
      void start(std::shared_ptr<const ChamferMotion> motion, quint32 ownerId)
      void resume()
      Q_INVOKABLE void pause()
      Q_INVOKABLE void stop()
      void end()
      const std::optional<RoboCrap3D::Kr10KinematicModel>& robotModel() const
      bool robotAvailable() const
      void setRobotModel(const RoboCrap3D::Kr10KinematicModel& model)
      void setRobotAvailable(bool available)
      void setRobotController(RoboCrap3D::RobotController* controller)
    signals: stateChanged(), playbackChanged(), failed(quint32 ownerId, const QString& error),
             robotModelChanged(), robotAvailabilityChanged()

`start` receives the fresh-start body of `SceneMachining::dryRun`: it stores the motion and owner,
presents time zero and starts the clock and the 16 ms precise timer. `resume` receives the
paused-motion body. `pause`, `stop`, `end` (former `endPlayback`), `present` (former
`presentPlayback`), `advance` (former `advancePlayback`) and the failure path move unchanged; a
failure calls `end()` and emits `failed(ownerId, error)` instead of setting a coordinator error,
and a missing robot controller emits `failed` without ending playback, as before. `stateChanged`
is emitted where `SceneMachining` emitted `availabilityChanged` (start, resume, pause, end);
`playbackChanged` is emitted for every presented time, as before. The constructor connects
`SceneModel::objectRemoved` (ending playback whose owner was removed) and
`QCoreApplication::aboutToQuit` (ending playback). `setRobotAvailable(false)` ends playback.

`SceneMachining(SceneModel*, SceneEndEffectors*, ScenePlayback*)` keeps Generate, input selection,
compatibility, `canPlay`, a `dryRun()` that resumes paused playback or starts the selected
compatible path, and `stop()`, which stops the playback only while a machining path owns it. It
loses the playback properties, `pause`, `setRobotModel`, `setRobotAvailable`,
`setRobotController` and the unused `QObject* parent` parameter. Its former `setRobotModel` work
(joint speeds into settings, then `robotSetupChanged`) runs from `ScenePlayback::robotModelChanged`;
`refreshCompatibility` runs from `robotAvailabilityChanged`; `availabilityChanged` is emitted on
`stateChanged`; `failed` with a machining owner sets the machining error; `robotSetupChanged` ends
only machining-owned playback. `RobotController(OccController*, SceneEndEffectors*,
ScenePlayback*)` calls the playback object where it called `SceneMachining`. `WorkspaceController`
is unchanged and still calls `SceneMachining::stop` when leaving Machining. `main.cpp` constructs
`ScenePlayback` after `SceneModel` and before `SceneMachining`, sets its singleton instance and
calls `setRobotController` where it called `machining.setRobotController`.

QML: in `Main.qml`, `ModeToolBar.playing`, `playbackActive`, `onPauseRequested` and
`onStopRequested` use `App.Playback`; `playbackAvailable` and `onDryRunRequested` stay on
`App.Machining`; `generationAvailable` uses `!App.Playback.active`. `MachiningPanel.qml` gets
`required property Backend.Playback playback` and reads `playing`, `active`, `phase`, `elapsed`
and `duration` from it; its creator in `Main.qml` passes `App.Playback`.

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

Scene value. In `scenegeometry.h` add `struct SceneSphere { V3d center; double radius; double
ballCenterRadius; double compensatedRadius; double rms; QList<V3d> roughPoints; }`: the fitted
center, the nominal radius (half the file's diameter), the radius of the sphere through the ruby
ball centers, that radius minus the ruby ball radius (`SceneEndEffectors::rubyBallDiameter / 2`,
1.5 mm), the RMS and the readings. Add it to `SceneObject::Shape` with a `sphere()` lookup, and add
the kinds `Sphere` and `CalibrationPath` to `SceneObject::Kind`. `SceneObject` gains
`showBallCenterSphere` and `showCompensatedSphere` (both true initially, notify
`sphereDisplayChanged`), changed through `SceneModel::setSphereDisplay(object, ballCenter,
compensated)`, which reinstalls the object's drawing and restores the flags if the viewport
rejects it. `SceneObject::drawing` returns two `DrawingSphere` parts at the fitted center: the
compensated sphere (opaque, default color) and the ball-center sphere (green, 20 % transparent (DrawingStyle::transparency 0.2) so
the compensated sphere stays visible inside it), each with `DrawingPart::visible` from its flag;
the viewport does not install a hidden part. `drawingDiagnostics` returns the rough points as
markers. `properties` returns, in this order (user layout, 2026-10-25): "Nominal radius", "Nominal
diameter", a separator, the heading "Ball-center sphere" with its visibility check box,
"Radius/Diameter" of the ball-center sphere, a separator, the heading "Compensated sphere" with its
check box, "Radius/Diameter" of the compensated sphere, a separator, "Fitted center", "Fit RMS" and
"Points"; a separator row is `{separator: true}` and a heading row is `{label, toggle}` with the
`SceneObject` property name. Headings may be wider than the label column; their check box follows
the text. Every value stays on one line: the right column
of `Main.qml` (Properties, end-effector and Machining panels) is 350 px instead of 330 px, which
leaves 202 px for values next to the unchanged 120 px label column. The fitted center
"[1054.1403; -331.9692; 454.2727]" measures 185 px in Roboto 13 px design metrics and about
194 px as rendered (it was clipped by 12 px in the 182 px of the 330 px panel). Add `SceneObject*
SceneModel::addSphere(SceneSphere sphere, const QString& name)` like `addBoundedPlane`. In
`ScenePanel.qml` add a "Calibration" group for kinds `Sphere` and `CalibrationPath` (both are
`Unclassified`, so the "rough" and "precise" filters of `objectsForGroup` already exclude them),
and return
`qrc:/pics/calibration.svg` and `qrc:/pics/path.svg` from `objectIcon`. Add `required property bool
calibration` (bound in `Main.qml` to "mode is Measuring and submode is Calibration"): when true
the panel lists only the "Calibration" group, otherwise it lists every group except
"Calibration". A `Sphere` row shows `resources/pics/sphere.svg` (user-supplied, green) with its
own colors, without the group tint.

Sphere file. `resources/json/calibration_sphere.json` already exists (created 2026-10-25 before
this milestone) from the ten lines of `resources/calibration_sphere_jog_points.txt`: one
`roughPoints` entry per `DECL E6POS` line with its name and X, Y, Z, A, B, C, S and T values
(E1–E6 omitted), the controller's number text unchanged, one entry per line:

    {
      "version": 1,
      "sphereDiameter": 25.0,
      "roughPoints": [
        {"name": "XP1", "X": 1053.17981, "Y": -332.410950, "Z": 468.076385, "A": -179.999786, "B": 0.00114061497, "C": 179.999619, "S": 2, "T": 10},
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
QML singleton `Calibration`. Milestone 4 implements `SceneCalibration(SceneModel*)` with the
property `error` and `setActive(bool)`; Milestone 5 adds the `SceneEndEffectors*` and
`ScenePlayback*` collaborators, `unavailableReason`, `canGenerate`, `calculating` and
`generate()`, and Milestone 6 adds `canPlay` and `dryRun()`. `setActive(true)` reads `QCoreApplication::applicationDirPath() +
"/resources/json/calibration_sphere.json"` with `QFile` and `QJsonDocument` on the GUI thread,
validates it, calls `fitSphere`, and builds `SceneSphere` with radius `sphereDiameter / 2` at the
fitted centre. If no sphere object exists, or its centre, radius or points differ from the new
value, it inserts the new object, then removes the old sphere and any calibration path. It then
records the ids of all other visible objects in a member list, hides them with
`SceneModel::setObjectVisible`, and makes the sphere and the path visible. While active, it hides
and records every object added by `SceneModel::objectAdded` that is neither the sphere nor a
calibration path. `setActive(false)` calls `ScenePlayback::stop` if the playback owner is the
calibration path, hides the sphere and the path, shows the recorded objects that still exist and
clears the list. An unreadable or invalid file removes nothing and sets `error` to the path and
the failed condition, for example "Calibration sphere file <path>: roughPoints needs at least 4
points with finite X, Y, Z."; the other objects are hidden all the same, and an existing sphere
and path are shown.

Submode. Add `Calibration` to `WorkspaceController::Submode` and to the QML enum in
`WorkflowPanel.qml`; accept it in `setSubmode`; add the submode button "Calibration", first in the
row before "Rough" and "Precise", with a
new icon `resources/pics/calibration.svg` registered in `resources/pics.qrc`. `WorkspaceController`
emits `calibrationActiveChanged(bool)` whenever "mode is Measuring and submode is Calibration"
changes, emitting false before the active tool changes so that `SceneCalibration::setActive(false)`
stops calibration-owned playback first, and clears the selection on both transitions; `main.cpp` connects it to `SceneCalibration::setActive`. In the Calibration submode
`WorkspaceController::deleteSelected` returns without removing anything, so neither the sphere nor
the calibration path can be deleted. In `ModeToolBar.qml` add `readonly property bool
calibration` and the inputs and signals the calibration buttons need; in the Calibration submode
show only
"Measuring EE", "Generate Path", "Dry Run"/"Pause" and "Stop", and hide "Intersect Selected", the
JSON imports, "Scan" and "Dry Scan". `Main.qml` binds those buttons to `App.Calibration` and
`App.Playback`, its footer label reads "Mode: Measuring / Calibration", and its status message
shows `App.Calibration.error`, else `App.Calibration.unavailableReason`, in the existing label.
Inspect the toolbar, Workflow panel and viewport sizes when switching submodes and when messages
change (`AGENTS.md`, Mandatory interface stability).

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report. Then, in the
running application, clicking Calibration shows the compensated sphere inside the translucent
ball-center sphere; selecting the object shows Nominal radius 12.5000, Nominal diameter 25.0000,
Ball-center sphere Radius/Diameter 13.8342 / 27.6684, Compensated sphere Radius/Diameter 12.3342 /
24.6684, Fitted center [1054.1403; -331.9692; 454.2727] (completely, with the closing bracket),
Fit RMS 0.0694 and Points 10, and each sphere check box hides and shows its sphere (the values from `Surprises & Discoveries`); the points overlay shows the ten
readings on a shell about 14 mm from the centre; with the sample plane, cylinder and edge loaded
before entry, Calibration shows only the robot with the MEE and the sphere, the Scene panel lists
only the "Calibration" group with the green sphere icon, the sphere can be hidden and shown, and
Delete removes nothing; clicking Rough hides the sphere and shows the plane, cylinder and edge again,
except one the user had hidden before entry; changing one coordinate in the deployed file and
clicking Calibration again moves the sphere; renaming the file shows the path error in the status
bar without moving any control.

### Milestone 5: touch plan, touch motion, Generate and path drawing

Scope: Generate Path in the Calibration submode computes the 31-touch motion of "The touch
geometry" and draws it. Read "Machining and playback" and "Geometry and scene". Before any edit of
this milestone, remind the user of the movement-time estimate in `Artifacts and Notes` and of the
open table clearance at the end of "The touch geometry" (the user asked on 2026-10-25 to be
reminded of the movement-time estimate here).

CIRC. `LinearProfile` gains `V3d centre`, `V3d axis` and `double angle` (rad, 0 for a straight
LIN). `target(t)` keeps the slerped rotation and, for a nonzero angle, places the TCP at
centre + AngleAxis(u · angle, axis) · (first position − centre), u = progress(t).
`std::optional<LinearProfile> circularProfile(const LinearProfile& chord, const V3d& centre)`
takes the validated straight profile of the same end poses and limits, rejects a nonfinite centre,
end radii that differ by more than 0.01 mm and nearly coincident or opposite end directions
(‖from × to‖ ≤ 1e-6 · radius²), sets the axis, angle = atan2(‖from × to‖, from · to) and
length = radius · angle, and recomputes peak, ramp and cruise with the timing code that
`linearProfile` uses, extracted into one internal function. `linearSchedule` adds the normal
acceleration bound peak² · angle / length (= peak² / radius) to the acceleration of every stage;
it is 0 for a straight LIN, so chamfer counts are unchanged. Example, a 29.53° arc of the λ = −10°
ring: length 8.762 mm, ramps 2 × 0.5 s with ceil(0.5 · √(25.88 / 0.08)) = 9 intervals each,
cruise 0.376 s with ceil(0.376 · √(5.88 / 0.08)) = 4 intervals; the sagitta between stored points
is 0.94² / (8 · 17) = 0.0065 mm.

Constant-speed LIN. `linearProfile` (`src/trajectories/motion/jointsegments.cpp`) now rejects
`limits.acceleration` = 0: the requested ramp distance feed / 0 is infinite, so the profile is
treated as triangular and its peak speed √length · √0 = 0 fails the check. It gains one branch:
acceleration exactly 0 means constant speed, with peak = feed, ramp = 0 and cruise =
length / feed, accepted when peak and cruise are finite and positive; every other input keeps the
present arithmetic. `LinearProfile::progress(t)` then returns peak · t / length, and
`linearSchedule` gives the single constant-speed stage max(1, ceil(T · √(0 / 0.08 mm))) = 1
interval, so an Approach stores two points: its start and its contact 3 s later. Chamfer is
unaffected, because `validTiming` requires a positive `transferAcceleration`.

Add to `RoboCrapTrajectories` the published headers `include/trajectories/probetouchplan.h` and
`include/trajectories/probetouchmotion.h` with sources `probe/probetouchplan.cpp` and
`probe/probetouchmotion.cpp`, all listed in `src/trajectories/CMakeLists.txt`.

`probetouchplan.h` defines `struct ProbeTouchPlanParameters { V3d sphereCentre = V3d::Zero();
double sphereRadius = 12.5; double ballRadius = 1.5; double strutRadius = 8.9; double
strutRecess = 2.0; }` (the values that come from the sphere file and `SceneEndEffectors`; the
other values of "The touch geometry" and the touch table, λ, μ, τ and the unwind flag per touch,
are constants in `probetouchplan.cpp`), `struct ProbeTouch { M4d start; M4d contact; V3d
normal; bool unwindAfter = false; }`, `struct ProbeTouchPlan {
ProbeTouchPlanParameters parameters; M4d entry; QVector<ProbeTouch> touches; M4d exit; }` and
`struct ProbeTouchPlanResult { std::optional<ProbeTouchPlan> plan; QString error; }` returned by
`ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters)`, which
rejects nonfinite values, nonpositive radii or recess and a recess of at least the sphere
diameter, builds the entry pose, the 31 touches and the exit pose of "The
touch geometry", and rejects a touch whose start or contact has a strut clearance below g with
"Touch <n>/31: strut clearance <mm> mm is below 3 mm".

`probetouchmotion.h` defines `enum class ProbeSegment { ToEntry, Descend, Transfer, Approach,
Retract, Unwind, Retreat, ReturnHome }`, `struct ProbeMotionPhase { int touch; ProbeSegment
segment; }`, and `class ProbeTouchMotion` with `static ProbeTouchMotionResult create(const
ProbeTouchPlan& plan, const ChamferRobotSetup& robot)`, `const QVector<ChamferJointPoint>&
points() const`, `const ChamferRobotSetup& robot() const`, `double duration() const`, `double
minimumTransferClearance() const`, `double minimumStrutClearance() const` and
`std::optional<std::pair<V6d, ProbeMotionPhase>> evaluateJoints(double seconds) const`; `struct
ProbeTouchMotionResult { std::optional<ProbeTouchMotion> motion; QString error; }`. The motion
stores its points, one entry per segment (first and last point index and `ProbeMotionPhase`) and
one coefficient array per Descend, Transfer and Retreat segment. The interpolation of a segment
follows from its `ProbeSegment`, so the entry stores no separate kind.

`create` validates the robot with `validRobot`, copies the rated joint speeds from the model as
`ChamferMotion::create` does, rejects `transferFeed` above `MaximumTransferFeed` or
`transferAcceleration` above `cartesianAcceleration` with the same error text as chamfer, and
builds the segments in order with one `ChamferTcpSolver`: the HOME point; ToEntry, a PTP to
`plan.entry` (target joints from `ChamferTcpSolver::solveTcp` seeded with the HOME configuration,
which `kinematics().configuration(qHome, kinematics().solveFK(qHome))` provides; HOME has A5 = 0,
which the IK classifies as the positive wrist branch, so the motion starts with A5 > 0); Descend,
a LIN from the entry to the start of T1; for each touch after T1, Transfer, a CIRC from the
start of the preceding touch to its start (`circularProfile` of the LIN profile with the sphere
centre); Approach (LIN start→contact) and Retract (LIN contact→start), both with
`LinearLimits{1.0, 0.0}`, and, after a touch with `unwindAfter`, Unwind, a PTP to the target from
`solveTcp(start, seed)` whose seed is the current solution with A4, A5 and A6 changed as in "The
touch geometry" and the wrist bit of its status (bit 2) toggled; finally Retreat, a LIN from the
start of the last touch to `plan.exit`, and ReturnHome, a PTP to `qHome`. Descend, Transfer and
Retreat use `{transferFeed, transferAcceleration}`. Every LIN is built with `linearProfile`,
`linearSchedule` and `solveLinear` (which includes the joint-speed check). PTP segments are sampled
with `ChamferTcpSolver::samplePtp` and timed with `ptpDuration` and `inverseProgress` as
`HomeTimer::timePtp` does. Each Descend, Transfer and Retreat segment gets its own
`clampedJointSpline`; Approach and Retract get none. Every stored point of ToEntry, Descend,
Transfer, Unwind, Retreat and ReturnHome must satisfy ‖p_tcp − c‖ − d ≥ g − 0.0001 mm =
2.9999 mm; the smallest ‖p_tcp − c‖ − d is stored as
`minimumTransferClearance`. Every stored point of every segment must have a strut clearance of at
least g − 0.0001 mm; the smallest one is stored as `minimumStrutClearance`. Every error starts with
"Touch <n>/31, <segment>: " (segment texts as in Milestone 6) for the segments of a touch (Transfer into it, Approach, Retract,
Unwind after it), "Entry: " for ToEntry and Descend, or "Exit: " for Retreat and ReturnHome,
followed by the underlying message. The total point count is limited to 400000 as in chamfer.

`evaluateJoints` returns the first joints at 0 and the last at `duration()`. Between, it
evaluates stored values only and solves no IK: on PTP segments the smoothstep between the two end
joint vectors; on Descend, Transfer and Retreat the stored spline coefficients; on Approach and
Retract the joint-linear interpolation between the two stored points, q(t) = q_a + (t − t_a) /
(t_b − t_a) · (q_b − q_a), where q_a and q_b are the joint vectors stored at the times t_a and
t_b. The joint-linear interpolation keeps the ball at a constant speed: in the scratch check of all
31 Approaches the TCP stays within 0.0022 mm of the straight line and moves at 0.9961–1.0039 mm/s,
whereas a clamped spline through only two points would run 0 → 1.5 → 0 mm/s. It checks joint
limits like `ChamferMotion::evaluateJoints`.

In `SceneCalibration::generate`: refuse while calculating, while `ScenePlayback::active()`, without
a sphere, without a robot model or availability; build `ProbeTouchPlanParameters` from the sphere
centre, `sphereRadius` = nominal sphere radius, `ballRadius` = `rubyBallDiameter / 2`,
`strutRadius` = `strutDiameter / 2` and `strutRecess` from the sphere file (the sphere file
reader requires `strutDiameter` and `strutRecess`, finite, positive, the recess below the sphere
diameter, and stores them in `SceneSphere`); copy the model,
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

    Probe calibration preparation: 31 touches, <points> points, <seconds> s movement, minimum transfer clearance <mm> mm, minimum strut clearance <mm> mm

or `Probe calibration preparation failed: <error>`. `Main.qml` shows "Calculating trajectory…"
with the busy indicator while `App.Calibration.calculating` is true.

Acceptance: ask the user to build `robocrap` and `robocrap_qmllint` and report; then, in
Calibration, Generate Path either prints the success line with a minimum transfer clearance of at
least 2.9999 mm and a minimum strut clearance of at least 3.0000 mm (scratch estimates: about
947 points, about 246.2 s movement, strut clearance 3.2588 mm at the λ = −30° contacts) and draws
the path around the sphere, with approach segments ending on the 14 mm contact shell at the
latitudes and longitudes listed in "The touch geometry", or prints a failure naming touch and
segment. Record a failure, the sphere position and the user's decision in this plan before
changing any parameter.

### Milestone 6: Dry Run of the calibration motion

Scope: animate the generated motion. Replace the `ScenePlayback` motion member and the motion
parameter of `start` by a `std::variant` of `std::shared_ptr<const ChamferMotion>` and
`std::shared_ptr<const ProbeTouchMotion>` (Milestone 2 kept a plain chamfer pointer until this
second motion type exists); `ScenePlayback::present` uses `std::get_if` and evaluates it with
`motion->robot().flangeToTcp` and sets the phase to "Entry: to entry", "Entry: descend",
"Touch <n>/31: <segment>" with the segment texts "transfer", "approach", "retract" and
"wrist unwind", "Exit: retreat" or "Exit: return HOME".
`SceneCalibration::canPlay` is true when a calibration path exists, the robot is available and
nothing is calculating; `dryRun` calls `ScenePlayback::start(motion, pathId)`; `failed` with the
calibration path as owner sets `SceneCalibration::error`.

Acceptance: ask the user to build `robocrap` and report, then to run: Calibration → Generate Path
→ Dry Run. The robot with the measuring tool moves from HOME through the 31 touches, the ball
moves at a constant speed in every approach and retract and stops on the sphere surface in every
approach, the wrist turns over in place after T21, and the motion ends at HOME; Pause, resume and Stop
behave as in chamfer Dry Run; the path's visibility button in the Scene panel hides and shows the
path without stopping playback; clicking Rough during playback returns to HOME; Apply of a
different measuring TCP removes the path and disables Dry Run.

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

Working directory: the repository root, `E:\Qt\QtProjects\RoboCrap` or
`D:\Qt\QtProjects\RoboCrap` depending on the computer. For every milestone, in this order:

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
measuring TCP editable (Milestone 3); the sphere shown with the fit values listed in Milestone 4,
with only the robot, the sphere and the calibration path visible in the Calibration submode and
the other objects' visibility restored after leaving it;
a successful Generate with a minimum transfer clearance of at least 2.9999 mm and a minimum
strut clearance of at least 3 mm (Milestone 5); a Dry Run of all 31 touches that ends at HOME
(Milestone 6); and no layout movement of the toolbar,
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

The motion duration is movement time, not computation time. Scratch estimate for the 31-touch
motion with the default timing (2026-10-26): Approach and Retract 31 · (3 s + 3 s) = 186 s; 35
transfer chords about 44 s in total (a 30° chord on the 17 mm shell is 8.8 mm long and takes
0.5 s + 3.8 mm / 10 mm/s + 0.5 s = 1.38 s); Descend 4.8 s and Retreat 3.0 s; the PTPs HOME to
entry 3.1 s, wrist unwind 4.2 s and exit to HOME 3.2 s; in total about 248 s, about 4 minutes.

Notes for the later RSI plan (agreed with the user on 2026-10-26, not implemented here): a real
touch starts at p_start and moves along −n at a constant 0.004 mm per 4 ms RSI cycle (1 mm/s)
without a ramp, stops when `$IN[8]` (the WP-500 trigger through X2.5, Y1.7 and EL1809 input 8)
switches, records `$POS_ACT`/RIst at that cycle, and then retracts to p_start. The search ends
without contact after 4.5 mm (g plus an overtravel of 1.5 mm), which is a failed touch. The
animation's 3 mm Approach at 1 mm/s matches this motion up to the contact.


## Interfaces and Dependencies

No new external dependency. New published headers: `geometry/spherefit.h`,
`trajectories/probetouchplan.h`, `trajectories/probetouchmotion.h`. New internal Trajectories
files: `motion/jointsegments.{h,cpp}`, whose `linearProfile` accepts acceleration 0 as constant
speed and which provides `ratedRobotSetup` from Milestone 5; the internal `ChamferTcpSolver` gains `forward`, `inverse`, `samplePtp`
and `solveLinear`, and `ChamferMotion::prepareJointSpline` is removed.
New application files: `scene/sceneplayback.{h,cpp}`,
`scene/scenecalibration.{h,cpp}`; new resources: `resources/json/calibration_sphere.json`,
`resources/pics/calibration.svg`. New QML singletons: `Playback`, `Calibration`. Changed
signatures: `RobotController(OccController*, SceneEndEffectors*, ScenePlayback*)`;
`SceneMachining(SceneModel*, SceneEndEffectors*, ScenePlayback*)`;
`DrawingPart::geometry` gains `DrawingSphere`; `SceneObject::Kind` gains `Sphere` and
`CalibrationPath`; `SceneObject::Shape` gains `SceneSphere` and `SceneTouchPath`;
`WorkspaceController::Submode` gains `Calibration` and the signal
`calibrationActiveChanged(bool)`; `SceneEndEffectors` gains `measuringTcp`,
`measuringTcpFrame`, `applyMeasuringTcp` and `measuringTcpChanged`; `EndEffectorPanel.qml` gains
`measuringTcpRequested`; `MachiningPanel.qml` gains a required playback property;
`ScenePanel.qml` gains `required property bool calibration`. Every four-parameter signature above
must be justified by failed reduction attempts recorded in the `Decision Log` before it is
written (Milestone 1, "Four-parameter signatures").


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
- 2026-10-24: Milestone 2 now names `robotSetupChanged` (formerly `calibrationChanged`) and calls
  the `SceneEndEffectors` collaborator "tool transforms", because the project renamed
  `SceneMachining::calibrationChanged` and `SceneMachiningData::calibrationRevision` to
  `robotSetupChanged` and `robotSetupRevision`: `AGENTS.md` forbids calling a transform matrix
  "calibration".
- 2026-10-25: the repository root may be on drive E: or D: depending on the computer (user);
  Context and Concrete Steps name both.
- 2026-10-25: `AGENTS.md` now permits four parameters only after recorded attempts to reduce the
  count to three or fewer (user); Context, Milestone 1 ("Four-parameter signatures") and
  Interfaces follow the new rule.
- 2026-10-25: the Calibration submode shows only the robot with the MEE, the sphere and the
  calibration path, and only the path's visibility can be changed there (user); Purpose, the
  Decision Log and Milestone 4 (Scene panel, `SceneCalibration::setActive`, `WorkspaceController`,
  acceptance) now describe hiding, restoring, deletion and selection.
- 2026-10-25: recorded the corrected Retract and Descent durations in `Surprises & Discoveries`
  and a reminder at the start of Milestone 5 (user request).
- 2026-10-25: neither the sphere nor the calibration path can be deleted, and the touch geometry
  is fixed in code (user); `deleteSelected` removes nothing in the Calibration submode. Purpose,
  the Decision Log and Milestone 4 changed accordingly.
- 2026-10-25: comments that justified three- and four-parameter signatures were removed from the
  code base (user); the line-count baseline of the affected files was recounted.
- 2026-10-25: Milestone 1 implemented. Its Plan of Work and the Milestone 5 motion construction
  now name `ChamferTcpSolver` instead of the dropped `TransferSolver`, the reduced signatures and
  the rate check inside `solveLinear`; Progress records the baseline line and the line counts.
- 2026-10-25: Milestone 2 implemented with the accepted deviations (`resume()`, unchanged
  `WorkspaceController`, no `SceneMachining` parent parameter, machining-owned stop and
  invalidation, plain motion pointer until Milestone 6) and the `stateChanged`/`playbackChanged`
  split. The Milestone 2 interface, the superseded `WorkspaceController` decision, the
  Milestone 4 submode wiring, Milestone 6 and Interfaces now describe that design.
- 2026-10-25: Milestone 2 accepted by the user; Milestone 3 implemented as planned, with Progress
  recording its line counts.
- 2026-10-25: Milestone 3 accepted by the user. The sphere file was created ahead of Milestone 4
  at the user's request; each `roughPoints` entry also carries the reading's `name` (XP1–XP10),
  which the application ignores like any unknown key.
- 2026-10-25: Milestone 4 implemented. The coordinator paragraph now describes the reduced
  Milestone 4 `SceneCalibration` and when Milestones 5 and 6 add the rest; the Scene panel
  paragraph notes that the rough/precise filters already exclude the new kinds; Progress records
  the fit check and the line counts.
- 2026-10-25: the sphere object shows the ball-center and compensated spheres with separate
  visibility check boxes and the user's Geometry layout; `SceneSphere::fittedRadius` became
  `ballCenterRadius` plus `compensatedRadius`. C++ 1566/1375 to 1637/1441 and QML 787/687 to
  828/722 physical/nonblank lines in the changed files.
- 2026-10-25: user review of Milestone 4: the Calibration button comes first, the sphere row uses
  the green `sphere.svg` without tint, the sphere's visibility stays switchable, the sphere
  Geometry rows follow the user's list, and property values stay on one line because the right
  panel column is 350 px instead of 330 px (a first attempt wrapped long values and a second one
  removed the 120 px label minimum; the user rejected the line break and the missing gap after
  "Nominal diameter"). The sphere is drawn without its seam line (`faceBoundary` part flag,
  viewport C++ 801/658 to 807/664 physical/nonblank lines in occpartprops.h, occpart.h,
  occpart.cpp, occscene.cpp and occsceneadapter.cpp). Purpose, the Decision Log
  and Milestone 4 changed accordingly; `SceneSphere::fittedContactRadius` is now `fittedRadius`.
- 2026-10-26: all Milestone 4 checks reported as passed. The strut under the sphere (user drawing)
  was added to "The touch geometry" (inputs, strut clearance rule, the β = 180° exclusion numbers)
  and to Milestone 5 (plan parameters, plan rejection, motion check, Generate inputs, console line,
  acceptance); `resources/json/calibration_sphere.json` gained `strutDiameter` and `strutRecess`.
- 2026-10-26: the 27-touch via-point motion was replaced by the 31-touch ring scan (user). "The
  touch geometry" now defines the rings, the rotation rule, the transfers on the start shell, the
  wrist unwind, entry and exit, the constant-speed Approach and Retract, the exact strut clearance
  and the scratch results, with the table clearance during the unwind left open. Milestone 5 now
  describes the zero-acceleration `linearProfile` branch, the new plan and motion interfaces and
  checks, and the joint-linear Approach/Retract playback; Milestone 6, Purpose, Validation,
  Artifacts and Notes (new estimate, RSI notes), Interfaces, Surprises and the Decision Log follow.
- 2026-10-26: Milestone 5 steps 1–4 implemented (user's split); Progress records the line counts
  and the compliance pass, and the Decision Log the implementation details (shared
  `ratedRobotSetup`, error format, published constants and data structs, sphere file messages).
- 2026-10-26: Milestone 5 steps 5–6 implemented; Progress records the line counts and the
  compliance pass, and the Decision Log the Generate details (motion inside `SceneTouchPath`,
  `addCalibrationObject`, availability signal, hidden late results, status text).
- 2026-10-26: Milestone 5 accepted (console line recorded in Progress); Milestone 6 implemented,
  with its details in the Decision Log and its line counts in Progress.
- 2026-10-26: CIRC transfers replace the chord transfers (user). "The touch geometry" (inputs,
  transfer, motion law, scratch results), Milestone 5 (CIRC profile, plan structs, motion
  construction, clearance rule, acceptance), Validation, Progress and the Decision Log changed.
- 2026-10-26: Milestone 7 documentation and compliance pass completed; Progress records the CIRC
  line counts and the documentation changes, and Outcomes & Retrospective the plan outcome.
- 2026-10-26: the user reported the CIRC build and all Milestone 6 checks as passed; the plan is
  complete.
