# Align saved motion and prepared-job reporting (step 8)

This living ExecPlan follows PLANS.md and implements step 8 only.

## Purpose / Big Picture

Users should see timing that describes the new prescribed trajectory, not a removed automatic multiplier. Display total Dry Run time and central path time separately, and report the mathematical central duration and controller-cycle duration after RSI Sampling. Distinguish pose samples (including initial pose) from corrections (one per subsequent controller cycle).

## Progress

- [x] (2026-10-09) Read saved settings/path wrappers, prepared job and coordinator, device state publication, queued delivery, affected QML and QxPanel base.
- [x] (2026-10-09) Add typed timing/count properties and device state keys; update concise UI status.
- [x] (2026-10-09) Review invalidation/defaults, snapshot restoration and consumers; update docs and whitespace checks.

## Context and Orientation

SceneMachiningPath in src/scene/scenegeometry.h wraps immutable ChamferMotion. SceneMachiningSettings::fromMotion/timing in scenemachining.cpp already round-trip machining speed, ramp fraction and transfer settings. SceneMachiningPreparation owns a const prepared job on the GUI thread; preparedChanged drives its QML properties. main.cpp sends preparedJobReady via queued connection to RsiDevice on ControlIO. publishPreparedJob sends value state through DeviceRunner to its GUI-owned property map. Main.qml shows preparation status; MachiningPanel.qml shows saved duration inside QxPanel, whose default content is a ColumnLayout. Both labels already wrap and fill available width.

## Plan of Work

Add saved-path centralDuration and eight absolute phaseTimes (HOME-relative), leaving duration as total Dry Run duration. Expose prepared controllerDuration, correctionCount, cyclePeriod and four job-relative phaseTimes through SceneMachiningPreparation, using preparedChanged for notification. Add correctionCount to PreparedChamferJob as sampleCount minus one, independent of encoding readiness. Publish equivalent new preparedJob state keys on the existing device signal. Keep existing duration/sample count/time-scale APIs and keys for compatibility; multiplier stays 1 and is no longer displayed.

Change saved-path label to Dry Run and Central path times. Preparation status shows samples/corrections and path/RSI-cycle durations to three decimals, with separate existing encoding readiness/error. Use RSI Sampling wording consistently in coordinator/status. Do not change behavior, transmission, worker scheduling, generation settings or hardware readiness.

## Decision Log

On 2026-10-09, add explicit data rather than redefine existing duration/sampleCount meanings. Clearing a job returns zero counts/durations, empty phase times and no encoding readiness, through the same preparedChanged and queued publication. Old multiplier properties remain compatibility-only. Saved settings already reflect the new snapshot and need no further schema change.

## Surprises & Discoveries

A prepared job can have valid samples but failed encoding/limits. Correction count therefore describes nominal controller cycles, not arming readiness. Existing status already distinguishes encodingReady and must keep doing so. Exact mathematical duration and controllerDuration were added in step 7; step 8 only exposes them through state/UI.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Edit saved-path/coordinator/job headers, coordinator wording, device state publisher and the two QML labels. Inspect final diff and run git diff --check plus untracked-plan whitespace checks. No build, lint target, runtime or hardware operation is requested; no project tests or git staging/commits.

## Validation and Acceptance

Trace each UI property to the same immutable motion/job snapshot. Prepared phase times must contain four central-relative boundaries; saved path phase times contain eight HOME-relative boundaries. Verify correctionCount=max(sampleCount-1,0), controllerDuration uses terminal controllerTime, and all new state clears when m_job is reset. Confirm unchanged path/calibration/selection/limit-file invalidation, worker stale-result rejection and queued delivery. For a mathematical duration 1.002 s and cycle .004 s, status should distinguish path 1.002 s and RSI cycles 1.004 s; this is an acceptance example, not an executed runtime result. No time-multiplier text should remain in application QML. Source-only verification cannot establish QML compilation or appearance; those are deferred until requested.

## Idempotence and Recovery

Preserve unrelated local edits, component APIs, module registration and device threading. New state fields are additive and old keys retain their meanings. No socket or command is added, and Start Machining stays disabled.

## Interfaces and Dependencies

Use existing QList<double>/QVariantList values for phase times. New Q_PROPERTY fields share existing NOTIFY signals or CONSTANT for immutable saved paths. No new modules, dependencies or asynchronous channels.

## Artifacts and Notes

The UI is concise status, not a tutorial. No controller limit values or installed configuration are inferred. Shared-folder publication and execution remain separate later work.

## Outcomes & Retrospective

Implemented additive saved-path and prepared-job timing/count properties, matching device state and concise QML reporting. Reviewed settings round-trip, immutable snapshots, preparedChanged notification, clearing defaults, stale-worker rejection and queued ControlIO delivery. Removed QML multiplier use while retaining compatibility APIs. Source review and whitespace checks completed; no build, QML lint, runtime or hardware verification was performed. Shared-folder/KRL publication remains outside this step.

Revision 2026-10-09: defined step-8 state/reporting scope and compatibility rules.
