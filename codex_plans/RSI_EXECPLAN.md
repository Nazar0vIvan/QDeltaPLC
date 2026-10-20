# Match nominal RSI motion to the saved animation


This living ExecPlan follows repository-root `PLANS.md` and `AGENTS.md`. It is the
single current RSI implementation plan. Update Progress, discoveries, decisions,
outcomes and file/line counts as work proceeds. Steps 1-4 are implemented in source;
the remaining repair and verification are tracked below.

## Purpose / Big Picture


Make `PTP PsIn` establish the configuration selected by the app animation and make
each 4 ms RSI offset produce the intended analytical TCP pose. TCP means tool
center point; PTP is a joint movement to a target, and LIN is a straight Cartesian
movement. The nominal sequence remains HOME, PsIn, LIN LeadStart, RSI lead-in /
machining / lead-out, LIN PsOut, HOME. Only successful RSI completion permits return.

Use the established names `PsIn`, `LeadStart`, `LeadEnd`, `PsOut` and `ToolFrame`.
`PsOut` copies only XYZ from `PsIn`; its ABC comes from `LeadEnd`, and its S/T comes
from the animation's own PsOut joint configuration. Do not add a controller S/T or
A1-A6 comparison before `RSI_ON`. Force correction and T1 Start-key pause/resume
are outside this repair. Pause/resume remains deferred rather than implemented.

## Progress


- [x] (2026-10-20) Existing foundation: analytical 4 ms sampling, queued stream delivery, nominal KRL sequence and shared-file export are implemented.
- [x] (2026-10-20) Existing diagnostics: paired plan/packet identifiers and bounded asynchronous raw-packet capture are implemented and present in user-provided records.
- [x] (2026-10-20) Existing resample reset: clear/replace the whole array, rewind index, discard buffered requests and reset learned peer/decode state; matching rebuilt records begin at offset zero.
- [x] (2026-10-20) Existing UI condition: RSI Sampling requires the socket's connected/bound state.
- [x] (2026-10-20) Consolidate five superseded RSI plans, preserve relevant evidence and deferred work, capture counts and record the user's final PsIn/PsOut and S/T decisions.
- [x] (2026-10-20) Step 1: calculate relative rotation offsets, reconstruct six-decimal offsets, check TCP residuals and remove obsolete absolute-ABC arithmetic/unwrapping.
- [x] (2026-10-20) Bring the dependent LeadEnd and summary corrections forward from Step 2: use the composed final pose and translation/shortest-angle excursions.
- [x] (2026-10-20) Before Step 2, trace existing checks through Generate, playback spline preparation, shared TCP IK and RSI Sampling; record cross-stage reuse and reduction requirements below.
- [x] (2026-10-20) Step 2: use existing IK/FK/rate checks on rounded reconstructed poses; share vector speed comparisons across Generate/LIN/RSI, remove repeated invariant/adjacent-rate work and replace rounding-sensitive endpoint equality with configuration/winding checks.
- [x] (2026-10-20) Step 3: reuse KUKA-compatible configuration classification on the four saved boundaries, check their exported poses and write the versioned eight-row layout with each point's own S/T.
- [x] (2026-10-20) Step 4: read version 2 and checked POS targets with global READ_POS, rename Ps to PsIn, retain PsOut's own S/T and construct its XYZ/ABC by explicit assignments; preserve cleanup and RSI completion without a live configuration comparison.
- [ ] Step 5: independently replay the full offset array and inspect the complete file/KRL exchange offline, including malformed inputs and resampling.
- [ ] Step 6: update affected contracts/comments, complete separate declaration/body/caller reviews, record final counts and run source/diff checks.
- [ ] Conditional verification: when explicitly requested, build/restart the app and inspect newly generated records; controller execution requires explicit hardware authorization or user-supplied results.

## Surprises & Discoveries


The matching `rsi_plan_20261020_110951_161.csv` and
`rsi_packets_20261020_110951_161.csv` are under
`build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/records/`. All six components of
all 2414 transmitted offsets equal plan row `stream_index + 1`, starting at index
zero. Maximum saved-versus-sent difference is zero. The calculation defect occurs
before storage; the socket does not change planned offsets. Older captures started
at stale indices with an unrecompiled executable and do not describe current reset.

That capture spans 9.660 s and ends with about 19.99 degrees of intended/reported
orientation difference. Replaying successive fixed-frame rotation offsets matches
reported orientation with about 0.053 degrees RMS error at a fitted 32 ms alignment.
RMS is root mean square across compared rotation errors; the fitted alignment is
not measured network latency. These records do not contain the substituted fault
axis or peak commanded joint velocity, so they do not prove KSS01102's exact trigger.

The same historical capture starts with planned A4/A5/A6 of
94.534516156/-102.098908149/-293.381370959 degrees and reported values of
-85.5/102.1/-113.4 degrees. They are different wrist configurations of nearly the
same TCP pose. These are capture-specific observations, not a claim about every
later run. A5's sign alone is not KUKA's definition of wrist-up/wrist-down.

`D:/RSI_Chamfer_Hole.rsi:93` specifies POSCORR1 `RefCorrSys="World"`.
`$BASE=$NULLFRAME` makes KRL BASE coincide with WORLD; it does not itself establish
the robot base's location in WORLD. The supplied snapshot
`resources/files/KR C4/R1/Mada/$machine.dat:98` defines a null `$ROBROOT`, the
robot-base pose in WORLD. This supports the nominal aligned setup but is not a
live reading. Preserve that explicit assumption; no station-transform UI or new
runtime alignment gate is planned here.

KSS 8.3 SI section 9.10, pages 301-304, states that POS/E6POS contain S/T, PTP uses
them, and LIN ignores them. S selects a Cartesian pose's joint configuration; its
bits describe shoulder, elbow and wrist alternatives. T records the six joint-angle
signs, not a revolution count. The CREAD/CWRITE manual section 6.16.10, pages 43-44,
permits at most eight `krl_fscanf` values, so XYZABC plus two integer S/T fields fit
one call. Both manuals are in `E:/Qt/QtProjects/manuals/KUKA/`.

Step 1 cleanup confirmed that the absolute Euler branch selection, A/C singular
coupling, 90-degree jump gate and six-component cumulative sums were specific to
Euler subtraction. None is needed to form a relative rotation from two matrices.
The existing atan2-based principal converter handles CSV/export representation,
including pitch at +/-90 degrees; emitted offsets are checked by matrix replay.
The accepted Generate arc maps are reused: progressAtDistance performs a knot search
and polynomial evaluation, not another integral or inverse-map construction.

Step 4 reader review confirmed that POS exposes six REAL and two INT scalar
components. Reading them directly keeps READ_POS to 16 lines and avoids eight
temporary scalars plus subsequent copies. The version guard can skip all body
reads on a mismatch while retaining false read flags, then use the existing
common file-close/cleanup path before rejecting the job.

Step 3 review confirmed model-compatible KUKA bit meanings without changing the
classifier. An allowed alternative A6 revolution left the staging TCP unchanged
while changing Turn from 50 to 18; saved PsOut therefore needs its own S/T.

## Decision Log


Decision (2026-10-20, Step 3 implementation): KSS section 9.10 and Figure 9-39,
reviewed with the fixed KR10 model, require no classifier change. Its shoulder
test uses wrist-center X in the rotating A1 frame; its elbow threshold includes
the A3/A4 offset; its wrist test uses the model's A4-zero convention. The current
KR10 has A5 HOME=0 and positive local axis, so negative A5 tilts the flange axis
upwards at A4=0; this is model-specific, not a universal wrist-direction rule.
Turn's negative-angle bits match the manual exactly. Keep existing IK semantics.

