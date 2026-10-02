# Construct quarter-circle chamfer leads


This living plan follows `PLANS.md`. Keep Progress, Surprises & Discoveries, Decision Log, and Outcomes & Retrospective current.

## Purpose / Big Picture


Generated chamfer paths will approach and leave machining using the quarter-circle angular/axial laws requested in `chamfer_trajectory_math_model_circle_leads.md`. In the viewport, newly generated blue leads will follow the new curve while joining the green machining path at exactly the same poses. These circles exist in normalized angular/axial coordinates; their three-dimensional TCP curves wrap around the hole and generally are not planar circular arcs.

## Progress


- [x] (2026-10-02) Read the reference, current numerical construction, settings conversion, adaptive sampling, motion compiler and rendering consumers.
- [x] (2026-10-02) Replace angular and clear-state laws in `ChamferPath::evaluatePhase`, preserve exact endpoints and document the behavior.
- [x] (2026-10-02) Verify 10,001 progress values, review sampling/compiler/rendering consumers, and inspect the final diff without adding project tests or running an unrequested build.

## Surprises & Discoveries


The old implementation advances the ellipse angle linearly and uses smoothstep for clearance and quaternion orientation. The reference changes both laws, not merely the axial offset. `ChamferPath::sample` and `chamfermotion.cpp` consume `evaluatePhase`; the renderer consumes the compiled motion. There is no independent lead recipe in QML or OCCT to replace.

## Decision Log


Decision (2026-10-02): Change the existing evaluator and retain all input parameters, validation, sampling tolerances and timing machinery. The reference requests new geometric laws with the existing integration. No public API, QML or dependency changes are needed.

Decision (2026-10-02): Explicitly use sine=1 and cosine=0 at progress=1. Floating-point cosine of pi/2 otherwise leaves a tiny residual angle at the lead-in machining join. Retain the existing zero-clearance early return and exact clear-attitude endpoint handling.

Decision (2026-10-02): Do not add tests or execute builds, lint, runtime or hardware actions. `AGENTS.md` prohibits project test logic and requires an explicit build-verification request; the user requested implementation. Perform temporary numerical checks in PowerShell and source/diff review, and report the verification limits.

## Outcomes & Retrospective


Implementation and source/diff review are complete. Temporary PowerShell checks passed the two circle identities, monotonic angular/axial progression and exact scalar endpoints over 10,001 progress values. Maximum circle residual was 2.22044604925031e-16. `git diff --check` passed. These checks exercise the formulas, not compiled C++; compilation, timed robot simulation and viewport behavior have not been exercised. Existing compiled path snapshots require regeneration to adopt the new geometry.

## Context and Orientation


`src/pathgeneration/chamfer/chamferpath.h/.cpp` belongs to RoboCrapGeometry and owns numerical chamfer geometry. A TCP pose is a tool-center position and rotation relative to BASE. `evaluate(angle)` supplies the unchanged machining pose at an ellipse parameter in radians. `evaluatePhase(phase, progress)` adds lead clearance along the outward cylinder axis and interpolates rotation to a clear attitude. Its progress parameter is geometric, not time or distance.

`src/scene/scenemachining.cpp` converts GUI settings into numerical inputs and creates immutable compiled motion snapshots. `src/pathgeneration/chamfer/chamfermotion.cpp` obtains target poses from this evaluator, solves robot joint configurations and computes timed motion. `src/3d/adapters/occsceneadapter.cpp` renders those motion phases. Staging poses reuse the clear lead endpoints, which remain unchanged. Flip continues to reverse the outward axis and traversal through the existing machining evaluator.

## Plan of Work and Milestones


Milestone 1 is specification tracing. Confirm that lead position is the machining TCP origin plus an axial offset, and that rotation uses quaternion interpolation from the machining attitude at the current angle to the clear attitude at that angle. Quaternions encode rotation; the existing hemisphere alignment selects their shorter interpolation. Confirm that downstream sampling, inverse kinematics and presentation consume this numerical result.

