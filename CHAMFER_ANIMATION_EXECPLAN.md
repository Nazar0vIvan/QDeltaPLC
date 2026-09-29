# Generate and animate an internal-hole chamfer

This is a living ExecPlan maintained according to `PLANS.md`. `AGENTS.md` takes precedence where it prohibits tests, unsolicited builds, or Git mutations. Steps 1 through 8 are implemented; the remaining steps are pending.

## Purpose / Big Picture


The user selects a plane-cylinder intersection edge, enters chamfer and movement parameters, and presses Apply. A complete seven-stage trajectory appears under Machining Paths and in the viewport. Selecting it and pressing Dry Run animates the KR10 and its spindle end effector according to calculated elapsed time. The simulation never sends commands to PLC, RSI, or FTS.

The stages are HOME to P_s, straight transfer to lead-in start, lead-in, machining around one complete hole edge, lead-out, straight transfer back to P_s, and return HOME. The first and last stages represent direct KRL PTP movements without intermediate waypoints. The three central stages represent one continuous RSI motion. Actual KRL generation, RSI streaming, force compensation, material removal simulation, collision checking, persistence across application restarts, and exact KUKA controller emulation are outside this implementation.

## Progress


- [x] (2026-09-29) Review the conversation, corrected mathematical model, scene interfaces, toolbar, numerical KR10 interfaces, and build ownership; write this plan.
- [x] (2026-09-29) Step 1: Implement the corrected internal-chamfer geometry and elliptical moving frames. Source review and whitespace checks performed; build/runtime verification not requested.
- [x] (2026-09-29) Step 2: Add phase evaluation and adaptive sampling for lead-in, machining and lead-out. Static review and whitespace checks only; no build/runtime verification requested.
- [x] (2026-09-29) Step 3: Add untimed KR10 motion compilation, bounded starting-configuration search, seeded IK refinement and direct joint-space HOME stages. Static checks only; build/runtime verification not requested.
- [x] (2026-09-29) Step 4: Add time/feed settings, cubic central interpolation, synchronized quintic HOME timing, rate scaling, interpolation refinement and elapsed-time evaluation. Static/whitespace checks only; build/runtime verification not requested.
- [x] (2026-09-29) Step 5: Add SceneMachining, immutable scene path ownership, typed draft settings, backend registration, robot-readiness wiring and transactional Apply/regeneration. Source and whitespace checks only; build/runtime verification not requested.
- [x] (2026-09-29) Step 6: Render phase polylines with shared scene identity, global path visibility and non-selectable outward-axis preview; restore display settings on surface recreation. Static checks only; runtime visual verification pending.
- [x] (2026-09-29) Step 7: Add the parameter editor, selection/Flip preview, Generate Path focus and Apply/regeneration workflow. Playback controls remain Step 8. Source/whitespace checks only; no build/runtime verification requested.
- [x] (2026-09-29) Step 8: Implement monotonic elapsed-time playback, direct GUI-thread pose delivery, phase/time UI, Stop/restart and lifecycle/manual-pose guards. Source/whitespace checks only; build/runtime verification not requested.
- [ ] Step 9: Review integration and perform authorized verification.
- [x] (2026-09-29) UI follow-up: inspect application layers; move measuring EE to a toolbar-opened tool window and spindle EE plus chamfer editor to a second tool window; show cylinder diameter; clarify HOME percentage and simulation tuning. Source/diff checks completed; build/lint/runtime checks remain pending.

- [x] (2026-09-29) TCP follow-up: add calibrated RGB axes with zoom scaling and visibility, allow repeated Apply TCP, and default new hole inputs toward negative scene X. Source/diff review and an offline numerical reproduction of the current XP/XC geometry completed; build/lint/runtime remain unverified.

- [x] (2026-09-29) Drawing correction steps 1–3: separate local/TCP orientations, construct the midpoint from actual surface setbacks, and correct clear-end TCP Z. Source/diff checks only; build and animation verification pending. Auxiliary point P_s is deferred.

- [x] (2026-09-29) Follow-up step 4: add positive axial staging distance, P_s position and approach/return poses, and editable saved settings. Source/diff checks only; build/runtime pending.
- [x] (2026-09-29) Follow-up step 5: route timed HOME and Cartesian transfers through P_s; extend phase rendering and playback labels. Source/diff checks only; build/runtime pending.

- [x] (2026-09-29) Follow-up step 6: geometry-only labelled hole/staging/selected local or TCP frame preview, phase/progress controls and zoom sizing. Source/diff checks only; build/runtime pending.

## Decisions and agreed meanings


The operation is an internal chamfer. In the user's section drawing the hole is on the left and material is on the right. The outward hole axis points through the opening into free space. A visible arrow and Flip direction switch establish its sign; fitted geometry alone cannot establish outside. The local chamfer normal points into the hole and toward the opening, with strictly positive projection on the outward hole axis.

The TOOL frame is rigidly attached to the cylindrical burr's cutting surface. Its Y axis is the burr axis. During cutting TCP X equals local X, while TCP Y and Z oppose local Y and Z; the origins coincide. Local X is perpendicular to the longitudinal section, not necessarily tangent to the tilted ellipse. The existing flange-relative XYZABC calibration places this frame. It is never relocated on the CAD to put it on the workpiece. The trajectory prescribes where that fixed frame moves in robot BASE.

The user enters chamfer size c in mm as equal surface setbacks in the longitudinal section containing the hole axis and the selected edge point. One endpoint lies c along the end plane away from the hole; the other lies c down the cylinder generator. Their midpoint is the machining origin. A perpendicular opening gives radial and axial legs c and a 45-degree diagonal; the tilted opening has a varying section angle. This supersedes the earlier fixed-45-degree approximation. No cutter-radius offset or allowance field is added.

Clearance is an additional displacement along the outward hole axis from the machining TCP path, as in the corrected model. It is not guaranteed to equal perpendicular distance from the original end plane, especially for an oblique ellipse or nonzero chamfer size. Positive clearance goes toward the opening. Lead-in starts with the burr axis parallel to the hole axis and ends in machining orientation; lead-out reverses that progression.

Use the actual measured intersection ellipse, with a circle as a natural special case. Choose a deterministic start and traversal direction; do not add UI fields for them. No intermediate HOME waypoints. The existing robot model's HOME joint pose is authoritative for simulation. Scene coordinates are treated as the existing robot BASE coordinates; adding workpiece registration is outside scope.

## Context and Orientation


`src/CMakeLists.txt` owns RoboCrapGeometry, RoboCrapScene, RoboCrapDevices, and backend foreign registration. Geometry already compiles `src/3d/robot/kinematics/kr10kinematics.*` despite that directory name. Do not move it as part of this feature. Scene depends on Geometry; no new Scene-to-viewport dependency is allowed.

`src/geometry/surfaceintersection.*` defines immutable EdgeGeometry with ellipse center, axes, and radii. `src/scene/scenegeometry.h` contains SceneEdgeGeometry with immutable geometry and source IDs. `src/scene/scenemodel.*` creates intersections and emits targeted addition/removal notifications. `src/scene/sceneobject.*` already has Kind::MachiningPath, and `qml/Views/Workspace/ScenePanel.qml` already groups it into Machining Paths. An edge does not contain the source cylinder axis, so generation must resolve its source plane and cylinder and copy the necessary numerical values. Source deletion currently does not cascade to edges; preserve that behavior.

