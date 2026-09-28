# Rough surface measurements

For plane and cylinder measurements made with the WP-500 V6 probe, store ball-center coordinates and
stylus ball radius in the same units (millimeters for the supplied datasets):

```json
{
  "radius": 3,
  "dir": 1,
  "points": [
    [0, 0, 10],
    [20, 0, 10],
    [0, 20, 10]
  ]
}
```

`radius` is the ball **radius**, not diameter. It must be a finite nonnegative
number. `dir` must be the number `1` or `-1`. Both fields are required in the object
format. Zero radius explicitly disables compensation. Each point must contain
three finite coordinates, already expressed in the scene coordinate system;
the importer does not transform robot flange poses into probe-center positions.

The importer fits the original ball centers, then applies the correction once:

- **Plane:** move the surface by `dir * radius` along its fitted unit normal.
  The largest-magnitude normal coordinate is positive, with exact ties preferring
  X, then Y, then Z. For `n · p + d = 0`, the corrected offset is
  `d - dir * radius`. The bounded rectangle moves with the plane.
- **Cylinder:** the corrected radius is `fittedRadius + dir * radius`.
  `dir: 1` expands the surface (typical internal/bore probing);
  `dir: -1` contracts it (typical external probing).
  A corrected radius at or below the geometry tolerance (`1e-9`) is rejected.
  The axis, midpoint, and length remain unchanged.

For planes, choose `dir` by the required movement from ball centers toward the
surface relative to the displayed normal. The normal's sign is determined by the
coordinates, not by which side was probed. The example above produces `z = 13`;
using `dir: -1` instead produces `z = 7`.

Show Points and Show Normals remain anchored at the original measured ball
centers. They need not lie on the corrected surface. Cylinder fit residuals still
describe the original fit before compensation.

Legacy JSON point arrays remain supported with zero compensation. Direct C++
`fromPoints` calls also continue fitting the supplied points without compensation.

The two `rough-*-sample.json` files and all four files in `edge_2planes` and
`edge_cyl_plane` use `radius: 3` and `dir: 1`. Change `dir` to `-1` wherever the
required correction is in the opposite direction.

## Circle samples

Import the files in `circ` using **Circle from JSON**:

- `circle_3_points.json` contains three exact, non-collinear points. The resulting
  circle has center `(100, 200, 300)`, radius `50`, and unit normal `(0, -0.6, 0.8)`.
  It should pass through all three points, with fit RMS effectively zero.
- `circle_12_noisy_points.json` contains twelve points distributed around the same
  reference circle, with small radial and out-of-plane offsets. The fitted center,
  radius and normal should be close to the reference values, with a nonzero RMS.
  RMS is the root mean square of the 3D distances from samples to the circumference.

These files contain raw `[x, y, z]` arrays without probe metadata. Circle imports
also accept an object containing `points`; `radius` and `dir` are not used for
circle fitting or compensation. Show Points displays the original samples;
Show Normals displays one normal arrow at the fitted center.