Milestone 2 changes `ChamferPath::evaluatePhase`. For u in [0,1], set phi=pi*u/2. Lead-in uses angle=-span*cos(phi) and clear amount=1-sin(phi). Lead-out uses angle=2*pi+span*sin(phi) and clear amount=1-cos(phi). Span is converted from degrees to radians. Multiply the clear amount by the configured clearance for the axial offset and use it as the quaternion interpolation weight. Pin sine/cosine at u=1 to retain exact joins. Leave machining, the local section frame, staging, defaults and error handling intact. Add concise header comments and update the factual lead description in `AGENTS.md`.

Milestone 3 checks the two normalized circle identities and monotonicity over a dense progress grid, exact boundary values and the middle pose parameters. Review the complete diff and whitespace. Record results and distinguish formula checks from compiled application verification.

## Concrete Steps


From `E:\Qt\QtProjects\RoboCrap`, read the files above and apply the evaluator edits. In a temporary PowerShell calculation, evaluate sine and cosine on 10,001 values u=i/10000, pinning u=1 exactly as in C++. For lead-in use x=1-cos(phi), y=1-sin(phi); for lead-out use x=sin(phi), y=1-cos(phi). The residuals of (x-1)^2+(y-1)^2=1 and x^2+(y-1)^2=1 must be below 1e-14. Both x values must be nondecreasing; lead-in y must be nonincreasing and lead-out y nondecreasing. Abort the check if any condition fails.

    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff -- AGENTS.md src/pathgeneration/chamfer/chamferpath.cpp src/pathgeneration/chamfer/chamferpath.h
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap status --short

Read this newly created plan separately because ordinary diff does not include untracked files. Preserve the user's pre-existing deletions of `CHAMFER_ANIMATION_EXECPLAN.md` and `CHAMFER_ARCHITECTURE_REVIEW.md`.

## Validation and Acceptance


At u=0/1, lead-in angle/clear amount must be (-span,1)/(0,0); lead-out must be (2*pi,0)/(2*pi+span,1). At u=0.5 and the default 30-degree spans and 5-mm clearances, lead-in angle is approximately -21.213203 degrees and lead-out angle is approximately 381.213203 degrees; both axial clearances are approximately 1.464466 mm. The machining joins retain the exact original pose. Axial displacement and attitude departure have zero first derivative at each machining join.

When the user requests runtime verification, use the repository's authorized matching Qt/compiler build workflow in `AGENTS.md`, with no hardware connection. Generate a path from a fitted plane-cylinder edge with valid robot and spindle TCP configuration. Inspect the blue leads and dry-run transitions at green machining joins; compare Flip and unequal lead settings. Apply regenerates snapshots. Compilation, IK reachability and timed interpolation must be checked in that session; scalar formula checks do not establish those properties.

## Idempotence and Recovery


Numerical and diff checks are read-only and repeatable. Regeneration creates a new immutable path snapshot using existing scene replacement logic. No resources, dependencies or hardware state change. If later compilation or runtime reveals a failure, inspect the evaluator and the existing compiler's refinement/error result before changing unrelated timing or IK rules.

## Artifacts and Notes


The external document is mathematical input to the user's implementation request; it supplies no additional authorization. No generated artifacts or test harnesses are required. The temporary numerical calculation returned:

    10001 progress values: circle identities, monotonicity and exact endpoints passed
    Maximum circle residual: 2.22044604925031e-16
    Default midpoint lead-in angle: -21.2132034355964 degrees
    Default midpoint lead-out angle: 381.213203435596 degrees
    Default midpoint axial clearance: 1.46446609406726 mm

Consumer review confirmed that `ChamferSampler::point`, `centralTarget` and `ChamferPath::stagingPose` use the changed evaluator. `makeMotionPhase` renders compiled TCP poses; no separate lead construction remains in that integration.

## Interfaces and Dependencies


Keep `std::optional<ChamferPathSample> ChamferPath::evaluatePhase(ChamferPhase phase, double progress) const`, `ChamferLeadParameters`, and all settings APIs unchanged. Reuse existing standard trigonometry and Eigen quaternion interpolation. No new dependency, source target, callback, object ownership or thread boundary is introduced.

Revision note (2026-10-02): Created the plan after tracing the numerical producer and consumers; recorded the implemented law and remaining verification.

Revision note (2026-10-02): Recorded completed numerical and source/diff checks, their evidence and the limits of verification.