Decision (2026-10-20, user): retain Cartesian motion targets, rename Ps to PsIn,
include animation-derived S/T for all four named motion points, and preserve
PsOut XYZ copying from PsIn. Do not add the previously proposed S/T comparison
before RSI_ON. The former axis-row export and READ_AXES proposal are superseded.

Decision (2026-10-20, Codex): transmit PsOut's own S/T as two integers, construct
its XYZ from PsIn and its ABC from LeadEnd in KRL. A second XYZABC row for PsOut
would duplicate data the user explicitly wants copied. Copying a complete POS
from PsIn would also copy inappropriate S/T. ToolFrame remains FRAME because it
describes a TCP relative to the flange, not a robot motion configuration.

Decision (2026-10-20, Codex): replace rotation-offset arithmetic in the sampler,
preserving the sender, reset, diagnostic pairing and six-decimal wire protocol.
A preceding reconstructed rotation is required for the next offset; replace/reuse
obsolete sampler state before adding a worker-local matrix. No new class is needed.

Decision (2026-10-20, Step 1 implementation): replace the summed XYZABC state with
one reconstructed TCP transform; summed ABC cannot represent composed rotations.
Retain its initial rotation once for the displayed angular excursion, avoiding
constant Euler-to-matrix work in every cycle; no new helper/class is required.
Replace the two six-component range fields with maximum translation-component and
shortest rotation-angle excursions from the start, used only by the GUI summary.
Their old values and the private largestOffset helper cannot describe composed
rotations; update their sole consumer together. This diagnostic is not MaxRotAngle.

Decision (2026-10-20, Step 1 cleanup): remove absolute Euler unwrapping, the previous
absolute-angle field and the 90-degree Euler-coordinate rejection. These supported
the old subtraction and are unnecessary for matrix offsets; principal Z-Y-X ABC
still represents every CSV target and exported pose. Check exported orientation
representability and rounded reconstruction with the existing residual tolerances.
LeadEnd and range corrections move forward from Step 2 because retaining sums of
rotation offsets would leave Step 1 internally inconsistent. Rounded-pose IK/FK
and its endpoint-winding review remain Step 2; no second IK path is introduced.

Decision (2026-10-20, user clarification before Step 2): inspect producers, shared
kinematics and all check callers across the project, rather than confining reuse
to the RSI sampler. Step 2 changes inputs/order of existing checks; it must not add
another validation pass or copy checks already available elsewhere. Consolidate
matching operations only when total code shrinks and failure behavior is retained.

Decision (2026-10-20, Step 2 implementation): reuse solveTcp for the rounded start
and each rounded reconstructed sample, and use the preceding reconstructed position
already stored by the sampler for its velocity calculation. Remove the separate
ideal-position field and old ideal-pose IK/rate work; no additional IK pass is needed.

Requirement for the shared speed helper: Generate central points, LIN preparation
and RSI Sampling repeat the same finite-vector/per-axis speed comparison with
the same 1e-6 relative tolerance and accepted rated bounds. No common comparison
currently exists, so extract one two-input free function in the existing
chamfertcpsolver.h and remove the duplicate bodies; do not add a class/header or
general validator. Preserve calculation-specific times and failure context at callers.

Decision (2026-10-20, Step 2 reduction): keep unchanged model/tool invariants at
ChamferMotion::create/validTiming. The only ChamferCentralPreparation::validate
caller is MotionCompiler::compile, borrowing that same immutable accepted setup;
remove its repeated rigid/validRobot helpers while retaining the changing seed check.
Use the already resolved timing.jointSpeed directly; keep spline extrema checks
and their scalar comparisons because they examine different polynomial results.

Decision (2026-10-20, Step 2 Generate-rate audit): appendPoint already accepts
each adjacent joint secant speed. In inspectRates use an anchor index instead of
copying the complete prior target/sample, and skip the identical speed check for
adjacent indices. Keep the comparison for a grouped interval spanning skipped
nodes because its average is a separately calculated result; preserve its original
time grouping and acceleration/terminal-stop checks.

Decision (2026-10-20, Step 2 endpoint): replace the 1e-6 degree endpoint equality
with the saved endpoint's existing configuration classification and nearest joint
revolution, retaining all preceding TCP residual and joint-limit/rate checks.
Saved animation points retain angles/TCP but not configuration/link arrays, so reuse
configuration with one endpoint FK rather than adding stored state or a new classifier.
Reject different Status or a per-joint difference of 180 degrees or more; that is
the halfway boundary between 360-degree representatives, not a pose tolerance.
Do not compare Turn sign bits, which can change for tiny angles crossing zero.

Evidence for the endpoint decision: a reference replay of the recorded targets,
using the source KR10 model and rounded offsets, gives an A4 change of
2.8862824e-6 degrees versus the ideal solution at t=70.628 s. Position error is
7.2638e-7 mm, maximum rotation-entry error 1.0112e-8, with identical Status.
Using that valid rounded pose as an endpoint would fail the old scalar comparison.
The supplied full record's actual terminal difference is only 7.0113e-7 degrees
and passes both criteria; this is evidence of the criterion's rounding sensitivity,
not a claim that the user's recorded endpoint itself failed Sampling.

Decision (2026-10-20, Codex): reuse saved ChamferMotion joints and existing
Kr10Kinematics::configuration; do not run another IK to choose export configurations.
A small READ_POS KRL helper is justified because READ_FRAME returns six components
and cannot preserve S/T. Keep READ_FRAME for ToolFrame. New C++ helpers, fields or
three/four-parameter signatures require their own short justification before edits.

Decision (2026-10-20, Codex): the shared-file layout changes together with its app
producer and KRL consumer. A numeric version prevents the new reader accepting the
old layout. No extra robot configuration/alignment comparison is added. Existing
file, numeric and motion preparation failure handling remains required.

Decision (2026-10-20, Codex): delete the five superseded RSI plans after preserving
their implemented behavior, evidence, remaining repair and deferred pause/resume
here. Keep unrelated research and POST_MERGE_CLEANUP, which still owns deferred
architecture items; correct its references to the current RSI plan.

Decision (2026-10-20, Step 3 preparation): merge sharedDataPayload into the existing
two-input shared-file writer (stream and immutable motion). The new association
checks can then return the existing QString error before opening QSaveFile; a
separate payload/error type, persistent S/T fields or another IK are unnecessary.
Reuse configuration with one FK per exported boundary, since saved points retain
TCP and angles but not the link frames required by that classifier. Keep each
FK array local and use it once; do not add configuration storage to the stream.

Step 3 baseline: one affected C++ file, scenemachiningpreparation.cpp, has 208
physical / 187 nonblank lines. Four documentation files have 1763/1510 lines:
AGENT_CONTRACTS 344/327, ARCHITECTURE 519/443, RSI contract 167/139 and this plan
733/601. No QML, build inputs, tooling or controller source is scheduled to change.
Capture fresh finish counts and separate declaration/body/caller findings below.

Decision (2026-10-20, Step 4 preparation): READ_FRAME returns only XYZABC, so a
small global READ_POS will read eight scalar POS components directly and return
the POS plus the established KRL success flag. Reusing FRAME would need a second
read and six component copies, or change an existing cwrite_test consumer; the
direct eight-value operation is smaller and keeps failure handling local.
The CREAD/CWRITE manual pages 27-28 and 35 define RET1 #DATA_OK as successful
execution; pages 43-44 permit exactly the required six REAL and two INT outputs.
Use initialized S/T sentinels and ranges 0-7 / 0-63. A single local ReadState can
check version and PsOut S/T immediately; retain the existing rate/close states.
Wrong versions skip body reads and retain false read flags; close the file once
before rejecting those flags, avoiding a duplicated cleanup branch.

