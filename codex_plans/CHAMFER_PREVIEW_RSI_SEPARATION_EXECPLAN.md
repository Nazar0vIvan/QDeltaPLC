# Separate chamfer preview preparation from RSI preparation

This living ExecPlan follows PLANS.md. The user requested the plan and subsequently authorized step 1 with runtime timings on 2026-10-12. Step 1 is complete. The user subsequently authorized only the straight LIN preview optimization (step 4), preserving HOME, central motion and RSI algorithms. Follow AGENTS.md: no added project tests, no builds without explicit build verification authorization, no git mutations, no live equipment operations.

## Purpose / Big Picture

Generate should quickly prepare a faithful robot animation, including HOME and staging transfers. RSI Sampling should separately prepare only lead-in, machining and lead-out at the controller's 4 ms period. Both must evaluate the same immutable analytical geometry, orientation and timing. A simplified animation must never become the geometric source for controller corrections. Reducing speed should lengthen animation playback without forcing thousands of additional preview samples simply because the duration increased.

## Progress

- [x] Segment profiling follow-up: all seven segments instrumented per attempt; two offline Debug runs completed. Detailed timing/count tables and Ps angles are in CHAMFER_SEGMENT_TIMINGS.md.

- [x] (2026-10-12) Inspected generation, central validation, RSI sampler, prepared-job ownership and QML entry points; drafted this plan.
- [x] (2026-10-12) Step 1: added returned diagnostics, built robocrap Debug, ran the screenshot case three times offline and recorded timings/counts in CHAMFER_PREPARATION_PROFILE.md.
- [ ] Step 2: separate and retain shared analytical input.
- [ ] Step 3: generate adaptive central preview joints.
- [x] (2026-10-12) Step 4: only straight LIN preview transfers changed to bounded adaptive intervals with temporary chronological IK rate samples; full build and offline trapezoidal/triangular cases passed.
- [ ] Step 5: remove RSI dependence on dense preview samples.
- [ ] Step 6: run dedicated central RSI preparation and validation.
- [ ] Step 7: integrate lifecycle, readiness and diagnostics.
- [ ] Step 8: review and, when authorized, validate behavior and performance.

## Context and Orientation

qml/Views/Workspace/MachiningPanel.qml emits applyRequested from Generate. qml/Main.qml forwards it to SceneMachining::apply in src/scene/scenemachining.cpp. A copied-input async worker calls ChamferMotion::create in src/pathgeneration/chamfer/chamfermotion.cpp. MotionCompiler tries starting joint configurations, constructs staging transfers and calls validateChamferCentral. This currently constructs ChamferCentralMotion again for each candidate and retains dense IK samples for playback. Central validation subdivides time into at most 4 ms intervals and retains quarter samples; transfers start from 1 ms intervals. Multiple FK checks and candidate attempts multiply this work.

ChamferPath defines analytical TCP position and orientation. ChamferArcLength provides quintic inverse arc-length maps: given distance along a phase, it returns the curve parameter. ChamferCentralMotion owns these maps and the prescribed distance-versus-time laws. ChamferMotion owns the seven-phase preview with time-stamped joint samples, robot model and TCP calibration. Dry Run interpolates those joints and applies FK.

src/pathgeneration/chamfer/chamfercartesiansampler.cpp already evaluates central analytical poses at RSI timestamps, but it also obtains branch/winding reference joints from motion.points(), compares them within 5 degrees, and inserts saved preview timestamps into validationTimes(). Those dependencies must change together with preview sparsification. A branch is one robot configuration solving a pose; winding is the selected full-turn representation of each joint angle. src/pathgeneration/chamfer/chamferjob.h/.cpp retains the source motion and boundary poses; src/scene/scenemachiningpreparation.cpp asynchronously prepares and publishes it. Preserve its identity/calibration checks, immutable ownership and existing export/encoding rules.

## Plan of Work

### Step 1: identify and measure the repeated work

