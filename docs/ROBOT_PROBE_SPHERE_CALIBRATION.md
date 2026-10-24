# Calibration of a Robot-Mounted Touch Probe Using a Reference Sphere

## Goal

Determine the position of the center of the probe's ruby ball relative to the robot flange.

Setup:

- Robot: KUKA KR 10 R1420 with KR C4
- Probe: WP-500 V6
- Ruby ball diameter: \(3\,\mathrm{mm}\)
- Reference sphere diameter: \(25\,\mathrm{mm}\)
- Reference sphere center position is unknown

## Coordinate Systems and Notation

- \(\{B\}\) — robot base coordinate system (KUKA `$BASE`). The program sets
  `$BASE = $NULLFRAME`, and the transformation `$ROBROOT` is the null frame in the machine data,
  so \(\{B\}\) coincides with the KUKA world coordinate system. The application's
  "BASE-relative" poses are expressed in \(\{B\}\).
- \(\{F\}\) — robot flange coordinate system (KUKA FLANGE; the last transform returned by
  `Kr10Kinematics::solveFK`).

- \(\{P\}\) — probe TCP coordinate system. Its origin is the ruby-ball center \(r\), its axes are
  parallel to the axes of \(\{F\}\), and its z-axis is directed away from the flange along the
  stylus. On the controller it is TOOL_DATA[5] (A = B = C = 0).
- \(\{S\}\) — calibration-sphere coordinate system. Its origin is the reference-sphere center
  \(s\), and its axes are parallel to the axes of \(\{B\}\).

The calibration determines only the origins \(r\) and \(s\); the axes of \(\{P\}\) and \(\{S\}\)
are fixed by these definitions.

Notation:

- \({}^{j}A_i\) — transform matrix from coordinate system \(i\) to coordinate system \(j\)
  (4×4, homogeneous). It converts coordinates given in \(\{i\}\) into coordinates in \(\{j\}\).
- \({}^{j}R_i\) — the 3×3 rotation part of \({}^{j}A_i\).
- \({}^{j}\bar{p}_i\) — position of the origin of \(\{i\}\), expressed in \(\{j\}\), in mm.
- \({}^{j}\bar{p}_a\) — position of point \(a\), expressed in \(\{j\}\), in mm.
- \(\bar{q}\) — the six joint angles A1…A6 in degrees; \(\bar{q}_i\) are the joint angles at contact \(i\).

KUKA reports a pose as X, Y, Z in mm and Z-Y-X Euler angles A, B, C in degrees:

\[
{}^{B}R_F(A, B, C) = R_z(A)\,R_y(B)\,R_x(C).
\]

For each probe contact \(i\), forward kinematics of the joint angles \(\bar{q}_i\) gives the flange pose

\[
{}^{B}A_F(\bar{q}_i) =
\begin{bmatrix}
{}^{B}R_F(\bar{q}_i) & {}^{B}\bar{p}_F(\bar{q}_i) \
0 & 1
\end{bmatrix},
\]

where:

- \({}^{B}R_F(\bar{q}_i)\) — rotation matrix that converts vectors from \(\{F\}\) to \(\{B\}\)
- \({}^{B}\bar{p}_F(\bar{q}_i)\) — position of the flange origin, expressed in \(\{B\}\)

The same flange pose is read on the KR C4 as `$POS_ACT` when `$TOOL = $NULLFRAME` and
`$BASE = $NULLFRAME`.

## Unknowns

Let

\[
{}^{F}\bar{p}_r =
\begin{bmatrix}
{}^{F}p_{r,x}\\
{}^{F}p_{r,y}\\
{}^{F}p_{r,z}
\end{bmatrix}
\]

be the position of the ruby-ball center \(r\), expressed in the flange coordinate system \(\{F\}\).
It is constant: it does not depend on \(\bar{q}\).

Let

\[
{}^{B}\bar{p}_s =
\begin{bmatrix}
{}^{B}p_{s,x}\\
{}^{B}p_{s,y}\\
{}^{B}p_{s,z}
\end{bmatrix}
\]

be the unknown position of the reference-sphere center \(s\), expressed in the robot base coordinate system \(\{B\}\).
It is constant as well.

For contact \(i\), the ruby-ball center expressed in \(\{B\}\) is

\[
\boxed{
{}^{B}\bar{p}_r(\bar{q}_i)
=
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
}
\]

## Contact Constraint

Probe-ball radius:

\[
r_r = 1.5\,\mathrm{mm}
\]

Reference-sphere radius:

\[
r_s = 12.5\,\mathrm{mm}
\]

At external contact, the distance between their centers is

\[
d = r_r + r_s = 14\,\mathrm{mm}.
\]

Therefore every recorded contact must satisfy