Step 4 baseline: two controller sources, SRC 119/105 and DAT 12/12, total
131 physical / 117 nonblank lines. READ_POS is absent (zero files/lines).
Unchanged READ_FRAME is 46/33, so including that inspected helper the baseline
is three files, 177/150. Documentation is four files, 1916/1645: AGENT_CONTRACTS
354/337, ARCHITECTURE 519/443, RSI contract 182/152, plan 861/713. No C++, QML,
build or tooling source change is required. Record finish counts below.

## Context and Orientation


`src/trajectories/chamfer/chamferrsistream.cpp` owns RsiStreamSampler. It evaluates
ChamferCentralMotion at 0.004 s intervals and the exact final time, then appends
50 zero settle offsets. Targets come from the analytical path and the existing
piecewise quintic distance-to-parameter inverse, not stored-pose interpolation.
It now reconciles XYZ against the reconstructed position and extracts ABC from
the desired rotation multiplied by the preceding reconstructed rotation's transpose.
It composes rounded offsets and checks the resulting TCP residual. Existing IK/FK,
joint-speed and XYZ acceleration checks use the rounded reconstructed movement;
the composed TCP stays the offset reference and accepted joints seed the next IK.

`src/trajectories/include/trajectories/chamferrsistream.h` publishes the returned
stream. Six-decimal initialCoordinates represents exported LeadStart; finalCoordinates
comes from the composed final pose. Two scalar summary fields replace the old
six-component minimum/maximum ranges. `src/application/scene/scenemachiningpreparation.cpp` writes
`C:/RoboCrapJobs/shared_data.txt` after a successful current worker result, then emits
the array and identifier. `src/devices/rsi/rsidevice.cpp` sends each entry unchanged.

The immutable ChamferMotion already saves joint angles and FK-derived TCP poses.
FK means calculating link/TCP transforms from joint angles; IK calculates joint
angles for a requested TCP pose. Its `points()[boundaries()[1]]` is PsIn,
`[boundaries()[2]]` is LeadStart, `[boundaries()[5]]` is LeadEnd and
`[boundaries()[6]]` is PsOut. Use these exact four points, not approximate playback
times. `src/kinematics/kr10kinematics.cpp` supplies configuration classification.

`resources/files/rsi/RSI_Chamfer_Hole.src` requires version 2 and reads three POS
targets with global `resources/files/READ_POS.src`, PsOut's S/T pair, ToolFrame
with `resources/files/READ_FRAME.src`, and the two CP settings. It closes the file
before rejecting input/close/rate errors, assigns ToolFrame, executes `PTP PsIn`
and `LIN LeadStart`, then enables RSI. On successful completion it assigns PsOut
XYZ from PsIn and ABC from LeadEnd, preserving PsOut's own S/T. Its DAT declares
the four POS values and FRAME ToolFrame. MOUNT_JOBS/CLOSE_FILES already work;
preserve their behavior and never copy share credentials into plans or diagnostics.

## Plan of Work


### Step 1: calculate relative WORLD rotation offsets


In startCycle initialize reconstruction from the accepted LeadStart XYZABC rounded
to the same six decimals written to shared_data.txt. In appendCycle evaluate the
desired analytical pose at the existing sample time. Derive XYZ offsets from desired position minus the preceding rounded
reconstructed position. Derive the rotation offset by multiplying the desired
WORLD rotation matrix by the transpose of the preceding reconstructed WORLD
rotation matrix. Extract Z-Y-X Euler ABC in degrees from this relative rotation.
Round all six offset components with the existing wireValues operation.

Reconstruct the next position by adding rounded XYZ. Reconstruct the next rotation
by multiplying the rounded offset's rotation matrix by the preceding reconstructed
TCP rotation matrix, in that order. Use existing euler2rot and robust local principal
Euler conversion/error helpers. CSV targets and exported poses use principal Z-Y-X
ABC; do not unwrap or reject absolute Euler-coordinate jumps to generate matrix offsets.
Leave the 4 ms schedule, analytical geometry, feed and UDP sender unchanged.

Acceptance for this step is an offset array whose independent WORLD replay follows
the requested poses. Check finite matrices and existing residual tolerances before
accepting each entry. Do not claim an unbuilt implementation has run.

### Step 2: check reconstructed movement and fix dependent calculations


Implemented by moving the existing ChamferTcpSolver::solveTcp call after rounded
reconstruction, including at LeadStart. Position limits and flange/TCP FK residual
checks remain in that shared operation; there is one IK/FK pair per sampled pose.
Joint rates use its returned angles. XYZ velocity uses the previous reconstructed
position already held by the sampler, removing the separate ideal-position field.
The existing acceleration/terminal-stop checks, exact endpoint time, 50 zero settle
offsets and disabled joint-acceleration magnitude rejection remain unchanged.

chamferJointSpeedError now owns the finite-vector/per-axis comparison for central
animation points, LIN preparation and RSI, using accepted model rates and the
existing 1e-6 relative tolerance. Central inspectRates skips comparisons of identical
adjacent secants accepted in appendPoint, retains grouped-interval checks, and uses
an index instead of copying a full sample. Pose/configuration checks, calculation-
specific time intervals, diagnostic context and failure exits remain at their callers.

Model/tool/timing invariants are accepted once at ChamferMotion::create/validTiming.
The inner central validator borrows this immutable setup; its redundant rigid and
validRobot helpers were removed, retaining its changing seed check. Duplicate
min(model speed, resolved timing speed) calculations were removed. Scalar spline
extrema checks remain because their inputs differ from sampled vector rates.

Generate's nodal checks and jointSplineIntervalError do not establish the bounds
of the different sequence of analytical 4 ms RSI poses. For example, hypothetical
Generate nodes at 0 and 0.020 s do not contain the RSI IK solution at 0.004 s;
the checked cubic joint spline between those nodes is not the analytical TCP curve.
Retain checks of genuinely new poses/rates. Do not revalidate unchanged model/tool
inputs, rebuild inverse maps or repeat IK/FK for the same accepted sample.

Step 1 supplies composed finalCoordinates and translation/shortest-angle excursions.
Step 2 replaces 1e-6 degree endpoint equality with the saved endpoint's Status and
nearest 360-degree representatives: reject a different Status or any angle difference
of at least 180 degrees. TCP accuracy continues to use the existing residual checks.
One endpoint FK supplies the saved configuration through the existing classifier;
there is no second endpoint IK or new persistent configuration state. The Decision
Log records rounding evidence and why Turn sign bits are not a winding test.

The summary's rotation diagnostic is the maximum shortest angle from LeadStart
in [0,180] degrees; it does not count full turns or claim a POSCORR MaxRotAngle bound.
Preserve the stable one-line UI and controller limits. Keep existing CSV columns
and paired IDs unless a required verification cannot use them; ideal targets and
checked reconstructed joints differ.

### Step 3: derive S/T and export the agreed shared-file format


