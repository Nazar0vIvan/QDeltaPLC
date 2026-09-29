# Architecture and chamfer animation review

Reviewed 2026-09-29 within root files, `src`, `qml`, `resources` and `cmake`, excluding build and old directories. This is a source-level architecture review and focused trace of the chamfer feature, not a claim that every algorithm or packaged asset has been runtime-validated. The implementation plan remains `CHAMFER_ANIMATION_EXECPLAN.md`.

## Layers and ownership

| Layer | Authoritative files | Responsibility |
| --- | --- | --- |
| Application composition | Root `CMakeLists.txt`, `src/main.cpp` | Package application QML/resources; create devices, scene, tool configuration, machining coordinator, viewport controller and QML engine in lifetime order. |
| Numerical Geometry | `src/CMakeLists.txt`, `src/geometry`, `src/pathgeneration/chamfer`, numerical KR10 files under `src/3d/robot` | Fit surfaces, calculate intersections, construct TCP frames, solve kinematics, compile and evaluate timed joint motion. Uses Qt Core and Eigen; no viewport or device calls. |
| Scene | `src/scene` | Own IDs, objects, immutable geometry and saved paths; import samples; own EE configuration, generation drafts, compatibility and playback clock. Depends on Geometry and Qt QML. |
| Backend registration | `src/backendqmltypes.h` | Expose existing C++ objects/value types through RoboCrap.Backend. Registration is not another owner or motion engine. |
| Viewport | `src/3d/CMakeLists.txt`, `occcontroller`, adapters, `occt` | Load robot/tool CAD, render application geometry, map picks to IDs, apply robot poses, preserve presentations across window recreation. Depends on Scene and Geometry. |
| Devices | `src/network` | Own PLC/FTS/RSI sockets and protocols on I/O threads; publish value state to GUI-thread runners. No dependency on the chamfer scene coordinator. |
| UI | `qml/Main.qml`, workspace views, Components and Styles | Own selection and display preferences, edit drafts, invoke typed backend operations, compose panels/tool windows. Does not fit surfaces or advance robot time. |
| Assets and deployment | `resources/*.qrc`, `resources/json/kr10.json`, `resources/cad/kr10`, `cmake/DeployOcct.cmake` | Embed selected JSON/icons/fonts/protocol files; deploy CAD and OCCT resources/DLLs beside the executable. |

Build dependencies, not directory names, define the layers. KR10 numerical kinematics physically lives below `src/3d` but compiles in RoboCrapGeometry. The robot visual model and OCCT adapters compile in RoboCrap3D. Backend links Scene and Devices; the viewport links Scene directly. Eigen is public because numerical public headers expose its types.

`main.cpp` creates SceneModel before SceneEndEffectors, SceneMachining, OccController and the QML engine. Reverse destruction therefore removes consumers first. Scene and tool state survive UI recreation. The QML singleton adapters retain C++ ownership. Logger is the existing context-property exception.

## From measurements to an animated robot