`src/scene/sceneendeffectors.*` owns spindle TCP calibration. `src/3d/robot/robotpreviewstate.*` owns the loaded robot model and committed preview pose. `src/3d/occcontroller.*` applies poses and exposes solveFK/solveIK/solveTcpIK. Its interactive IK methods also change the displayed robot: do not use them in the offline generation loop. Use Kr10Kinematics directly with an explicit preceding joint seed.

`src/3d/adapters/occsceneadapter.*` maps scene IDs to presentations. `src/3d/adapters/occrobotadapter.*` moves the robot and attached tools. `qml/Main.qml` coordinates selection, mode, and viewport. `qml/Views/Workspace/ModeToolBar.qml` opens setup and wires Generate Path, Dry Run, and Stop to Main and SceneMachining. Start Machining must stay inactive.

The existing `src/pathgeneration/rsi/rsipath.*` is an offset generator, not a suitable trajectory model: its six-coordinate length mixes mm and degrees. Do not change or call its device execution path for this feature. The current kinematic model has joint position limits but no calibrated joint speed/acceleration limits. Timing must identify its configured simulation limits rather than pretend these are controller limits.

## Plan of Work


### Follow-up step 6 — Inspect target frames without IK


Add SceneCoordinateFrame (identity-initialized numerical transform and label) in scenegeometry.h, and SceneMachining preview controls separate from saved machining settings. showFrames defaults false; framePhase indexes LeadIn, Machining, LeadOut, P_s approach, P_s return; progress is finite in [0,1]; localFrame selects the reference frame instead of target TCP. previewGeometry validates the selected visible edge/path parameters and displays geometry errors without requiring robot readiness or TCP calibration. It never invokes apply, IK, playback or hardware.

The hole frame origin is the ellipse center O_H, Z is the chosen outward axis, X is the normalized major-axis projection onto its transverse plane and Y=Z cross X. Display the appropriate P_s TCP attitude (return for lead-out/return selection, approach otherwise). For a selected central phase, display one frame at the chosen progress. TCP uses evaluatePhase directly. The local reference uses the cutting frame at the same angle times Rx(180), translated to the lead sample origin; its labels are {i}, {i_L}, {i_O}. Thus the local lead reference does not rotate with the TCP lead correction. Toggle local/TCP to compare at the same origin without overlapping opposing trihedra.

MachiningPanel reuses commitDraft for Apply and Preview frames (no IK), and offers Show geometry frames, a phase selector, progress slider and Local/TCP selector. No registration or module changes: new properties belong to the existing registered SceneMachining class. OccController synchronizes diagnostics on preview, settings, input and availability changes. New setMachiningFrames methods pass numerical frames through OccViewWindow (retained for surface recreation), OccViewport and OccSceneAdapter; the adapter converts to OCCT transforms. OccScene owns independent OccWorldAxes and AIS_TextLabel presentations. Remove old handles on updates, disable selection, exclude fit bounds, and resize axes with the existing camera-scale convention. Robot/calibrated TCP presentation is unaffected.

Acceptance after an authorized build: import/intersect XP/XC, enter geometry, press Preview frames with no calibration or even after IK failure. Inspect {H}, P_s and selected frame; scrub both lead endpoints, cutting quadrants and seam; toggle local/TCP, check coincident origins and opposite cutting Y/Z; choose both P_s attitudes and check Y opposite hole Z. Zoom, hide/show input, switch selection/mode and recreate the surface; decorations must resize, clear or restore appropriately without moving robot, changing camera fit or capturing picks. These checks are pending; source/diff review is the available validation under the current no-build/no-tests instruction.



### Follow-up step 5 — Timed motion through P_s


This section supersedes the earlier five-stage implementation descriptions and the step-4-only deferral below. ChamferMotionPhase now orders Approach, TransferIn, LeadIn, Machining, LeadOut, TransferOut, ReturnHome; eight inclusive boundary indices delimit seven phases. Update the motion compiler, timer, elapsed-time evaluator, SceneMachining phase labels and OccSceneAdapter rendering together. The public boundary array necessarily grows from six to eight elements; all consumers are updated.

Start the existing bounded IK branch search at stagingPose(LeadIn), then append synchronized joint-space HOME approach. A shared centralTarget helper evaluates five central phases: fixed-attitude linear TCP transfers at the ends, and existing lead/machining evaluators in between. Transfers begin with 16 intervals and use existing recursive IK refinement, FK validation and timed interpolation checks. Each transfer matches the exact clear-lead orientation, preserving Y=-k; the return pose may have a different roll at the same P_s position. No isolated IK reset occurs between phases.

Timing uses leadInFeed for TransferIn/LeadIn, machiningFeed for cutting, and leadOutFeed for LeadOut/TransferOut. Set Cartesian speed and joint interpolation velocity to zero at both transfer/lead corners, since a straight segment is generally not tangent to the lead curve. Also stop at both P_s PTP junctions. Keep the lead/cutting joins continuous. Existing sampled 0.01 mm/0.1 degree interpolation checks and rate scaling cover transfers too; central time scale includes all five Cartesian phases. HOME return is phase six, after the return P_s pose.

Render HOME phases purple, transfers and leads blue, and cutting green. Update the panel explanation to describe actual routing. Acceptance after an authorized build: regenerate XP/XC with Apply, inspect seven rendered phases, confirm both P_s visits at the configured distance, fixed transfer orientation, stops at transfer corners, no jumps at cutting joins, and final HOME. Inspect errors for approach P_s or the named failing central phase. Changing P_s distance must change approach/return and transfers without changing the cutting loop. No build or runtime is authorized by this implementation request; no tests/harnesses are added. No collision-envelope calculation is available, so this sequence is not verified for tool clearance.



### Follow-up step 4 — Axial staging point


Add stagingDistance to ChamferPathParameters and SceneMachiningSettings, default 10 mm, finite and strictly positive. This is measured along the outward hole axis from O_H, the intersection of the end plane and cylinder axis, which equals the validated ellipse center. It is not a perpendicular plane distance or a displacement from the cutting midpoint. ChamferPath::stagingPoint() returns O_H+d_s*k. Check the resulting position for overflow at creation.

ChamferPath::stagingPose(ChamferPhase) returns an optional BASE-relative TCP transform. LeadIn copies the clear lead-in start rotation; LeadOut copies the clear lead-out end rotation. Both replace the origin with P_s, giving Y=-k and retaining each lead's roll. Machining returns an empty optional. This reuses the corrected lead frame definition instead of duplicating orientation math. Different lead spans may produce different approach/return roll at the same point.

Expose stagingDistance in the existing typed settings, copy it in applyTo/fromMotion, and add a positive-distance field in MachiningPanel's existing draft loop. Apply saves it in the immutable path snapshot; Reset and saved-path selection restore it. Explain in the panel that movement through P_s is the next step. No motion phases, timing, joint limits, calibration or renderer behavior change in step 4.

Acceptance: after an authorized build, default distance is 10 mm; zero/negative/nonfinite drafts cannot apply; changing it and generating a path preserves it when reselecting the path. Source review establishes identical staging positions for both sides and exact clear-end rotations. Flip reverses displacement from O_H and Y direction consistently. No tests/harnesses or unsolicited builds are permitted. Timed motion routing and marker visualization are not supplied by this step.