writeSharedData now takes the stream and saved motion; the separate
sharedDataPayload forwarding operation was removed so pose failures use the
writer's existing QString error result before file access.
Timing and all four saved configurations then come from the same immutable motion;
do not duplicate joints/configurations in ChamferRsiStream or calculate IK again.
Use existing configuration(joints, FK frames) to classify each saved point.
Classification needs link frames that the saved TCP alone does not retain; one FK
per exported boundary supplies them without adding an IK or persistent state. Review
KUKA bit compatibility first: shoulder area comes from wrist-center position
in the A1 frame, elbow classification accounts for the KR10 A3/A4 offset, and the
wrist definition refers to A4's zero position. Do not infer physical wrist direction
from the sign of A5 alone. T sets bit 0 through bit 5 for negative A1 through A6.
Fix classification only if this review demonstrates a mismatch; record the evidence.

Export PsIn XYZ at the six-decimal staging position with LeadStart ABC; export LeadStart from
initialCoordinates and LeadEnd from the corrected finalCoordinates. Check these
poses, including the PsOut constructed from rounded PsIn XYZ and LeadEnd ABC,
against their corresponding saved TCP poses with existing tolerances. Their
S/T comes from their own saved joints. Export PsOut S/T from boundary 6 independently
of PsIn and LeadEnd, because the return-staging joint signs/winding can differ.

The file has eight CRLF rows. Floats use six decimals and XYZABC order; XYZ is mm,
ABC degrees, S/T decimal integers. The following comments explain the layout and
are not written to the numeric file:

    2                                      # Format version for this layout.
    PsIn.X;PsIn.Y;PsIn.Z;PsIn.A;PsIn.B;PsIn.C;PsIn.S;PsIn.T
                                           # Incoming staging PTP pose/configuration.
    LeadStart.X;LeadStart.Y;LeadStart.Z;LeadStart.A;LeadStart.B;LeadStart.C;LeadStart.S;LeadStart.T
                                           # Incoming LIN endpoint; RSI starting pose/configuration.
    LeadEnd.X;LeadEnd.Y;LeadEnd.Z;LeadEnd.A;LeadEnd.B;LeadEnd.C;LeadEnd.S;LeadEnd.T
                                           # RSI lead-out endpoint pose/configuration.
    PsOut.S;PsOut.T                         # Return-staging configuration only.
    ToolFrame.X;ToolFrame.Y;ToolFrame.Z;ToolFrame.A;ToolFrame.B;ToolFrame.C
                                           # TCP relative to flange; assigned to $TOOL.
    TransferVelocity                       # App transfer feed in mm/s divided by 1000, m/s.
    TransferAcceleration                   # App transfer acceleration in mm/s^2 divided by 1000, m/s^2.

Preserve QSaveFile replacement, stale-result rejection, file deletion and empty-array
publication on every resample. Steps 3 and 4 are one coordinated producer/consumer
change; do not deploy their intermediate states separately.

### Step 4: read POS targets and preserve PsOut construction in KRL


Implemented `resources/files/READ_POS.src`, following the existing small READ_FRAME helper:
read `%f;%f;%f;%f;%f;%f;%d;%d` directly into a local POS's six REAL and two INT components and return that POS
plus the existing KRL success flag convention. The C++ prohibition on output
references does not change KRL's established function convention. Keep READ_FRAME
for ToolFrame; avoid another generic parser or an axis-data helper.

Update RSI_Chamfer_Hole.dat to declare PsIn, LeadStart, LeadEnd and PsOut as POS;
ToolFrame remains FRAME. Remove the old local FRAME PsOut declaration. Update SRC
to read/check version 2, read the three POS rows, read PsOut.S/T with `%d;%d`, then
read ToolFrame and both rates. Keep cleanup on failures and reject incomplete
values, invalid S/T ranges, wrong versions and invalid transfer settings before
trajectory-specific movement. Use KUKA bits 0-2 for exported S (0-7) and bits 0-5
for T (0-63); offline-taught informational Status bit 4 is not generated.

Rename every active Ps reference to PsIn. Preserve `PTP PsIn`, `LIN LeadStart`,
RSI_ON/MOVECORR and successful completion handling. For PsOut assign X, Y and Z
individually from PsIn; assign A, B and C individually from LeadEnd. Its own S/T
has already been read. Do not copy the complete POS or overwrite PsOut.S/T with
another point's values. Preserve `LIN PsOut` and PTP HOME.

There is no S/T or A1-A6 comparison before RSI_ON and no extra alignment gate.
S/T affects PTP PsIn; LIN ignores those fields. LeadStart/LeadEnd/PsOut still carry
the app's intended configuration as complete point data. This does not turn them
into LIN configuration constraints, change RSI offsets or add a new motion stage.

### Step 5: replay the complete result and inspect producer/consumer behavior


Replay every offset from the exported LeadStart pose using WORLD translation and
successive WORLD rotation composition. Compare reconstructed poses with analytical
targets at every sample, all phase boundaries and the final pose. Include the
50 zero settle cycles. Report maximum XYZ error in mm and rotation error as a
rotation-matrix difference/angle; distinguish movement duration from computation.

The supplied example interval is central time 9.652 to 9.656 s. Desired absolute
ABC changes from (-101.881475251,-88.117426209,-13.517949417) to
(-101.952582786,-88.127795681,-13.463622358) degrees. Current saved/sent ABC offsets
are (-0.071108,-0.010370,+0.054327); relative WORLD rotation between those ideal
poses instead gives about (-0.016810,+0.000398,-0.010514) degrees. Reconciliation
against the preceding rounded rotation can change the corrected last digits.
Use matrix replay, not Euler subtraction, to judge agreement.

Inspect all four exported S/T pairs against their own saved joints and model frames.
Check PsOut copies PsIn XYZ and LeadEnd ABC without copying their S/T. Check the
eight-row producer and reader agree on types/order, wrong version and truncated
rows fail before staging, and an old six-field Ps row is not accepted as a new POS.
Preserve Start/Result/Stop ordering, empty-array behavior, index-zero restart and
shared log identifiers. Do not change correct transport code to compensate for
incorrect geometry. Use ephemeral offline calculations, not new project tests or
harness/build targets. CSV reference calculations prove the mathematics, not the
execution of uncompiled C++ or KRL.

### Step 6: document the final implementation and review the actual changes


Update only affected constraints in docs/AGENT_CONTRACTS.md,
docs/ARCHITECTURE.md and resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md: WORLD
offset composition, corrected final/range calculations, file layout, POS values,
PsIn/PsOut construction and the fact that LIN ignores S/T. Correct stale claims
that RSI has never run by referring to user-supplied captures without claiming a
successful complete run. Keep tutorials and completion reports in this plan.

Re-read Mandatory C++ style/reduction before every logical code change. Separately
review changed declarations, bodies and callers, recording and fixing concrete
file/line violations. Check result channels, borrowed-model lifetimes, owner-only
state mutation, no lambdas/templates/project throws, and justified parameter counts.
Count helpers, headers, callers and configuration together. Prefer replacing obsolete
state and reusing existing operations; explain required growth against a smaller
implementation. Record fresh before/after physical and nonblank counts per step.

## Concrete Steps


Work in `E:/Qt/QtProjects/RoboCrap`. Read AGENTS.md, PLANS.md and the Geometry and
scene, Machining and playback, and Devices and protocols contracts before the
affected implementation work. Inspect the named files and current callers. Execute
Steps 1-6 in order, updating Progress and recording checks at each stopping point.

Run these read-only commands from the repository root; expect diff --check to exit
zero without whitespace diagnostics. The per-command safe.directory exception is
needed on this executor; do not change global Git configuration:

    git -c safe.directory=E:/Qt/QtProjects/RoboCrap status --short
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check

Inspect task diffs and new/untracked contents separately. No staging, commits,
branch switches, stashing or discarding unrelated edits. Do not configure/build/lint
or connect to equipment during source/offline verification.

