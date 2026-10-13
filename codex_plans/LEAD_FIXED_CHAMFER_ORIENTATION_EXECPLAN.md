# Move tool tilting from leads to LIN transfers

This living plan follows PLANS.md and AGENTS.md.

## Purpose / Big Picture

The first animated LIN shall turn the TCP from its existing staging orientation to the machining orientation defined by chamferAngleDegrees (45 degrees by default). Lead-in retains the machining start rotation; lead-out retains the machining end rotation. During machining the frame still follows the hole, preserving the chamfer angle rather than freezing BASE ABC for the whole ellipse. Return LIN restores staging orientation. Generate prepares everything; Dry Run only plays saved motion.

## Progress

- [x] (2026-10-13) Inspect path construction, central evaluation and LIN pose consumers.
- [x] (2026-10-13) Remove the old lead ABC interpolation, retain staging rotation separately and interpolate transfer rotation.
- [x] (2026-10-13) Update instructions and review source/diff without building or running RSI.

## Context and Orientation

src/pathgeneration/chamfer/chamferpath.h/.cpp owns numerical curves and lead frames. Its private LeadOrientation currently mixes free-end/staging rotation with cubic ABC coefficients. stagingPose is used for HOME and LIN endpoints; evaluatePhase supplies lead and machining poses. src/pathgeneration/chamfer/chamfermotion.cpp TransferProfile::target currently interpolates XYZ only. Its LIN subdivision already checks both position and orientation against target, and sequential seeded IK samples supply joint checks. Central playback uses precomputed joint splines and remains an approximation between knots.

## Plan of Work / Milestones

First remove LeadOrientation, its result type, Euler branch helpers and machining angular derivative used only by the old lead orientation law. Retain a separately named stagingRotation in each LeadArc. Construct it directly from the existing axial TCP Y and radial TCP Z. evaluatePhase starts from the cached machining junction pose and changes translation only. Existing circular positions, timing and machining frames remain unchanged.

Second cache normalized endpoint quaternions in TransferProfile. target interpolates their shortest rotation with slerp at the same traveled-distance fraction used for XYZ. Return exact endpoint matrices at fractions zero and one. This is the preview's chosen rotation law, not a claim about KUKA's exact internal #VAR implementation. The first LIN reaches machining orientation before lead-in and the return LIN leaves it only after lead-out.

Third update AGENTS.md and inspect callers, removed-symbol references, and git diff --check. No public API changes, dependencies, QML changes or build-list changes are needed.

## Validation and Acceptance

Statically check that lead evaluatePhase cannot overwrite the junction rotation, stagingPose retains its former axis construction, and transfer endpoint rotations match central endpoints. Existing LIN orientation-error checks must use the new target rotation. Run git diff --check from D:/Qt/QtProjects/RoboCrap and inspect this untracked plan separately. Do not build, add tests, run profiling or run RSI. After a user build, regenerate the hole: the tilt should occur on the first LIN, remain at the configured chamfer angle through central targets, and return during the last LIN. Joint spline interpolation can deviate between knots; no exact Cartesian playback guarantee is added.

## Surprises & Discoveries

Path geometry is shared by the retained analytical central evaluator. No RSI preparation sources or algorithms are edited or run, but any future regeneration/sampling using this shared geometry will naturally see the new lead poses. Removing lead Euler interpolation also removes its unnecessary Euler-singularity rejection. Constant lead orientation joins rotating machining orientation continuously in value but not necessarily in angular derivative; the existing joint spline still smooths playback across those joins.

## Decision Log

2026-10-13: Treat 45 degrees as the configured chamfer angle, not an absolute BASE Euler coordinate. Preserve machining orientation around the ellipse and hold the respective junction rotation on each lead. Preserve staging orientation so the first LIN actually changes orientation; move the inverse tilt to return LIN. Remove superseded lead orientation code rather than retaining a second mode.

## Outcomes & Retrospective

Implemented in chamferpath.h/.cpp and chamfermotion.cpp. Lead targets copy the cached junction rotation and change only translation. Staging axes retain the old construction; transfer targets now interpolate both position and rotation. Old private lead-orientation code has no remaining references. git diff --check passed (unrelated pre-existing line-ending warnings only). No build, tests or RSI execution occurred; visual behavior and actual calculation cost have not been measured.

## Concrete Steps / Interfaces and Dependencies

Edit the two chamferpath files and chamfermotion.cpp plus AGENTS.md and this plan. Use existing Eigen quaternion operations; keep all public signatures and settings. Exact target endpoints ensure the compiler's junction checks still compare equal rotations. Geometry exposes no new controls.

## Idempotence and Recovery

Generate replaces immutable snapshots only after successful preparation. Existing saved motion requires regeneration. Preserve other dirty files and do not stage or commit. No device action or shared-folder export is involved.

## Artifacts and Notes

Plan created 2026-10-13 for the explicitly requested change of orientation during LIN, with animation scope retained.

Completion note 2026-10-13: reviewed first/last transfer target identity, shared machining junction rotations, non-45-degree settings and unchanged flip traversal. LIN quarter-time target checks and rate samples call the same modified TransferProfile::target, so they include the new rotation. No special-case Euler angle assignment was introduced. The three central geometries keep their original positions and timing. User regeneration is required after their build.