### TCP follow-up — Calibrated frame and example opening direction


For the current XP/XC example, initialize a newly selected edge's Flip draft from the sign of the fitted cylinder axis X component so the outward hole axis projects toward negative scene X. Do not change measured geometry or overwrite saved path settings; Flip remains available. This convention is explicit for the current workpiece, not an inferred universal inside/outside classifier. The hole axis is distinct from the spindle TCP's local Z axis.

Add showSpindleTcp as an OccController display preference. EndEffectorPanel receives an explicit visibility input/signal and shows a Visible checkbox in a TCP trihedron row. Main commits validated TCP through SceneEndEffectors and makes the axes visible on successful Apply. Apply requires valid values, not the modified flag; unchanged calibration remains a backend no-op, avoiding unnecessary path invalidation. Reset retains its existing draft behavior.

Controller supplies an optional flange-relative numerical transform using the existing XYZABC helpers. OccViewWindow retains this presentation input across surface recreation; OccViewport forwards it to OccRobotAdapter. The robot adapter combines committed flange and TCP transforms and converts them using its existing OCCT transform helper. OccScene renders a separate OccWorldAxes instance, with RGB axes, no picking and no fit contribution. Reuse world-axis camera scale sizing on calibration, zoom, resize and fit; hide while no robot is attached or the spindle is inactive. Retain the user's visibility preference across mode changes. No new C++ output parameters, lambdas or exception syntax.

Acceptance: enter six finite flange-relative values and Apply; RGB axes appear at BASE_FLANGE * FLANGE_TCP and follow FK/playback. Repeated Apply stays enabled and restores hidden axes. Visible hides only TCP axes. Zoom in/out and resize keep their visual size comparable to world axes. Changing calibration moves axes without moving the robot CAD, and actual calibration changes still invalidate paths. Surface recreation and tool switching restore the chosen preference. Select the current XP/XC edge and verify the magenta outward arrow has negative X projection before generating a new path. Perform source/diff checks; build/lint/disconnected runtime remain pending until explicitly requested.

### UI follow-up — Tool windows and understandable simulation controls


Keep the main workspace's right column for Properties. In `qml/Main.qml`, use two persistent nonmodal `Window` objects with `Qt.Tool` and the application as transient parent, following the existing Settings window. The measuring window contains EndEffectorPanel with fixed measuring inputs. The machining window contains the spindle EndEffectorPanel and MachiningPanel side by side in a resizable splitter. Their existing scroll views handle smaller window heights. Closing a window hides it and retains drafts; switching workflow hides the other mode's window. Closing the machining window does not stop playback; the main toolbar retains Stop. Main continues owning the CAD dialog and captures its destination tool at Browse time.

Add a mode-specific setup button to `qml/Views/Workspace/ModeToolBar.qml`, available even before calibration or input selection. Generate Path opens the machining window and focuses its editor. Keep generation prerequisites on Apply. Add Diameter beside Radius in `PropertiesPanel.qml`, calculated from the scene's compensated radius without adding a C++ property.

In `MachiningPanel.qml`, present the existing auxiliary scale as HOME PTP simulation percentage, converting percent to/from its unchanged (0,1] backend representation. Explain that it scales simulated joint speed and acceleration limits, not calibrated controller timing. Keep advanced controls collapsed and label them simulation tuning; explain minimum sampling intervals, TCP acceleration and joint limits. Do not change the numerical timing algorithm or silently discard saved tuning values.

Acceptance: each mode's setup button opens and raises its own window without requiring geometry/TCP readiness; both measuring submodes use the measuring window. A saved path restores HOME percentage correctly; 50 percent maps to 0.5 and Reset restores the saved value. Both panels scroll at the machining window's minimum height. CAD Browse and Retry target the correct tool. Properties remains usable while windows are closed and shows diameter as twice the compensated radius. Source/diff checks are required now; build, lint and disconnected runtime checks remain pending unless explicitly requested.

### Step 1 — Numerical chamfer curve and local frames


Create `src/pathgeneration/chamfer/chamferpath.h/.cpp` in RoboCrapGeometry. Keep cohesive input/result structs in this header rather than distributing constants and structs across additional headers. Inputs contain an ellipse, cylinder axis line, outward-axis sign, and chamfer size. Lead settings are deferred until Step 2. Return a result value with either valid geometry or an explanatory error. No output parameters, exceptions, or OCCT types.

Let k be the outward unit hole axis, r the unit radial direction from the cylinder axis, p the sharp ellipse point, and n the end-plane normal obtained from the ellipse axes. Keep ellipse traversal selected by its normal's sign relative to k. Define u=normalize(r-k*(n dot r)/(n dot k)). This is the direction along the end plane away from the hole in the longitudinal section. The factory rejects parallel-axis planes; normalization failures propagate as an empty optional.

The chamfer endpoints are p+c*u on the actual end plane and p-c*k on the actual cylinder generator. Calculate their midpoint as p+(c/2)*(u-k) to avoid averaging large world coordinates. For zero c the origin is p but orientation remains defined. In a perpendicular opening u=r and this reduces to p+(c/2)*r-(c/2)*k.

Construct local X=normalize(r cross k), local Y=normalize(u+k), local Z=normalize(X cross Y). These are right-handed section axes matching the drawing: local Y points along the diagonal toward the plane, local Z into the hole/toward the opening. X is opposite the positive circumferential direction and need not coincide with the tilted ellipse tangent. Compose BASE_TCP=BASE_LOCAL*Rx(180 degrees); TCP X=X_i, Y=-Y_i, Z=-Z_i, and origins coincide. Validate the local basis and projection signs. Position is independent of this fixed tool rotation.

Acceptance by source review and eventual disconnected visualization: determinant +1, local Z dot k positive, TCP Y dot k negative, TCP Z opposite local Z, periodic seam, and the circular c=2 origin offset (+1 radial, -1 axial). For oblique geometry the endpoints must lie on their respective surfaces and have setback length c. These section axes do not claim a full cutter-contact solution or a differential normal to a swept chamfer surface.

### Step 2 — Continuous lead-in and lead-out


Extend the same chamferpath files. Use s(u)=3u^2-2u^3, angular spans from the model, hL=cL*(1-s), and hO=cO*s. Positions are pM(theta)+h*k. The surface-endpoint midpoint construction stays constant in meaning through the leads. There is no former radial-clearance term.

Construct a clear-end frame with Y=-k, Z=+r, and X=Y cross Z. Interpolate from this frame to the corrected machining TCP frame using unit quaternions and smoothstep, aligning quaternion hemispheres. Both frames share X at each angle, so the interpolation rotates within the longitudinal section, from inward axial Y to inward diagonal Y, preserving Y dot k < 0. Reject any evaluated lead pose violating this sign or finiteness. At lead-in progress 1 and lead-out progress 0 return the exact machining pose. Keep axial displacement h*k, span, traversal and phase ordering unchanged. No P_s is introduced in this correction.

Use N_L and N_O as interval counts, explicitly yielding N+1 requested samples. Refine internally when needed for geometric/orientation interpolation accuracy; never let a small N authorize a visibly faceted or poorly solved path. Sample machining adaptively using the same accuracy criteria. Keep one copy of shared boundary samples, with phase ranges recording the transitions. Preserve the nonduplicated endpoint semantics while still storing the completed revolution's final pose and joint winding.

