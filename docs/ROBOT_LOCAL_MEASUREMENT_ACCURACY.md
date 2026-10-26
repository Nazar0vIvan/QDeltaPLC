# Robot accuracy: measuring near the machined feature

This document explains why probing the surfaces that form an edge close to that edge, with the
same robot that machines it, gives a smaller machining error than probing a global workpiece
coordinate system. All numerical values marked "hypothetical" are illustrations, not
measurements of the KR10 R1420.

## 1. Definitions

All positions are in the robot BASE coordinate system, in mm.

- $\bar{q}$: the joint coordinate vector, the six angles reported by the joint encoders, in degrees.
- $\bar{\pi}$: the kinematic parameter vector. It contains the link lengths and offsets (mm), the
  joint zero offsets (degrees, the difference between the encoder angle and the actual link
  angle), and the TCP (tool centre point) transform of the mounted tool.
  - $\bar{\pi}_N$: nominal parameter values, as stored in the controller.
  - $\bar{\pi}_R$: real parameter values of the physical robot and tool.
  - $\delta\bar{\pi} = \bar{\pi}_R - \bar{\pi}_N$: the parameter deviation. Its joint-offset elements
    are the joint angle errors.
- $\bar{p}(\bar{q}, \bar{\pi})$: forward kinematics, the TCP position for joint coordinates $\bar{q}$
  and parameters $\bar{\pi}$.

**Position error of the TCP.** It is the difference between the real TCP position and the TCP
position that the controller computes for the same encoder readings $\bar{q}$:

$$\bar{e}(\bar{q}, \delta\bar{\pi}) = \bar{p}(\bar{q}, \bar{\pi}_N + \delta\bar{\pi}) - \bar{p}(\bar{q}, \bar{\pi}_N)$$

**First-order approximation:**

$$\bar{e}(\bar{q}, \delta\bar{\pi}) \approx S(\bar{q}, \bar{\pi}_N) \cdot \delta\bar{\pi}$$

$$S(\bar{q}, \bar{\pi}_N) = \frac{\partial \bar{p}(\bar{q}, \bar{\pi})}{\partial \bar{\pi}}$$

The derivative is evaluated at $\bar{\pi} = \bar{\pi}_N$. $S(\bar{q}, \bar{\pi}_N)$ is a matrix with 3
rows and $n$ columns, where $n$ is the number of parameters. The error is linear in the deviation
$\delta\bar{\pi}$, and its value depends on the joint coordinates $\bar{q}$ through
$S(\bar{q}, \bar{\pi}_N)$. The error is therefore caused by the parameter deviations (including
the joint angle errors), and its value at a given pose depends on the joint coordinates.

**Example (hypothetical deviation value).**

- Let the only deviation be a joint 1 zero offset $\delta\theta_1$ = 0.01°, which is
  $1.745 \cdot 10^{-4}$ rad.
- The TCP then moves along a circle around axis 1 by
  $\lvert \bar{e}(\bar{q}, \delta\bar{\pi}) \rvert \approx r(\bar{q}) \cdot \delta\theta_1$, where
  $r(\bar{q})$ is the distance of the TCP from axis 1, in mm.
- At $r(\bar{q})$ = 800 mm the error is 0.14 mm; at $r(\bar{q})$ = 1200 mm it is 0.21 mm.
- One and the same deviation $\delta\theta_1$ therefore produces different errors at different
  joint coordinates $\bar{q}$.

**Not covered by this definition:** elastic deformation under load (gravity, process force),
thermal expansion, and repeatability scatter (KR10 R1420 datasheet: ±0.04 mm). These are
treated in section 5.

## 2. Probing

For each probe point $j = 1, \dots, m$:

1. The robot moves the Measuring EE until the probe touches the surface. At that moment the
   controller reads the joint coordinates $\bar{q}_j$.
2. The controller computes the recorded point $\bar{m}_j = \bar{p}(\bar{q}_j, \bar{\pi}_N)$ with the
   nominal parameters, including the nominal Measuring EE TCP.