1. Main's file dialog calls SceneSurfaceImporter (QML name PlaneImporter). JSON decoding, fitting and insertion are synchronous on the GUI thread. Plane/cylinder points are fitted before probe compensation; original points remain diagnostic samples. The scene cylinder radius already includes compensation.
2. Main passes two selected IDs to SceneModel::intersect. Only a fitted plane/cylinder pair is accepted. Geometry computes the complete ellipse using the infinite plane and axially extended cylinder, not their finite display bounds. A perpendicular plane produces a circle. A plane parallel to the axis is rejected.
3. SceneEdgeGeometry stores immutable ellipse data and source IDs. For a new chamfer, SceneMachining resolves the live cylinder axis and plane from those IDs. An orphaned edge remains visible but cannot supply new generation input. A previously generated path owns enough snapshots to regenerate after source removal.
4. ChamferPath validates the ellipse against the cylinder line. Its local frame follows the longitudinal chamfer section; TCP is that frame rotated 180 degrees around local X with the same origin. Flip reverses the outward axis and traversal. Chamfer size is a leg dimension; for the perpendicular circular case size 2 mm puts the calibrated cutting-point TCP 1 mm radially into material and 1 mm axially into the opening.
5. Lead-in/out add axial clearance and interpolate orientation with quaternions. Lead span uses ellipse-parameter degrees, which are not arc-length degrees or generally cylinder azimuth. Adaptive sampling supplies position/orientation checks and shared phase junctions.
6. ChamferMotion converts BASE-relative TCP targets to flange targets with the saved flange-to-TCP calibration. Numerical IK tries bounded initial configurations, follows preceding joint seeds and retains continuous joint angles. FK checks solutions. HOME stages are synchronized joint-space moves to/from P_s; their displayed TCP curves come from FK. Fixed-attitude Cartesian transfers join P_s to the clear lead endpoints using the corresponding lead feeds. Timing stops at transfer/lead corners.
7. Timing uses lead/chamfer feeds and TCP acceleration, then checks joint and sampled Cartesian rates. Central motion uses monotone cubic joint interpolation and a uniform time multiplier when limits require slower motion. HOME uses synchronized quintic progress. Apply inserts a complete immutable saved result only after successful compilation; regeneration inserts the replacement before removing the old path.
8. OccSceneAdapter renders seven phase polylines under one application ID: HOME purple, transfers/leads blue, machining green. Show Path combines with object visibility. The outward-axis preview is separate, non-selectable and excluded from camera fitting.
9. Dry Run owns a saved-motion snapshot. SceneMachining's 16 ms timer schedules display work; its monotonic clock supplies actual elapsed time to evaluate(). A direct GUI-thread signal sends joints to OccController, which creates a FK pose candidate and commits state after presentation succeeds. QML does not transport trajectory arrays or perform frame-by-frame motion calculations.

Stop retains the last displayed pose. Restart starts at HOME. Changing selection does not replace active motion. Leaving machining mode, losing readiness, changing model/TCP calibration, deleting the active path or shutting down stops playback. Model/TCP revisions invalidate saved paths until regeneration; temporary viewport loss only disables readiness.

## The four UI observations

The main right column now contains Properties. Both measuring submodes offer a Measuring EE toolbar button. Machining offers Machining Setup, with Spindle End Effector and Internal chamfer side by side. These are nonmodal tool windows, following existing application window conventions; they can be resized and their panels scroll independently. Closing retains drafts; leaving the mode hides its window. Closing setup does not stop playback, and Stop remains on the main toolbar. Generate Path opens and focuses the chamfer editor. Setup is available before calibration or input selection so users can resolve prerequisites.

Cylinder Diameter is `2 * CylinderGeometry.radius`; it therefore describes the compensated surface, not the fitted probe-ball-center cylinder. This requires no duplicated geometry state or new C++ API.

The former HOME speed scale was a dimensionless number in (0,1]. The UI now displays HOME PTP simulation in percent while preserving the backend value: 50 means 0.5. The current algorithm scales both configured joint speed and joint acceleration. At defaults, 100 percent uses 30 deg/s and 60 deg/s² for each axis, while 50 percent uses 15 deg/s and 30 deg/s². These are chosen simulation limits; the robot model has no manufacturer rate data. Consequently the number must not be interpreted as a calibrated KUKA percentage or an exact controller cycle-time prediction.

For a HOME joint displacement d and scale s, the shared duration is the maximum across all joints of `1.875 * abs(d)/(jointSpeed*s)` and `sqrt((10/sqrt(3))*abs(d)/(jointAcceleration*s))`, with a 1 ms floor for stationary phases. Halving the percentage therefore need not exactly double duration when acceleration dominates. This documents the existing implementation; this UI change does not alter it.

Advanced controls are not necessary as everyday operator inputs, but the current timing algorithm needs values for them. They remain collapsed as Simulation tuning, with saved values preserved. Minimum lead intervals seed adaptive sampling rather than set display frame rate. TCP acceleration determines feed ramps and Cartesian acceleration checks. Joint speed and acceleration limit simulated motion and may lengthen the entire central sequence. These belong to simulation configuration; a later calibrated robot profile could remove the need to expose per-axis editing in the operation panel. Sampling defaults can remain internal if operator-facing simplification is requested later.

## Other application paths