Add compact returned numerical diagnostics for analytical-map construction, candidate count, IK/FK calls, retained preview points, temporary checks and total preview/RSI preparation times. Numerical workers return these values; report one summary through the existing coordinator/logging mechanism, not per-sample logs or new tutorial labels. Without a build, only source attribution is possible; collect a runtime baseline later when authorized. This establishes whether the largest cost is dense sampling, repeated candidates or another operation and avoids unsupported performance promises.

Acceptance: distinguish construction from sampling, and distinguish retained data from temporary evaluations. The screenshot case has 1 mm/s machining and transfer speed, 1 mm/s squared transfer acceleration, full-length lead ramps, 30 mm staging distance, 5 mm lead clearances, 30 degree spans, 0.5 mm chamfer at 45 degrees and flipped axis. Its roughly 76.77-second central duration currently implies roughly 76800 central retained samples before refinement, not a measured IK-call count.

### Step 2: prepare the analytical trajectory once

Move ChamferCentralMotion::create out of the candidate-configuration loop in ChamferMotion::create. Pass the already prepared immutable motion to preview compilation. Retain it in the accepted ChamferMotion and reuse it from RSI Sampling; do not reconstruct its maps during preparation. Change the internal validator/compiler interface to accept prepared motion rather than reconstructing it from ChamferPath. Keep a delegating wrapper for any existing public validation API if needed and update all callers. Geometry-only work must not depend on the chosen IK seed.

Retain robot model, flange-to-TCP calibration, original geometry/settings, exact phase times and exact boundary poses as one consistent saved input. Prefer existing ChamferMotion ownership and accessors to a new general framework. Separate preview sample storage from analytical inputs conceptually and in consumers. Boundary metadata must remain valid with sparse samples.

Acceptance: one set of three maps per generation, reused for every candidate and later RSI sampling. Pose formulas and nominal phase durations do not change.

### Step 3: replace dense central preview sampling with adaptive sampling

Create a dedicated preview path in the existing numerical module. Begin with a small bounded set of phase-local intervals, explicit phase/ramp boundaries and full-turn endpoints. Seed a closed machining loop with multiple intervals so coincident endpoints cannot hide a complete revolution. Split intervals when their temporal quarter/midpoint analytical targets cannot be reproduced by interpolating endpoint joint angles and applying FK within the existing 0.01 mm and 0.1 degree playback tolerances, or when joint continuity exceeds the existing 5-degree criterion. A fixed small initial partition, such as 16 intervals per curved phase plus exact switches, resolves orientation/geometry before adaptive refinement; the partition is not a controller clock.

Use preceding joints as seeds. During a split, solve samples chronologically and propagate the accepted midpoint seed into the right child; do not retain an endpoint from an incompatible seed path. Reuse computed values within the refinement operation, with bounded recursion, evaluations and stored points. Keep finite-value, IK/FK, joint-position and sampled speed checks. They are preview checks, not execution certification. Preserve the current temporary joint-acceleration bypass in preview; do not silently restore or widen limits. Keep the active Cartesian acceleration check using separate local time stencils at analytical targets, not coarse chord accelerations; preserve initial/final rest and switch handling. Such temporary rate samples are not automatically retained as playback points.

Interpolation samples have timestamps from the unchanged motion law. They approximate joint playback, not the analytical geometry used by RSI. A visible pose that passes at endpoints alone is insufficient. Retain extra points only where the approximation needs them, not every transient validation point.

Acceptance: no mandatory 1 ms central grid in Generate; full-turn motion and exact joins are preserved. Point count is driven principally by path complexity and interpolation error, not duration. Do not promise an exact point count in advance.

### Step 4: make staging preview preparation adaptive as well

Replace the fixed 1 ms transfer loop in chamfermotion.cpp with adaptive evaluation of its existing fixed-orientation straight-line trapezoidal/triangular law. Preserve start, acceleration/cruise/deceleration transitions and end, then apply the same joint interpolation checks. A geometrically straight TCP path can require nonlinear joint motion, so it still needs subdivision. Reuse a local helper only where the central and transfer requirements actually agree. Keep HOME's existing synchronized joint-space time law and auxiliaryScale; no RSI samples are needed for HOME or staging.

