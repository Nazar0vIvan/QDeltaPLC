# Add TCP arc-length lookup for analytical chamfer phases

This living ExecPlan follows PLANS.md and implements authorized step 3 only.

## Purpose / Big Picture

Later timing code needs to convert a travelled distance in millimetres into a position on each analytical lead-in, machining and lead-out curve. This step provides that conversion without using animation joints or timestamps. Existing playback and RSI preparation remain unchanged until their separate timing steps.

## Progress

- [x] (2026-10-09) Read geometry equations, numerical conventions, existing sampling/timing callers, CMake ownership and repository instructions.
- [x] (2026-10-09) Added independent, bounded arc-length tables using analytical TCP origins and registered them in RoboCrapGeometry.
- [x] (2026-10-09) Reviewed forward/inverse lookup, exact endpoints/knots, failure paths, CMake registration and documentation. Tracked and new-file whitespace checks passed; no build or runtime validation was performed.

## Surprises & Discoveries

ChamferPath::evaluatePhase describes lead quarter circles in angular/axial coordinates, not necessarily circles in XYZ. Existing ChamferPath::sample controls pose interpolation error, which does not directly control length error or distance-to-parameter error. A closed machining curve has equal endpoints but positive length; endpoint separation must not determine degeneracy.

## Decision Log

On 2026-10-09, use adaptive polygon lengths evaluated through the existing analytical pose function. This avoids duplicating geometry or differentiating orientation. Compare two-chord and four-chord lengths per interval, allocating an absolute length-error budget by parameter width. Independently compare intermediate cumulative distances with linear distance interpolation to resolve nonuniform progress even on nearly straight portions. Keep the finer four chords when accepted. Both tolerances default to 0.00001 mm; these are numerical refinement criteria, not analytic guarantees.

On 2026-10-09, build one immutable value per phase, with checked optional lookups and exact endpoint mappings. Seed 16 intervals to resolve the full turn before refinement. Bound recursion at 20 levels, retained knots at 200000, and analytical evaluations at 800000 per phase. Reject invalid poses, nonfinite distances, unresolved precision and insufficient refinement rather than returning a partial table. No timing integration is included in this milestone.

## Context and Orientation

src/pathgeneration/chamfer/chamferpath.h/.cpp defines ChamferPath. evaluatePhase takes a ChamferPhase and a dimensionless progress between zero and one and returns an analytical BASE-relative TCP pose. Only its translation column contributes to arc length: the accumulated distance of the TCP origin, in millimetres. Orientation still comes from the same evaluatePhase call when a future caller evaluates the recovered progress. chamfermotion.cpp currently produces animation timing, while chamfercartesiansampler.cpp projects timed FK poses to analytical curves. Neither changes here. src/CMakeLists.txt owns RoboCrapGeometry, where the new numerical files belong.

## Plan of Work

Create chamferarclength.h/.cpp beside chamferpath. Implement a value factory, total length, progress-to-distance and distance-to-progress methods. A local builder samples analytical origins and subdivides until both numerical criteria pass. Store strictly increasing progress and cumulative distance. Use binary search and piecewise linear interpolation for both inverse operations on the same table. Register both files in RoboCrapGeometry and document their boundary in AGENTS.md.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Inspect affected source with Get-Content and rg. After edits run git diff --check and read the new files explicitly because untracked files are absent from ordinary diffs. Check new-file whitespace separately. Do not add test logic or run builds without the separately required user request.

## Validation and Acceptance

Review that all three phases use evaluatePhase, no animation samples or time values enter the builder, and only TCP XYZ contributes to lengths. Verify mappings of progress 0 and 1 to distances 0 and length, and the inverse endpoints; interior lookups must use the same strictly increasing segments. Invalid phase, NaN/infinite or out-of-range arguments must fail. Refinement failures must discard the whole table. For later authorized numerical verification, a perpendicular circular opening of radius r with chamfer setback c has machining TCP radius r+c/2, hence length 2*pi*(r+c/2); forward/inverse lookups must agree, including the full-turn endpoint. A tilted opening and unequal lead clearances/spans must have independently computed lead lengths. Such checks are not added as project test code. Build and execution validation are deferred under AGENTS.md.

## Idempotence and Recovery

All additions are source-only. Preserve unrelated working-tree changes and make exact edits to the existing CMake and instructions. Failed table construction returns an error and exposes no partial result. No robot, files shared with KRL, sockets or installed controller settings are touched.

## Artifacts and Notes

The table approximates only scalar arc length. It must never be used to linearly interpolate a TCP pose; the recovered progress goes back to ChamferPath::evaluatePhase to retain analytical position and orientation.

## Interfaces and Dependencies

ChamferArcLength::create(path, phase, parameters) returns ChamferArcLengthResult containing optional arcLength and error. ChamferArcLengthParameters holds lengthTolerance and lookupTolerance, both in millimetres. The returned immutable value exposes length(), distanceAt(progress), progressAtDistance(distance). Existing Qt, Eigen and C++17 are sufficient; no new dependency, QML API, device API or timing behavior is introduced.

## Outcomes & Retrospective

Step 3 is implemented as a numerical library API. Source review confirmed all phases use the existing analytical TCP poses, a full turn is seeded before measuring, cumulative distances and progress must strictly increase, and both lookup directions share the same table with exact endpoints and stored-knot matches. Construction limits reject incomplete results. CMake source registration and whitespace checks passed. No build, numerical execution or hardware validation was run; project instructions prohibit adding tests and require an explicit request for build verification. Acceleration timing, use by Dry Run and direct 4 ms RSI sampling remain later work.

Revision 2026-10-09: established step 3 scope and numerical refinement approach before implementation.

Revision 2026-10-09: completed the per-phase API and source review. Recorded exact stored-knot lookup and retained the later timing integration boundary.
