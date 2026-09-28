# Generate and animate an internal-hole chamfer

This is a living ExecPlan maintained according to `PLANS.md`. `AGENTS.md` takes precedence where it prohibits tests, unsolicited builds, or Git mutations. This document records a plan only; implementation has not started.

## Purpose / Big Picture


The user selects a plane-cylinder intersection edge, enters chamfer and movement parameters, and presses Apply. A complete five-stage trajectory appears under Machining Paths and in the viewport. Selecting it and pressing Dry Run animates the KR10 and its spindle end effector according to calculated elapsed time. The simulation never sends commands to PLC, RSI, or FTS.

The stages are HOME to lead-in start, lead-in, machining around one complete hole edge, lead-out, and return HOME. The first and last stages represent direct KRL PTP movements without intermediate waypoints. The three central stages represent one continuous RSI motion. Actual KRL generation, RSI streaming, force compensation, material removal simulation, collision checking, persistence across application restarts, and exact KUKA controller emulation are outside this implementation.

## Progress


- [x] (2026-09-29) Review the conversation, corrected mathematical model, scene interfaces, toolbar, numerical KR10 interfaces, and build ownership; write this plan.
- [ ] Step 1: Implement the corrected internal-chamfer geometry and elliptical moving frames.
- [ ] Step 2: Assemble lead-in, machining, and lead-out with continuous pose and boundary tangents.
- [ ] Step 3: Solve the robot path and add direct HOME movements.
- [ ] Step 4: Calculate motion timing and produce immutable playback data.
- [ ] Step 5: Add scene ownership and transactional generation.
- [ ] Step 6: Render trajectories and the outward-axis arrow.
- [ ] Step 7: Add the parameter editor and Apply workflow.
- [ ] Step 8: Implement time-based Dry Run and lifecycle handling.
- [ ] Step 9: Review integration and perform authorized verification.

## Decisions and agreed meanings


The operation is an internal chamfer. In the user's section drawing the hole is on the left and material is on the right. The outward hole axis points through the opening into free space. A visible arrow and Flip direction switch establish its sign; fitted geometry alone cannot establish outside. The local chamfer normal points into the hole and toward the opening, with strictly positive projection on the outward hole axis.

The TOOL frame is rigidly attached to the cylindrical burr's cutting surface. Its Y axis is the burr axis, X is tangential, and Z is the surface normal. The existing flange-relative XYZABC calibration places this frame. It is never relocated on the CAD to put it on the workpiece. The trajectory prescribes where that fixed frame moves in robot BASE.

The user enters chamfer size c in mm, meaning the leg dimension in a callout such as 2 x 45 degrees. It is not sloping face width, a radial TCP adjustment, or a normal offset. The tilt magnitude is fixed at 45 degrees. For an ideal perpendicular opening, a 2 mm chamfer has 2 mm radial and axial legs. The derived shift from the sharp corner to the middle of its straight chamfer section is 2/sqrt(2) mm opposite the outward chamfer normal. Do not expose the old mathematical allowance a as another field.

Clearance is an additional displacement along the outward hole axis from the machining TCP path, as in the corrected model. It is not guaranteed to equal perpendicular distance from the original end plane, especially for an oblique ellipse or nonzero chamfer size. Positive clearance goes toward the opening. Lead-in starts with the burr axis parallel to the hole axis and ends in machining orientation; lead-out reverses that progression.

Use the actual measured intersection ellipse, with a circle as a natural special case. Choose a deterministic start and traversal direction; do not add UI fields for them. No intermediate HOME waypoints. The existing robot model's HOME joint pose is authoritative for simulation. Scene coordinates are treated as the existing robot BASE coordinates; adding workpiece registration is outside scope.

## Context and Orientation


`src/CMakeLists.txt` owns RoboCrapGeometry, RoboCrapScene, RoboCrapDevices, and backend foreign registration. Geometry already compiles `src/3d/robot/kinematics/kr10kinematics.*` despite that directory name. Do not move it as part of this feature. Scene depends on Geometry; no new Scene-to-viewport dependency is allowed.

`src/geometry/surfaceintersection.*` defines immutable EdgeGeometry with ellipse center, axes, and radii. `src/scene/scenegeometry.h` contains SceneEdgeGeometry with immutable geometry and source IDs. `src/scene/scenemodel.*` creates intersections and emits targeted addition/removal notifications. `src/scene/sceneobject.*` already has Kind::MachiningPath, and `qml/Views/Workspace/ScenePanel.qml` already groups it into Machining Paths. An edge does not contain the source cylinder axis, so generation must resolve its source plane and cylinder and copy the necessary numerical values. Source deletion currently does not cascade to edges; preserve that behavior.

