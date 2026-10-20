# Post-merge cleanup and deferred work

Audit of the tree after merging RSI sampling (commits `7b3e82b`, `554db09`) onto the code
reduction base. Items marked **Done** are implemented (see "Implemented"); items marked
**Deferred** were postponed by the user on 2026-10-18 and are collected under "Deferred work",
together with the open items of the completed plans deleted that day. Line counts of open items
are estimates from reading the code, not measured diffs.

## Merge check result

- All nine completion searches of the code-reduction plan (deleted as completed on 2026-10-18)
  were still clean: circle import, STEP/cache loading, the profiler, `kr10.json`, manual FK/IK,
  geometry view classes, old singleton checks and window forwarding methods did not come back.
- No conflict markers. Every source file is listed in a `CMakeLists.txt`, and every CMake entry
  points at an existing file.
- The RSI rewrite removed `src/network`, `src/pathgeneration` and `src/scene`; Devices now
  links Geometry instead of Trajectories.
- The uncommitted local edits (SVGs, `ModeToolBar.qml`, `occviewcube.cpp`,
  `CMakeLists.txt.user`, untracked `create_edge.svg`) are outside this list.

## A. Dead because the legacy RSI was its only user

| # | Remove | Files | Reason | ≈ lines |
|---|---|---|---|---|
| A1 | **Done.** `Cylinder` class and `axisVec` | `geometry/include/geometry/cylinder.h`, `geometry/cylinder.cpp`, `geometry/utils.{h,cpp}`, `geometry/CMakeLists.txt`, `docs/ARCHITECTURE.md` | No code outside `cylinder.cpp` used `Cylinder` (`fromAxis` + `surfaceRing` pose rings for the old RSI/scan); `axisVec` was used only by `cylinder.cpp`. `BoundedCylinder` is now the only cylinder representation: "Cylinder from JSON" uses `decodeProbeSamples` → `BoundedCylinder::fromSamples` → `SceneModel::addBoundedCylinder`, which all stay. | −211 (measured) |
| A2 | **Done.** `Pose` class | `geometry/include/geometry/pose.h`, `geometry/pose.cpp`, `geometry/CMakeLists.txt`, `application/scene/sceneendeffectors.cpp`, `docs/ARCHITECTURE.md` | After A1 the only remaining use was `Pose::fromAxes(...).transform()` twice in `SceneEndEffectors`. That is `makeTransform(basis2rot(vecs2basis(t, b, n)), origin)` after a finiteness and `isBasis` check; a local helper with the same steps replaces it. Unused: `fromFrame`, `fromTransform`, `frame()`, `rot()`, `t()`/`b()`/`n()`, `poses2Frames`, `poses2Transforms`. | −164 net (measured) |
| A3 | **Done.** `Kr10Kinematics::solveIK(targetT06, currentQ)` | `kinematics/kr10kinematics.{h,cpp}`, `docs/AGENT_CONTRACTS.md`, `docs/ARCHITECTURE.md` | Checked IK kept for the old RSI consumer; no caller. New RSI uses `ChamferTcpSolver`. | −16 (measured) |
| A4 | **Done.** `ChamferMotion::evaluate(double)` and `ChamferPlaybackPose` | `trajectories/include/trajectories/chamfermotion.h`, `trajectories/chamfer/chamfermotion.cpp`, `docs/AGENT_CONTRACTS.md`, `docs/ARCHITECTURE.md` | TCP-returning evaluator kept for the old RSI job; no caller. Dry Run uses `evaluateJoints`. | −20 (measured) |
| A5 | **Done.** `Plane::explicitCoeffs` and stale includes | `geometry/include/geometry/plane.h`, `geometry/plane.cpp` | No caller. `plane.h` still included `QFile`, `QJsonDocument`, `QJsonParseError` from the removed `fromJsonFile`. | −17 (measured) |
| A6 | **Done.** Legacy settings fields `leadInFeed`/`leadOutFeed` and lead `intervals` (`leadInIntervals`/`leadOutIntervals`) | `chamfermotion.h`, `chamferpath.h`, `chamferpath.cpp`, `application/scene/scenegeometry.h`, `application/scene/scenemachining.cpp`, `docs/AGENT_CONTRACTS.md`, `docs/ARCHITECTURE.md` | Kept for frozen RSI/QML compatibility. Feeds were commented "no longer used for generation"; `intervals` was only validated and round-tripped. Neither QML nor the new RSI reads them. | −20 (measured) |