Acceptance: 1 mm/s staging does not force tens of thousands of stored preview joints. All seven preview phases, stop/resume behavior, durations and phase boundaries remain coherent.

### Step 5: decouple RSI configuration validation from preview density

Update CartesianSampler::inspectSource, targetAt, sampleAt and validationTimes together. Read analytical time/boundary metadata from the retained definition, not from the spacing of motion.points(). Keep the exact incoming-transfer joint endpoint as the initial central configuration. Continue IK chronologically using the preceding accepted RSI/validation joint sample. Retain explicit configuration/winding information using the solver's existing branch/status/turn conventions, but do not compare raw turn bits as if they were invariant: a joint crossing zero legitimately changes its sign. Enforce continuous angles, positions and exact boundary agreement. Reject an unexpected branch switch or unresolved near-singularity interval instead of independently choosing another configuration.

Remove the old nearest-preview-point 5-degree comparison, whose meaning depends on preview sample density. Preserve a step-continuity criterion on the sampler's own consecutive IK samples and refine validation locally when needed. Remove animation timestamps from validationTimes; generate intermediate checks from the RSI interval itself and exact phase/ramp boundaries. Compare exact central boundary solutions with the retained preview boundary configurations so a different full-turn route is not silently accepted. Sparse preview samples may be diagnostic references but never define TCP targets or allow validation to be skipped.

Acceptance: changing preview tolerance/density does not change analytical RSI targets, correction count or clock times for the same definition and starting configuration. A configuration mismatch yields an explicit preparation failure.

### Step 6: prepare and validate RSI central motion on demand

RSI Sampling traverses only lead-in, machining and lead-out. At each global controller time j*0.004 seconds, compute prescribed distance, evaluate the existing quintic inverse, evaluate ChamferPath's original pose, solve/check IK, then perform existing continuous Euler encoding and increment serialization. The final analytical endpoint appears exactly once at the final complete controller tick. Preserve phase boundary metadata without inserting extra wire cycles.

Retain necessary intermediate IK/FK and rate/interpolation checks, including final-stop and complete-cycle checks, but reuse already computed endpoints and shared intermediate times within this preparation. Extra validation samples are temporary unless required as useful diagnostics. Preserve the current RSI joint-acceleration checks, rated joint speeds, XYZ bounds, encoder continuity and configured correction-limit checks. Preview's temporary acceleration bypass does not propagate to RSI. A failure returns an error; no automatic retiming or limit relaxation. Do not run both the old full dense central validator and the new RSI sampler over the same path. Preserve PreparedChamferJob, five-row shared_data export and existing encoding-readiness semantics.

Acceptance: stored output contains only central controller samples plus the initial pose and separate exact metadata. No HOME/transfer correction sequence is generated. Dense RSI preparation remains proportional to duration, because these are actual 4 ms controller cycles.

### Step 7: integrate lifecycle and readiness

Generate publishes a preview-capable saved path; Dry Run uses it without per-frame IK. RSI Sampling publishes a separately validated prepared job. Reuse current calculating/preparing flags and worker-return conventions. Preserve selection/deletion, model/TCP revision, viewport readiness and stale-worker-result handling. Geometry/calibration changes invalidate dependent preview and RSI data. Draft edits alone must not change saved inputs. Failure of RSI preparation must preserve the preview, while clearing unusable prepared output as currently specified. Start Machining remains disabled until its independent controller integration is implemented.

Keep QML APIs unless a demonstrated dependency requires a change. Update AGENTS.md to describe the separation and diagnostic checks accurately. Update src/CMakeLists.txt only if a focused preview implementation file is introduced. No new runtime dependency is required.

Acceptance: preview success never sets encodingReady or controller readiness; an RSI failure does not prevent viewing the saved motion. Existing stop/pause/reset-to-HOME behavior remains valid.

### Step 8: review correctness and measure the result