Acceptance: the clear ends have the requested added axial displacement and axis alignment; lead/machining junctions agree in position and orientation; the first derivatives agree after arc-length timing. No intentional dwell exists at either internal junction. Smoothstep alone does not establish bounded robot acceleration; Step 4 handles timing and checks.

### Step 3 — Robot solutions and direct HOME stages


Create `src/pathgeneration/chamfer/chamfermotion.h/.cpp` in RoboCrapGeometry for KR10 motion compilation. Keep the spatial curve separate from robot-specific joint motion. A robot setup value includes a copy of Kr10KinematicModel, flange-to-TCP transform, and simulation movement limits. Convert every target with T_BASE_FLANGE=T_BASE_TCP*inverse(T_FLANGE_TCP), using existing rigid-transform helpers and conventions.

Solve the first lead-in pose, then every central pose using the previous solution as seed. Preserve existing KR10 tolerances and branch logic. The current solver uses its seed to choose a branch; inspect its singularity behavior before choosing a first-pose seed. Try a small deterministic set of valid branch seeds when HOME's singular wrist seed is unsuitable, without changing HOME itself. Choose a complete continuous valid solution, not a nearest isolated first solution that fails later. If none works, return a phase/location-specific error; do not force joint jumps or silently edit the curve.

Check all joints against limits and FK against each requested pose, including the revolution seam and lead-out. Retain joint winding: a TCP revolution does not imply that its final joints equal the starting joints. Do not wrap joint angles modulo 360 to disguise limit violations.

Add direct synchronized joint-space movement from qHome to the solved first lead-in joints and from final lead-out joints to qHome. These simulate PTP; obtain their displayed TCP paths through FK, never a straight Cartesian connection. HOME itself needs no inverse solve. Keep generation independent of the current displayed pose and do not animate as a side effect of Apply.

Acceptance: the compiled path begins and ends at exact qHome, passes the five stages, retains continuous valid joints, and either solves the entire motion or reports failure without partial insertion. Report the distinction between approximate offline PTP simulation and actual KUKA interpolation.

### Step 4 — Calculate time from speeds


Finish chamfermotion. The user enters speed, not timestamps. Compute arc length from TCP translation only; use rotation and joint changes for independent rate constraints. Use per-stage feed limits for lead-in, machining, and lead-out, with acceleration/deceleration across stage boundaries. Begin and finish the central RSI sequence at rest, with no mandatory stop at its two internal joins. Use an acceleration-limited path time law and validate resulting joint rates/accelerations; refine or reduce the feasible speed instead of claiming requested feed can always be achieved. Report achieved duration and any feed reduction.

Use synchronized smooth joint progress for HOME movements. Because calibrated joint rate limits are absent from the model, add explicitly named simulation speed/acceleration settings in the motion configuration, with units deg/s and deg/s^2. Do not invent manufacturer limits or advertise exact hardware timing. Auxiliary speed is a scale of this simulation profile, not mm/s along a straight line. Store effective settings with the generated trajectory.

Return immutable motion data containing input snapshots, phase boundaries, sample times, TCP transforms, and continuous joint positions. Avoid duplicating full robot link transforms at every sample; evaluate FK for displayed joints. Bound joint interpolation error by refining the stored samples and verifying intermediate FK deviation from the intended curve. Handle orientation through matrices/quaternions internally; XYZABC remains a boundary representation in degrees and Z-Y-X order.

Acceptance: timestamps increase, durations reflect feed and acceleration, central junctions have continuous speed, endpoints are at rest, and playback is independent of display refresh rate. The RSI packet interval is not the display timer interval and is not needed as a user parameter for animation.

### Step 5 — Scene ownership and transactional Apply


Add `src/scene/scenemachining.h/.cpp` containing SceneMachining, a small generation/playback coordinator. Add SceneMachiningPath, a read-only view of shared immutable motion data, alongside existing geometry wrappers in `src/scene/scenegeometry.h`. Extend SceneObject with typed machining-path access and a named setter, and SceneModel with insertion using its existing IDs and notifications. Do not introduce a second object collection or global SceneState.

Own SceneMachining in `src/main.cpp` before the QML engine and register it through `src/backendqmltypes.h` as Backend.Machining, matching current singleton ownership. Supply a copy of the loaded numerical robot model from OccController to SceneMachining through a C++ setter when robot loading succeeds. Scene never includes OCCT/controller headers. OccController observes SceneMachining; dependency direction remains viewport to Scene. Invalidate readiness on robot detach/reload.

SceneMachining resolves one selected intersection edge and its fitted source plane/cylinder. If those source objects were deleted, report that a new path cannot be generated from that edge; preserve the edge itself. A successful trajectory retains the needed numerical snapshot and survives later source deletion. Snapshot TCP calibration, robot setup, flip state, and all motion settings too.

Generate everything into a temporary result. Apply adds one complete path only on success; failure leaves existing objects untouched. For a new edge Apply creates a path. To revise a selected existing path, build its replacement first, then replace through existing targeted remove/add operations and select the new ID. This avoids expanding every immutable geometry notification API for this feature. Existing source snapshots permit regeneration after source deletion.

Changing drafts does not mutate generated data. TCP or robot calibration changes mark existing playback incompatible until regenerated. Preserve the trajectory display and show the reason. No per-frame QObject insertion or QML transfer of full joint arrays.

Acceptance: failed Apply adds nothing, successful Apply adds exactly one MachiningPath, source deletion leaves generated paths usable, and changed calibration cannot silently replay an old trajectory with a new tool frame.

### Step 6 — OCCT trajectory display and axis preview


Extend `src/3d/adapters/occsceneadapter.*` and only the necessary OccScene operations to display the five phase polylines under one scene ID. Use distinct readable styles for HOME movements, leads, and machining, reusing project colors where suitable. All phase presentations pick the same trajectory and follow its visibility and deletion. Generalize the adapter's current one-surface presentation record only as much as multiple phase parts require; do not add another renderer.

Add a viewport-only outward-axis arrow for the selected generation input. It updates immediately when Flip direction changes, does not allocate a scene-tree object, and does not interfere with picking. Give it bounded size and keep it from disturbing camera fitting. Optional sparse local frame display is a development verification aid, not thousands of persistent scene objects.

Show Path combines the global machining-path display preference with each object's visibility; it does not overwrite individual visibility. Preserve presentation restoration on viewport recreation and existing point/normal overlays.

Acceptance: all five stages are visible, HOME TCP curves reflect FK, selecting any segment selects one tree item, hiding/deleting removes all its segments, and Flip reverses the arrow without altering measured geometry.

### Step 7 — Parameter editor and UI workflow


Create `qml/Views/Workspace/MachiningPanel.qml`, with explicit typed inputs and signals. Place it beside the spindle EndEffectorPanel in the nonmodal Machining Setup window, opened from the machining toolbar. The Measuring EE panel has a separate window opened from either measuring toolbar. Keep Properties in the main right column. Reuse Components controls after reading their definitions, and Styles metrics. Add it to the root application QML source list and the appropriate hot-reload inputs if required by the existing packaging.