## B. Dead for other reasons

| # | Remove | Files | Reason | ≈ lines |
|---|---|---|---|---|
| B1 | **Done.** `EdgeGeometry::Ellipse::isCircle`, `BoundedPlane::axisV` with `m_axisV`, `BoundedCylinder::rmsResidual` with `m_rmsResidual` | `surfaceintersection.h`, `boundedplane.{h,cpp}`, `boundedcylinder.{h,cpp}` | Getters left after Milestone 7 with no caller. The RMS computation stays; it is the fit acceptance check. | −5 (measured) |
| B2 | **Deferred** (kept for future force-controlled motion). FTS signals `dataBatchReady` and `dataSampleHFReady` | `devices/include/devices/fts/ftsdevice.h:33-34`, `devices/fts/ftsdevice.cpp:291,422` | Both are emitted but have no receiver; the `FtsDevice → RsiDevice::setForce` link was removed with force correction. | −6 |
| B3 | **Deferred** (with B2). `RDTResponse` queued metatype registrations and the `rdtResponse` QML value type | `application/main.cpp:59-60`, `application/backendqmltypes.h:153-158` | After B2 no signal carries `RDTResponse`, and no QML file uses `rdtResponse`. Depends on B2. | −9 |
| B4 | **Done.** `main.cpp` leftovers | `application/main.cpp` | Debug window dump (`Root objects/Windows/Window:`), unreachable second `rootObjects().isEmpty()` check, unused `QWindow` include and duplicate `QStandardPaths` include. | −14 (measured) |
| B5 | **Done.** `MachiningPath.phaseTimes` Q_PROPERTY and getter | `application/scene/scenegeometry.h` | No QML reader. `ChamferCentralMotion::phaseTimes()` stays; it is used by validation and motion. | −8 (measured) |

## C. Needs a decision

| # | Candidate | Note |
|---|---|---|
| C1 | **Done.** `resources/json/circ/` sample files | Circle import no longer exists. |
| C2 | **Deferred.** Duplicated `rigid()`/`validRobot()` in `chamfermotion.cpp:15,96` and `chamfercentralvalidation.cpp:177,185` | Central validation re-checks a robot already checked by `ChamferMotion::create`, but also checks timing fields. Merge only after proving the conditions match. ≈ −30. |
| C3 | **Deferred.** `MotionCompiler::inverse` vs `ChamferTcpSolver::solveTcp` | Same IK selection; error texts and the `IgnorePreviewJointPositionLimits` handling differ. Reuse changes diagnostics. |
| C4 | **Deferred.** `FtsDevice::streamReset` and the commented chart-bridge block | `ftsdevice.cpp:232,246,261`, `main.cpp:77-79`. Only the retained `QmlChartBridge` would use them; the comment names `SocketFTS`/`bufferReady`, which no longer exist. Also decide B2 with this if force correction may return soon. |
| C5 | **Deferred.** Q_PROPERTY exposure with no QML reader | `OccController.ready/loading/errorString/viewportReady`, `MachiningPath.sourceEdgeId`, `DeviceRunner.socketState`, … Only the `Q_PROPERTY` lines go; C++ getters stay in use. |

## D. Stale documentation