Review all producers and consumers of points(), boundaries(), phaseTimes and centralMotion(). Review device/preparation publication even if their APIs stay unchanged. Run git diff --check and inspect new files separately. Do not add project tests or run builds under the current request.

When separately authorized to build and run offline, use an inspected compatible build directory and run cmake --build <verified-directory> --target robocrap --parallel, then the appropriate application QML lint target if QML changes require it. Do not connect to equipment. Exercise XP/XC with the screenshot calibration (ER translation 142.187099,-0.182323,122.895995 mm; ER orientation approximately -90.017876372,0,-0.179278797 degrees; TCP relative to ER 0,15,1.5 mm with zero orientation), comparing screenshot speeds and the earlier 5 mm/s settings. Use the exact backend calibration snapshot for comparisons, not rounded displayed values.

Compare warmed preparation timings and IK/FK/sample counts, recording build configuration and host. Compare analytical endpoint poses, phase times, full turn and RSI XYZABC reconstructions. For fixed saved inputs, preview density must not alter RSI timing/targets. Exercise pause/resume, selection, regenerate, calibration changes and failed RSI preparation. Check near-singularity/high-curvature intervals remain bounded failures rather than skipped validation. No specific speedup is claimed before profiling; success requires eliminating duration-proportional dense preview storage and repeated map construction, not just hiding latency behind a progress window.

## Surprises & Discoveries

The RSI sampler currently imports all preview knot times as validation times and requires IK to remain within 5 degrees of a preceding saved joint knot. Sparse preview alone would invalidate that assumption. Prepared jobs also obtain boundary poses from preview boundaries; those endpoints must stay exact even when interiors become sparse. Both dependencies were confirmed by reading chamfercartesiansampler.cpp and chamferjob.cpp.

## Decision Log

2026-10-12: choose one analytical source with separate preview and RSI sampling policies, rather than a single dense universal sample list. This lets preview complexity be independent of controller duration while keeping exact RSI geometry. Reuse quintic maps; they are not the same thing as IK samples. Keep existing physical/numerical checks assigned explicitly to their stage; a preview is not motion authorization. Do not bundle restoration of temporary acceleration checks or changes to limits into this refactor.

## Interfaces and Dependencies

Keep ChamferMotion::create(path, robot), evaluate(seconds), robot(), parameters() and the scene-facing phase/duration contract where practical. Internal preview compilation accepts a const prepared ChamferCentralMotion and returns owned sample/boundary data plus errors/diagnostics. sampleChamferCartesian may keep its current public motion/period signature, but must consume analytical input and explicit configuration metadata instead of requiring dense animation points. Preserve result-value error handling, at most four arguments, no C++ lambdas or exceptions, and GUI-thread ownership of scene objects. Numerical work stays in RoboCrapGeometry on existing copied-input workers.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Inspect the named source files before each milestone. Keep implementation in existing source inputs and any narrowly justified new numerical helper; register new files in src/CMakeLists.txt. Use rg to recheck callers after interface changes. Run git diff --check and inspect untracked new files. Do not stage, commit, move unrelated plans or change generated build outputs. Update this document's Progress and Outcomes after each implemented milestone.

## Validation and Acceptance

Source acceptance is the elimination of the fixed preview time grids and dense-preview dependency in RSI, one retained analytical definition per generated path, chronological IK continuity and exact endpoint ownership. Runtime acceptance, requiring later authorization, is a materially faster Generate with far fewer retained preview samples, faithful playback within the existing sampled interpolation tolerances, and RSI output independent of preview density. Timing and sampled checks are estimates rather than analytic guarantees. Unexpected RSI acceleration or correction-limit failures remain visible and are not removed to make a benchmark pass.

## Idempotence and Recovery

Step 1 adds diagnostic accounting without changing sampling, kinematics, timing, tolerances or limits. During future implementation preserve unrelated local edits and keep current APIs working between milestones via narrow wrappers where needed. No persistent robot files are changed by the refactor itself; existing authorized preparation export behavior remains part of RSI preparation. Failed workers return no partial successful result. Existing snapshots remain immutable; regenerate after implementation to obtain the new representation.