3. The real position of the probe tip centre at that moment is $\bar{p}(\bar{q}_j, \bar{\pi}_R)$.
   Therefore:

$$\bar{m}_j = \bar{p}(\bar{q}_j, \bar{\pi}_R) - \bar{e}(\bar{q}_j, \delta\bar{\pi})$$

Each recorded point $j$ differs from the real tip position by its own error
$\bar{e}(\bar{q}_j, \delta\bar{\pi})$.

In RoboCrap, the points $\bar{m}_j$ are the coordinates in the imported JSON files, for example
`resources/json/edge_cyl_plane/XP.json` and `XC.json`. These files were recorded by jogging the
robot until the WP-500 touched the surface: `XP.json` holds the touches of the end plane,
`XC.json` the touches of the cylinder hole.

`BoundedPlane::fromSamples` and `BoundedCylinder::fromSamples` fit surfaces to the points
$\bar{m}_j$. Because these points contain the errors $\bar{e}(\bar{q}_j, \delta\bar{\pi})$, the fitted
surface parameters, and the edge computed from them, contain errors too.

## 3. Machining, and which error terms cancel

1. **Target points.** `ChamferPath` computes the target TCP positions $\bar{c}_k$, for
   $k = 1, \dots, K$, from the fitted plane and cylinder.
   - If the fit had used the real tip positions $\bar{p}(\bar{q}_j, \bar{\pi}_R)$ instead of
     $\bar{m}_j$, it would have produced the targets on the real edge, $\bar{c}_k^R$.
   - Write $\bar{c}_k = \bar{c}_k^R + \bar{d}_k$, where $\bar{d}_k$ is the target error caused by
     the measurement errors.
2. **Commanded motion.** For each target, the controller solves inverse kinematics with the
   nominal parameters, including the nominal Spindle EE TCP. It finds the machining joint
   coordinates $\bar{q}_k^M$ with $\bar{p}(\bar{q}_k^M, \bar{\pi}_N) = \bar{c}_k$.
3. **Real tool position:**

$$\bar{p}(\bar{q}_k^M, \bar{\pi}_R) = \bar{c}_k + \bar{e}(\bar{q}_k^M, \delta\bar{\pi}) = \bar{c}_k^R + \bar{d}_k + \bar{e}(\bar{q}_k^M, \delta\bar{\pi})$$

The machining error of target $k$, the deviation of the real tool position from the target on
the real edge, is:

$$\bar{\varepsilon}_k = \bar{d}_k + \bar{e}(\bar{q}_k^M, \delta\bar{\pi})$$

4. **When the two terms cancel.**
   - Least-squares plane and cylinder fits are translation-equivariant: if every recorded point
     is displaced by the same vector, the fitted surfaces, the edge and every target are
     displaced by exactly that vector.
   - If all measuring errors are equal, $\bar{e}(\bar{q}_j, \delta\bar{\pi}) = \bar{u}$ for every $j$,
     then every $\bar{m}_j$ is displaced by $-\bar{u}$, and $\bar{d}_k = -\bar{u}$. Then:

$$\bar{\varepsilon}_k = \bar{e}(\bar{q}_k^M, \delta\bar{\pi}) - \bar{u} = \bar{e}(\bar{q}_k^M, \delta\bar{\pi}) - \bar{e}(\bar{q}_j, \delta\bar{\pi})$$

5. **Result.** The machining error $\bar{\varepsilon}_k$ is the difference between the TCP
   position error at the machining joint coordinates $\bar{q}_k^M$ and the TCP position error at
   the measuring joint coordinates $\bar{q}_j$. With the first-order approximation:

$$\bar{\varepsilon}_k \approx (S(\bar{q}_k^M, \bar{\pi}_N) - S(\bar{q}_j, \bar{\pi}_N)) \cdot \delta\bar{\pi}$$

$S(\bar{q}, \bar{\pi}_N)$ is a continuous function of $\bar{q}$. This difference, and with it
$\bar{\varepsilon}_k$, therefore approaches zero as $\bar{q}_k^M$ approaches $\bar{q}_j$.

