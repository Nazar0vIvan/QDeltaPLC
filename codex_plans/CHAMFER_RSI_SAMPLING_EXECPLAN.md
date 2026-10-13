# Sample prescribed central motion directly for RSI (step 7)

This living ExecPlan follows PLANS.md.

## Purpose / Big Picture

RSI Sampling will evaluate the saved analytical central trajectory directly on its 4 ms clock. Animation FK/projection will no longer choose the curve parameter or timestamps. Preserve continuous Euler encoding, reconstruction and configured correction-limit checks.

## Progress

- [x] (2026-10-09) Read sampler, central evaluator, job, encoder, wire audit and preparation consumers.
- [x] (2026-10-09) Replaced projection with saved analytical evaluation; added separate cycle times, full-cycle/final-stop checks and prepared-job duration/audit correspondence.
- [x] (2026-10-09) Reviewed boundaries, branch witnesses, sequential validation seeds, endpoint clock counting, encoder/limit call path and audit consumers; updated documentation and passed whitespace checks. No build/runtime verification performed.

## Context and Orientation

ChamferMotion owns centralMotion(), the immutable analytical evaluator created during generation. chamfercartesiansampler.cpp currently projects animation FK onto ChamferPath. PreparedChamferJob invokes it on the existing preparation worker and passes poses to rsiposeencoder.cpp. The encoder produces increments and checks reconstruction and limits; it does not require FK-derived targets. RsiHoleProtocol's offline audit already computes controller duration as correction count times cycle period.

## Plan of Work

Replace the sampler's curve projection with centralMotion.evaluate(time). Source phase times from that evaluator and cross-check saved seven-phase boundaries within floating-point resolution. Keep saved joint samples only as nearby configuration witnesses, without joint interpolation or playback FK. Keep sequential IK/FK, positions, rates and numerical interpolation checks. Add analytic phase and ramp switch times to validation points; extra validation must not add controller cycles.

Use initial sample t=0, subsequent clock times j*period and exact terminal t=T once. Each sample retains its separate controllerTime=j*period; T is analytical duration and controllerDuration is the last controllerTime. Count cycles so the last clock contains T, without early epsilon snapping. Validate the final full controller interval as well as the shorter mathematical interval. Preserve exact machining-end pose as metadata even if it lies between clock samples. Prepared job duration and phase metadata must use the same central evaluator.

## Decision Log

On 2026-10-09, keep preceding saved IK knots as branch witnesses only. Direct analytical evaluation supplies all TCP targets; no motion.evaluate or FK closest-point search remains in the sampler. Use machiningFeed for every central phase's maximum speed, since leads now ramp to that common speed. Missing analytical snapshots reject with a regeneration instruction.

On 2026-10-09, retain existing sample.time semantics (analytical evaluation time), adding controllerTime for cycle placement. Add PreparedChamferJob::controllerDuration for consumers; broader UI/state reporting remains step 8. Preserve encoder inputs, correction-limit gates and no-device-side-effects preparation.

## Surprises & Discoveries

Old sampling snaps a final near-clock point to the endpoint within 1 ns; this can place a future endpoint on an earlier cycle. Exact clock inclusion removes that ambiguity. Subtracting HOME/transfer offsets from saved absolute timestamps introduces rounding, so compare saved boundaries but use the original central-relative phase times as authoritative.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Edit sampler/header and necessary prepared-job metadata; inspect encoder/audit consumers. Run git diff --check and untracked-plan whitespace checks. Do not add tests or run builds/runtime without the explicit request required by AGENTS.md. Preserve unrelated edits and do not stage/commit or contact hardware.

## Validation and Acceptance

Source review must find no projection, joint interpolation or playback evaluation in target construction. All poses use the saved central evaluator. For T=1.002 s and period=.004 s, final analytical time is 1.002 and controller time is 1.004; for T=1.000 s the endpoint occurs once on the 1.000 s cycle. Phase switches remain separate metadata and never increase correction count. Check speed, acceleration and interpolation over both analytical and controller clocks including final stop. Encoder and correction-limit validation remain mandatory. No numerical execution or build success is claimed without running it.

## Idempotence and Recovery

All work remains inside the existing copied-input preparation worker and immutable return-value job. Reject invalid profiles and bounds without retiming. No shared KRL files, network sockets or hardware actions are introduced.

## Interfaces and Dependencies

Add controllerTime to ChamferCartesianSample and controllerDuration() to PreparedChamferJob; preserve existing APIs. Use existing Qt/Eigen and the saved ChamferCentralMotion. No new dependencies or QML changes.

## Artifacts and Notes

Interpolation/rate checks are numerical estimates, not guarantees of installed controller behavior. Cartesian interpolation comparison remains a geometric check; continuous Euler serialization and bounds remain in the unchanged encoder.

## Outcomes & Retrospective

Step 7 is implemented in the active preparation call path. The sampler directly evaluates the retained analytical central motion, preserves separate metadata and exact endpoint once, and returns controllerTime for every stored sample. Prepared-job mathematical phase times come from the same evaluator, while controllerDuration reports complete cycles. The wire audit checks correspondence to correction count and sample indices. Continuous Euler encoding, serialization reconstruction and configured correction-limit checks remain unchanged. Source review verified that no playback evaluation, joint interpolation or projection chooses a target; saved joints remain branch witnesses. Additional sampled checks reject violations without retiming. Tracked/untracked whitespace checks passed. Build, numerical execution, UI and hardware validation were not performed under repository instructions. Step 8 remains broader state/reporting cleanup.

Revision 2026-10-09: recorded direct sampling, branch-witness and final-cycle contracts.

Revision 2026-10-09: completed direct active sampling and clock metadata; aligned offline wire audit with explicit controller times, leaving the existing encoder and configured-limit gates intact.