| # | Location | Fix |
|---|---|---|
| D1 | **Done with A3.** `docs/AGENT_CONTRACTS.md:169` | Drop "Keep checked solveIK(target, previousAngles) compatible with separate RSI consumers" (with A3). |
| D2 | **Done** (for the deferred state). `docs/AGENT_CONTRACTS.md:35`, `docs/ARCHITECTURE.md:85,451,506` | `ARCHITECTURE.md:506` now lists all three unconnected FTS signals and says they and the `RDTResponse` metatypes are kept for future force-controlled motion. The `rdtResponse` statements stay correct while B3 is deferred. |
| D3 | **Done.** RSI planning, now consolidated in `codex_plans/RSI_EXECPLAN.md` | Removed the obsolete profiler measurement step. |
| D4 | **Done.** `AGENTS.md` routing table | "CAD/cache loading" is now "CAD loading". |

## E. Filesystem leftovers

| # | Remove | Reason |
|---|---|---|
| E1 | **Done.** Empty folder `src/3d/` (contains only the empty `src/3d/math/`) | Its files were moved out in `9a40e08`. Git tracks no files there, and no CMake file references it, so `git status` and file searches do not show it. Delete it from disk; there is nothing to commit. It is the only empty folder outside build, libs, python and old* folders. |

## Keep

- `QmlChartBridge`: retained by request.
- `IgnorePreviewJointPositionLimits` and the commented joint-acceleration checks: deliberate.
- `resources/json/edge_2planes`: input for future straight edges.
- RSI resource files: in use.
- `ChamferTcpSolver`: already the shared IK check for central validation and RSI sampling.
- `decodeProbeSamples`/`ProbeSamples`: used by `SceneSurfaceImporter`.

## Deferred work

Not scheduled. Reopen any item only on an explicit user request.

From this audit (deferred 2026-10-18):
- B2, B3: unconnected FTS signals and the `RDTResponse` metatypes/QML value type. Kept for
  future force-controlled motion.
- C2: merge the duplicated `rigid()`/`validRobot()` checks (`chamfermotion.cpp`,
  `chamfercentralvalidation.cpp`) after proving the conditions match.
- C3: reuse `ChamferTcpSolver::solveTcp` in `MotionCompiler::inverse`; error texts and the
  `IgnorePreviewJointPositionLimits` handling differ.
- C4: `FtsDevice::streamReset` and the stale commented chart-bridge block in `main.cpp:77-79`.
- C5: Q_PROPERTY declarations with no QML reader.

From the completed architecture plan and review (both deleted 2026-10-18; deferred by the user
on 2026-10-16):
- Typed PLC codec and pending records (review finding F6, proposal P7). PLC data is converted
  repeatedly through untyped dictionaries. Replacement: typed requests/pending records with
  correlation, keeping module 1/2 and bit 0-7 validation, the six-byte header, 32-byte response
  extraction and byte swapping. A deadline and late-response policy is required before
  recycling request IDs.
- One pending FTS sample (review O5, proposal P8). Every accepted packet is appended to
  `FtsDevice::m_batch`, but `processBatch` uses only the last sample for recording and GUI
  publication. Replacement: one optional last sample, keeping per-packet timestamps, the
  `dataSampleHFReady` emission, the 16 ms packet-driven gate, the deadband and the recording rules.
- Analytical cylinder-fit Jacobian (review O6). `BoundedCylinder` builds its Jacobian from five
  perturbed residual passes over all samples per iteration, for up to six seed axes. Replacing it
  changes fit numerics and needs a separate numerical comparison on accepted and near-degenerate
  datasets.
- Device operation results and QML error display: a returned operation-status contract and
  error properties for device commands, instead of console-only diagnostics.

## Implemented

### A1 — `Cylinder` and `axisVec`

Changes:
- Deleted `src/geometry/include/geometry/cylinder.h` and `src/geometry/cylinder.cpp`, including
  their file-local helpers `isValidInput`, `leastParallelUnit`, `radialAxis`,
  `projectedWorldAxis`, `projectedRef` and `basisFromAxis`.