## Artifacts and Notes

Reference files: resources/json/edge_cyl_plane/XP.json, XC.json and resources/json/ellipse_sample.json. Previous offline Python calculations established central durations and numerical inverse behavior but are not C++ profiling evidence. The earlier 60 deg/s squared A4 rejection was reproduced as smooth joint deceleration, not a quintic spike; do not treat preview/RSI separation as a fix for that independent bound.

## Outcomes & Retrospective

Step 1 is complete. Numerical workers return diagnostics and coordinators print one summary. The offline application mode runs the production preparation before device initialization. Full Debug build and three runs succeeded. Clean repeated means: preview 62.514 s, RSI 19.453 s; 132067 preview points. Four candidates are attempted, three fail, and discarded work consumes 34.096 s. All analytical map constructions together consume 0.471 s. See CHAMFER_PREPARATION_PROFILE.md and chamfer_profile_debug.json for reproducible inputs, limits and evidence. Steps 2–8 remain unimplemented. The key architectural requirement is shared exact input with different sampling responsibilities, not feeding approximated animation poses into RSI.

Revision 2026-10-12: initial plan based on current source and the user's slow-motion example.

Revision 2026-10-12: user authorized step 1 and requested actual timings. Added an offline application profiling mode, not a test target, to measure the production functions with no devices or shared-data export. Diagnostic FK counts exclude internal IK solver calls; IK time includes them. Input and raw reports live in codex_plans.

Revision 2026-10-12: completed step 1 with measured Debug baseline. Dense IK/FK and repeated failed candidates dominate; reusing quintic maps alone cannot remove the delay. Steps 2–8 remain pending authorization.

Follow-up: user requested the missing seven-segment breakdown. Instrument per-attempt segments, record actual Ps joint angles, retain partial failed work and separate shared construction/rate checks/assembly/HOME timestamp work. Rebuild and repeat the identical offline case. No sampling or trajectory behavior changes.

Segment profiling outcome: two runs averaged 62.569265 s. Successful segment times now directly measured, with shared checks and assembly separate. Source edits only add diagnostics; counts and failure sequence match the preceding baseline.

## Straight LIN-only implementation decision

Only MotionCompiler staging transfers change. Seed each acceleration/cruise/deceleration section with one interval; recursively halve it using the existing 0.01 mm/0.1 degree quarter-time FK comparison and 5-degree joint step. Recompute the right endpoint after the left child commits to maintain sequential IK seeding. Failed endpoint IK also requests bounded subdivision. Cap at 20 levels and 200000 interval evaluations per transfer. On accepted intervals, evaluate sequential quarter-time IK witnesses including the endpoint; reject unresolved winding/continuity by subdivision. These temporary samples feed the existing speed/finite-acceleration pass, including rest context, but do not become animation points. The temporary joint acceleration magnitude bypass stays in force. Checks remain numerical samples, not proofs between samples. Exact acceleration/cruise/deceleration boundaries and central seed must remain intact. Use existing offline profiling to compare timings, counts, central durations/failures and RSI correction counts; no test harness is added.

Implementation outcome: step 4 is complete independently of steps 2 and 3. The earlier fixed-grid descriptions and pending-step statements above describe the profiling baseline. Only straight LIN transfers now use adaptive intervals. Two screenshot-case Debug runs averaged 36.302630 s for Generate versus 62.569265 s before. Successful incoming/outgoing transfers averaged 8.404/8.534 ms versus 5.245/5.232 s and each added 12 animation points instead of 27535. Central sample counts, all three rejected-candidate errors, exact motion durations and 19194 RSI corrections match the baseline. A separate triangular-transfer case also passed. Full robocrap build passed; actual GUI playback was not exercised. See CHAMFER_ADAPTIVE_LIN_RESULTS.md and its raw reports. Steps 2, 3 and 5–8 remain unimplemented; no central, HOME or RSI algorithm was changed.
