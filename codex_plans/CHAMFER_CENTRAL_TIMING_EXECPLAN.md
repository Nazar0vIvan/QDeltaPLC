# Implement prescribed central chamfer timing (step 4)

This living ExecPlan follows PLANS.md. Only step 4 of the user's ten-step redesign is authorized in this task.

## Purpose / Big Picture

Provide one mathematical central trajectory whose TCP position and orientation can be evaluated directly at elapsed time. Lead-in accelerates from rest to machining speed over a chosen fraction of its length, machining maintains that speed, and lead-out decelerates over the same fraction of its own length. Future Dry Run and RSI sampling will consume this trajectory. Their integration belongs to steps 5-7; this milestone does not replace the active seven-stage animation timer.

## Progress

- [x] (2026-10-09) Recovered the exact agreed plan from this chat, read existing geometry, arc-length API, settings, timing consumers and repository instructions.
- [x] (2026-10-09) Implemented immutable central motion, checked profiles, whole-motion/phase-local analytical evaluation and CMake source registration.
- [x] (2026-10-09) Reviewed equations, boundary ownership, full-turn endpoint, all-ramp leads, invalid inputs, numeric range and error handling. Updated factual documentation and completed tracked/new-file whitespace checks. No build or runtime verification was performed.

## Surprises & Discoveries

The repository's MACHINING_RSI_EXECPLAN.md describes the older projection-based pipeline, not the newer numbered redesign. The agreed chat plan explicitly separates the new profile (step 4), IK validation (step 5), Dry Run integration (step 6), and direct RSI sampling (step 7). ChamferArcLength already provides measured lengths and distance-to-progress inversion without timestamps. Its numerical error remains applicable to the new timing.

## Decision Log

Decision on 2026-10-09: add ChamferCentralMotion to RoboCrapGeometry, reusing ChamferTimingParameters as factory input but reading only machiningFeed and accelerationDistanceFraction. Retain a copied ChamferPath and three owned arc-length tables so later evaluation cannot observe changed inputs. Do not apply centralTimeScale, joint constraints or the legacy cartesianAcceleration in this mathematical definition. The next stage must validate requested motion and reject unsuitable profiles without silently slowing them.

Decision on 2026-10-09: provide whole-motion and phase-local evaluation. Phase-local evaluation allows the machining full-turn endpoint to be retrieved explicitly even though whole-motion evaluation assigns a shared junction to the following phase. Reject nonfinite and out-of-domain times; pin exact endpoints. Report scalar tangential speed and signed tangential acceleration, not total XYZ acceleration. Acceleration is right-sided at internal switches and zero at the terminal lead-out endpoint.

## Context and Orientation

src/pathgeneration/chamfer/chamferpath.h/.cpp owns analytical TCP geometry. chamferarclength.h/.cpp maps travelled TCP-origin distance in mm to a dimensionless curve progress in [0,1]. chamfermotion.h defines saved ChamferTimingParameters and the currently active seven-stage IK simulation. Its implementation and chamfercartesiansampler.cpp remain unchanged here. src/CMakeLists.txt registers numerical source files in RoboCrapGeometry. AGENTS.md describes these boundaries.

## Plan of Work

Create chamfercentralmotion.h/.cpp. A factory validates the saved speed and fraction, constructs all three arc-length tables, then derives per-phase profiles and cumulative phase times. A profile exposes length, speed, ramp distance, ramp duration, cruise duration and acceleration magnitude. Evaluation obtains travelled distance and speed from the profile, uses the owned arc-length table to recover progress, and calls the owned path's evaluatePhase for both XYZ and rotation. Return results through optional/value/error conventions.

Let V be machining speed in mm/s, f the fraction in (0,1], and L a phase length in mm. For a lead, ramp distance D=f*L, ramp time R=2*D/V, acceleration magnitude a=V/R=V*V/(2*f*L), cruise time C=(L-D)/V and duration T=R+C. Machining has duration L/V and zero ramp. With phase-local time t, lead-in uses distance D*(t/R)^2 and speed V*t/R during its ramp, then distance D+V*(t-R) and speed V. Lead-out uses distance V*t during cruise; during deceleration use remaining fraction q=(T-t)/R, distance L-D*q*q and speed V*q. Evaluate geometry at that distance, not at t/T. When f=1 the lead has no cruise segment. Unequal lead lengths produce unequal acceleration magnitudes.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Read the new files in full and inspect affected CMake/AGENTS changes. Run git diff --check and separately check untracked source/plan files for whitespace. Preserve all unrelated local changes. No staging, commits, project tests, build, runtime launch, network or hardware operation is part of this task.

## Validation and Acceptance

Source review must verify the formulas at start, ramp switches, both machining junctions and final stop, including f=1. For an illustrative L=10 mm, V=5 mm/s and f=0.30, D=3 mm, R=1.2 s, C=1.4 s, T=2.6 s and a=25/6 mm/s^2. Lead-in at 0.6 s has travelled 0.75 mm at 2.5 mm/s; at 1.2 s it reaches 3 mm at 5 mm/s. The corresponding lead-out at 2.0 s has travelled 9.25 mm at 2.5 mm/s, and ends at 10 mm with zero speed. These are mathematical review values, not executed C++ results. Machining takes L/V seconds with constant V. Invalid fractions/speeds and unrepresentable timing must return errors. Phase-local full-turn evaluation must preserve progress=1. Optional numerical/build verification remains deferred until requested under AGENTS.md; no test harness may be added.

## Idempotence and Recovery

All new state is owned value data, with no callbacks, device access or GUI objects. Failed construction returns no partial motion. Source-only additions can be revised independently while preserving the existing playback pipeline and user edits. No shared KRL folder is written.

## Artifacts and Notes

The scalar acceleration is the derivative of path speed; it excludes the normal acceleration caused by curvature. The new model contains no IK solution, controller configuration or readiness claim. Four-millisecond sampling and Euler encoding remain later consumers.

## Interfaces and Dependencies

ChamferCentralMotion::create(path, timing) returns optional motion and QString error. phaseTimes() returns four central-relative boundary times in seconds; duration() returns the final time. profile(phase) returns checked phase data. evaluate(seconds) and evaluatePhase(phase, seconds) return optional ChamferCentralPose containing BASE-relative TCP matrix, phase, progress, phase-local distance, speed and signed tangential acceleration. Use Qt, Eigen and C++17 already provided by RoboCrapGeometry. No new dependency or public existing API changes are required.

## Outcomes & Retrospective

Step 4 is implemented in chamfercentralmotion.h/.cpp and registered in RoboCrapGeometry. Construction owns copied analytical geometry and independent per-phase arc-length tables. Evaluation supplies position/orientation, distance, speed and signed tangential acceleration at the requested time, without time stretching, FK projection or accumulated time integration. Source review covered the numerical examples above, distinct representable phase/ramp boundaries, exact shared endpoint speed, all-ramp leads and error returns. Source registration and whitespace checks passed. No C++ build or numerical execution was performed under the repository verification rules. The next authorized steps must validate IK and interpolation, connect Dry Run, and replace RSI projection sampling. Active application timing remains the legacy implementation until that integration.

Revision 2026-10-09: recorded recovered step-4 scope and precise profile equations before implementation.

Revision 2026-10-09: completed the central profile implementation and source review; documented pending IK/playback/RSI integration rather than claiming the active pipeline has changed.