Expose chamfer size in mm; an explanation of equal surface setbacks (45 degrees for perpendicular openings); Flip direction; lead-in/out axial clearance in mm; lead-in/out angular spans in degrees; lead-in/out interval counts in an advanced section; lead-in/out speeds and machining feed in mm/s; and HOME PTP simulation percentage, converting to the existing auxiliary scale for the backend. Allow independent lead settings but initialize them symmetrically. Keep acceleration/rate simulation configuration in an advanced section with explicit units. Numerical defaults are simulation defaults, not hardware recommendations; document chosen values during implementation. Reference the existing TCP configuration rather than duplicate six fields.

Wire Generate Path in `ModeToolBar.qml` to open/focus the editor for one eligible edge. Apply requests SceneMachining generation; select and reveal the resulting path. Selecting a path displays its saved parameters for regeneration. Show errors in the panel, such as missing TCP or unsolved pose with phase and progress. Derive availability from valid selection, robot readiness, calibration, and playback state.

Wire Dry Run and Stop, display elapsed/total time and phase, and retain Start Machining as inactive. Do not add pause, scrub, export, or speed-multiplier controls in the initial scope. `qml/Main.qml` coordinates these components; it contains no fitting, IK, or per-frame timing calculations.

Acceptance: the complete select-edge -> parameters -> Apply -> selected tree path workflow works at narrow and wide panel sizes without binding warnings. The chamfer-size label never says normal offset or allowance.

### Step 8 — Playback using elapsed time


SceneMachining owns a GUI-thread QTimer and monotonic elapsed clock. The timer schedules display updates; it does not advance simulation by a fixed amount per callback. At each callback evaluate stored motion at actual elapsed time, including interpolation between samples, and emit the requested joint pose by value. Connect this in C++ to a narrow OccController operation that validates and applies it through RobotPreviewState::forward and existing applyPose. Keep one robot presentation authority; do not move OCCT objects from QML or Scene.

Dry Run starts the visual simulation at its stored HOME pose and time zero. Clearly label this preview reset; it is not an additional physical approach movement. Normal completion ends at HOME. Stop freezes the current preview pose and ends playback; pressing Dry Run again restarts at HOME. Disconnect or guard conflicting manual FK/IK controls while playing.

Stop playback on active-path deletion, leaving machining mode, incompatible calibration changes, robot reload, viewport loss, or application shutdown. Report any pose-application failure and stop rather than letting the time indicator continue while the robot freezes. Selecting unrelated objects need not stop playback; it must not silently change the active path. No device invoke or network operation is part of any playback action.

Acceptance: wall-clock duration matches the displayed duration within one display interval, slower rendering skips display frames rather than slowing motion, SEE follows the flange throughout, Stop freezes immediately, restart begins at HOME, and all five phases execute in order.

### Step 9 — Integration review and verification


Update owning CMake lists, backend foreign registration, UI wiring, and factual AGENTS.md descriptions once implementation exists. Keep all existing module URIs and minimum versions. Avoid making creatable QML types final because Qt may derive registration wrappers from them. Inspect changed callers and preserve unrelated working-tree changes.

No test source, test targets, or harnesses are to be added. Build or QML lint only when explicitly authorized. Manual disconnected checks cover circular and slightly elliptical inputs, Flip in both directions, a 2 mm chamfer, zero-size diagnostic if supported, invalid values, missing source/calibration, IK failure, full-turn continuity, tree operations, repeated Apply, Stop/restart, mode switching, viewport recreation, and calibration invalidation. No live hardware connection is authorized by this plan.

## Interfaces and Dependencies


Use ChamferPathParameters for cohesive geometry settings, ChamferPath for curve/lead evaluation, and ChamferPathResult for geometry plus error. Use ChamferRobotSetup for numerical model/calibration/rate limits, ChamferMotion for the immutable timed result, and ChamferMotionResult for success/error. Keep related values in chamferpath.h and chamfermotion.h. Prefer factory/generation functions with one or two cohesive inputs. APIs must obey four-parameter maximum and return-value-only results.

SceneMachining exposes generation availability/error, draft settings or a typed cohesive settings object, active path ID, playing state, current phase, elapsed time, duration, and Apply/Dry Run/Stop actions. SceneMachiningPath exposes saved parameters and summary data rather than full arrays to QML. Generation and playback stay synchronous on the GUI thread initially; no event pumping or new worker is planned. Revisit only if measured generation latency warrants it.

The planned new implementation files are the two numerical .h/.cpp pairs, one Scene .h/.cpp pair, and one QML panel. Existing scenegeometry.h holds the read-only wrapper. Add no generic task framework, new device abstraction, duplicate robot loader, or general scene-state service.

## Concrete Steps


Work from `E:\Qt\QtProjects\RoboCrap`. Before each implementation milestone inspect its named files and affected callers, including component base definitions for QML. Read AGENTS.md. Update this plan's progress and discoveries after each milestone. Git staging, commits, branch changes, and destructive operations are not authorized.

After edits run:

    git diff --check
    git status --short

Read new files separately because ordinary git diff omits their contents. If build verification is explicitly requested, first inspect the chosen build's CMakeCache.txt for CMAKE_HOME_DIRECTORY, Qt paths, and compiler. Use the matching installed kit and hot reload OFF for application packaging. With PowerShell variable $buildDir set to that verified directory, run:

    cmake --build "$buildDir" --target robocrap --parallel
    cmake --build "$buildDir" --target robocrap_qmllint

Verify target availability first. Do not reconfigure an existing user build to another kit or mode. A separate compatible build may be configured only under the user-authorized build workflow. In a disconnected runtime, execute the acceptance scenarios above and record actual observations; compilation alone cannot establish geometry or animation correctness.

## Validation and Acceptance


The final observable result is one selected, rendered machining trajectory and a robot animation that starts at HOME, approaches without intermediate waypoints, leads into one circuit, leads out, and returns HOME. Timing derives from specified speeds and documented simulation limits. The user sees an outward-axis arrow that can be flipped, a chamfer size expressed as the drawing's leg dimension, and useful generation failures without partial scene changes. All geometry calculations remain outside QML/OCCT and all device execution remains untouched.

For the ideal circular case, inspect the 2 mm input: the nominal machining TCP is shifted by +1 mm in radial material direction and -1 mm along the outward axis, not by 2 mm along the normal. Inspect a nearly circular ellipse at multiple quadrants and the seam: axes stay orthogonal, the chamfer normal points inward/outward-opening, and animation has no configuration jumps. For the tilted plane inspect equal setbacks on both actual surfaces, midpoint placement and the varying longitudinal-section angle; do not claim a constant 45-degree tilt.

## Idempotence and Recovery


Each generation builds an immutable candidate before scene mutation. Failed generation preserves previous paths. Regeneration replaces only the selected path after success. Playback never mutates the saved plan; stopping requires no rollback. Maintain source provenance and numerical snapshots separately. Preserve all unrelated user edits, including existing untracked files. No assets, caches, or build directories are deleted by this work.

## Surprises & Discoveries

Existing frame display depended on committed robot FK and calibration. Target diagnostics must be separate presentations derived from ChamferPath so an IK failure does not prevent geometry inspection. The old outward-arrow cache is insufficient for frames because it ignores chamfer/lead parameters; frames use an independent refresh path.


Transfer/lead joins are geometric corners. Extending the former central timing without zero-speed boundaries would attempt to traverse them at nonzero speed. Both the feed pass and joint cubic slope calculation now enforce stops there.