## Validation and Acceptance


Source/offline acceptance means the first offset is still index zero, every saved
offset is the one serialized, matrix replay follows the requested poses within
existing TCP tolerances, checked reconstructed joints retain the intended branch/
winding and satisfy existing bounds, and exported finalCoordinates represents the
actual composed final pose. File/KRL acceptance requires the stated eight-row
format, PTP PsIn with animation-derived S/T and the exact PsOut copying behavior.
No pre-RSI S/T comparison is an acceptance requirement or implementation step.

When a build is explicitly requested, inspect CMakeCache.txt for source directory,
Qt/compiler paths and mode before reuse. Build RoboCrapTrajectories and robocrap
for these numerical/application changes using `cmake --build <verified-build-dir>
--target RoboCrapTrajectories robocrap`; replace the placeholder with the inspected
directory. Do not infer C++ execution from a Python reference replay. A disconnected
runtime can inspect Generate/Dry Run and UI stability; existing CSV captures support
offline numerical checks without opening an RSI socket. Actual Sampling remains
gated by the connected/bound socket, whose use requires hardware authorization.

For an explicitly authorized controller deployment/user-run verification, deploy
the app, matching SRC/DAT and READ_POS alongside existing helpers. Check the supplied
RSI Visual context and Ethernet XML match the deployed versions; preserve WORLD
and limits, and do not hand-edit exports. The repository lacks the three context
export files, so retain their deployment/provenance check from the old plan.
User-supplied matching logs should show index-zero offsets, exact saved/sent equality,
the intended initial configuration, intended TCP orientation through lead-out and
normal Result/Stop completion before return. This planning request authorizes none
of those hardware actions. KRL compilation, physical clearance and a full successful
run remain unverified until actual results exist; no pre-RSI comparison is added.

## Idempotence and Recovery


Repeated sampling continues to clear and replace the whole stream and shared file.
Failure leaves no old array/file available for a different trajectory. Preserve
original CSV evidence and IDs. Update app/file/KRL format together; incomplete or
wrong-version input returns through existing cleanup rather than starting staging.
An old KRL reader must not be deployed with the new format. Preserve HOME, load
settings, $TOOL assignment, transfer units and normal completion acknowledgement.
No source recovery may discard the user's unrelated edits.

## Interfaces and Dependencies


Keep C++17, Qt >= 6.8, existing Eigen/Qt numerical APIs, QVector<V6d>/QString queued
delivery, current public C++/QML interfaces and thread ownership. The private
writeSharedData takes const stream and motion inputs and returns its error string;
serialization and export association checks complete before opening QSaveFile.
The shared-file protocol and matching KRL POS reader are the intended interface
changes. New helpers/classes/fields must be justified before implementation.
No force correction, network/XML changes, new limit profiles or new UI are required.

## Artifacts and Notes


Before this consolidation, the five RSI plans contain 2005 physical / 1553 nonblank
lines: sampling 728/533, overspeed diagnostics 492/410, resampling reset 317/250,
WORLD repair 292/220 and log pairing 176/140. POST_MERGE_CLEANUP is 453/370. Thus
the documentation edit scope starts at six files, 2458/1923. Replace these five
plans with this one and correct the three references in the retained cleanup file.
Unrelated research remains useful, including explicit citations from owning docs.

The future expected C++ edit scope is three files, 551/495: chamferrsistream.cpp
301/272, its public header 33/28 and scenemachiningpreparation.cpp 217/195. The
inspected sender is unchanged at 344/320. Kinematics implementation/header are
303/223 and 87/70; capture a new baseline if classification changes are justified.
No C++, QML, build input, tooling or controller source is changed by this planning task.

Step 1 implementation starts from three C++ files, 551 physical / 495 nonblank
lines: chamferrsistream.cpp 301/272, chamferrsistream.h 33/28 and
scenemachiningpreparation.cpp 217/195. Its documentation scope starts from four
files, 1413/1203: this plan 420/328 and the three owning docs listed below.
No QML, build input, tooling or controller source change is planned for Step 1.

Step 2 implementation baseline: five C++ files, 1505 physical / 1391 nonblank
lines: chamferrsistream.cpp 283/258, chamfertcpsolver.h 64/56,
chamfercentralvalidation.cpp 390/359, its header 63/54 and chamfermotion.cpp 705/664.
Documentation starts at four files, 1588/1357: this plan 583/471,
AGENT_CONTRACTS 335/318, ARCHITECTURE 512/437 and RSI contract 158/131.
No QML, build input, tooling or controller source changes are needed for Step 2.

Existing controller inputs are three files, 177/150: RSI_Chamfer_Hole.src 119/105,
DAT 12/12 and READ_FRAME.src 46/33. READ_POS.src has baseline zero files/lines and
must be included when created. Owning documentation is three files, 993/875:
AGENT_CONTRACTS 330/313, ARCHITECTURE 512/437 and the RSI contract 151/125.
Record future source and documentation growth separately from computation cost.

Consolidation after-counts and content/diff results are recorded in Outcomes below.

## Outcomes & Retrospective


Before Step 1, the overall repair was specified but Steps 1-6 were not implemented.
The completed sampling/reset/logging foundation is preserved in the current source. The deleted
plans contained superseded math, axis-export/start-check proposals or completed
implementation history; this plan retains current requirements and necessary evidence.
No build, runtime launch, socket connection or hardware action was performed.

Documentation consolidation: six files, 2458 physical / 1923 nonblank lines, become
two files, 874/699 (plan 420/328; cleanup 454/371). Net reduction:
1584 physical / 1224 nonblank lines. Content/reference checks and diff --check passed.
The retained cleanup file has only RSI-plan reference changes. All five obsolete
RSI plans are absent and no remaining document references their former filenames.

Revision (2026-10-20): consolidate RSI plans after the user selected Cartesian
PsIn/PsOut with animation-derived S/T, preserved PsOut XYZ copying, rejected the
pre-RSI configuration comparison and requested obsolete-plan removal. Separate
remaining offset/configuration repairs from already completed transport/reset work.

Step 1 outcome (2026-10-20): implemented relative rotation offsets and feedback of
the rounded reconstructed TCP pose. XYZ is added independently at the TCP; the
offset rotation is multiplied before the preceding rotation, without rotating
the TCP position. LeadStart matches the six-decimal shared-file values and LeadEnd
comes from the final composition. Required growth is the matrix composition,
initial/final representation checks and each new rounded-pose residual check;
reusing the old scalar ABC arithmetic cannot provide that behavior.

Removed four helpers: unwrapDegrees, unwrapEulerDegrees, continuousEulerDegrees
and largestOffset. Removed m_sent, m_previousAngles, the 90-degree absolute Euler
gate, both six-component range fields and the GUI's unused algorithm include.
Absolute ABC is now extracted only for initial/final/tool export or an open CSV
trace, never to derive a transmitted offset. No new helper/class/header/configuration
or forwarding layer was added. The fixed initial rotation is stored once for the
summary instead of reconstructing the same reference on every cycle.

The post-change audit followed SceneMachiningPreparation's copied-input worker,
the sampler, ChamferCentralMotion::evaluate, progressAtDistance and ChamferPath,
then the GUI/export consumer and unchanged RsiDevice serializer. Keep analytical
geometry/basis checks, inverse-map evaluation, seeded IK/FK, joint position/speed,
XYZ acceleration/terminal-stop checks, exact endpoint time, 50 settle zeros and
stale-result/reset/logging behavior: these have independent requirements. Analytical
position/velocity state remains necessary for the existing acceleration checks
until Step 2 changes their inputs; it is not the reconstructed-position feedback.

