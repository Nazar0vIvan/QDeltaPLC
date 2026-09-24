# Rough surface measurements

For measurements made with the WP-500 V6 probe, store ball-center coordinates and
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