The ellipse center already represents O_H; using the cylinder's finite fitted midpoint would measure staging distance from the wrong reference. Existing clear lead endpoint poses supply the required staging attitudes without introducing a second rotation convention.


The latest drawing distinguishes the local frame from the tool frame: equal origins do not imply equal orientations. The earlier TCP Y sign was correct in the circular case, but TCP Z and X were reversed. The old clear-end Z=-r was also reversed. The old nominal midpoint did not generally lie halfway between endpoints on both actual tilted surfaces.


The current XC file has seven points, with the earlier suspect point removed by the user. An in-memory NumPy reproduction of the existing fitting steps gives raw radius 5.9327153 mm, radial RMS 0.0162268 mm below the 0.1186543 mm tolerance, and constraint ratio 0.293958 above 0.01. With +1.5 mm compensation and the current three-point XP plane (zero compensation), ellipse radii are approximately 7.4344348 and 7.4327153 mm. Choosing the negative-X direction gives outward axis (-0.9998913, -0.0128776, -0.0071808). This is offline numerical evidence, not a compiled application check. No sample files were changed by this follow-up.

The UI review confirmed that auxiliaryScale scales both speed and acceleration in MotionTimer::timePtp, rather than simply scaling playback time. The model stores no calibrated rate limits. Setup access therefore must remain independent of canGenerate, which requires TCP calibration that users need the setup window to enter.

Step 5 registers Backend.Machining (application-owned singleton), the machiningSettings value type and uncreatable MachiningPath geometry. SceneMachining's inputId selects either an existing intersection edge or a saved path; settings is a typed value draft, canGenerate/unavailableReason describe prerequisites, apply() returns a new scene ID or zero and generated(id) enables future UI selection. Playback properties/actions remain Step 8 rather than adding inert APIs now. Typed settings and the read-only wrapper share scenegeometry.h; numerical arrays remain C++ only. SceneModel inserts only non-null immutable motions using its existing ID allocation and targeted signals. Ordinary additions/removals are handled individually; full compatibility refresh occurs only when global model/TCP readiness changes.

Regeneration inserts a complete replacement before deleting the old object, preserving its name and visibility; IDs are deliberately new. Saved geometry parameters allow regeneration after source deletion. Initial edge generation requires both live fitted sources. Model updates and TCP edits increment a calibration revision and mark existing paths incompatible until regeneration; transient viewport loss temporarily disables compatibility without changing the revision. The coordinator receives numerical model copies from OccController and observes no OCCT types. SceneModel and SceneEndEffectors outlive it in main.cpp, and it outlives the viewport controller and QML engine. Source geometry/calibration and generated arrays never pass through QML.

Step 4 stores timestamps and shared joint velocities on the existing points. Central timing starts with forward/backward acceleration-limited speeds using TCP chord lengths only. Monotone cubic joint interpolation preserves joint ranges and continuous velocity, including internal phase boundaries, but acceleration is piecewise continuous and may jump. Its joint-speed bound uses the quadratic Bezier control hull; joint-acceleration extrema occur at interval endpoints. Cartesian speed/acceleration are finite-difference estimates at 17 positions per interval, including both sides of junctions, with 5 percent time margin. They are numerical checks, not an analytic whole-curve guarantee. A uniform central time stretch enforces the calculated limits and is exposed as centralTimeScale; this is conservative and can slow all central phases for one difficult segment. Interpolation is checked against the exact phase curve at those positions with 0.01 mm and 0.1-degree tolerances; up to three whole-curve subdivision retries are made before rejecting a candidate. Projection onto the local TCP chord estimates the corresponding exact-curve parameter. This is suitable for offline preview, not certified controller execution.

Simulation defaults are 10/5/10 mm/s lead-in/machining/lead-out, 20 mm/s^2 Cartesian acceleration, 30 deg/s and 60 deg/s^2 on each joint, and auxiliary scale 1. These are stored with the model/calibration snapshot and are not manufacturer ratings. HOME timing uses quintic progress with analytic peak factors 1.875 and 10/sqrt(3). A coincident HOME phase retains a 1 ms stationary interval. evaluate(seconds) clamps finite times to the motion bounds, uses exact HOME endpoints, evaluates central cubic joints or whole-phase HOME progress, and computes TCP by FK. No timer, UI or device execution is introduced in this step.

Step 3 leaves Kr10Kinematics and interactive viewport APIs unchanged. Its seed chooses shoulder/elbow/wrist branches; a singular target is rejected, whereas a singular HOME seed can select a valid nonsingular target branch. The compiler tries HOME plus 18 valid interior arm/wrist seeds and deduplicates their first-pose solutions, then probes alternative initial A4/A6 turns. This is a bounded search, not an exhaustive reachability proof. It accepts the first candidate that completes the entire path. Curve intervals exceeding 5 degrees of joint change are subdivided up to depth 12, with a 200000-point central-path limit; unresolved jumps fail rather than wrapping. Every solved flange is independently checked against FK using the existing tolerances. HOME moves share synchronized joint progress and have at most 2-degree sample spacing, with TCP samples from FK. Intermediate interpolation error and rate/acceleration checks remain Step 4; discrete IK checks do not establish collision clearance or continuous singularity avoidance.

Step 2 supplies evaluatePhase(phase, progress) and sample(settings). Lead spans are ellipse-parameter degrees, not cylinder azimuth. Defaults are 5 mm clearance, 30-degree span and 16 minimum intervals per lead; they are editable numerical defaults, not hardware recommendations. Spans are restricted to (0, 360] and intervals to 1..100000. The sampler checks quarter/mid/three-quarter interpolation errors, limits rotation steps to 5 degrees, and recursively refines with a depth limit of 20 and total-point limit of 200000. It returns no partial result on failure. This is a numerical refinement criterion, not a proof of a global analytic error bound. Samples store matrices, avoiding quaternion sign exposure to consumers. Four inclusive indices delimit the three phases; internal boundaries share one point while the completed revolution retains its own endpoint. Future consumers must interpret a shared starting point as local progress zero for the next phase.

Step 1 review corrected the circular Y-axis explanation: with x=t and z=(k-r)/sqrt(2), z cross x is -(k+r)/sqrt(2), not +(k+r)/sqrt(2). This matches the downward burr-axis arrow in the section drawing. Step 2 must therefore account for this axis direction when choosing continuous lead orientation; do not assume that undoing a 45-degree rotation yields +k.

The factory verifies the whole ellipse by projecting its center and both harmonic coefficients onto the plane perpendicular to the cylinder axis. A centered circular projection establishes cylinder compatibility without a sample grid. Evaluation uses local offsets for radial directions to avoid cancellation at large world coordinates. evaluate(angleRad) returns an optional sample containing edge point, radial normal and BASE-relative TCP transform; radians are ellipse parameter, not arc length or cylinder azimuth.

The corrected source document still calls a a normal allowance; the user's later drawing explicitly supersedes that input definition with chamfer leg size. The nominal c/sqrt(2) conversion and inward normal direction are therefore required corrections.

The original circular lead rotation cannot align the burr to the true hole axis for an oblique ellipse; full orientation interpolation is needed. Also, hole-frame X/Y cannot both lie in an oblique end plane while remaining perpendicular to a cylinder-axis Z. Use a transverse hole frame and represent the measured ellipse in BASE coordinates.

