# Add chamfer settings and documented joint-speed bounds

This living ExecPlan follows PLANS.md. Only steps 1 and 2 of the agreed Cartesian timing redesign are authorized here. Do not implement arc-length timing, new Dry Run, KRL publication, or controller execution in this task.

## Purpose / Big Picture

Internal Chamfer will expose machining speed, acceleration/deceleration distance percentage, and a shared speed and acceleration for staging transfers. Generated paths must retain these settings and restore them when selected. The robot loader will read the existing rated-payload speed fields from resources/json/kr10.json and cap numerical motion speeds with them. Controller acceleration values remain unknown.

## Progress

- [x] (2026-10-09) Read panel/base controls, settings conversion, model loader, generation and sampling consumers, and PLANS.md.
- [x] (2026-10-09) Added new panel settings, percent conversion, numerical validation, snapshot storage and restoration.
- [x] (2026-10-09) Loader validates rated speeds; motion creation caps configured speeds; sampling verifies the saved bounds.
- [x] (2026-10-09) Reviewed field conversions, loader/generation/sampling callers, model snapshot lifetime and documentation. JSON parsing and tracked/new-file whitespace checks passed. Build/runtime checks were not run.

## Surprises & Discoveries

The old lead feed fields are used by the current timer for both leads and transfers. Replacing that algorithm belongs to later steps. New fields must be persisted now without claiming the new percentage law already runs. Existing jointSpeed is an explicit simulation setting (default 30 deg/s); jointAcceleration defaults to 60 deg/s^2 and is not a KUKA rating. The datasheet provides speeds but no accelerations.

## Decision Log

On 2026-10-09, preserve existing C++/QML properties for compatibility, removing separate lead speeds only from the panel. Introduce accelerationDistanceFraction (default 0.30, UI percent), transferFeed (default 10 mm/s), and transferAcceleration (default 20 mm/s^2). Those transfer defaults retain existing simulation defaults, not installed KRL settings. All new fields are stored and checked; their use by the redesigned timer is deferred. Preserve the current timer behavior during this settings-only stage.

On 2026-10-09, cap each configured simulation joint speed by the corresponding rated-payload JSON speed inside ChamferMotion::create so every numerical caller is covered. Store this effective setup in the immutable motion; sampling checks its model bounds too. Keep position and acceleration checks. Require positive finite rated speeds in model JSON; do not silently assume unlimited speed or fabricate acceleration data.

## Context and Orientation

qml/Views/Workspace/MachiningPanel.qml edits a SceneMachiningSettings gadget defined in src/scene/scenegeometry.h. src/scene/scenemachining.cpp copies settings into ChamferTimingParameters and restores them from a saved ChamferMotion. src/pathgeneration/chamfer/chamfermotion.h defines that numerical snapshot. Motion creation and timers live in chamfermotion.cpp; the Cartesian sampler reads its setup in chamfercartesiansampler.cpp. src/3d/robot/model/kr10model.cpp reads JSON into JointModel in kr10kinematicmodel.h. Geometry owns the numerical model; the viewport owns the JSON loader. No dependencies or target lists need changing.

## Plan of Work

First add the three settings to both structures and to both conversion directions. Update panel fields and percent conversion, preserving draft cancellation and saved-path restoration. Extend timing validation to reject invalid fractions and transfer values.

Next add ratedPayloadSpeedDegreesPerSecond to JointModel, read and validate it in readJoint, and construct the result without adding a five-argument constructor. Validate it in numerical generation, copy/cap the setup before compilation, and ensure Cartesian sampling rejects an inconsistent saved speed bound. Preserve all existing position and acceleration checks.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Inspect changes using git diff for the named files and git diff --check. Parse resources/json/kr10.json with ConvertFrom-Json. Search all new field names and legacy feed references with rg in src and qml, excluding build/old directories. No staging, commits, tests, build, launch or hardware operations are authorized.

## Validation and Acceptance

Source review must follow each new field from panel draft through timing snapshot and back into saved-path settings. Reject fractions outside (0,1] and nonpositive/nonfinite transfer parameters. Validate all six JSON speeds: 220, 210, 270, 381, 311, 492 deg/s. A configured joint speed above its model rating must be capped before timing; lower configured speeds must remain unchanged.

When separately authorized, build robocrap and the application QML lint target with the matching kit and hot reload OFF. In a disconnected runtime, enter 30 percent, change transfer values, generate a path, select another object and reselect the path: entered values must return. Cancel must restore the original draft. This stage does not promise a 30-percent acceleration profile, because the timing implementation is out of scope. No project test harnesses are to be added.

## Idempotence and Recovery

Preserve existing unrelated edits, especially resource moves and toolbar/icon changes. Make exact source edits and inspect diffs. Fix only task-introduced errors; do not reset or discard the checkout.

## Artifacts and Notes

KUKA KR 10 R1420 datasheet, identifier 0000-255-784 / V20.0 / 24.10.2017, lists axis speeds with rated payload. These values are not proof of the installed controller configuration. No $ACC_MA value has been supplied.

## Interfaces and Dependencies

No new libraries or public methods are needed. New settings are numeric values within the existing settings/snapshot structs. Existing preparation threading and immutable ownership stay intact. New C++ uses no lambdas, exceptions, output parameters, or functions with more than four arguments.

## Outcomes & Retrospective

Steps 1 and 2 are implemented. The panel has the new fields and all new values survive the saved-motion conversion in both directions. The existing numerical timer remains active, and the new percentage/transfer inputs are intentionally stored only until the authorized later timing steps. Model-rated speed caps apply to HOME timing and central joint-speed checks via the same bounded setup. Existing lower speed and joint-acceleration assumptions remain. Source review, JSON parsing and whitespace checks are complete; builds and runtime observations were not performed. No tests or hardware actions were added.

Revision 2026-10-09: completed the authorized settings/model-bound stage and recorded the intentionally pending new timing behavior. The only schema requirement added is a positive finite ratedPayloadSpeedDegreesPerSecond for every JSON joint; the bundled model already provides all six values. No controller acceleration values were inferred.