- Removed both files from `qt_add_library(RoboCrapGeometry ...)` in `src/geometry/CMakeLists.txt`.
- Removed `axisVec` from `src/geometry/include/geometry/utils.h` and `src/geometry/utils.cpp`.
- Removed the "`Cylinder::fromAxis`: analytic, not a fit" line from `docs/ARCHITECTURE.md`.
  Historical plans and reviews in `codex_plans/` that mention `cylinder.h` are left as history.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/geometry/cylinder.cpp` | 170/126 | deleted |
| `src/geometry/include/geometry/cylinder.h` | 31/20 | deleted |
| `src/geometry/utils.cpp` | 292/227 | 283/219 |
| `src/geometry/include/geometry/utils.h` | 41/32 | 40/31 |
| `src/geometry/CMakeLists.txt` (build input) | 22/22 | 20/20 |
| `docs/ARCHITECTURE.md` (documentation) | 513 | 512 |

C++: −211 physical, −155 nonblank lines, 2 files fewer. Build inputs: −2. Documentation: −1.

Compliance review:
- Declarations: removed `Cylinder` (with `fromAxis`, `surfaceRing`, `originPose`, `axisDir`,
  `radius`, `axis`) and `axisVec`. No declaration or signature was added or changed.
- Bodies: only deletions. No helper, class, field, lambda, template or exception was added.
- Callers: searches of `src` for `Cylinder::`, `cylinder.h` and `axisVec` find nothing. `Axis` is
  still used by `makeRotation` and `chamferpath.cpp`. `pose.h` is still included directly by
  `sceneendeffectors.cpp`; no file received it through `cylinder.h`.
- Includes: `utils.h` includes are unchanged; none was needed only by `axisVec`.
- No violations found. Not built (no build was requested).

### A2 — `Pose`

Changes:
- Deleted `src/geometry/include/geometry/pose.h` and `src/geometry/pose.cpp` and removed them
  from `qt_add_library(RoboCrapGeometry ...)`.
- `src/application/scene/sceneendeffectors.cpp`: removed the `geometry/pose.h` include and added
  the file-local `frameTransform(const M3d& axes, const V3d& origin)`. It rejects a nonfinite
  origin, then runs `vecs2basis` on the three axis columns (normalizes them; rejects a near-zero
  or nonorthonormal set), `basis2rot` and `makeTransform`.
  - `spindleColletTransform` builds the axis matrix with columns `y × z`, `y`, `z` and calls it
    with the fixed ER collet origin.
  - `spindleTcpInCollet` calls it with the rotation block and translation column of
    `colletToTcp`, then passes the result to the unchanged `transformPose`.
- `docs/ARCHITECTURE.md`: removed the `Pose` geometry entry; the Eigen dependency note now names
  `utils.h` instead of `Pose`.

Helper requirement: both call sites need the same normalized, validated frame construction that
`Pose::fromAxes` provided. No existing `utils` function combines basis validation with an
origin into a transform, and inlining the steps twice repeats the same failure handling.

Behavior: `Pose::fromRotAndOrigin` additionally checked `rot.allFinite()` and repeated `isBasis`
on the already validated basis; `vecs2basis` already rejects nonfinite vectors through
`normalize`, and the second `isBasis` call used the same vectors and tolerance. Its `rot2euler`
result was never read at these call sites. Results and rejected inputs are unchanged.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/geometry/pose.cpp` | 132/95 | deleted |
| `src/geometry/include/geometry/pose.h` | 39/29 | deleted |
| `src/application/scene/sceneendeffectors.cpp` | 115/98 | 122/104 |
| `src/geometry/CMakeLists.txt` (build input) | 20/20 | 18/18 |
| `docs/ARCHITECTURE.md` (documentation) | 512 | 511 |

C++: −164 physical, −118 nonblank lines, 2 files fewer. Build inputs: −2. Documentation: −1.
The 7-line growth in `sceneendeffectors.cpp` is the helper; the smaller alternative of inlining
its four steps at both call sites repeats them.

Compliance review:
- Declarations: removed `Pose` and `poses2Frames`/`poses2Transforms`. Added one file-local
  function with two input parameters, returning `std::optional<M4d>`; no output references.
  The public `Pose` API was removed by request because it had no remaining callers.