Before this plan, MachiningPath was only an enum and tree category; Step 5 added immutable motion data. Edge geometry retains only source IDs, so generation needs source geometry before making a self-contained snapshot. Kr10KinematicModel stores position limits but no calibrated rate limits; do not claim exact KUKA timing from currently available data.

## Decision Log

2026-09-29: Preview one selected local or TCP frame at a time alongside H and P_s; use phase/progress controls instead of cluttering the small hole with many overlapping trihedra. Keep preview state separate from saved motion and explicitly distinguish local lead reference orientation from rotating TCP orientation.


2026-09-29: Add two named Cartesian phases and grow boundary arrays to eight entries. Reuse lead feeds and the existing IK/timing checks. Stop at transfer corners without adding dwell time or altering continuous machining joins. Keep the temporary joint-position-limit bypass unchanged.


2026-09-29: Implement only requested follow-up step 4. Store an independent staging distance with default 10 mm and reuse clear lead attitudes; defer motion integration to step 5. The default is an editable geometric distance, not verified tool clearance.


2026-09-29: Implement the approved correction steps 1–3 using explicit local-frame composition and equal surface setbacks in longitudinal sections. This supersedes the earlier fixed-45-degree skew-plane approximation and tangent-based frame. Update the UI explanation accordingly. Keep the five stages, calibration, timing APIs, and temporary position-limit bypass unchanged; P_s and restoring limits belong to later steps.


2026-09-29: Use the existing world-axis presentation for the spindle TCP, composed in OccRobotAdapter from flange and calibration. Keep visibility as an OccController display preference; window restoration stores numerical frame data. Repeated Apply shows axes but unchanged calibration preserves compatibility. Default new edges to negative scene X for the explicitly identified workpiece; keep saved-path direction and manual Flip. The TCP local Z is not the hole axis.

2026-09-29: Use persistent nonmodal tool windows for the two requested setups. Keep the numerical HOME profile unchanged and convert only its display to percent, explicitly describing its simulation meaning. Retain saved tuning fields behind a collapsed Simulation tuning control: acceleration and joint limits affect timing, while minimum intervals seed adaptive sampling. No calibrated controller rates are available to justify replacing them with KUKA timing.

2026-09-29: Record the user's latest chamfer-size interpretation as authoritative over the attached model's a parameter. Fix internal normal direction according to the section drawing. Keep 45 degrees, ellipse support, axial clearance, Flip, deterministic start/direction, and direct HOME approach as agreed.

2026-09-29: Choose a nominal 45-degree construction in the cylinder-normal/projected-axis section for the slightly oblique ellipse. Exact equal legs on both skew measured surfaces are not implied. This makes the approximation explicit rather than silently changing tilt or measured geometry.

2026-09-29: Propose minimal simulation controls and no additional worker, devices, or global SceneState. Store immutable generation snapshots and use existing scene identity, registration, and presentation mechanisms. These are implementation proposals, not claims of completed code.

## Outcomes & Retrospective

Follow-up step 6 adds geometry-only frame inspection through the existing scene/viewport layers. It does not establish visual correctness until a disconnected runtime review, nor does it change IK limits or timing. Source and whitespace checks completed; no build, lint, tests or hardware actions performed.


Follow-up step 5 integrates P_s into generation, timing, rendering and playback. The approach search now targets P_s, and the return remains on the continuous solved branch until P_s before HOME. Source/diff checks completed; compilation, runtime animation and physical clearance remain unverified. Earlier milestone descriptions below record historical states.


Follow-up step 4 adds the numerical staging position/poses and UI-to-snapshot settings round trip. Existing Dry Run does not yet pass through P_s. Source review and whitespace checks completed; build, QML lint and runtime not requested and not run. No test logic added.


Drawing correction steps 1–3 are implemented in chamferpath.cpp/.h with the existing return APIs. Source review covers the surface endpoint identities, midpoint, right-handed frame and negative TCP Y projection through the lead interpolation. No tests/harnesses, builds, QML lint or runtime were run under the current instructions. After an authorized build, regenerate the XP/XC path with Apply before judging the animation; existing immutable motion snapshots are not rewritten. Earlier milestone entries below describe historical implementations and are superseded by the current Step 1/2 definitions where they differ.


The TCP follow-up adds a Visible checkbox and repeatable Apply TCP. Applied calibration is rendered as RGB axes at the moving flange-relative TCP, with the existing 5-percent camera-scale length and exclusion from fit/picking. Mode changes hide/restore the spindle frame without losing visibility preference, and surface recreation restores the frame before robot attachment. New edge drafts initialize the outward axis toward negative scene X; immutable saved paths remain unchanged. No C++ numerical APIs were replaced, no hardware actions performed, and no tests/builds added or run. Source/diff checks and an offline fit reproduction passed; UI/OCCT runtime behavior remains to be verified in the authorized integration step.

The UI follow-up adds toolbar-opened Measuring EE and Machining Setup windows, leaving Properties full-height. The machining window keeps spindle calibration and the chamfer editor side by side. Generate Path opens and focuses that editor; setup access does not require a ready robot, selected edge or TCP. Cylinder diameter uses compensated radius. HOME percentage converts to/from the existing auxiliary scale, preserving saved motion meaning. Simulation tuning stays collapsed and its purpose is explained. `CHAMFER_ARCHITECTURE_REVIEW.md` records the layer boundaries and outstanding validation/design limits. Source and diff inspection completed; no build, lint, runtime or hardware execution was performed.

Step 6 extends OccSceneAdapter's presentation record to a vector of shape parts per scene ID. HOME approach/return are purple, leads blue, and machining green with a thicker line; stationary phases have no line. All phase parts participate in one object's picking, selection, visibility and removal. Show Path binds to OccController.showMachiningPaths and combines with each object's own visibility; its initial setting is installed before camera fitting. OccViewWindow retains that preference, selection and preview parameters across surface recreation. The magenta outward-axis arrow is a bounded 5–40 mm decoration at the ellipse center, non-selectable and marked infinite for exclusion from fit bounds; infinite decorations do not trigger world-axis display. Repeated identical preview updates reuse the presentation. SceneMachining.previewParameters supplies numerical input; its future settings panel/input selection wiring is Step 7. The renderer responds to existing input/settings notifications and hides the preview when the input object is hidden or its source is lost. No UI parameter editor or playback was added. Source/whitespace checks passed; build and visual runtime checks were not requested or run.

Step 5 is complete at the C++/registration level. main.cpp owns SceneMachining, OccController supplies the successfully loaded model and readiness, and Apply produces scene-owned timed paths transactionally. No rendering, parameter-panel wiring or playback timer was added; those are Steps 6–8. Existing user IDE edits were preserved. New QML registrations have been source-reviewed but not compiled or exercised, because build/runtime verification was not requested.

Step 4 completes numerical timing and pose evaluation. ChamferMotion::create now returns timed data while preserving its two-input API; timing settings are part of ChamferRobotSetup. duration(), centralTimeScale() and evaluate(seconds) support future Scene playback. No standalone tests/builds were run under AGENTS.md. Runtime timing, geometry and performance remain unverified until authorized integration checks. Earlier milestone descriptions below record their state when completed; timing/interpolation work formerly deferred to Step 4 is now implemented as described above.

