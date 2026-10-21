# Calibration of a Robot-Mounted Touch Probe Using a Reference Sphere

## Goal

Determine the position of the center of the probe's ruby ball relative to the robot flange.

Setup:

- Robot: KUKA KR 10
- Probe: WP-500 V6
- Ruby ball diameter: \(3\,\mathrm{mm}\)
- Reference sphere diameter: \(25\,\mathrm{mm}\)
- Reference sphere center position is unknown

## Coordinate Systems

Let:

- \(\{B\}\) — robot base coordinate system
- \(\{F\}\) — robot flange coordinate system

For each probe contact \(i\), the robot provides the flange pose

\[
{}^{B}T_{F,i} =
\begin{bmatrix}
{}^{B}R_{F,i} & {}^{B}\vec{p}_{F,i} \\
0 & 1
\end{bmatrix},
\]

where:

- \({}^{B}R_{F,i}\) — rotation matrix that transforms vectors from \(\{F\}\) to \(\{B\}\)
- \({}^{B}\vec{p}_{F,i}\) — position of the flange origin, expressed in \(\{B\}\)

## Unknowns

Let

\[
{}^{F}\vec{p}_r =
\begin{bmatrix}
{}^{F}p_{r,x}\\
{}^{F}p_{r,y}\\
{}^{F}p_{r,z}
\end{bmatrix}
\]

be the position of the ruby-ball center, expressed in the flange frame \(\{F\}\).

Let

\[
{}^{B}\vec{p}_s =
\begin{bmatrix}
{}^{B}p_{s,x}\\
{}^{B}p_{s,y}\\
{}^{B}p_{s,z}
\end{bmatrix}
\]

be the unknown position of the reference-sphere center, expressed in the robot-base frame \(\{B\}\).

For contact \(i\), the ruby-ball center expressed in \(\{B\}\) is

\[
\boxed{
{}^{B}\vec{p}_{r,i}
=
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
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
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s
\right\|
=
14
}
\]

or equivalently

\[
\boxed{
\left(
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s
\right)^T
\left(
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s
\right)
=
14^2
}
\]

## Calibration Problem

There are six unknown scalar parameters:

\[
\vec{x} =
\begin{bmatrix}
{}^{F}p_{r,x} &
{}^{F}p_{r,y} &
{}^{F}p_{r,z} &
{}^{B}p_{s,x} &
{}^{B}p_{s,y} &
{}^{B}p_{s,z}
\end{bmatrix}^{T}.
\]

For each contact define the residual

\[
e_i =
\left\|
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s
\right\|
-
14.
\]

Estimate \({}^{F}\vec{p}_r\) and \({}^{B}\vec{p}_s\) by nonlinear least squares:

\[
\boxed{
\min_{{}^{F}\vec{p}_r,\,{}^{B}\vec{p}_s}
\sum_{i=1}^{N}
\left(
\left\|
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s
\right\|
-
14
\right)^2
}
\]

The resulting vector \({}^{F}\vec{p}_r\) is the calibrated position of the ruby-ball center relative to the flange.

## Measurement Procedure

1. Rigidly fix the 25 mm reference sphere on the table.
2. Use an approximate TCP only for safe robot motion.
3. Approach the reference sphere slowly with the ruby ball.
4. At the probe trigger event, record the actual flange pose \({}^{B}R_{F,i}\) and \({}^{B}\vec{p}_{F,i}\).
5. Retract and repeat from different directions and flange orientations.
6. Collect preferably 15–30 well-distributed contacts.
7. Solve the nonlinear least-squares problem above.
8. Store the resulting \({}^{F}\vec{p}_r\) as the probe TCP position.

Contacts should be distributed around the reference sphere and should use significantly different flange orientations.

The reference-sphere center does not need to be known beforehand because \({}^{B}\vec{p}_s\) is estimated together with \({}^{F}\vec{p}_r\).

## Jacobian

Define

\[
{}^{B}\vec{v}_i
=
{}^{B}\vec{p}_{F,i}
+
{}^{B}R_{F,i}{}^{F}\vec{p}_r
-
{}^{B}\vec{p}_s,
\]

and

\[
{}^{B}\vec{u}_i
=
\frac{{}^{B}\vec{v}_i}
{\left\|{}^{B}\vec{v}_i\right\|}.
\]

Then

\[
e_i
=
\left\|{}^{B}\vec{v}_i\right\|
-
14
\]

and the Jacobian row for contact \(i\) is

\[
\boxed{
J_i =
\begin{bmatrix}
({}^{B}\vec{u}_i)^T\,{}^{B}R_{F,i}
&
-({}^{B}\vec{u}_i)^T
\end{bmatrix}.
}
\]

This Jacobian can be used with Gauss-Newton or Levenberg-Marquardt optimization.
