# Adaptive straight LIN preview results

Scope: only Ps to clear lead-in and clear lead-out to Ps in ChamferMotion animation preparation. No change to HOME, lead-in, ellipse, lead-out, quintic maps, RSI sampling or correction encoding.

The old 1 ms transfer grid is replaced by one initial interval for each nonempty acceleration/cruise/deceleration section, followed by bounded temporal bisection. The existing quarter-time interpolated-joint FK tolerances remain 0.01 mm and 0.1 degrees. Sequential temporary IK witnesses at quarter times feed the existing sampled joint speed/finite-acceleration checks. Joint-acceleration magnitude rejection stays disabled in preview as before. These are sampled checks, not continuous mathematical guarantees. Right endpoints are recalculated from accepted preceding joints.

## Screenshot case, same Debug kit

| Quantity | Before | After |
|---|---:|---:|
| Incoming transfer computation | 5.245218 s | 0.008404 s |
| Outgoing transfer computation | 5.231783 s | 0.008534 s |
| Added animation points per transfer | 27,535 | 12 |
| IK calls per transfer | 27,535 | 69 |
| Explicit FK calls per transfer | 137,675 | 144 |
| Complete saved animation points | 132,067 | 77,021 |
| Complete Generate computation | 62.569265 s | 36.302630 s |

Each transfer now stores 13 poses including the shared start, i.e. adds 12. There are 49 transient rate poses including the start, and 21 adaptive interval evaluations. Rejected interval checks are included in the operation counts. The four incoming transfers plus the one outgoing transfer together now take approximately 0.043 s instead of 26.17 s. Numerical profiling includes validation; it does not measure UI rendering.

After-change total times: 36.2777941 and 36.3274669 s. Preview duration remains exactly 140.88378977663504 s; central duration remains exactly 76.77219975533788 s in the JSON reports. Phase intervals are [149,12,14248,48280,14248,12,71]. All four central IK counts and all three rejected-candidate errors match the baseline. RSI numerical preparation succeeds with 19,194 corrections; no controller limit profile is supplied, so encoding readiness is not established. No controller files or hardware are used.

Raw evidence: chamfer_profile_adaptive_lin.json; input: chamfer_profile_segments_input.json. Baseline: chamfer_profile_segments_debug.json. Build: existing Felgo Qt 6.8.3 / GCC 13.1 Debug, full robocrap build passed (existing deployment-policy warnings). git diff --check passed. Actual GUI playback was not exercised.

## Triangular transfer check

Passed with the same central settings, transfer feed 10 mm/s and transfer acceleration 1 mm/s². The approximately 26.535 mm transfers are too short to reach the requested feed, exercising the acceleration/deceleration junction without a cruise interval. Incoming preparation took 48.7221 ms and outgoing preparation took 47.8518 ms. Each added 64 animation points, used 382 IK calls and 824 explicit FK calls, and retained 257 temporary rate poses during validation. Preview and RSI numerical preparation both returned no error. Raw report: chamfer_profile_triangular_lin.json; input: chamfer_profile_triangular_lin_input.json. This was an offline numerical run, not a visual GUI playback check.
