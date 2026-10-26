# RoboCrap: application description

RoboCrap is a Windows desktop application for preparing and visually checking robotic edge
machining with a KUKA KR 10 R1420 on a KR C4 controller (KSS 8.3, RobotSensorInterface (RSI) 3.3).
The current implementation supports chamfering the edge of a hole; other edge geometries are part
of the broader intended scope. Implementation details are in [ARCHITECTURE.md](ARCHITECTURE.md);
the controller/PC protocol is in
[RSI_Chamfer_Hole_CONTRACT.md](../resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md).

Terms used below: TCP is the tool center point. PTP is a synchronized joint motion to a target;
LIN is a straight Cartesian motion. P_s is the staging point on the hole axis, outside the hole.

## Fixed equipment and editable settings

- The robot model and both end-effector models (measuring tool MEE and spindle tool SEE) are fixed
  by the application. They cannot be loaded or replaced from the UI.
- Spindle EE (Properties panel, Machining mode): the ER collet pose relative to the flange is
  displayed read-only. The TCP pose relative to the ER collet is edited and committed with Apply.
  Changing it invalidates every generated path until it is regenerated.
- Measuring EE (Measuring mode): name, ruby ball diameter and stylus length are displayed
  read-only. Measuring-tool calibration is not implemented.
- Only workpiece surfaces are imported, as JSON point files measured with a touch probe.

## Workflow

1. **Load measured geometry (Measuring mode, Rough submode).**
   Import the workpiece end surface with *Plane from JSON* and the hole with *Cylinder from JSON*.
   Each file is fitted and compensated for the probe ball radius. Select both surfaces and run
   *Intersect Selected* to create their intersection edge: an ellipse when the end plane is
   inclined to the hole axis. *Cone from JSON* and the Precise submode (Dry Scan, Scan) are
   placeholders without an implementation.

2. **Generate the animation trajectory (Machining mode).**
   Select the intersection edge and open *Generate Path*. Settings: chamfer size (mm) and angle
   (degrees from the hole axis), outward direction, axial distance of P_s from the hole (mm),
   lead-in and lead-out axial clearance (mm) and span (degrees of the ellipse parameter), machining
   speed (mm/s), acceleration/deceleration distance as a percentage of each lead, LIN transfer
   speed `$VEL.CP` (mm/s) and acceleration `$ACC.CP` (mm/s²), and the HOME PTP simulation speed (%).
   *Generate* computes the motion on a background thread and saves it as a machining path object.

   The motion is: PTP HOME to P_s, LIN P_s to the lead-in start, lead-in, one full machining turn,
   lead-out, LIN lead-out end to P_s, PTP P_s to HOME. Lead-in, machining and lead-out are timed
   by the machining speed with an acceleration ramp over the chosen part of each lead. Generation
   fails, without retiming, if inverse kinematics (joint angles for a TCP pose) has no solution,
   or if joint limits, rated joint speeds or the Cartesian acceleration bound (10000 mm/s², the
   controller's `$ACC_MA.CP`) are violated.

3. **Inspect and play (Dry Run).**
   The path is drawn in the 3D scene; the Scene and Properties panels select objects, control
   visibility and show saved path settings. *Dry Run* plays the saved joint motion without
   computing new points. Pause/resume keeps the current position; *Stop* returns the preview to
   HOME. Generation is blocked during playback.

4. **Prepare the robot run (RSI Sampling).**
   With a compatible generated path selected and the RSI UDP socket bound (Network settings),
   *RSI Sampling* computes one Cartesian correction per 4 ms RSI cycle for lead-in, machining and
   lead-out, followed by 50 zero corrections. It writes `C:/RoboCrapJobs/shared_data.txt`
   (version 2): the PTP/LIN targets PsIn, LeadStart and LeadEnd with their KUKA Status/Turn
   (S/T) joint-configuration values, PsOut's S/T, the flange-to-TCP frame and the transfer
   `$VEL.CP`/`$ACC.CP` in m/s and m/s². Only after a successful write are the corrections handed
   to the RSI device. Each sampling saves a
   diagnostic plan CSV in `records/` beside the executable; UDP exchanges are captured with the
   same identifier.

5. **Run on the controller (T1, manual start).**
   The operator starts `RSI_Chamfer_Hole.src`/`.dat` with the helpers in
   `resources/files/rsi/FILE_UTILS/` and the RSI Visual context. KRL reads the shared file,
   moves HOME, PTP PsIn, LIN LeadStart, follows the streamed corrections under RSI, and only after
   the PC reports completion moves LIN PsOut and PTP HOME. The file is deleted whenever its path
   is removed, regenerated or invalidated, at a new sampling request, at startup and at shutdown,
   so KRL stops after HOME instead of using stale targets.

## Scope and status

- In scope: animation preparation and playback, and nominal RSI Sampling/streaming for the
  `RSI_Chamfer_Hole` T1 run. Force correction is out of scope.
- A successful Generate, Dry Run or Sampling, a bound socket or a delivered reply does not
  establish controller readiness or permission to move the robot.
- User-supplied captures from an earlier version ended in a controller fault. The user reported
  on 2026-10-24 that RSI Sampling works and the real robot motion matches it.
- Device panels (PLC, force/torque sensor, network settings) show and control the connected
  devices; *Start Machining* is disabled.
