# Connect prescribed Cartesian motion to Dry Run (step 6)

This living ExecPlan follows PLANS.md and implements step 6 of the agreed redesign.

## Purpose / Big Picture

Generate Path / Apply will compile the prescribed lead-in, constant-speed machining and lead-out profile into validated joint samples. Dry Run will display those samples at their prescribed times. Straight staging transfers use the saved transfer speed and acceleration, with triangular motion when too short for cruise. Existing synchronized quintic HOME motion and all playback controls remain.

## Progress

- [x] (2026-10-09) Read numerical compiler, central validator, playback coordinator, rendering and preparation consumers.
- [x] (2026-10-09) Replaced legacy central timing with validated prescribed samples and added trapezoidal/triangular transfer compilation.
- [x] (2026-10-09) Added quarter/midpoint FK checks on actual retained playback intervals, linear joint playback and owned central evaluator snapshots.
- [x] (2026-10-09) Reviewed seven shared phase ranges, full-turn progress, shifted timestamps, worker ownership, playback controls and existing preparation callers. Updated AGENTS.md; tracked and untracked whitespace checks passed. Build/runtime checks not performed.

## Context and Orientation

src/pathgeneration/chamfer/chamfermotion.cpp currently compiles geometric IK points, applies a separate timer, and interpolates joints cubically. SceneMachining runs create asynchronously, stores the returned ChamferMotion and calls evaluate(seconds) from its GUI timer. OccSceneAdapter renders seven point ranges using boundaries(). Steps 3-5 supplied ChamferCentralMotion and validateChamferCentral with prescribed times but no active callers. PreparedChamferJob and the old Cartesian sampler also read motion points and boundaries; preserve those contracts while leaving their projection-based sampling for step 7.

## Plan of Work

Keep bounded initial staging-seed search and quintic HOME geometry/timing. Replace geometric lead compilation and legacy central timer with a timed staging transfer, validateChamferCentral seeded from its final joints, and a timed return staging transfer. Retain the owned central evaluator in ChamferMotion for future direct RSI sampling. Store phase boundary points once, with progress 1 for the ending phase, and preserve the full-turn machining endpoint. Add HOME duration to all subsequent timestamps and use existing HOME timing for the last phase.

Use linear interpolation of dense validated joint samples for the five Cartesian phases, with quarter/midpoint FK checks against time-based analytic targets for the actual retained intervals. This is display interpolation, not a claim of finite acceleration for a piecewise linear joint function. Joint-rate validation remains on prescribed IK samples; no automatic retiming is allowed. Quintic HOME evaluation remains unchanged. Keep velocity fields/API for compatibility, but Cartesian linear playback does not consume them.

For transfer length L, requested speed V and acceleration a, peak speed is min(V,sqrt(a*L)), ramp time is peak/a and cruise time is (L-peak*rampTime)/peak. Total duration is two ramps plus cruise. Both transfer endpoints have zero speed. Evaluate position along the fixed-attitude straight line using this scalar law. Reject nonfinite or unresolved cases and validate sequential IK, positions, FK, sampled joint speeds/accelerations, endpoint rest and playback interpolation. Subdivide at acceleration switches; bound sample counts and refinement.

## Decision Log

On 2026-10-09, preserve public motion/scene APIs and avoid QML changes: existing controls already drive evaluate(seconds) through a monotonic clock. Central time scale becomes 1 for newly compiled paths. Retain that getter for existing preparation/reporting consumers. Store the shared analytical evaluator alongside samples, so step 7 can consume it without reconstructing timing.

On 2026-10-09, leave the old RSI sampler active until step 7, but preserve its expected phase progress/boundary representation. Its old separate lead-feed checks may still reject new profiles; do not claim the new direct RSI sampler is delivered here. Transfer settings model nominal KRL motion only, not exact controller execution.

## Surprises & Discoveries

The step-5 validator checked linear interpolation over parent intervals but retained quarter samples. Step 6 must also check the actual shorter interpolation intervals consumed by playback. Shared boundary progress belongs to the ending phase, while the central evaluator assigns a junction to the next phase; explicitly restore ending-phase progress in saved motion points.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Edit the numerical motion sources and central validator, read changed code and callers, run git diff --check and separately inspect new plan whitespace. No tests/harnesses, builds, runtime launch or hardware actions are requested. Preserve unrelated resource and IDE edits; do not stage or commit.

## Validation and Acceptance

Source review must follow Apply's async worker to the new compiler and Dry Run evaluate through Pause/resume/Stop. Confirm seven ordered ranges, shared exact endpoints, full turn, zero-speed transfer corners, original central duration and unchanged HOME quintic law. Check triangular transfers have zero cruise, and transfer profile rejects invalid numeric ranges. Verify no IK runs in playback, no old time stretch is invoked, and rejected candidates never reach scene insertion. Check active linear interpolation against analytic targets and retain finite/position checks. Build and disconnected runtime checks remain deferred under AGENTS.md; no numerical runtime success is claimed.

## Idempotence and Recovery

Return complete owned values only. Worker input remains copied, GUI insertion and stale-result rejection are unchanged. Keep interfaces for prepared-job consumers. No deployed/controller files or shared folder are changed.

## Interfaces and Dependencies

ChamferMotion retains an optional ChamferCentralMotion and exposes const access for future preparation. Existing points(), boundaries(), evaluate() and scene/QML contracts remain. RoboCrapGeometry already owns all needed sources, Qt/Eigen and IK. Local transfer helpers remain in the motion implementation with no device/viewport dependency.

## Artifacts and Notes

No GUI or QML changes are needed. Timing settings formerly only saved now control generated simulation. Step 7 will replace FK projection with direct sampling; step 8 will update remaining preparation reporting.

## Outcomes & Retrospective

Step 6 is implemented in chamfermotion.h/.cpp and chamfercentralvalidation.cpp. Existing SceneMachining Apply reaches the new compiler through its unchanged copied-input async worker. The assembled motion stores validated central samples and its analytical timing, two nominal timed transfers, and original quintic HOME phases. Playback interpolates stored Cartesian-phase joints and performs FK only. Pause/resume/Stop, stale-result rejection, calibration invalidation and seven-phase rendering contracts remain unchanged. Central timestamps are offset by preceding movement duration without stretching; centralTimeScale reports 1 for compatibility. Transfer and central validation reject failures instead of silently slowing the request.

Source review traced shared boundary progress=1, incoming branch seed continuity, strict timestamp ordering after HOME offsets, transform order, initial/final rest checks and bounded point/refinement counts. Linear display interpolation is checked numerically, not an acceleration-continuous motion model. Whitespace checks passed. No build, runtime or robot operation was performed under repository instructions. Direct RSI sampling remains step 7; the current sampler still projects playback FK and uses legacy lead-feed validation, which can reject new higher-feed profiles until replaced.

Revision 2026-10-09: recorded active integration scope and interpolation/phase-boundary requirements.

Revision 2026-10-09: completed active Dry Run integration, clarified remaining RSI dependency and recorded source-only verification. Added full-motion point budget and preserved explicit endpoint/FK checks during assembly.
