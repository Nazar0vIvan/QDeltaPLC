# Integrate the quintic inverse into central chamfer motion

## Purpose / Big Picture

Replace the piecewise linear distance-to-progress conversion with a smooth fifth-degree polynomial approximation of its inverse. The same immutable central trajectory must continue to supply generation validation and RSI Sampling, including lead-in, machining and lead-out. XYZ and orientation still come from ChamferPath; neither is interpolated by the new polynomials. This plan follows PLANS.md. AGENTS.md overrides its generic commit/test instructions: do not commit, add test harnesses or build without a separate request.

## Progress

- [x] (2026-10-10) Inspected source, defaults, numerical prototype, central evaluator and RSI callers.
- [x] (2026-10-10) Added analytical first/second position derivatives without orientation construction; shared phase coordinate law with pose evaluation.
- [x] (2026-10-10) Replaced chords and linear lookup with bounded adaptive Gauss/Kronrod integration and monotone quintic inverse segments; retained exact endpoints and reverse lookup API.
- [x] (2026-10-10) Reviewed central/RSI integration, endpoints, finite/error checks and work bounds; updated AGENTS.md. Source/whitespace review and an independent arithmetic derivative cross-check completed; no C++ build/runtime verification.

## Context and Orientation

src/pathgeneration/chamfer/chamferpath.h/.cpp owns the analytical TCP curve. Progress u ranges from zero to one separately in each phase. For TCP position p(u), g(u)=norm(p'(u)) is distance per unit progress, and g'(u)=p'(u).dot(p''(u))/g(u). Arc length S(u) integrates g from zero. The inverse F(s) recovers progress from distance s. Its derivatives are F'=1/g and F''=-g'/g^3.

src/pathgeneration/chamfer/chamferarclength.h/.cpp currently integrates chords and linearly interpolates both directions. ChamferCentralMotion constructs three maps, retains them with geometry, and calls progressAtDistance after applying the existing acceleration/cruise/deceleration time law. validateChamferCentral and sampleChamferCartesian both use that evaluator. No caller uses distanceAt today, but preserve it and keep it consistent with the inverse. Geometry compiles these existing files, so no CMake additions are required.

## Plan of Work and Milestones

First add a return-value differential API to ChamferPath for g and g'. Differentiate the existing normalized radial vector, end-plane direction, chamfer midpoint and axial lead clearance analytically. Share the phase angle/clearance law with evaluatePhase so endpoint pinning and traversal agree. Position derivatives exclude the translation of the ellipse center to avoid cancellation. Existing pose evaluation remains unchanged geometrically.

Second replace map internals. Integrate g with embedded 7-point Gauss / 15-point Kronrod quadrature. Their disagreement and a roundoff floor estimate integration error. Subdivide intervals that fail the allocated total-length budget. Store interval endpoint progress/distances and normalized quintic coefficients matching F,F',F'' at both ends. Validate local distance residual, speed ratio g*F', and its distance derivative g'*F'^2+g*F''. Subdivide failures; never silently accept the refinement limit. Check positive polynomial derivative throughout the interval using the convex-hull property of Bernstein polynomial coefficients with bounded recursive subdivision. This establishes polynomial monotonicity to floating-point precision; sampled geometric error checks remain estimates.

Defaults become 1e-9 mm total integration tolerance, 1e-10 mm inverse residual tolerance, 1e-8 relative speed tolerance, and 1e-7/mm speed-ratio derivative tolerance. These are numerical tolerances, not physical robot limits. Keep the existing two parameter names and add the two derivative tolerances. Construction retains 16 initial intervals, 20 adaptive levels, 200000 knots and 800000 differential evaluations per phase. Queries use binary search and polynomial evaluation only. distanceAt reverses the accepted monotone polynomial by bounded bisection; it performs no curve integration and is not on the RSI query path.

Third review the entire change and update AGENTS.md's stale chord/linear description. Do not modify timing, encoder, shared KRL data, UI, robot checks or controller limits. The observed ABC singularity peaks are independent of the inverse and remain subject to existing checks.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Read affected files and callers with Get-Content and rg. Edit the existing geometry files and this plan. Run git diff --check and inspect untracked files explicitly (the arc-length files are already untracked user work). Inspect the final source for signatures, finite checks, endpoint preservation, bounds and prohibited C++ constructs. No project tests or build commands are authorized by this task.

## Validation and Acceptance

Review that progressAtDistance evaluates no geometry, integration or root solver. Ensure exact endpoints and knots remain exact and inverse derivatives agree across adjacent segments within floating-point roundoff. Degenerate inputs, nonfinite derivatives, exhausted budgets and unattainable tolerance must return a failed result, never a partial map. Review distanceAt/progressAtDistance consistency and all central sampling call paths. The previous offline ellipse experiment provides a numerical reference: lengths about 7.123626147, 48.277695167, 7.123626147 mm and total central duration about 13.35982463 seconds with the saved negative-X outward axis and defaults. It does not validate the C++ implementation. A future separately authorized build/runtime check should regenerate the XP/XC path and exercise RSI Sampling offline; an IK or configured controller-bound rejection must remain a rejection. No C++ performance claim is justified until measured in a built application.

## Surprises & Discoveries

The existing RSI sampler already uses the retained central evaluator directly. Replacing that evaluator's maps changes all three RSI phases without touching device or encoder code. Maps are rebuilt for each existing generation candidate; moving construction outside candidate search is a separate optimization and is out of this change.

## Decision Log

2026-10-10: Preserve public APIs and add only the differential API and derivative tolerance fields. Use analytic derivatives rather than prototype complex steps. Use embedded quadrature with bounded refinement rather than unchecked fixed quadrature. Use Bernstein positivity instead of relying only on sampled monotonicity. Reject unresolved intervals instead of adding a runtime integration fallback, so accepted RSI queries have predictable work.

## Idempotence and Recovery

Keep unrelated user edits. Do not stage, commit, reset or write robot files. Failed numerical creation returns an error and destroys temporary intervals. No persistent data migration is needed; regenerate saved paths to obtain new maps.

## Artifacts and Notes

CHAMFER_ARCLENGTH_STUDY.md records the numerical recommendation and earlier Python measurements. This change is implementation, not a repetition of those benchmarks.

## Interfaces and Dependencies

ChamferPath gains phaseArcDifferential(phase, progress) returning an optional value with speed and speedDerivative, both with respect to geometric progress. ChamferArcLength preserves create, length, distanceAt and progressAtDistance. Existing Qt/Eigen/C++17 are sufficient; no new library or QML contract.

## Outcomes & Retrospective

Implemented in the existing geometry files; no CMake, UI, device, encoder or timing source changes were necessary. Public lookup methods remain available, and the new differential method returns its values directly. RSI distance queries perform binary search and value-only Horner evaluation; they perform no integration or root solve.

A standalone in-memory numerical calculation compared the new analytic derivative equations with the earlier complex-step prototype at 101 progress values in each of the three phases for ellipse_sample.json. Maximum differences were 1.42e-14 for g and 7.72e-14 for g'. This checks the mathematics, not compiled C++ execution. git diff --check passed with only pre-existing line-ending warnings. No test files or build targets were added. Build, C++ runtime validation and C++ timing remain unperformed; previous Python timing must not be presented as application performance.

Revision 2026-10-10: established implementation scope, numerical criteria and bounded failure behavior before edits.

Revision 2026-10-10: completed implementation and recorded review/math-check evidence and remaining build/runtime limitations.