DeviceHub places PLC on GeneralIO and FTS/RSI on ControlIO. DeviceRunner and its QQmlPropertyMap stay on the GUI thread. Requests cross queued signals; AbstractDevice performs its direct meta-call only once execution is on the I/O thread. Device sockets/timers are created and destroyed there. Shutdown waits for stop calls before stopping threads. FTS-to-RSI force delivery relies on their shared thread.

The Network UI selects connection profiles and invokes connection changes through runners. Dashboard device controls include PLC outputs, FTS streaming/bias/logging and RSI trajectory/streaming. Main currently instantiates the workspace plus separate Network and PLC windows, not the entire packaged Dashboard or old navigation shell. Being listed in QML_FILES does not mean a view is part of the current main workflow.

The existing `pathgeneration/rsi` and blade utilities also compile in Geometry. RsiDevice::generateTrajectory uses its own hard-coded/example geometry and offset pipeline; it does not consume SceneMachiningPath. RsiPath's six-component distances mix position and orientation units, so it is not interchangeable with the chamfer compiler's TCP translation arc length. `pathgeneration/planemesh.*` exists in source but is not listed in the current Geometry target. Start Machining in the workspace remains inactive.

CAD loading is worker-based and serializes OCCT parsing/cache access. Scene JSON import and chamfer compilation are synchronous. Application assets resolve the embedded robot JSON and deployed CAD/resource directories; content/version-keyed BREP caches belong under QStandardPaths::CacheLocation. The deployment script copies CAD and OCCT runtime resources/DLL dependencies. Root QML packaging has distinct Felgo hot-reload and embedded paths; FAST_QML_BUILD skips cache generation.

## Remaining limits and verified review observations

Step 9 of the animation ExecPlan remains open: source implementation exists, but its earlier milestones explicitly did not establish successful end-to-end build/runtime behavior. This follow-up also performs source and diff checks only, respecting the repository's explicit-request requirement for builds and QML lint. No hardware actions were performed.

Chamfer size now specifies equal setbacks on the actual plane and cylinder generator in each longitudinal section; the TCP origin is their midpoint. This is 45 degrees for a perpendicular opening, with a varying section angle for a tilted plane. Section axes are not a complete swept-surface contact model. The calibrated TCP is a point on the burr's cutting surface, not a tool center automatically corrected by cutter radius. No tool-contact/material-removal or collision calculation is present.

Generation runs on the GUI thread and can perform many IK/refinement attempts; responsiveness should be measured before introducing a worker. Bounded IK seed search is not an exhaustive reachability proof. Sampled Cartesian/interpolation checks are not analytic guarantees. Central uniform slowdown may be conservative, including its built-in 5 percent timing margin.

There are existing differences between current agent rules and older source: OCCT code still contains try/catch, and RobotPreviewState methods return errors through QString references. These were observed but not refactored in this UI task. Similarly, the numerical cylinder import currently collapses different fit failures into one generic message, as investigated for XC.json earlier. The user's existing XC/XP and IDE-setting edits are preserved.

`resources/json/README.md` says all edge example datasets use radius 3; the current locally edited XC uses radius 1.5 and XP uses radius 0. The review leaves those user-owned dataset edits untouched. Asset examples and deployed/embedded resource lists should not be mistaken for an automated verification suite.

## TCP visualization follow-up

Spindle Apply now shows an RGB TCP frame and stays available whenever its six input values are valid. A separate Visible checkbox controls OccController.showSpindleTcp. Unchanged TCP values remain a no-op in SceneEndEffectors, so reapplying does not invalidate saved paths. Controller converts the applied flange-relative XYZABC using existing Z-Y-X helpers; OccViewWindow retains the optional numerical frame and OccRobotAdapter composes it with the current flange. OccScene uses a separate OccWorldAxes instance, sized at 5 percent of camera scale and excluded from picking/fit bounds. Spindle mode and robot readiness gate presentation without erasing the visibility preference.

For the current XP/XC workpiece, new edge selection initializes Flip so outward hole Z projects toward negative scene X; saved paths retain their direction. This does not redefine the spindle TCP's Z. The current XC has seven points. An offline NumPy reproduction gives 0.01623 mm RMS versus 0.11865 mm tolerance and outward axis approximately (-0.999891, -0.012878, -0.007181). Compensated intersection radii are approximately 7.434435 and 7.432715 mm. No dataset was edited and no compiled/runtime verification is implied.
