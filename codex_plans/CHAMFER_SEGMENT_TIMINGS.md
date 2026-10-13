# Animation preparation: seven-segment timing breakdown

Two uninterrupted offline runs of the same XP/XC screenshot case, Debug Qt 6.8.3 / MinGW 13.1. All times below are computation times, not simulated movement durations. Input: chamfer_profile_segments_input.json. Raw evidence: chamfer_profile_segments_debug.json. No algorithm, sampling density, IK order, tolerances or limits changed.

Attempt 1–4 means calculation using the first through fourth joint-angle sets at Ps, in the actual search order. Attempts 1–3 fail during machining; attempt 4 supplies the animation. A dash means not reached. A partial machining time includes work up to its failure.

| Segment | Attempt 1 | Attempt 2 | Attempt 3 | Attempt 4 |
|---|---:|---:|---:|---:|
| HOME to Ps | 0.001292 s | 0.001090 s | 0.002702 s | 0.003264 s |
| Ps to lead-in start | 5.233636 s | 5.215930 s | 5.243440 s | 5.245218 s |
| Lead-in | 3.082706 s | 3.073951 s | 3.051184 s | 3.039929 s |
| Machining ellipse | 1.461460 s (partial) | 6.065502 s (partial) | 1.402603 s (partial) | 9.675135 s |
| Lead-out | — | — | — | 3.033499 s |
| Lead-out end to Ps | — | — | — | 5.231783 s |
| Ps to HOME | — | — | — | 0.001555 s |

The segment rows measure pose generation and its local validation. HOME rows include generating joint poses and FK; HOME timestamps are assigned later. Central segment rows include target evaluation, IK/FK, continuity/speed and interpolation checks. The whole-central acceleration/rate pass is separate because it runs after all three central phases complete.

| Additional work | Attempt 1 | Attempt 2 | Attempt 3 | Attempt 4 |
|---|---:|---:|---:|---:|
| Analytical maps and central timing | 0.118831 s | 0.117019 s | 0.117167 s | 0.117742 s |
| Assemble central points, including repeat FK | — | — | — | 1.629874 s |
| Whole-central rate checks | — | — | — | 0.164686 s |
| Whole-motion pose/position checks | — | — | — | 0.216098 s |

| Attempt | Total computation time |
|---|---:|
| 1 | 9.898434 s |
| 2 | 14.474115 s |
| 3 | 9.817551 s |
| 4 | 28.359678 s |

Total Generate numerical preparation: **62.569265 s** (runs 62.594677 and 62.543853 s). Finding the four Ps joint sets takes 2.959500 ms. Assigning both HOME time laws, shifting timestamps and checking timestamp/velocity finiteness takes 15.925350 ms after successful compilation.

## Per-segment operation counts

Counts are identical across both repetitions. Added points excludes the shared starting endpoint. Each attempt additionally evaluates the initial HOME pose once with FK and the initial central pose once with IK/FK. Central assembly adds 76,776 explicit FK calls only in the successful attempt. FK counts below exclude calls inside IK.

| Attempt | Segment | Added points | IK calls | Explicit FK calls |
|---|---|---:|---:|---:|
| 1 | HOME to Ps | 59 | 0 | 59 |
| 1 | Ps to lead-in start | 27,535 | 27,535 | 137,675 |
| 1 | Lead-in | 14,248 | 14,248 | 56,992 |
| 1 | Machining ellipse (partial) | 7,300 | 7,304 | 29,204 |
| 2 | HOME to Ps | 50 | 0 | 50 |
| 2 | Ps to lead-in start | 27,535 | 27,535 | 137,675 |
| 2 | Lead-in | 14,248 | 14,248 | 56,992 |
| 2 | Machining ellipse (partial) | 30,360 | 30,364 | 121,444 |
| 3 | HOME to Ps | 122 | 0 | 122 |
| 3 | Ps to lead-in start | 27,535 | 27,535 | 137,675 |
| 3 | Lead-in | 14,248 | 14,248 | 56,992 |
| 3 | Machining ellipse (partial) | 7,004 | 7,006 | 28,018 |
| 4 | HOME to Ps | 149 | 0 | 149 |
| 4 | Ps to lead-in start | 27,535 | 27,535 | 137,675 |
| 4 | Lead-in | 14,248 | 14,248 | 56,992 |
| 4 | Machining ellipse | 48,280 | 48,280 | 193,120 |
| 4 | Lead-out | 14,248 | 14,248 | 56,992 |
| 4 | Lead-out end to Ps | 27,535 | 27,535 | 137,675 |
| 4 | Ps to HOME | 71 | 0 | 71 |

## Actual joint angles at Ps

All values are degrees, in A1, A2, A3, A4, A5, A6 order. HOME is the same for all attempts.

| Attempt | A1 | A2 | A3 | A4 | A5 | A6 |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 8.405969 | -63.184819 | 125.133010 | -81.424400 | 93.728644 | -117.322056 |
| 2 | 8.405969 | -63.184819 | 125.133010 | 98.575600 | -93.728644 | 62.677944 |
| 3 | 8.405969 | -63.184819 | 125.133010 | -81.424400 | 93.728644 | 242.677944 |
| 4 | 8.405969 | -63.184819 | 125.133010 | 98.575600 | -93.728644 | -297.322056 |

## Accounting and verification

The elapsed segment intervals are disjoint. Analytical construction, assembly and whole-vector validation are also disjoint from them. Initial pose evaluations and function/diagnostic bookkeeping explain the small per-attempt remainder. Overall Generate additionally contains configuration search, HOME timestamp assignment and inter-attempt cleanup. Do not add IK/FK internal timings to segment elapsed times: they are already included.

Run 1 attempt 1: recorded disjoint work 9896.002400 ms, attempt total 9896.294600 ms, unassigned bookkeeping 0.292200 ms.
Run 1 attempt 2: recorded disjoint work 14506.773900 ms, attempt total 14507.239000 ms, unassigned bookkeeping 0.465100 ms.
Run 1 attempt 3: recorded disjoint work 9815.859400 ms, attempt total 9816.106600 ms, unassigned bookkeeping 0.247200 ms.
Run 1 attempt 4: recorded disjoint work 28355.903500 ms, attempt total 28356.533900 ms, unassigned bookkeeping 0.630400 ms.

The final preview still has 132,067 points and 301,900 total IK / 1,422,375 explicit FK calls across attempts, matching the preceding baseline. Failure messages and phase counts are unchanged. Full robocrap Debug build succeeded, offline process exited 0, and git diff --check passed. Build emitted the existing CMake deployment-policy warnings. No equipment connections or shared_data.txt writes occurred.
