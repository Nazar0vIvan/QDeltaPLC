# Strategy draft: machining edges of different shapes

Status (2026-10-21): research and open questions only. No ExecPlan yet; it is written after
the user answers the questions in section 5. Nothing here changes current code or contracts.

## 1. How CAM systems handle edge finishing

- **Geometry.** CAM works on a CAD boundary representation (B-rep): faces bounded by edges,
  with face adjacency. An edge is a curve shared by two faces; its curve is a line, circle,
  ellipse or B-spline/NURBS. Edge finishing needs, at every curve point, the position, the
  tangent and the two adjacent face normals.
- **Selection and chaining.** Edges are picked manually or detected automatically (Mastercam
  and BobCAD deburr: automatic edge detection with exclusions). Picked edges are chained end to
  end into open or closed contours with a direction and a start point. Corners between chained
  edges are joined round or mitred (FreeCAD CAM Deburr).
- **Toolpath.** A drive curve plus tool-axis control: the axis is derived from the two face
  normals (chamfer bisector), optionally with lead/lag, tilt and a tilt range. Chamfers are
  either constant width or constant depth (Autodesk Fusion deburr). Mastercam 2026 adds
  asymmetric edge breaking via tool-axis control.
- **Linking.** Lead-in/lead-out (tangent, arc, ramp), approach/retract to a clearance, links
  between passes, and gouge/collision checks against adjacent faces and fixtures.
- **Output.** A post-processor converts the toolpath into controller code. Robot CAM
  (RoboDK curve follow, SprutCAM X Robot chamfering, Robotmaster, PowerMill Robot) adds IK,
  configuration choice, singularity and joint-limit handling and uses the free rotation about a
  symmetric tool's axis; it writes KRL SRC/DAT with LIN/SPLINE blocks or streams to the robot.
  Large generated SRC files can exceed controller program memory (third-party reports).

## 2. Where RoboCrap's current pipeline depends on the hole shape

Shape-specific (must be generalized):
- `EdgeGeometry` holds only an ellipse; `intersectSurfaces` supports only plane x cylinder and
  rejects `ParallelCylinderAxis` (the line edges of an axis-parallel cylinder).
- `ChamferPath` builds the TCP frame from the cylinder axis, the radial direction and the end
  plane normal; leads follow the ellipse parameter plus axial clearance; P_s lies on the axis.
- `chamfercentralvalidation` uses ellipse-coefficient curvature bounds to fix the point schedule.
- The machining phase is a closed full turn; lead-in and lead-out meet at the same edge point.
- The shared file and KRL assume a closed edge: PsOut copies PsIn XYZ.

Shape-independent (reusable as is): arc-length maps (they need only position derivatives from
`phaseArcDifferential`), central timing, seeded checked IK, LIN transfers, joint splines,
Dry Run, RSI sampling, RSI streaming and the KRL motion sequence.

The seam is therefore one curve abstraction that provides, for a parameter `t`, the edge point
and its first two derivatives, both face normals, and a curvature bound or estimate. Analytic
lines, arcs and ellipses and a fitted B-spline can all supply it.

## 3. Controller programs: generate code or generate data

The current RSI architecture already makes the SRC shape-independent: KRL executes PTP PsIn,
LIN LeadStart, an RSI stream and LIN PsOut, and only the data and the stream depend on the edge.
A universal SRC does work if:
- the shared file carries PsOut as a full POS (required for open edges), and
- several edges per job are a loop over arrays of targets plus one stream per edge.

Generated SRC/DAT is needed only to run native KRL motion (LIN/SPLINE blocks) instead of RSI.
A middle option keeps one SRC and reads point arrays from the file, executing
`LIN P[i] C_DIS` in a loop. SPLINE blocks cannot be built from a loop, because a spline block
contains only spline segments.

Ways to get files onto a KR C4 without WorkVisual:

| Method | Source | Notes |
| --- | --- | --- |
| Navigator copy from USB or network, or File > Restore from the network archive path | KSS 8.3 SI 7.4, 7.10 | Manual operator step, Expert user group |
| Data file read at runtime (current: `krl_mount`, `krl_fscanf`) | CREAD/CWRITE 6.16 | No program transfer; already in use |
| `$CMD` `RUN/STOP/CANCEL` from a submit program | CREAD/CWRITE 7.2 | Selects existing programs; does not transfer files |
| C3 Bridge Interface Server on the controller (TCP 7000) | RoboDK docs, c3 project | Third-party; uploads/downloads programs, selects them, reads/writes variables |
| KukaVarProxy / OpenShowVar | GitHub projects | Third-party; global variables only |
| Windows share of the controller drive | Third-party reports | Unofficial; copying outside the HMI is discouraged; selection compiles |
| KUKA.DirectoryLoader | KUKA option, third-party reports | Licensed; reportedly AUT EXT only (unverified) |

Recommendation: keep generating data, not code, while RSI is the execution method. Revisit
code generation only if native KRL motion is chosen (question G1).