Offline verification used ephemeral Python/NumPy calculations, with no project
test/harness files or build targets. It read every analytical XYZABC target in
build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/records/rsi_plan_20261020_110951_161.csv,
rounded initial XYZABC and every offset with half-away-from-zero rounding, and
included euler2rot's 1e-9 small-component cleanup in reconstruction. All 20114
movement intervals passed 1e-4 mm position / 1e-6 rotation-entry tolerances.
Maximum errors: 8.3714e-7 mm position, 1.4687e-8 rotation entries and 9.1446e-7 degrees.
Final six-decimal LeadEnd adds at most 8.3422e-9 rotation-entry error relative to
the composed endpoint. Fifty zero offsets leave the endpoint exactly unchanged.
The 9.656 s offset is XYZ (0.003747,0.000094,-0.001397) mm and ABC
(-0.016810,0.000398,-0.010513) degrees; rounded feedback explains the last digit
relative to the earlier two-ideal-pose example. Eight additional calculations
covered the accepted 90/0/0 examples, +/-180 wrapping, both +/-90 pitch crossings
and a zero offset. These are reference-math checks, not execution of edited C++.

Manual declaration review: chamferrsistream.h:12-23 has explicit zero defaults and
documents units/new scalar result meanings; its public sampling entry still has
one input and returns the optional stream/error. chamferrsistream.cpp:219 takes
two inputs for recordSample and mutates only its owner's trace. No new signatures
have three/four inputs, output references, templates, lambdas or project throws.
The existing SceneMachiningPreparation constructor's three distinct responsibilities
remain explained in its unchanged header. No external private state is accessed.

Manual body review: chamferrsistream.cpp:157-215 checks initial finiteness before
Euler reconstruction, forms each rotation product as a materialized M3d before
updating its prior pose, and rejects rounded-pose residual failures before append.
The initial Euler-to-matrix calculation was briefly repeated during implementation;
lines 168-170 now reuse m_initialRotation for both its residual and later composition.
Old branch checks were replaced by the actual rounded-pose check, not bypassed.
Lines 133-139 check final exported ABC; line 223 constructs principal CSV ABC only
after confirming the trace is open. No C++ style violation remained in changed bodies.

Manual caller review: scenemachiningpreparation.cpp:110-120 is the only reader of
the new range fields; old field/helper searches in active src return no matches.
The sharedDataPayload calls at lines 41-44 keep the six-row layout and XYZABC order.
RsiDevice still serializes each stored six-decimal offset unchanged; its reset,
Start/Result/Stop state and queued delivery are unaffected. Main.qml:218-262 reserves
footer height and elides status text; only formatted values change, with no layout
or control additions. No runtime visual check was performed.

Step 1 source counts: three C++ files, 551 physical / 495 nonblank lines, become
524/473 (sampler 301/272 to 283/258; header 33/28 unchanged; GUI preparation
217/195 to 208/187). Net reduction: 27 physical / 22 nonblank lines and four helpers.
There are no changes to QML, build inputs, tooling, numerical geometry/inverse-map
sources or controller sources in this step. Source size is not a computation-time
measurement. Owning docs and this living plan are counted separately below.

At completion of Step 1, rounded-pose IK/FK/limit and endpoint-winding work remained
open. KRL still uses
the old FRAME/shared-file layout without animation S/T. No build, runtime launch,
RSI socket connection or hardware action was performed during Step 1.

Step 1 documentation counts: four files, 1413 physical / 1203 nonblank lines,
become 1547/1321 (plan 542/435; AGENT_CONTRACTS 335/318;
ARCHITECTURE 512/437; RSI contract 158/131). Growth 134/118 records
the implementation decisions, separate compliance review, evidence and remaining work.
Focused diffs, legacy-symbol searches, untracked plan/contract content checks and
git diff --check passed. Pre-existing edits outside these seven files were preserved.

Revision (2026-10-20): implement the user-authorized Step 1 and remove obsolete
Sampling calculations. Bring dependent endpoint/summary changes forward to avoid
retaining invalid ABC sums; keep rounded-pose joint checks and S/T/KRL changes pending.

Revision (2026-10-20, Step 2 preparation): the project-wide read-only audit confirmed
that the required Sampling checks already exist and share TCP IK with Generate.
Clarify Step 2 as replacing their input sequence and removing old work, with a
cross-stage review of repeated invariant checks and speed-bound comparisons.
No C++ source was changed and no build or runtime/hardware action was performed.

Step 2 outcome (2026-10-20): rounded LeadStart and each reconstructed TCP pose now
enter the existing checked TCP solver. Existing joint-speed, XYZ acceleration and
terminal-stop checks use that movement; each offset is appended only after they
pass. The ideal-position field and ideal-pose IK/rate path are gone. The composed
TCP remains feedback for offset generation, independently of the checked FK result.
The 4 ms schedule, quintic maps, geometry, shared-file layout and sender are unchanged.

Cross-stage reduction: one two-input chamferJointSpeedError replaces the central
and RSI methods and the LIN comparison loop. It preserves the finite-vector check,
axis order, resolved model bounds and 1e-6 relative tolerance; diagnostic text is
normalized while phase/time context remains at callers. The scalar spline check
uses those same resolved bounds without a redundant min. Removed central rigid and
validRobot helpers and their unused geometry-utils include; no class/header was added.
Four helper bodies were removed and one shared helper added, a net reduction of three.

Proof for removed invariant checks: MotionCompiler is constructed only after
ChamferMotion::create accepts model/tool and validTiming accepts the resolved timing.
Its const robot reference is passed to the sole central validate caller at
chamfermotion.cpp:260; no intervening operation mutates that setup. The removed
guard checks a subset of these same fields with the same rigid/axis tolerances.
The per-candidate seed and every newly solved pose still receive their own checks.

Proof for skipped adjacent rate checks: appendPoint calculates the same signed
joint difference divided by the same two stored times as inspectRates when its
anchor is index-1; samples are immutable during inspection. Grouped averages retain
their own check. Joint angles are finite and bounded by the accepted model (at most
1e6 degrees) and the rate pass requires at least 1e-4 s, so its joint velocities
cannot overflow; their repeated allFinite check was removed while XYZ finiteness
and joint/XYZ acceleration finiteness remain. The shared helper checks new vectors.

Offline numerical verification used the existing recorded XYZABC targets and
tool metadata plus the joint rows/HOME from src/application/robot/robotdefinition.cpp.
An ephemeral NumPy reference follows the source IK/FK equations, offset rounding,
reconstruction and rate calculations; it is not execution of compiled C++.
All 20114 intervals and the rounded start passed joint position, flange/TCP FK,
joint-speed, XYZ acceleration and final-stop checks. Maximum A1-A6 speeds were
(1.740905,1.867695,1.266402,19.111227,5.614089,17.684492) deg/s, below
(220,210,270,381,311,492). Maximum XYZ acceleration was 0.492126 mm/s^2;
the final stop was zero. Maximum FK residuals were 8.9359e-8 mm / 5.8651e-9 entries
at the flange and 1.2051e-6 mm / 5.9078e-9 entries at the TCP.
Offset reconstruction retained Step 1's 8.3714e-7 mm / 1.4687e-8 entry maxima.

