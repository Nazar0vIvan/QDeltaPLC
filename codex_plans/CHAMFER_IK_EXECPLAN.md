# Validate prescribed central motion with IK (step 5)

This living ExecPlan follows PLANS.md.

## Purpose / Big Picture

Produce joint samples for the step-4 analytical central trajectory, rejecting unreachable or over-limit motion without retiming. Later Dry Run integration (step 6) and RSI sampling (step 7) consume this work; active playback remains unchanged here.

## Progress

- [x] (2026-10-09) Read the current compiler, Cartesian sampler, analytical evaluator, IK implementation, model, CMake and instructions.
- [x] (2026-10-09) Implemented copied-input validation, retained numerical samples and CMake registration.
- [x] (2026-10-09) Reviewed transform order, preceding-seed use, limits, FK, rest endpoint differences, bounded work, lifetimes and errors; updated documentation and completed whitespace checks. Build/runtime not run.

## Context and Orientation

ChamferCentralMotion in src/pathgeneration/chamfer defines analytical TCP poses at prescribed times. ChamferRobotSetup in chamfermotion.h contains the robot model, flange-to-TCP calibration and configured simulation bounds. Kr10Kinematics selects its branch from the supplied joint seed, owns a reference to its model, returns optional IK and computes FK. The current ChamferMotion compiler and Cartesian sampler supply useful conventions but derive timing from the old animation. The new chamfercentralvalidation.h/.cpp belongs to RoboCrapGeometry in src/CMakeLists.txt.

## Plan of Work

Add validateChamferCentral(path, robot, initialSeed), where the seed chooses the initial IK branch and winding, not a motion from the seed to the first TCP. Construct the central trajectory from the same robot timing snapshot. Check finite rigid calibration/model inputs and positive configured limits before solving. At every sample convert BASE_TCP to BASE_FLANGE by multiplying the inverse of FLANGE_TCP on the right, solve with preceding joints and verify TCP reconstruction by FK.

Partition time at phase boundaries and acceleration switches, then into intervals no longer than 4 ms. Evaluate quarter points sequentially inside each interval. Check continuous joint steps (at most 5 degrees) and linear joint-interpolation FK against the analytical target (0.01 mm and 0.1 degree). Refine failed continuity/interpolation intervals, bounded by 12 subdivisions, 200000 retained samples and 800000 IK evaluations. Store quarter points as validation samples, not controller cycles. Check sampled speed and finite-difference acceleration, including initial/final rest. Merge derivative windows below 100 microseconds while retaining all geometry samples. No analytic guarantee between samples is claimed.

## Decision Log

On 2026-10-09, retain model/calibration/timing and analytical trajectory together with successful samples in return-value data. The result is independent of input lifetime. Use explicit caller seed rather than inventing a staging branch; step 6 must connect it to its preceding transfer. Failed branches return a useful error, never a partial successful path. The validator does not automatically change branch or wrap joint angles.

On 2026-10-09, use the minimum of each configured speed and model rated-payload speed, preserving acceleration assumptions. Keep configured total XYZ acceleration checking as agreed in the compatibility discussion; it is a simulation bound, not controller ACC_MA. Report axis/value/limit/time for joint failures and label acceleration bounds accordingly. Do not apply any automatic time multiplier.

## Surprises & Discoveries

Kr10Kinematics::isValid checks only a subset of model dimensions; its branch logic divides by joint axis Z and depends on finite HOME/position limits. Validate those before calling the solver. The solver retains a model reference, so its owner must outlive it. Very close exact boundaries make naive finite differences unstable; retain them geometrically but use resolved derivative windows, as the existing sampler does.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Add numerical source files and CMake entries. Read new files and inspect git diff; run git diff --check and separate untracked-file whitespace checks. Do not add tests, stage/commit, build, launch or contact hardware. Build verification requires a separate explicit request under AGENTS.md.

## Validation and Acceptance

Source review must confirm FK checks at each IK point, preceding-seed continuity across the full machining turn, bounded refinement and exact phase/ramp boundary insertion. Successful data starts at t=0 and ends at the original analytical duration with unchanged timestamps. A smaller speed/acceleration bound rejects rather than retimes. Nonfinite seed/calibration/model, failed IK, position limit, winding discontinuity and FK mismatch return no data. Finite differences use joint degrees per second and per second squared; speed limit uses min(configured,rated). Include initial and terminal rest by comparing interval velocity at its midpoint to zero at the endpoint. Report source checks only; numerical execution/build remains unverified until requested. No test harness is added.

## Idempotence and Recovery

Preserve unrelated edits and existing active implementations. New source has no QObject, callbacks, files or device effects. The worker-style function uses copied inputs and returns a cohesive value; callers must only publish successful data. Fix task-introduced errors without resetting the checkout.

## Interfaces and Dependencies

ChamferCentralJointSample stores time, analytical ChamferCentralPose and continuous joints. ChamferCentralValidationData owns motion, robot setup and samples. ChamferCentralValidationResult contains optional data plus error. validateChamferCentral takes three meaningful inputs: analytical geometry, robot/calibration/limits and initial branch seed. Existing Qt/Eigen/C++17 suffice.

## Artifacts and Notes

Linear interpolation is inspected numerically here, not adopted as a controller motion model. Future playback must validate whichever interpolation it actually uses. Joint acceleration and Cartesian acceleration defaults remain simulation assumptions; no controller acceleration values are inferred.

## Outcomes & Retrospective

Step 5 is implemented as a numerical return-value API, without active GUI/device integration. Construction and validation preserve the step-4 timing. Source review covered transform order, finite/position checks, model lifetime during IK, per-interval joint speed and resolved-window accelerations including start/stop. No limits are used to retime the motion. Documentation and tracked/new-file whitespace checks passed. No compilation or numerical execution was performed; those remain unverified. Step 6 must integrate these samples with transfers/HOME and playback; step 7 must retain direct analytical RSI sampling.

Revision 2026-10-09: established step-5 scope, sampled validation policy and explicit initial-branch contract.

Revision 2026-10-09: completed implementation and review. Every retained interval checks joint speed separately, so merging short derivative windows does not omit that check. Active integration remains step 6.