`src/scene/sceneendeffectors.*` owns spindle TCP calibration. `src/3d/robot/robotpreviewstate.*` owns the loaded robot model and committed preview pose. `src/3d/occcontroller.*` applies poses and exposes solveFK/solveIK/solveTcpIK. Its interactive IK methods also change the displayed robot: do not use them in the offline generation loop. Use Kr10Kinematics directly with an explicit preceding joint seed.

`src/3d/adapters/occsceneadapter.*` maps scene IDs to presentations. `src/3d/adapters/occrobotadapter.*` moves the robot and attached tools. `qml/Main.qml` coordinates selection, mode, and viewport. `qml/Views/Workspace/ModeToolBar.qml` has unwired Generate Path, Dry Run, and Stop controls. Start Machining must stay inactive.

The existing `src/pathgeneration/rsi/rsipath.*` is an offset generator, not a suitable trajectory model: its six-coordinate length mixes mm and degrees. Do not change or call its device execution path for this feature. The current kinematic model has joint position limits but no calibrated joint speed/acceleration limits. Timing must identify its configured simulation limits rather than pretend these are controller limits.

## Plan of Work


### Step 1 — Numerical chamfer curve and local frames


Create `src/pathgeneration/chamfer/chamferpath.h/.cpp` in RoboCrapGeometry. Keep cohesive input/result structs in this header rather than distributing constants and structs across additional headers. Inputs contain an ellipse, cylinder axis line, outward-axis sign, chamfer size, and lead settings. Return a result value with either valid geometry or an explanatory error. No output parameters, exceptions, or OCCT types.

Let k be the outward unit hole axis. Parameterize the stored ellipse as p(theta)=o+A*cos(theta)*u+B*sin(theta)*v. At each point, compute the normalized derivative t and outward cylinder radial normal r from the point's projection onto the cylinder axis. Choose the fixed traversal sign such that q=r cross t has positive dot product with k. Cylinder intersection geometry makes t perpendicular to r; validate this within numerical tolerance and form an orthonormal basis consistently. Do not introduce frame sign flips independently at individual samples.

For the internal chamfer choose z=(q-r)/sqrt(2), x=t, and y=z cross x. This is a right-handed orthonormal machining frame, with z pointing toward the hole interior and z dot k positive. In the perpendicular circular case y=(k+r)/sqrt(2). The supplied image establishes the internal normal direction; the corrected document's outward-radial formula must not be copied unchanged.

Define the first version's nominal chamfer center path as pM=p-(c/sqrt(2))*z. In the perpendicular circular case this equals p+(c/2)*r-(c/2)*k: for c=2 mm the TCP lies 1 mm radially into material and 1 mm axially inside the opening, halfway across the 2 mm chamfer section. This assumes the calibrated TCP is the selected cutting-surface point. No cutter-radius offset is added a second time.

For an oblique measured opening this is an explicit nominal 45-degree construction relative to the projected hole axis q and cylinder radial normal r. It does not claim exact equal legs on the actual tilted end face: the measured wall/end-face angle is no longer exactly 90 degrees. Keep this convention visible in documentation; do not silently claim that constant 45-degree tilt simultaneously produces exact equal legs on both skew measured surfaces. Exact chamfer contact on skew surfaces would be a separate geometric requirement.

Acceptance: inspecting representative frames verifies unit axes, zero mutual dot products, determinant +1, positive z dot k, no seam flip, and the circular 2 mm example above. Verify using code review and eventual on-screen frame diagnostics, not a new test harness.

### Step 2 — Continuous lead-in and lead-out


Extend the same chamferpath files. Use s(u)=3u^2-2u^3, angular spans from the model, hL=cL*(1-s), and hO=cO*s. Positions are pM(theta)+h*k. The normal-derived chamfer shift stays constant in meaning through the leads. There is no former radial-clearance term.

Construct a clear-end frame with Y=k, Z=-r, and X=Y cross Z. This keeps the tool surface normal facing into the hole and the burr axis exactly parallel to the hole axis. Interpolate orientation from that frame to the machining frame using unit quaternions and smoothstep. Preserve quaternion sign continuity along the curve. At lead-in progress 1 and lead-out progress 0, use the exact machining orientation. Do not interpolate Euler angles or keep the circular R_x correction for an oblique ellipse. Translation along the axis is h*k in world coordinates; its components in the rotating local frame are not constant for the ellipse.

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


Create `qml/Views/Workspace/MachiningPanel.qml`, with explicit typed inputs and signals. Place it in the existing right-side workspace in machining mode, preserving the end-effector panel and access to its TCP calibration. Reuse Components controls after reading their definitions, and Styles metrics. Add it to the root application QML source list and the appropriate hot-reload inputs if required by the existing packaging.

Expose chamfer size in mm; read-only 45-degree angle; Flip direction; lead-in/out axial clearance in mm; lead-in/out angular spans in degrees; lead-in/out interval counts in an advanced section; lead-in/out speeds and machining feed in mm/s; and auxiliary simulation speed. Allow independent lead settings but initialize them symmetrically. Keep acceleration/rate simulation configuration in an advanced section with explicit units. Numerical defaults are simulation defaults, not hardware recommendations; document chosen values during implementation. Reference the existing TCP configuration rather than duplicate six fields.

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