- Bodies: no lambda, template, exception or new state.
- Callers: searches of `src` and the docs for `pose.h`, `Pose::` and `poses2` find nothing.
  `sceneendeffectors.cpp` still gets `M3d`/`V3d`/`M4d` and the used functions through
  `geometry/utils.h`. "Pose failures" in `docs/ARCHITECTURE.md` refers to robot poses and stays.
- No violations found. Not built (no build was requested).

### A3 — `Kr10Kinematics::solveIK` (with D1)

Changes:
- `src/kinematics/include/kinematics/kr10kinematics.h`: removed the public `solveIK(T06,
  currentQ)` declaration and its comment.
- `src/kinematics/kr10kinematics.cpp`: removed its definition (validity check, FK on the
  previous angles, `configuration`, `calculateIK`, second FK and `matchesFlange`).
- `docs/AGENT_CONTRACTS.md:169`: replaced the "keep checked solveIK" rule with "There is no
  angles-only checked IK; callers carry the accepted IkSolution as seed."
- `docs/ARCHITECTURE.md:120`: `matchesFlange` is now described as used by motion generation,
  central validation and RSI sampling (`ChamferTcpSolver`) after their own FK.
  Historical plans in `codex_plans/` that mention `solveIK` are left as history.

Why no caller remains: the last callers were the old RSI sampler
(`src/pathgeneration/chamfer/chamfercartesiansampler.cpp`, deleted in `7b3e82b`) and the
profiler wrapper `ChamferProfiledKinematics` (deleted by the code reduction). Current callers
carry the full `IkSolution` (angles, Status, Turn) between targets and call `calculateIK`
directly, so rebuilding Status/Turn from angles is not needed.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/kinematics/include/kinematics/kr10kinematics.h` | 89/72 | 87/70 |
| `src/kinematics/kr10kinematics.cpp` | 317/236 | 303/223 |
| `docs/AGENT_CONTRACTS.md` (documentation) | 314 | 314 |
| `docs/ARCHITECTURE.md` (documentation) | 511 | 511 |

C++: −16 physical, −15 nonblank lines. Documentation: two lines reworded.

Compliance review:
- Declarations: removed one public member function by request (no callers). Nothing added or
  changed; `calculateIK`, `configuration`, `matchesFlange` and `solveFK` keep their signatures.
- Bodies: only deletion. `qWarning` is still used by `calculateIK`, so includes stay.
- Callers: searches of `src`, `docs` and `AGENTS.md` for `solveIK` find nothing. `matchesFlange`
  callers are `chamfermotion.cpp:326` and `chamfertcpsolver.h:50`.
- No violations found. Not built (no build was requested).

### A4 — `ChamferMotion::evaluate` and `ChamferPlaybackPose`

Changes:
- `src/trajectories/include/trajectories/chamfermotion.h`: removed the `ChamferPlaybackPose`
  struct (joints, TCP, phase) and the public `evaluate(double seconds)` declaration.
- `src/trajectories/chamfer/chamfermotion.cpp`: removed its definition. It called
  `evaluateJoints(seconds)`, returned the saved first/last TCP at the ends, and otherwise ran
  one `solveFK` on the interpolated angles and multiplied by `flangeToTcp`.
- `docs/AGENT_CONTRACTS.md:226`: removed "Legacy evaluate(seconds) keeps its TCP result and saved
  endpoint matrices for existing consumers, sharing the same interpolation code."
- `docs/ARCHITECTURE.md:161`: the whole-motion row now names `evaluateJoints(seconds)` (no FK or
  IK) instead of `evaluate(seconds)`, and its stale member citations now read
  `chamfermotion.h:69-73`, `chamfercentralmotion.h:58`.

Why no caller remains: the old RSI job used it to obtain TCP poses from the saved motion. Dry
Run uses `evaluateJoints`, and `RobotController::applyPlayback` runs the FK and TCP check itself.
`m_motion->evaluate(seconds)` in `chamfercentralvalidation.cpp:387` is
`ChamferCentralMotion::evaluate`, a different function, and stays.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/trajectories/include/trajectories/chamfermotion.h` | 90/79 | 82/72 |
| `src/trajectories/chamfer/chamfermotion.cpp` | 717/675 | 705/664 |
| `docs/AGENT_CONTRACTS.md` (documentation) | 314 | 313 |
| `docs/ARCHITECTURE.md` (documentation) | 511 | 511 |