The actual recorded endpoint keeps Status 6 and passes both old/new comparisons;
the earlier Decision Log distinguishes it from the larger interior rounding example.
Reference wrong-wrist and extra-360-degree endpoint alternatives are rejected.
Three additional alternatives stay within joint limits and retain the endpoint TCP
to below 1.2e-13 mm / 4.2e-16 rotation entries; the branch/winding criterion rejects
all three, confirming that it checks information absent from TCP pose agreement.
Additional ephemeral checks covered 42 signed finite speed-bound cases, 18 NaN/Inf
cases and three time schedules: uniform adjacent intervals, grouped short intervals,
and a mixture. Identical repeated comparisons were skipped; grouped checks and
terminal grouping were retained. No project tests, harnesses or build targets were added.

Separate declaration review: chamfertcpsolver.h:24 has two const input references
and returns its error through QString; solveTcp:55 retains its optional/error result
and borrowed setup lifetime. No new signature exceeds two inputs or introduces
output channels, project templates, lambdas or exceptions. The unchanged three-input
spline-audit and four-input motion constructor retain their documented requirements.
Central's header now states its already accepted model/tool/timing precondition.

Separate body review: chamferrsistream.cpp:160-217 checks reconstructed poses before
one IK/FK, then speed and acceleration, then updates its own state/appends. M3d/M4d
temporaries avoid Eigen aliasing; existing initial/velocity zero defaults remain.
The endpoint at lines 119-123 reuses configuration plus one saved-endpoint FK, with
no new classifier/state or second IK. Central inspectRates:267-295 holds references
into an unmodified vector and advances a local anchor index only after acceptance.
No private state is accessed outside its owner's methods. A redundant initial matrix
calculation had already been removed in Step 1; no new repeated FK/IK path was added.

Separate caller review: the four shared speed call sites are central appendPoint
and grouped inspectRates, LIN inspectTransferRates and RSI appendCycle. Their
durations/contexts are retained. Central and LIN already checked vector finiteness
before per-axis limits; RSI differences are bounded finite outputs of checked IK
over 0.004 s, so helper finite-first ordering preserves reachable first failures.
Sampler acceleration retains its fixed period; central midpoint acceleration and
its disabled joint-magnitude rejection are untouched. No C++ style violation remained
in the changed declarations, bodies or callers. The affected solver declaration's
existing tab indentation was corrected to match the file.

Step 2 C++ counts: five files, 1505 physical / 1391 nonblank, become 1460/1349:
sampler 283/258 to 270/246; solver header 64/56 to 77/68;
central validator 390/359 to 347/319; its header 63/54 unchanged;
motion compiler 705/664 to 703/662. Net reduction is 45 physical / 42 nonblank lines.
The shared header grows to replace three comparison implementations, not add a
parallel checker. Endpoint growth is required to distinguish pose accuracy from
configuration/winding and reuses the existing classifier. No QML, build input,
tooling, controller or geometry/inverse-map source changed; computation time was not measured.

Step 2 documentation counts and final diff checks are recorded below. Builds/lint,
runtime and hardware checks were not requested or performed. At completion of Step 2,
Steps 3-6 still covered S/T export/KRL integration and their final verification.

Step 2 documentation counts: four files, 1588 physical / 1357 nonblank lines,
become 1763/1510 (plan 733/601; AGENT_CONTRACTS 344/327;
ARCHITECTURE 519/443; RSI contract 167/139). Growth 175/153 records
cross-stage reuse proofs, the endpoint decision, numerical evidence and compliance review.
Focused source/documentation diffs, untracked plan/contract content and whitespace
checks passed; git diff --check passed. Pre-existing unrelated edits were preserved.

Revision (2026-10-20): implement Step 2 with reused checks on reconstructed movement
and reductions across animation construction and Sampling, preserving independent
checks of new results and recording the evidence for the endpoint criterion.

Step 3 outcome (2026-10-20): the app exports version 2 with eight numeric CRLF rows,
six-decimal floats and integer S/T. PsIn, LeadStart, LeadEnd and PsOut use joints
at saved boundaries 1, 2, 5 and 6. Existing configuration performs the classification;
no IK, classifier, stored S/T fields, header or class was added. PsOut has only its
own S/T row; its checked pose uses rounded PsIn XYZ and composed LeadEnd ABC.
ToolFrame remains six floats and both transfer rates retain division by 1000.

The new check concerns a new boundary: the serialized/constructed pose must match
the saved joint point whose S/T is being attached. Earlier Generate checks relate
FK to analytical targets; Sampling checks relate rounded offsets to central targets.
Neither establishes all four exported pose/configuration associations, including
the rounded staging XYZ and PsOut construction. Reuse the same direct position-norm
and maximum rotation-entry comparisons and tolerances; do not add another solver
or repeat model/joint/rate checks already accepted by the immutable motion.

Classification review used KSS 8.3 SI pages 301-304, including the complete wrist
diagram on page 303, and robotdefinition.cpp's actual geometry. A1's origin-height
difference does not affect wrist-center X. The model elbow formula reduces to an
A3 transition near atan2(20,660) = 1.735704589 degrees, accounting for the offset;
comparisons immediately below/above it select the two alternatives. At A4=0 with
HOME arm angles, A5=-30/+30 degrees gives flange-axis Z=+0.5/-0.5, matching wrist
bit 2. Bit 2 is zero at A5=0. Do not interpret the physical tilt at arbitrary A4
as a replacement for the manual's A4-zero reference. No numerical change to the
existing classifier was warranted; controller-specific S/T execution is unverified.

Ephemeral NumPy reference checks covered 18 shoulder cases (nine per area), 15
wrist cases across five A4 values, both sides of the elbow threshold, all 64 Turn
patterns, signed zero and the manual's T=19 sign example. An alternate allowed
A6 revolution preserved the staging TCP but changed T from 50 to 18, confirming
that pose equality cannot justify copying another point's S/T.

An independent reference export used the captured staging joints, initial/final
central joints, tool metadata and a replay of all 20114 rounded offset intervals.
PsIn/LeadStart/LeadEnd produced S/T 6/50, 6/50, 6/19 respectively. The captured CSV
does not contain saved PsOut joints: its reference-only PsOut was obtained from
the endpoint to staging with the existing IK equations, giving 6/19. That reference
calculation is not an IK added to the exporter or evidence about a new saved motion.
Illustrative transfer settings of 5 mm/s and 2 mm/s² produced 0.005000/0.002000.
The eight field counts were 1,8,8,8,2,6,1,1; all rows used CRLF and integer S/T.
All four associations passed; maximum position error was 5.2151e-7 mm and maximum
rotation-entry error 1.1549e-8. Sixteen association cases accepted unchanged poses
and rejected each point with 0.001 mm displacement, 0.01-degree orientation change
or a nonfinite coordinate. These are reference calculations, not edited C++ execution.

Separate declaration review: scenemachiningpreparation.cpp:32 changes frameRow to
one const V6d input and returns QString. writeSharedData:41 has two const inputs
and returns its QString error; the old payload helper is removed. No public API,
output argument, project template, lambda or exception was introduced. The unchanged
constructor's three responsibilities remain explained in its header. No C++ style
violation was found in changed declarations.

Separate body review: lines 43-64 borrow the immutable model/points through public
methods. Four loop iterations use the required boundary mapping; V3d/M3d/V6d values
materialize Eigen expressions and their lifetimes precede string conversion.
The staging position is rounded once; the central coordinates already have six
decimals. Pose checks precede QSaveFile creation and all S/T comes from each point's
own joints plus its local FK array. That array is reused by configuration and not
stored. Four export FKs are performed once per successful Sampling result, not
per RSI cycle. The worker's initial-seed/endpoint classification remains separate:
sharing those temporary arrays would require new retained state or a wider API.
Lines 67-80 preserve tool/rate formatting, direct-write prohibition, cancellation
and atomic commit. Continuation indentation at lines 68-69 was corrected during
review. No Mandatory C++ style violation remained in changed bodies.