Work from `D:\Qt\QtProjects\RoboCrap`. Before each implementation milestone inspect its named files and affected callers, including component base definitions for QML. Read AGENTS.md. Update this plan's progress and discoveries after each milestone. Git staging, commits, branch changes, and destructive operations are not authorized.

After edits run:

    git diff --check
    git status --short

Read new files separately because ordinary git diff omits their contents. If build verification is explicitly requested, first inspect the chosen build's CMakeCache.txt for CMAKE_HOME_DIRECTORY, Qt paths, and compiler. Use the matching installed kit and hot reload OFF for application packaging. With PowerShell variable $buildDir set to that verified directory, run:

    cmake --build "$buildDir" --target robocrap --parallel
    cmake --build "$buildDir" --target robocrap_qmllint

Verify target availability first. Do not reconfigure an existing user build to another kit or mode. A separate compatible build may be configured only under the user-authorized build workflow. In a disconnected runtime, execute the acceptance scenarios above and record actual observations; compilation alone cannot establish geometry or animation correctness.

## Validation and Acceptance


The final observable result is one selected, rendered machining trajectory and a robot animation that starts at HOME, approaches without intermediate waypoints, leads into one circuit, leads out, and returns HOME. Timing derives from specified speeds and documented simulation limits. The user sees an outward-axis arrow that can be flipped, a chamfer size expressed as the drawing's leg dimension, and useful generation failures without partial scene changes. All geometry calculations remain outside QML/OCCT and all device execution remains untouched.

For the ideal circular case, inspect the 2 mm input: the nominal machining TCP is shifted by +1 mm in radial material direction and -1 mm along the outward axis, not by 2 mm along the normal. Inspect a nearly circular ellipse at multiple quadrants and the seam: axes stay orthogonal, the chamfer normal points inward/outward-opening, and animation has no configuration jumps. Document the nominal skew-plane interpretation instead of presenting it as an exact equal-leg chamfer of nonperpendicular measured surfaces.

## Idempotence and Recovery


Each generation builds an immutable candidate before scene mutation. Failed generation preserves previous paths. Regeneration replaces only the selected path after success. Playback never mutates the saved plan; stopping requires no rollback. Maintain source provenance and numerical snapshots separately. Preserve all unrelated user edits, including existing untracked files. No assets, caches, or build directories are deleted by this work.

## Surprises & Discoveries


The corrected source document still calls a a normal allowance; the user's later drawing explicitly supersedes that input definition with chamfer leg size. The nominal c/sqrt(2) conversion and inward normal direction are therefore required corrections.

The original circular lead rotation cannot align the burr to the true hole axis for an oblique ellipse; full orientation interpolation is needed. Also, hole-frame X/Y cannot both lie in an oblique end plane while remaining perpendicular to a cylinder-axis Z. Use a transverse hole frame and represent the measured ellipse in BASE coordinates.

MachiningPath is already an enum and tree category, but has no actual motion data. Edge geometry retains only source IDs, so generation needs source geometry before making a self-contained snapshot. Kr10KinematicModel stores position limits but no calibrated rate limits; do not claim exact KUKA timing from currently available data.

## Decision Log


2026-09-29: Record the user's latest chamfer-size interpretation as authoritative over the attached model's a parameter. Fix internal normal direction according to the section drawing. Keep 45 degrees, ellipse support, axial clearance, Flip, deterministic start/direction, and direct HOME approach as agreed.

2026-09-29: Choose a nominal 45-degree construction in the cylinder-normal/projected-axis section for the slightly oblique ellipse. Exact equal legs on both skew measured surfaces are not implied. This makes the approximation explicit rather than silently changing tilt or measured geometry.

2026-09-29: Propose minimal simulation controls and no additional worker, devices, or global SceneState. Store immutable generation snapshots and use existing scene identity, registration, and presentation mechanisms. These are implementation proposals, not claims of completed code.

## Outcomes & Retrospective


Planning complete. No application code has been changed by this planning task, no build has been run, and no hardware action has occurred. Implementation and runtime validation remain pending. Update this section with actual results when milestones are delivered.

## Artifacts and Notes


Source references reviewed in the conversation were the user's chamfer_trajectory_math_model_axes_corrected.md, the internal-chamfer section drawing, and the final 2 x 45-degree dimension drawing. The essential corrected equations and definitions are embedded above so this plan does not depend on Desktop attachments. Earlier discussion of KUKA manuals established the KRL/RSI stage split; this offline plan deliberately does not generate controller programs or assume an RSI correction frame.

Revision note, 2026-09-29: Initial plan created after the final chamfer-size correction; supersedes earlier suggestions of a radial/normal-offset UI field and KRL lead motions.