C++: −20 physical, −18 nonblank lines. Documentation: −1 line, one row reworded.

Compliance review:
- Declarations: removed one public struct and one public member function by request (no
  callers). `evaluateJoints` and every other signature is unchanged.
- Bodies: only deletion. `Kr10Kinematics` and `<utility>` (`std::pair` in `evaluateJoints`) are
  still used, so includes stay.
- Callers: searches of `src`, `docs` and `AGENTS.md` for `ChamferPlaybackPose`,
  `ChamferMotion::evaluate(` and `evaluate(seconds)` find only the `ChamferCentralMotion` call.
- Unrelated: `src/trajectories/chamfer/chamfertcpsolver.h` has a working-tree change not made
  in this cleanup (the `solveTcp` signature joined onto one tab-indented line). It was left as is.
- No violations found. Not built (no build was requested).

### A5 — `Plane::explicitCoeffs` and stale includes

Changes:
- `src/geometry/include/geometry/plane.h`: removed the `explicitCoeffs(eps)` declaration and the
  `<QFile>`, `<QJsonDocument>`, `<QJsonParseError>` includes.
- `src/geometry/plane.cpp`: removed the `explicitCoeffs` definition. It rewrote the plane
  $a x + b y + c z + d = 0$ as $z = p x + q y + r$, returning $(p, q, r) = (-a/c, -b/c, -d/c)$, or
  no value when $|c| \le$ `eps`.
- No documentation mentioned `explicitCoeffs`.

Why the includes are safe to remove: `plane.h` is included only by `plane.cpp` and
`boundedplane.cpp`, and neither uses `QFile` or any `QJson*` type. `plane.cpp` still has `<cmath>`
for `std::abs`/`std::isfinite`, and `<optional>`/`<QVector>` remain for `fromPoints`.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/geometry/include/geometry/plane.h` | 21/15 | 17/11 |
| `src/geometry/plane.cpp` | 71/53 | 58/42 |

C++: −17 physical, −15 nonblank lines.

Compliance review:
- Declarations: removed one public member function by request (no callers). `fromPoints` and
  `normal` are unchanged.
- Bodies: only deletion; nothing added.
- Callers: searches of `src`, `docs` and `AGENTS.md` for `explicitCoeffs` find nothing.
- No violations found. Not built (no build was requested).

### A6 — legacy lead feeds and lead intervals

Changes:
- `src/trajectories/include/trajectories/chamfermotion.h`: removed `leadInFeed` and
  `leadOutFeed` (default 10 mm/s each) from `ChamferTimingParameters`.
- `src/trajectories/include/trajectories/chamferpath.h`: removed `intervals` (default 16) from
  `ChamferLeadParameters`.
- `src/trajectories/chamfer/chamferpath.cpp`: `validLead` no longer checks
  `0 < intervals <= 100000`. The `ChamferPath::create` error text is now "Leads require positive
  axial clearance and span in (0, 360] degrees."
- `src/application/scene/scenegeometry.h`: removed the `leadInIntervals`, `leadOutIntervals`,
  `leadInFeed`, `leadOutFeed` Q_PROPERTYs and members from the `SceneMachiningSettings` gadget.
- `src/application/scene/scenemachining.cpp`: removed their copies in `fromMotion`, `applyTo`
  and `timing`.
- `docs/AGENT_CONTRACTS.md:201`: removed "Keep saved lead intervals for QML compatibility."
- `docs/ARCHITECTURE.md:159`: the central timing row now says there are no separate lead feeds.

Behavior: the only values these fields ever had were the C++ defaults (16 intervals, 10 mm/s).
QML reads/writes only `flipAxis` of the settings gadget (`MachiningPanel.qml:45,67`), and no
saved file stores them. So the removed interval check could never fail, and no generation,
validation, playback or RSI calculation read either field. The only visible difference is the
shorter error text above.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/trajectories/include/trajectories/chamfermotion.h` | 82/72 | 80/70 |
| `src/trajectories/include/trajectories/chamferpath.h` | 99/85 | 98/84 |
| `src/trajectories/chamfer/chamferpath.cpp` | 346/321 | 345/320 |
| `src/application/scene/scenegeometry.h` | 118/112 | 110/104 |
| `src/application/scene/scenemachining.cpp` | 420/391 | 412/383 |
| `docs/AGENT_CONTRACTS.md` (documentation) | 313 | 312 |
| `docs/ARCHITECTURE.md` (documentation) | 511 | 511 |

