# Continuous joint speeds for central animation

This living ExecPlan follows repository-root PLANS.md and AGENTS.md.

## Purpose / Big Picture

Generate shall precompute cubic joint interpolation for lead-in, machining and lead-out. Dry Run shall use these coefficients without IK or further preparation. The same saved angles and timestamps remain; speed settings, HOME, LIN and RSI algorithms remain unchanged. Continuous first and second derivatives replace abrupt changes of angular velocity at central knots. This is an animation approximation, not proof of Cartesian path accuracy between knots.

## Progress

- [x] (2026-10-13) Read producer, playback consumer, boundary consumer, instructions and current dirty status.
- [x] (2026-10-13) Implement clamped cubic splines and analytical angle/speed extrema checks.
- [x] (2026-10-13) Connect generation, playback and diagnostic timing; update factual instructions.
- [x] (2026-10-13) Inspect changed implementation and callers, review equations and extrema cases, and pass git diff --check without building or executing RSI.

## Context and Orientation

src/pathgeneration/chamfer/chamfercentralvalidation.cpp supplies geometrically scheduled IK samples. src/pathgeneration/chamfer/chamfermotion.cpp assembles seven phases and assigns HOME timing. Central points occupy inclusive indices boundaries[2] through boundaries[5]. Before this change ChamferMotion::evaluate interpolated their joint angles linearly; it now uses the stored cubics. src/scene/scenemachining.cpp invokes create on a worker and evaluate during playback. A read-only inspection of chamferjob.cpp confirmed that its calls to evaluate use exact saved boundary times; these retain exact knot values without changing this consumer.

## Plan of Work / Milestones

First, add private central interval coefficients and a preparation method in chamfermotion.h/.cpp. Solve a tridiagonal system for the six angle second derivatives on the nonuniform saved times. Clamped means the first and last angular velocities are specified as zero. Interior equations enforce equal first derivatives; shared second derivatives enforce equal accelerations. The solve is linear in the number of points and does not call IK/FK or change sampling.

Second, construct each cubic in normalized interval time. Check finite coefficients, angle extrema (roots of its quadratic first derivative) and speed extrema (root of its linear second derivative), including endpoints. Enforce existing position and model speed bounds, retain the temporary acceleration-magnitude bypass, and reject nonfinite accelerations. Reject failed candidates without retiming. Keep candidate diagnostics including spline time and error. Use the coefficients only for central phases in evaluate and return original joint values at exact knots.

Third, update AGENTS.md to distinguish central cubic playback from LIN linear playback and verify the changed code and diff. No new dependencies, files in build lists or public API changes are needed.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Edit only chamfermotion.h/.cpp, AGENTS.md and this plan. Inspect git diff for these files and run git diff --check. Inspect the new plan separately because it is untracked. No build, test harness or RSI execution is authorized. Do not stage, commit or discard existing edits.

## Validation and Acceptance

Static review must verify the two-point case, unequal interval lengths, shared phase endpoints, zero boundary velocities, derivative continuity, finite arithmetic checks and complete extrema enumeration. After the user builds, regenerate the same hole and run Dry Run: central knots should no longer produce abrupt angular-speed changes, with identical point counts and duration for the same accepted IK candidate. Diagnostic centralSplineAttempts must report interval count, elapsed computation milliseconds and any failure. Previously generated motion must be regenerated. Visual behavior and actual runtime cost cannot be established by static review.

## Surprises & Discoveries

Existing branch selection can try multiple start configurations. Spline limits may reject a configuration that passed chord-average speed checks, so record those rejections separately rather than hide their computation cost. Saved point velocity fields describe HOME derivatives only and must not be repurposed.

## Decision Log

2026-10-13: Use a single clamped spline across all three central phases, preserving full-turn winding and every timestamp. Keep interval coefficients private to ChamferMotion so RSI analytical targets remain independent. Do not reenable the previously disabled joint acceleration magnitude limit. Analytical extrema checking adds no IK and no point search.

## Outcomes & Retrospective

Implementation and static verification complete. git diff --check passed; its only output was line-ending warnings on pre-existing unrelated files. Runtime verification remains for a user build; Cartesian interpolation accuracy is explicitly outside this change. Only the two existing motion source files and documentation are changed by this task. The six splines share derivatives across central phase boundaries; unchanged LIN interpolation does not provide the same derivative continuity at its own knots or a guarantee at the LIN/lead transitions.

## Idempotence and Recovery

Generate produces a replacement immutable motion. A failed spline returns an error and does not publish a partial motion. Repeating Generate is safe. Preserve unrelated dirty files when editing or reverting this task.

## Interfaces and Dependencies

Add only private ChamferMotion::prepareCentralSpline() returning QString and a QVector of four V6d polynomial coefficients per central interval. Existing Qt and Eigen types suffice. Coefficients use normalized local time from zero to one; physical derivatives divide by interval duration. Keep public create/evaluate/points/boundaries interfaces unchanged.

## Artifacts and Notes

Initial plan created 2026-10-13 from the approved animation-only spline proposal. All physical timing and RSI settings remain out of scope.

Implementation update 2026-10-13: coefficients use normalized interval time to reduce evaluation scale problems. The row-scaled tridiagonal solve has positive pivots. The two-point clamped equations give second derivatives +6*delta/h^2 and -6*delta/h^2, and therefore angle q0 + delta*(3*u^2 - 2*u^3), with zero endpoint speed. Here delta is the angle difference, h is the interval duration and u is elapsed interval time divided by h. This is an algebraic review, not an executed test. For unequal steps, the interior equation weights the two second derivatives by their adjacent durations; equating the two endpoint velocity formulas gives exactly that row. Position extrema require quadratic derivative roots, speed extrema require one linear derivative root; endpoints also suffice to check acceleration finiteness. Stable quadratic root evaluation avoids cancellation. Repeated or absent roots and constant/linear derivatives require no search.

Completion update 2026-10-13: reviewed exact phase-knot evaluation, terminal phase dispatch to unchanged LIN, coefficient ownership after moves, rejection diagnostics and private-only API changes. No build, runtime profiling, test harness or RSI preparation was run. The UI still requires Generate before Dry Run to obtain the new coefficients.