6. **Unequal errors.** If the measuring errors $\bar{e}(\bar{q}_j, \delta\bar{\pi})$ differ between
   points $j$, the fitted surfaces are also rotated, not only translated. $\bar{d}_k$ then also
   contains a rotation error, which grows with the distance of target $k$ from the measured
   points.

## 4. Global coordinate system versus local measurement (hypothetical values)

Assumed values, not measured: the error $\bar{e}(\bar{q}, \delta\bar{\pi})$ changes by 0.3 mm between
TCP positions 400 mm apart, which is $7.5 \cdot 10^{-4}$ mm per mm of TCP displacement.

- **Global:** 3-2-1 probing of a part 400 mm across, with the edge 300 mm from the frame origin.
  (3-2-1: three points on a primary plane, two on a secondary face, one on a third face.)
  - The errors of the measured frame points differ by up to 0.3 mm. The computed frame axes
    are therefore rotated by up to $0.3 / 400 = 7.5 \cdot 10^{-4}$ rad, which is 0.043°.
  - At the edge, 300 mm from the origin, this rotation alone gives
    $300 \cdot 7.5 \cdot 10^{-4}$ = 0.225 mm.
  - In addition, the error $\bar{e}(\bar{q}_k^M, \delta\bar{\pi})$ at the edge differs from the
    errors at the frame points by up to $300 \cdot 7.5 \cdot 10^{-4}$ = 0.225 mm.
- **Local:** probe points within 10 mm of the targets. The errors differ by up to
  $10 \cdot 7.5 \cdot 10^{-4}$ = 0.0075 mm.
  - This estimate takes only the TCP displacement into account. It assumes the tool orientation
    at machining equals the probe orientation at measuring, which section 5 shows is not the case.

Whether robotic deburring and chamfering setups generally measure per feature has not been
verified; the calculation above is the reason to do so, not an industry statistic.

## 5. Error terms that do not cancel

- **Different tools.** The Measuring EE TCP and the Spindle EE TCP are different elements of
  $\bar{\pi}$. Each is the transform matrix from its tool coordinate system T to the flange
  coordinate system F:

  $$
  {}^{F}A_T
  $$

  - The deviation of the Measuring EE TCP enters only the measuring errors
    $\bar{e}(\bar{q}_j, \delta\bar{\pi})$.
  - The deviation of the Spindle EE TCP enters only the machining error
    $\bar{e}(\bar{q}_k^M, \delta\bar{\pi})$.
  - Neither has a counterpart in the other operation, so both enter $\bar{\varepsilon}_k$ in full.
- **Different tool orientation.** The probe usually touches perpendicular to the surface, while
  the chamfer tool is tilted by the chamfer angle. The wrist joint coordinates in $\bar{q}_k^M$
  and $\bar{q}_j$ then differ by tens of degrees, and
  $S(\bar{q}_k^M, \bar{\pi}_N) - S(\bar{q}_j, \bar{\pi}_N)$ is not small in the columns of the wrist
  parameters.
- **Elastic deformation.** At the probe contact the process force is zero; during cutting the
  process force deflects the arm. That deflection exists only during machining.
- **Thermal expansion.** It changes $\delta\bar{\pi}$ between the time of measuring and the time
  of machining.
- **Repeatability scatter.** ±0.04 mm by datasheet, independently at measuring and at machining.
- **Probe radius compensation and fit residuals.**

## 6. Consequences for RoboCrap

1. Record the probe points of the surfaces that form the edge close to the targets $\bar{c}_k$.
   That makes the difference in section 3, step 5 small.
2. Determine the Measuring EE TCP and the Spindle EE TCP against a common reference, for example
   by touching the same reference point with both tools. Their deviations do not cancel and
   enter $\bar{\varepsilon}_k$ in full.
3. Machine soon after measuring. That keeps the change of $\delta\bar{\pi}$ from thermal expansion
   small.