C++: −20 physical, −20 nonblank lines. Documentation: −1 line, one sentence reworded.

Compliance review:
- Declarations: removed public struct fields and four QML-visible gadget properties by request
  (no QML or C++ reader). No signature changed; nothing added.
- Bodies: only deletions plus the shortened error string; evaluation order of the remaining
  `validLead` checks is unchanged.
- Callers: searches of `src`, `docs` and `AGENTS.md` for `leadInFeed`, `leadOutFeed`,
  `leadInIntervals`, `leadOutIntervals` and `lead*.intervals` find nothing. Other `intervals`
  names (`chamfermotion.cpp`, `chamfercentralvalidation.*`, `chamferarclength.cpp`) are local
  sampling counts and unrelated.
- No violations found. Not built (no build was requested).

### B1 — unused geometry getters

Changes:
- `src/geometry/include/geometry/surfaceintersection.h`: removed `Ellipse::isCircle()`
  (`majorRadius == minorRadius`). `Ellipse` stays an aggregate.
- `src/geometry/include/geometry/boundedplane.h`, `boundedplane.cpp`: removed `axisV()` and
  `m_axisV`. The private constructor's `frame` is now `std::array<Point, 2>` (origin, U axis);
  `fromPoints` no longer passes the V axis. The local `v` still computes the V bounds and the
  centered origin.
- `src/geometry/include/geometry/boundedcylinder.h`, `boundedcylinder.cpp`: removed
  `rmsResidual()`, `m_rmsResidual` and the private constructor's `residual` input.
  `fromPoints` still computes `rms` and rejects the fit when `rms > max(0.1 mm, 2 % R)`.

Why the storage went too: `m_axisV` and `m_rmsResidual` were written only by the constructors
and read only by the removed getters. Keeping them would leave dead state.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/geometry/include/geometry/surfaceintersection.h` | 40/32 | 39/31 |
| `src/geometry/include/geometry/boundedplane.h` | 39/33 | 37/31 |
| `src/geometry/boundedplane.cpp` | 92/79 | 92/79 |
| `src/geometry/include/geometry/boundedcylinder.h` | 36/30 | 34/28 |
| `src/geometry/boundedcylinder.cpp` | 258/233 | 258/233 |

C++: −5 physical, −5 nonblank lines; two members and one constructor input fewer. No
documentation mentioned these getters.

Compliance review:
- Declarations: removed three public getters by request (no callers). The private
  `BoundedCylinder` constructor goes from four to three inputs; the remaining three are distinct
  concepts (samples, axis frame, radius/length). The private `BoundedPlane` constructor keeps its
  four inputs (samples, coefficients, frame, bounds), unchanged in count; only `frame` narrows.
- Bodies: only deletions in initializer lists and call arguments; fit acceptance unchanged.
- Callers: searches of `src`, `docs` and `AGENTS.md` for `isCircle`, `axisV`, `m_axisV` and
  `rmsResidual` find nothing. Both constructors are private and called only by `fromPoints`.
- No violations found. Not built (no build was requested).

### B4 — `main.cpp` leftovers

Changes in `src/application/main.cpp`:
- Removed the debug dump after `loadFromModule` (`Root objects:`, `Windows:` and one
  `Window:` line per `QGuiApplication::allWindows()` entry).
- Removed the second `if (engine.rootObjects().isEmpty()) return -1;`. The first check already
  returned 77 for that condition, so it was unreachable.
- Removed `#include <QWindow>` and the unconditional `#include <QStandardPaths>`. The hot-reload
  block, the only `QStandardPaths` user, keeps its own include under `USE_FELGO_HOT_RELOAD`.