Step 3 adds chamfermotion.h/.cpp in RoboCrapGeometry. ChamferMotion owns read-only geometric inputs, robot/calibration snapshot, untimed joint/TCP points and six inclusive boundary indices for the five stages. Factory failure returns no motion. Each shared boundary retains the preceding phase's progress value; consumers treat it as zero at the next phase's start. Simulation rate settings are deferred until Step 4 so unused fields are not added now. No scene, UI, timing, hardware or interactive IK code was changed. Source/whitespace review completed; compilation and runtime correctness remain unverified because builds were not requested.

Step 2 adds lead evaluation and complete central-path sampling in the same numerical files. Exact junction poses reuse machining evaluation. Smoothstep's zero endpoint derivative makes lead clearance and relative-rotation derivatives vanish at the machining joins; common arc-length timing and acceleration checks remain Step 4. No UI, robot playback or device behavior is connected yet. Build/runtime validation remains pending under the repository's explicit-request rule.

Step 1 is implemented in chamferpath.h/.cpp and registered with RoboCrapGeometry. It provides validated numerical geometry and a machining-pose evaluator, including zero-size diagnostic geometry and flipped-axis traversal. No UI, scene insertion, lead, timing, IK, or device behavior was changed. Static review covered the circular 2 mm reduction, axis handedness, projection signs, whole-ellipse compatibility, finite inputs and seam periodicity. No build or runtime check was run because it was not requested; no test harness was added. Remaining milestones and runtime validation are pending.

## Artifacts and Notes


Source references reviewed in the conversation were the user's chamfer_trajectory_math_model_axes_corrected.md, the internal-chamfer section drawing, and the final 2 x 45-degree dimension drawing. The essential corrected equations and definitions are embedded above so this plan does not depend on Desktop attachments. Earlier discussion of KUKA manuals established the KRL/RSI stage split; this offline plan deliberately does not generate controller programs or assume an RSI correction frame.

Revision note, 2026-09-29: Initial plan created after the final chamfer-size correction; supersedes earlier suggestions of a radial/normal-offset UI field and KRL lead motions.

Revision note, 2026-09-29: Step 1 implemented. Corrected the explanatory Y-axis sign, deferred unused lead settings, and recorded validation/API details. No later milestone is claimed complete.

Revision note, 2026-09-29: Step 2 implemented. Corrected clear-end Y to -k to preserve the internal-frame convention and 45-degree circular transition; added settings, continuous phase evaluation, shared boundaries and bounded adaptive sampling.

Revision note, 2026-09-29: Step 3 implemented with an untimed immutable motion result, existing solver reuse, bounded seed/turn search and joint-space HOME stages. Documented the search and discrete-check limits; timing and interpolation verification remain Step 4.

Revision note, 2026-09-29: Step 4 adds explicit simulation limits, time parameterization, continuous joint interpolation, bounded refinement and elapsed-time evaluation. Documented conservative global slowdown and numerical Cartesian-check limitations. Scene/UI playback remains outside this step.

Revision note, 2026-09-29: Step 5 adds scene ownership, typed settings, generation/regeneration, provenance and calibration compatibility. Playback state is deferred to its actual implementation rather than exposing placeholders.

Revision note, 2026-09-29: Step 6 adds phase rendering, global visibility binding and an outward-axis preview with restoration. Internal OccViewport construction now receives initial path visibility so hidden paths do not affect initial fitting; its sole caller was updated.

Revision note, 2026-09-29: Step 7 adds MachiningPanel with local text drafts, finite/range/integer validation, fixed 45-degree angle, axial clearances, feed/HOME scaling, and advanced sampling/acceleration/joint simulation limits. It references existing TCP calibration. Main supplies one selected edge/path only in machining mode; Flip immediately updates the axis preview. Apply calls the existing coordinator and selects the generated result with Show Path enabled. Saved-path selection restores its parameters and displays duration/time scale. Generate Path focuses the first parameter. Dry Run, Stop and elapsed/phase display depend on Step 8 playback APIs and remain deferred; Start Machining remains inactive. Application packaging includes the new file. No numerical, device or robot-motion code changed. Build, QML lint and runtime checks were not requested and were not run.

Revision note, 2026-09-29: Step 8 uses a 16 ms precise GUI timer solely to schedule display updates; QElapsedTimer supplies absolute time to ChamferMotion.evaluate. A shared immutable active-motion snapshot and separate path ID keep selection independent of playback. Direct C++ signal delivery applies validated joints through RobotPreviewState.forward and OccController.applyPose; failure stops playback before publishing elapsed time. Dry Run presents HOME before starting the clock, Stop retains the last committed pose/time, and completion explicitly samples the endpoint. Main stops playback before switching tools out of machining mode. Calibration/model changes, readiness loss, active-path removal and application shutdown also stop it. Generation and manual FK/IK are guarded while playing. The panel explains the HOME reset and shows phase/elapsed/duration; Start Machining stays inactive. Existing preview-state error-output APIs are reused unchanged. No hardware calls, tests or unsolicited builds/runtime were added.

Revision note, 2026-09-29: UI follow-up moves setup panels into mode-specific tool windows, adds cylinder diameter and clarifies HOME percentage and simulation tuning. Updated current UI instructions and working directory; Step 9 remains pending because integration execution was not requested.

Revision note, 2026-09-29: Add calibrated spindle TCP axes with visibility and zoom scaling, permit repeated Apply TCP, and establish the current workpiece's negative-X opening convention. Record the current seven-point XC numerical fit and keep application verification pending.

Revision note, 2026-09-29: At the user's request, enable temporary IgnorePreviewJointPositionLimits in src/3d/robot/model/kr10kinematicmodel.h. Analytical IK chooses the nearest winding without min/max clamping; chamfer generation/evaluation and RobotPreviewState FK skip position bounds while retaining finite validation. Original model limits still seed the bounded branch search. The bypass includes manual robot preview, does not affect device control, and is announced through the existing viewport log. Set the constant false and rebuild to restore all three enforcement points. Speeds, accelerations, singularity rejection, FK accuracy and continuity/refinement checks remain active; this does not guarantee a complete path. Source/diff checks only; no build or runtime verification requested.

Revision note, 2026-09-29: Correct local/TCP orientation according to the latest drawing, replace the nominal oblique offset by the midpoint of equal surface setbacks, and change clear-end TCP Z to +r. Updated current geometry definitions and UI wording; retained historical decision entries for context. P_s remains deferred, and integration verification remains pending.

Revision note, 2026-09-29: Implement follow-up step 4 with positive staging distance, shared axial position and separate lead-matching attitudes. Preserve five-stage playback until requested step 5; document this boundary in the panel.

Revision note, 2026-09-29: Implement requested follow-up step 5, extending all phase consumers to the seven-stage P_s sequence and introducing stops at straight-transfer corners. No hardware actions, test harnesses or unsolicited builds.

Revision note, 2026-09-29: Implement step 6 target-frame diagnostics independent of IK with labelled camera-scaled axes, phase/progress and local/TCP selection, and a separate draft-preview action. Runtime validation remains pending.

Revision note, 2026-09-29: Rechecked step 6 lifecycle wiring (input removal/visibility, mode input clearing and surface restoration). Preview now clears stale frames on missing input and reports failure if a selected or staging frame cannot be evaluated. Source/diff review only; build and visual verification remain outstanding.