\[
\boxed{
\left\|
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
-
{}^{B}\bar{p}_s
\right\|
=
d
}
\]

or equivalently

\[
\boxed{
\left(
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
-
{}^{B}\bar{p}_s
\right)^T
\left(
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
-
{}^{B}\bar{p}_s
\right)
=
d^2
}
\]

## Calibration Problem

There are six unknown scalar parameters:

\[
\bar{x} =
\begin{bmatrix}
{}^{F}p_{r,x} &
{}^{F}p_{r,y} &
{}^{F}p_{r,z} &
{}^{B}p_{s,x} &
{}^{B}p_{s,y} &
{}^{B}p_{s,z}
\end{bmatrix}^{T}.
\]

For each contact \(i = 1 \dots N\) define the residual in mm

\[
e_i(\bar{x}) =
\left\|
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
-
{}^{B}\bar{p}_s
\right\|
-
d.
\]

Estimate \({}^{F}\bar{p}_r\) and \({}^{B}\bar{p}_s\) by nonlinear least squares:

\[
\boxed{
\min_{\bar{x}}
\sum_{i=1}^{N}
e_i(\bar{x})^2
}
\]

The resulting vector \({}^{F}\bar{p}_r\) is the calibrated position of the ruby-ball center relative to the flange.

## Manual Rough Measurement of the Sphere Center

Before the automatic touches, the sphere center is measured roughly by jogging, so that the touch
paths can be aimed at it.

1. Settings: T1, tool TOOL_DATA[5], base World, jogging in World coordinates.
2. Ten touches, five in each of two TCP orientations, both with the stylus pointing straight
   down: A = 180°, B = 0°, C = 180°, and A = 0°, B = 0°, C = 180° (rotated 180° about the
   stylus).
3. In each orientation: one touch from above (jog −Z), then four horizontal touches at about the
   height of the sphere's equator, from the +X, −X, +Y and −Y sides (jog towards the sphere).
   Start positions are set by eye, about 5 mm from the sphere.
4. Jog with the smallest increment until the probe input X2.5 switches; record the TCP pose
   shown by the smartPAD (X, Y, Z, A, B, C, S, T).
5. The sphere center is the least-squares fit of a sphere to the ten recorded positions; the
   fitted radius is expected near \(r_r + r_s = 14\,\mathrm{mm}\).

The recorded poses are in `resources/calibration_sphere_jog_points.txt` (KRL `E6POS`
declarations). They are planned to be stored in `resources/json/calibration_sphere.json`
together with the sphere diameter; the refined sphere data from the calibration will be written
to the same file.

## Measurement Procedure

1. Rigidly fix the 25 mm reference sphere on the table.
2. Use an approximate TCP only for safe robot motion.
3. Approach the reference sphere slowly with the ruby ball.
4. At the probe trigger event, record the actual flange pose \({}^{B}A_F(\bar{q}_i)\), that is
   \({}^{B}R_F(\bar{q}_i)\) and \({}^{B}\bar{p}_F(\bar{q}_i)\), or the joint angles \(\bar{q}_i\).
5. Retract and repeat from different directions and flange orientations.
6. Collect preferably 15–30 well-distributed contacts.
7. Solve the nonlinear least-squares problem above.
8. Store the resulting \({}^{F}\bar{p}_r\) as the probe TCP position.

Contacts should be distributed around the reference sphere and should use significantly different flange orientations.

The reference-sphere center does not need to be known beforehand because \({}^{B}\bar{p}_s\) is estimated together with \({}^{F}\bar{p}_r\).

## Jacobian

Define the vector from the sphere center to the ruby-ball center at contact \(i\)

\[
{}^{B}\bar{v}_i(\bar{x})
=
{}^{B}\bar{p}_F(\bar{q}_i)
+
{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
-
{}^{B}\bar{p}_s,
\]

and its unit vector

\[
{}^{B}\bar{u}_i(\bar{x})
=
\frac{{}^{B}\bar{v}_i(\bar{x})}
{\left\|{}^{B}\bar{v}_i(\bar{x})\right\|}.
\]

Then

\[
e_i(\bar{x})
=
\left\|{}^{B}\bar{v}_i(\bar{x})\right\|
-
d
\]

and the Jacobian row (1×6) for contact \(i\) is

\[
\boxed{
J_i(\bar{x}) =
\begin{bmatrix}
{}^{B}\bar{u}_i(\bar{x})^T\,{}^{B}R_F(\bar{q}_i)
&
-{}^{B}\bar{u}_i(\bar{x})^T
\end{bmatrix}.
}
\]

The first three columns are the derivatives with respect to \({}^{F}\bar{p}_r\), the last three
with respect to \({}^{B}\bar{p}_s\). This Jacobian can be used with Gauss-Newton or
Levenberg-Marquardt optimization.