## 4. Candidate architecture (to be confirmed by the answers)

1. Scene: an Edge object owns a curve value (line, arc, ellipse, B-spline) plus its two
   source surfaces, its trimming endpoints and open/closed state.
2. Geometry: intersections per surface pair (plane x plane, plane x cylinder including the
   axis-parallel case, plane x cone, cylinder x cylinder as a fitted B-spline), or a curve fitted
   directly to probe points along the edge.
3. Path: `ChamferPath` takes the curve abstraction; leads become shape-independent (tangent
   extension or bisector approach), with separate entry and exit staging points for open edges.
4. Validation: replace ellipse-coefficient curvature bounds with a per-curve bound.
5. Export: shared file version 3 with a full PsOut POS; later, arrays for multi-edge jobs.

## 5. Questions that decide the strategy

A. Edges and parts
1. Which edge types are needed first? Give 2-3 real parts: line segments, arcs (fillets in a
   2D contour), ellipses, free-form splines, cylinder x cylinder intersections.
2. Which surface pairs form them: plane x plane, plane x cylinder (axis tilted or parallel to
   the plane), plane x cone, cylinder x cylinder, free-form?
3. Open edges, closed loops, or both? Convex edges only, or concave (inside) edges too?
4. Operation: chamfer only, or also edge rounding/deburring? Size range and tolerance?

B. Geometry source
1. Only touch-probe measurements, or also CAD (STEP) of the nominal part?
2. If CAD exists: localize the CAD by probing (best fit), or measure every feature locally
   as ROBOT_LOCAL_MEASUREMENT_ACCURACY.md recommends?
3. How are free-form edges measured: points on both adjacent faces, or points along the edge?
4. How are the ends of an open edge defined: from the measured surface extents, by picking,
   or by probing the end faces?

C. Chains and corners
1. Must connected edges (for example a pocket outline of lines and arcs) be machined in one
   continuous pass, or edge by edge?
2. How should sharp corners be handled: stop and reorient, round join, or split into passes?

D. Tool
1. The same conical chamfer tool for all shapes? Is it rotationally symmetric, so rotation
   about its axis is free for reach and singularity avoidance?
2. Is the tool axis fixed by the chamfer angle (bisector), or are lead/lag/tilt settings needed?
3. Is the cutting direction fixed (climb or conventional)?

E. Leads and staging
1. For an open edge that ends at a wall, may the tool not overrun the end? Is a ramp-in along
   the edge acceptable?
2. Should PsIn/PsOut be per edge, or a common clearance plane for several edges?

F. Job
1. One edge per KRL run, or several edges per run? Who chooses the order?
2. Is collision checking against the workpiece and fixtures required, or does the operator
   verify clearance in T1?

G. Execution
1. Keep RSI streaming for every shape, or allow native KRL motion (generated or data-driven)?
2. Is a PC-in-the-loop 4 ms stream acceptable for long edges (minutes of motion)?
3. May third-party software (C3 Bridge) be installed on the controller? Which KUKA options are
   licensed besides RSI (EthernetKRL, DirectoryLoader)?
4. Will execution stay T1 with manual start, or move to AUT/EXT with a PLC?

H. Scope
1. Does force correction stay out of scope? Contact-force following would keep RSI mandatory.
2. Which first deliverable: one new shape end to end (for example a straight plane x plane
   edge), or the curve abstraction first with the ellipse ported to it?

## Sources

- FreeCAD CAM Deburr: https://wiki.freecad.org/CAM_Deburr
- BobCAD deburring: https://bobcad.com/tech-tuesday-3-ways-to-deburr-parts-with-cad-cam/
- Mastercam 2026 asymmetric deburr: https://my.mastercam.com/whats-new-2026/whats-new-2026_asymmetricoptions/
- Fusion deburr forum thread: https://forums.autodesk.com/t5/fusion-manufacture-forum/5-axis-deburr-touching-faces/td-p/13969936
- SprutCAM robotic deburring: https://sprutcam.com/el/?p=192506
- RoboDK Curve Follow Project: https://robodk.com/doc/en/Robot-Machining-Curve-Follow-Project.html
- RoboDK KUKA driver (C3 Bridge): https://robodk.com/doc/en/Robots-KUKA-RoboDK-driver-KUKA.html
- KukaVarProxy message format: https://github.com/kyjanond/kukavarproxy-msg-format
- KRC4 program size report: https://industrialmonitordirect.com/blogs/knowledgebase/resolving-kuka-krl-program-file-size-errors-on-krc4
- DirectoryLoader report: https://industrialmonitordirect.com/blogs/knowledgebase/configuring-kuka-directoryloader-on-krc4-without-plc
- KRC4 program transfer without WorkVisual: https://industrialmonitordirect.com/blogs/knowledgebase/kuka-krc4-program-edit-without-workvisual-methods
- KUKA manuals in D:/Qt/QtProjects/manuals/KUKA: KSS 8.3 SI 7.4/7.10, CREAD/CWRITE 6.16/7.2.