Separate caller review: finishPreparation:176-188 still rejects stale/failed worker
results before export and publishes offsets only after a successful write. Its
motion handle remains alive throughout the borrowed writer call. prepare:154-159
still clears the old file/array before starting the worker; clearPrepared:196-204,
shutdown and queued sender/reset behavior are unchanged. No new GUI element,
binding or layout change requires visual verification. No C++ style violation
was found in changed callers; unrelated edits remain preserved.

Step 3 C++ counts: one file, scenemachiningpreparation.cpp, 208 physical / 187
nonblank lines becomes 226/205, growth 18/18. The extra lines implement the required
four configuration associations and versioned serialization. The smaller old
six-row writer cannot express S/T; keeping separate payload/error helpers would
add a result channel and forwarding code. One helper was removed (five to four
namespace helpers), with no new source file/header/class/field. QML, build inputs,
tooling, numerical sampler and controller sources are unchanged. These source
counts are separate from computation; no timing measurement was performed.

At completion of Step 3, Step 4's SRC/DAT/READ_POS update was pending. The new app
writer and legacy KRL reader were not deployable together. Owning constraints
documented that intermediate compatibility requirement. No build, lint, runtime, socket connection
or hardware operation was requested or performed; subsequent offline exchange
verification remains Step 5. Documentation counts and final diff checks follow.

Step 3 documentation counts: four files, 1763 physical / 1510 nonblank lines,
become 1916/1645 (plan 861/713; AGENT_CONTRACTS 354/337;
ARCHITECTURE 519/443; RSI contract 182/152). Growth 153/135 records the new
protocol, compatibility boundary and required evidence/review. Focused source/doc
diffs, untracked plan/contract content, protocol-format/source checks and
git diff --check passed. Pre-existing edits were preserved.

Revision (2026-10-20): implement only the user-authorized Step 3, reuse verified
configuration semantics, remove the separate payload layer, document the new file
layout and preserve the remaining KRL integration and verification steps.

Step 4 outcome (2026-10-20): SRC/DAT now consume the app's version-2 eight-row
layout. DAT owns PsIn, LeadStart, LeadEnd and PsOut as POS; ToolFrame stays FRAME.
The global two-argument READ_POS initializes a local POS, reads its eight scalar
components in one documented krl_fscanf call, and returns it with the established
KRL BOOL success argument. It checks RET1 and the S/T ranges once per new result.
This KRL output argument is the existing language convention, not a new C++ API.

Main initializes all read flags false. Only a successful integer version 2 permits
body reads. PsOut's S/T is reset to -1 before its separate two-integer read and
range check. A wrong version or failed body read reaches the same fclose and
CLOSE_FILES calls before the flag guard returns. Existing CP read-state, positive
value and controller-maximum checks still precede PTP PsIn. Mount/open failures
retain their existing cleanup. There is one common post-open close path, with no
new parser framework or duplicated frame-to-position copy operation.

The PTP target is now PsIn. After successful RSI completion, six assignments copy
only PsIn XYZ and LeadEnd ABC into PsOut, preserving its independently read S/T.
The complete RSI Start/Result/Stop and OFF/DELETE/completion block is byte-for-byte
unchanged from this step's baseline. Initial/final HOME and LIN LeadStart/PsOut
remain; no live S/T, joint-angle or robot-base comparison was added.

Offline reference verification derived the format strings and scalar output order
from the edited KRL and unchanged READ_FRAME. The eight field counts are
1,8,8,8,2,6,1,1. It accepted 2052 cases: the reference job, all 512 S/T pairs at
each of four points, both inclusive CP ceilings and one independent PsOut pair.
It rejected 118 cases covering wrong/non-numeric versions, missing rows/fields,
non-numeric fields, each truncated row prefix, legacy six-row layouts (including
one starting with X=2), out-of-range S/T, invalid CP values and either close failure.
The CP reference ceilings were 3 m/s and 10 m/s² from the archived machine.dat;
the program still reads the controller's live $VEL_MA.CP / $ACC_MA.CP values.
With hypothetical PsOut S/T 7/63, its constructed XYZABC retained PsIn XYZ and
LeadEnd ABC while keeping 7/63, proving field-copy independence in the reference.
These are scalar-format/control-flow reference checks, not execution of KRL or
a claim that a physical pose with the hypothetical S/T pair is reachable.

Separate declaration review: SRC:7-18 adds one reusable read state, format version
and PsOut success flag; DAT:5-9 contains the four POS declarations and ToolFrame.
There is no shadowing local FRAME PsOut. READ_POS:1-6 is explicitly GLOBAL with
the established two-argument signature and a local POS owner; no extra scalar
storage or generic helper was added. No C++ declaration or public API changed.

Separate body review: READ_POS:9-15 initializes every returned component and
negative S/T sentinels, supplies exactly eight permitted scalar output variables,
and sets Ok only after #DATA_OK and valid S/T. SRC:52-71 gates reads on version,
retains failed read flags and closes before returning. The version-failure path
returns before unassigned rate states can be inspected. SRC:120-125 assigns only
the six agreed coordinates; PsOut.S/T is not copied from another target. No
declaration/body violation or introduced cleanup regression remained in review.

Separate caller review: SRC:54-56 supplies distinct Ok flags to the three READ_POS
calls, reads PsOut's pair at line 59, and keeps READ_FRAME for ToolFrame at line 61.
Its state/read/close/rate guards all precede PTP PsIn:92. All three new helper calls
require the separate READ_POS.src on the controller, as the updated contract states.
Existing READ_FRAME/cwrite_test and mounting helpers are unchanged. C++ mandatory
style audit is not applicable to new KRL syntax; no C++, QML, build or tooling
source changed. Source inspection checked balanced IF/ENDIF, declarations, call
order and absence of old Ps references and forbidden live-configuration comparisons.

Step 4 controller-source counts: two changed-existing files, 131 physical / 117
nonblank lines, become three files, 161/146: SRC 119/105 to 132/118; DAT 12/12 to
13/13; new READ_POS 16/15. Growth 30/29 implements the new version/typed reads,
S/T checks and explicit PsOut assignments. Including unchanged READ_FRAME 46/33,
the inspected controller-source group is 177/150 to 207/179 (three to four files).
One global helper was added; direct POS-member reading is smaller than a FRAME
read followed by a second read and coordinate copies. Application C++ size and
sampling calculations are unchanged; no computation time was measured.

Step 4 source/reference checks passed; actual KRL compilation and robot execution
remain unverified. No app build, runtime launch, socket connection, deployment or
hardware action was performed. Steps 5-6 retain full offset/exchange verification
and final overall review. Documentation counts and diff checks follow.

Step 4 documentation counts: four files, 1916 physical / 1645 nonblank lines,
become 2035/1750 (plan 972/810; AGENT_CONTRACTS 356/339;
ARCHITECTURE 519/443; RSI contract 188/158). Growth 119/105 records the matching
protocol, focused reviews, evidence and remaining verification. Focused source/doc
diffs, complete new/untracked READ_POS content, source/reference checks and
git diff --check passed. Unrelated existing edits were preserved.

Revision (2026-10-20): implement the user-authorized Step 4, align the KRL consumer
with version 2, use complete POS targets and preserve independent PsOut S/T and
the existing nominal RSI completion/cleanup sequence.