- Kept the `RDTResponse` metatypes (B3 deferred) and the commented chart-bridge block (C4
  deferred).

### B5 — `MachiningPath.phaseTimes`

Removed the `phaseTimes` Q_PROPERTY and its getter (the times of the eight saved phase
boundaries) from `SceneMachiningPath` in `src/application/scene/scenegeometry.h`. No QML file
reads it. `ChamferCentralMotion::phaseTimes()` is a different function and stays.

### C1 and E1 — files and folders

- Deleted `resources/json/circ/circle_12_noisy_points.json`, `circle_3_points.json` and the
  folder. No CMake, `.qrc`, QML or C++ file referenced them; only the deleted plans did.
- Deleted the empty folders `src/3d/math/` and `src/3d/` (untracked by Git).

### Documentation and plans

- `AGENTS.md`: routing table says "CAD loading" (D4).
- `docs/ARCHITECTURE.md:506`: FTS signal/metatype statement (D2).
- RSI planning (now `codex_plans/RSI_EXECPLAN.md`): removed the profiler measurement step
  (D3); completed and superseded RSI plans were consolidated on 2026-10-20.
- `codex_plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md`: corrected the claim that
  `spatial_leads_curvature_step1.json` sits next to it; that file was never committed.
- Deleted as completed and superseded by `docs/ARCHITECTURE.md`, `docs/AGENT_CONTRACTS.md` and
  Git history: `ARCHITECTURE_EXECPLAN.md` (1392 lines), `ARCHITECTURE_REVIEW.md` (735),
  `CODE_REDUCTION_EXECPLAN.md` (645) and `architecture_baseline/` (a 2026-10-14 snapshot of the
  pre-restructure tree: two JSON inventories, `SOURCE_CONTRACTS.md` and copies of `XP.json`,
  `XC.json` and the removed `kr10.json`; the XP/XC copies are byte-identical to
  `resources/json/edge_cyl_plane/`). Their open items are under "Deferred work".
- Kept: `RSI_EXECPLAN.md` (offset/configuration repair and verification remain;
  pause/resume is deferred), the research
  documents (`CHAMFER_ARCLENGTH_STUDY.md` is cited by `docs/ARCHITECTURE.md`,
  `SPATIAL_LEADS_CURVATURE_STEP1_RU.md` by `docs/AGENT_CONTRACTS.md`, and the lead-geometry and
  animation literature reviews remain background for future edge types) and this file.

Line counts (physical/nonblank):

| File | Before | After |
|---|---|---|
| `src/application/main.cpp` | 168/124 | 154/113 |
| `src/application/scene/scenegeometry.h` | 110/104 | 102/96 |

C++: −22 physical, −19 nonblank lines.

Compliance review:
- Declarations: removed one QML-visible property and its public getter (no reader). No signature
  changed; nothing added.
- Bodies: only deletions. `main.cpp` still uses `<QDebug>` for `qWarning`.
- Callers: no `phaseTimes` remains under `src/application`; `QWindow`, `QGuiApplication` and the
  debug strings no longer appear in `main.cpp`.
- No violations found. Not built (no build was requested).

## Totals

All requested items are done. Measured C++ change from this cleanup: A1 −211, A2 −164, A3 −16,
A4 −20, A5 −17, A6 −20, B1 −5, B4 −14, B5 −8, for −475 physical lines. Remaining work is listed
under "Deferred work".
