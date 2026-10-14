# Implement the reviewed architecture without RSI implementation


This is a living ExecPlan maintained according to repository-root `PLANS.md`. Update `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` whenever work stops or a decision changes. The user requested this plan on 2026-10-14 and subsequently authorized Step 01 using the existing executable without building. The user then explicitly skipped numerical input/result capture; Step 01 is complete as a source/configuration snapshot. Step 02 is complete: the user allowed handling external-library exceptions, and `AGENTS.md` records that policy. The user narrowed Step 03 to direct Qt console diagnostics and complete removal of the Logger mechanism; that implementation is complete with source checks only. Additional operation-result/QML error work is deferred. Milestones 04 onward have not been executed. `AGENTS.md` and the user's current instructions take precedence over conflicting generic instructions in `PLANS.md`.

## Purpose / Big Picture


After implementation, Generate will reuse calculations that are currently repeated for each starting robot configuration, Dry Run will calculate one robot pose per update, and the viewport will display supplied geometry and transforms without owning machining or robot decisions. Device and viewport diagnostics now use Qt console output; additional operation-result/QML error displays are deferred by the user. The source targets will express the operations they own. The user will still import the same geometry, calibrate the spindle, Generate, and play the same accepted motions with the existing pause, Stop, HOME, selection, and readiness behavior.

This plan implements the non-RSI portion of `codex_plans/ARCHITECTURE_REVIEW.md`: findings F2-F6, the console-diagnostic portion of F7, reductions O1-O5, and the six-owner architecture. F7's additional operation-result/QML error proposal is deferred by the user. Optional numerical/scheduling changes O6/O7 are isolated at the end. RSI means Robot Sensor Interface, the separate KUKA controller workflow. Its implementation, preparation, execution, demo repair, protocol changes, force correction, and removal of its numerical dependencies are excluded, apart from Step 03's explicitly recorded logging-only edits. Consequently the unrestricted final dependency graph cannot be completed by this plan: explicitly retained RSI dependencies are part of the scoped endpoint, not completed removals.

## Progress


`[x]` means completed and supported by recorded evidence; `[ ]` means not completed. The numbered implementation steps match the milestone headings below. Step 01 is complete in the user-narrowed source/configuration scope; numerical capture was skipped, not performed. Dates and times in this plan use Europe/Moscow (MSK, UTC+3). For partial work, split the corresponding entry into completed and remaining operations rather than checking an unfinished milestone.

- [x] (2026-10-14 18:51 MSK) Read `PLANS.md`, current `AGENTS.md`, and the current architecture review; inspect affected numerical, scene, renderer, device, QML, and CMake interfaces.
- [x] (2026-10-14) Define the RSI exclusion, conditional OCCT policy gate, priority order, and acceptance conditions in this ExecPlan.
- [x] (2026-10-14) Write the plan; check its required sections, heading spacing, progress placement, new-file whitespace, and repository diff/status.
- [x] (2026-10-14) 01 — Record source/configuration and executable identity, snapshot project geometry/model inputs, and freeze excluded RSI inputs/APIs. Numerical input/result capture explicitly skipped by the user; no build, profiler, application runtime, or RSI operation run. Evidence: `codex_plans/architecture_baseline/`.
- [x] (2026-10-14) Record the before-implementation size: 249 project input files / 24,045 physical text lines overall; 229 files / 23,201 lines excluding Components/Styles; 150 C++/application-QML files / 21,017 physical source lines within that scope. Counting rules and repeatable commands are in `Artifacts and Notes`.
- [x] (2026-10-14) 02 — Record the user's permission in `AGENTS.md` to handle external-library exceptions, preserve existing OCCT recovery and cleanup/rethrow, and retain the ban on project-defined/newly thrown exceptions. Recount confirmed 13 try blocks in eight files; no C++ behavior changed.
- [x] (2026-10-14) 03 — Replace the Logger mechanism with direct `qInfo()` console diagnostics, as requested: convert 50 device message sites plus the viewport bridge; remove the class, payload type, logging signal, metatype, QML context property, relay connections, and CMake entries. Additional operation-result/QML error work is deferred; no build/runtime check was requested.
- [ ] 04 — Build central maps, timing, and the point schedule once per Generate (F2/P3).
- [ ] 05 — Return verified IK transforms and calculate only three wrist-prefix transforms (F3/P2a).
- [ ] 06 — Reuse accepted FK TCP values when assembling LIN and central points (F3/P2b).
- [ ] 07 — Cache transforms reconstructed from accepted calibration coordinates (F4/P6a).
- [ ] 08 — Move model loading and accepted robot state into an application RobotController (F4/P5a).
- [ ] 09 — Replace renderer access to document/motion objects with concrete drawing values (F5/P5a).
- [ ] 10 — Use joint-only animation evaluation and returned presentation acceptance (F3/F4/P5b).
- [ ] 11 — Move mode and selection operations into WorkspaceController (F4/P6b).
- [ ] 12 — Reconcile selection changes and retain only live viewport parts (F5/P5c).
- [ ] 13 — Replace PLC internal QVariant dictionaries with typed codec/correlation values (F6/P7).
- [ ] 14 — Retain one pending FTS publication sample while preserving every high-frequency sample (O5/P8).
- [ ] 15a — Separate Geometry, Kinematics, and Trajectories targets and their published headers (P9).
- [ ] 15b — Separate Devices and Viewport from private Application code; preserve frozen RSI compatibility (P9).
- [ ] 15c — Move registration/application QML, update all imports/resources, and remove obsolete targets (P9).
- [ ] 16 — Remove unused saved velocities and temporary spline coefficient storage (O1/O2/P4a).
- [ ] 17 — Reuse arc-length endpoint/integral values with the original logical work limit (O3/P4b).
- [ ] 18 — Remove redundant analytical rotation work while retaining its rejection conditions (O4/P4c).
- [ ] 19 — Remove verified unused non-RSI code and unnecessary deployment payload (P1).
- [ ] 20 — Conditional, separate numerical change: analytical cylinder Jacobian (O6/B2).
- [ ] 21 — Conditional, only if measured responsiveness warrants it: asynchronous geometry import (O7/B3).
- [ ] 22 — Complete the scoped architecture audit, requested verification, and implemented-state documentation.

Steps 01-19 and 22 define the required scoped delivery. Steps 20/21 remain conditional and may be explicitly deferred with evidence and a Decision Log entry; do not mark them implemented when deferred. Step 02's policy is resolved; later OCCT failure-path edits must preserve the recorded recovery and comply with the external-library exception allowance. At every implementation stop record the changed files, checks actually run, outstanding checks, and the next incomplete operation here.

## Surprises & Discoveries


The review's inventory totals and recorded HEAD are historical. At plan creation the current HEAD is `3709a12`; existing user changes include root CMake/IDE settings, `resources/cad/kr10/SEE.stp`, architecture documents, and a deleted JSON documentation file. Preserve them and do not recreate deleted files. The before-implementation recount is 249 project files / 24,045 physical text lines overall and 229 files / 23,201 lines excluding Components/Styles. These totals are one file and 67 lines below the review's totals because the deleted JSON README is absent. The 129 C++ files still contain 17,584 physical lines. Compare later inventories using the rules in `Artifacts and Notes` rather than copying the review's historical totals.

The existing preview profiling input named by `AGENTS.md`, `codex_plans/chamfer_profile_spatial_leads_input.json`, is absent. `src/3d/chamferprofile.cpp` supports `previewOnly=true`, but defaults to the separate RSI path when that flag is omitted. A new accepted preview input must explicitly set the flag before any profiling invocation. No numerical baseline, build, runtime, timing, or hardware check was run while authoring this plan.

During Step 01 the existing Debug executable/cache/build-log identity and debug symbols were inspected without starting the application. The profiler does not export all saved joint/TCP arrays. The user explicitly skipped requesting accepted TCP/settings and numerical-case capture, so no profile fixture, result report, or debugger motion dump was created. `architecture_baseline/manifest.json` and `SOURCE_CONTRACTS.md` record this limit; source hashes do not establish numerical equivalence.

Source inspection confirms that `ChamferMotion` already prepares three clamped joint splines: separate straight transfers and one shared central spline. This plan preserves them. `ChamferMotion` stores parameters, while `ChamferCentralMotion` stores the full curve; do not describe the existing state as two full curve copies.

The renderer currently commits a pose after AIS transformations are accepted. AIS is OCCT's interactive presentation layer. Actual drawing is deferred by a zero-interval GUI timer. Returning success from the replacement operation must preserve transform acceptance, not invent a completed-frame guarantee.

The OCCT code has 13 try blocks in eight files, including catch-all cleanup/rethrow. The user resolved the previous policy conflict on 2026-10-14: external-library exceptions may be handled; project exception types and throwing new exceptions from project code remain banned. `AGENTS.md` now permits the existing library failure handling, including rethrowing the same exception after cleanup. A return-status wrapper alone still cannot intercept an exception.

The former diagnostic pipeline formatted text and emitted an unconsumed GUI signal. All 50 device log sites and the viewport message bridge now write through `qInfo().noquote()`. Qt supplies timestamps from the application's message pattern; calls retain category labels, source names, and message content. The FTS connection message uses plain text instead of HTML spacing/break tags, and viewport text is no longer HTML-escaped. Logging occurs directly on the caller's thread instead of queuing diagnostic payloads to the GUI. The previously built executable predates this source change and cannot verify it without a separately requested build.

## Decision Log


Decision (2026-10-14, Codex): create one master ExecPlan with concrete, independently assessable milestones and a single progress list. Rationale: the user requested a step-by-step execution plan, and progress must remain understandable without assembling several documents. Any later focused plan must retain the acceptance conditions here and link back to this master.

Decision (2026-10-14, Codex): prioritize correctness/lifetime prerequisites and missing error feedback, then major repeated computations and ownership changes, then device simplification, target enforcement, smaller reductions, and conditional numerical changes. Rationale: the review's critical F1 is RSI-specific and therefore excluded; its severity is not permission to implement it. Dependencies within a priority group determine execution order. Validation occurs during every milestone, not only at the final step.

Decision (2026-10-14, Codex): freeze RSI implementation files and preserve compatibility instead of claiming a zero-dependency Devices target. Rationale: Devices currently imports full prepared jobs, numerical correction types, and the legacy demo. Removing these requires excluded RSI changes. Keep their existing source locations, APIs, queued invalidation, and shutdown behavior. Only CMake ownership entries and non-RSI compatibility declarations may accommodate those frozen sources; do not edit or move their implementation files in this plan.

Decision (2026-10-14, Codex): include the reviewed application module migration as an intentional API change. Rationale: the requested destination removes registration-only `RoboCrapBackend` and imports its names from the existing `robocrap_qml_module`. Keep `RoboCrap.Viewport3D`, Components, Styles, and their behavior. Update affected callers together and document the retired Backend URI when this step is actually implemented.

Decision (2026-10-14, Codex): retain PLC deadline/retry work F8/R2 outside this architecture implementation. Rationale: the review explicitly defers it until a controller-compatible late-response/ID-reuse policy exists. Typed pending records and visible exhaustion errors are included; inventing expiry, retry, or controller behavior is not.

Decision (2026-10-14, Codex): no tests, test harnesses, or test build targets are introduced. Builds/configure/build-backed QML lint run only after an explicit user request for build verification. No Git staging, commits, branch changes, stashes, or destructive recovery is implied by this plan. Rationale: these repository instructions override the generic `PLANS.md` examples.

Decision (2026-10-14, user; recorded by Codex): complete Step 01 as source/configuration capture and skip numerical input/result capture. Rationale: after being asked for accepted TCP/settings, the user replied, "i don't think you need that. Skip it". Do not ask again or leave this skipped activity as a prerequisite to Step 01 completion. No numerical success/failure, timing, or pointwise baseline is claimed. Any later numerical verification is a separately requested check, not an unfinished action from this step.

Decision (2026-10-14, Codex): record file counts, physical lines, and nonblank lines before implementation, with separate project and architecture scopes. Rationale: the user requested a size baseline for later comparison. Include existing frozen RSI files in the measured tree while leaving their implementation excluded; exclude Components/Styles only from the architecture-scope subtotal. Physical source lines include comments and blanks; nonblank lines still include comments. No comment-stripped or statement count is claimed.

Decision (2026-10-14, user; recorded by Codex): allow exception handling only for exceptions originating in external libraries such as OCCT; retain the ban otherwise. `AGENTS.md` allows try/catch at those calls, standard C++ exceptions raised by those calls, and bare throw to propagate the same exception after necessary cleanup. Project-defined exception types and throwing new exceptions from project code, including library exception instances, remain prohibited. Rationale: existing CAD import, partial display installation, robot-pose restoration, and window failure handling rely on library exceptions. Preserve those operations instead of replacing them with a return-value wrapper that cannot intercept an exception. This policy decision completes Step 02 without changing C++ code.

Decision (2026-10-14, user; recorded by Codex): send all former Logger messages to Qt console output and remove the Logger mechanism entirely. The user said, "All logic related to the Logger class should be removed from the project, it no longer exists." Replace emission sites with direct qInfo calls; do not introduce a replacement singleton or logging payload/GUI relay. Preserve category, source, text, and timestamp information using Qt's message pattern. Defer the former Step 03 DeviceOperationResult and QML status/error proposal until separately reopened. This request includes the eleven diagnostic statements in RsiDevice: only their logging expressions and the QDebug include change; no RSI preparation, protocol, motion, or numerical behavior is reopened.

## Outcomes & Retrospective


Step 01 is complete in its user-narrowed scope. `codex_plans/architecture_baseline/manifest.json` records HEAD, dirty inputs, executable/cache/object metadata, 193 present tracked text inputs plus one deleted-file record, and 31 frozen RSI records. `interfaces.json` and `SOURCE_CONTRACTS.md` record current CMake/QML/device/numerical contracts; three byte-identical geometry/model input snapshots are retained. The 193 present source/configuration inputs remained unchanged, and every excluded RSI path is clean relative to HEAD. No architecture implementation, build, profiler, application runtime, or hardware operation was performed. Numerical capture was explicitly skipped. Later milestones remain pending.

The user-requested size baseline is now recorded before implementation: 249 project files and 24,045 physical text lines; the architecture scope contains 229 files and 23,201 lines. Within that scope, C++ and application QML contain 150 files and 21,017 physical source lines, of which 17,784 are nonblank. These counts describe the current tree, not an achieved reduction.

Step 02 is complete as a policy/documentation change. `AGENTS.md` now allows external-library exception handling and cleanup/rethrow while keeping project exceptions banned. The existing 13 try blocks in eight OCCT-related files are unchanged. Their cache fallback, partial-display removal, last-accepted-pose restoration, and failed-window release remain in place. No build or runtime check was needed for that documentation-only change.

Step 03 is complete in the user's narrowed scope: console diagnostics and removal of Logger, LoggerMessage, and the old logging signal/relay/context property. Fifty device sites retain their original message conditions and payload content, apart from the explicitly plain-text FTS connection format. The viewport bridge writes original text directly. The former DeviceOperationResult/UI proposal is deferred, not implemented. CMake lists and current architecture/instructions match the removal; no QML component or device control logic changed. No build, runtime, profiler, RSI preparation, or hardware operation ran. Original Step 01 evidence/counts remain immutable; changed source hashes are expected after this implementation. Milestones 04 onward remain pending.

The scoped completion report must distinguish actual eliminated calculations and dependency edges from file movement, raw payload savings from process memory, and retained RSI compatibility from the unrestricted final architecture. Do not claim a speedup or exact Cartesian interpolation guarantee without the relevant evidence.

## Context and Orientation


Run repository commands from `E:\Qt\QtProjects\RoboCrap`. Current code is C++17 with Qt 6.8 minimum and CMake 3.16 minimum, bundled Eigen numerical types, and OCCT 8.0.0 (Open CASCADE Technology, the CAD/presentation library) on a Windows 64-bit MinGW Qt kit. A source owner is one authored CMake target, distinct from Qt-generated helper/plugin targets. Root `CMakeLists.txt` owns the executable, application QML, and resources. `src/CMakeLists.txt` currently combines numerical geometry/kinematics/paths in Geometry, scene coordination in Scene, and workers plus GUI adaptation in Devices; Backend contains foreign QML registration. `src/3d/CMakeLists.txt` owns the native OCCT viewport and currently also owns robot model loading/state and the profile CLI. This graph is already acyclic; the problem is broad interfaces and misplaced responsibilities.

The numerical pipeline lives in `src/pathgeneration/chamfer/chamferpath.*`, `chamferarclength.*`, `chamfercentralmotion.*`, `chamfercentralvalidation.*`, `chamfermotion.*`, and `chamferpreparationdiagnostics.h`. A geometric progress value is dimensionless, arc length is traveled distance in mm, and elapsed motion time is seconds. Inverse-map knots approximate the conversion from distance to progress. Saved animation points hold accepted robot joint coordinates and timing. Neither is an RSI controller sample. FK, forward kinematics, calculates link transforms from six joint angles in degrees; IK, inverse kinematics, calculates a seeded joint solution from a flange transform. The flange is the robot's end mounting interface; BASE coordinates refer to its fixed robot-base frame. TCP is the tool center point. LIN is a prescribed straight Cartesian transfer; PTP is synchronized joint motion. A clamped cubic joint spline interpolates saved joints with zero endpoint joint velocities.

`src/scene/scenemachining.*` dispatches copied numerical inputs to an `std::async` worker, accepts complete immutable motion results on the GUI thread, and runs the Dry Run timer. `src/scene/scenemodel.*` owns stable object IDs and QObject geometry views; zero means no application object. `src/scene/sceneendeffectors.*` owns accepted end-effector calibration. `qml/Main.qml` currently sequences mode, selection, editor, and viewport operations. `src/3d/occcontroller.*`, `occt/occviewwindow.*`, `occt/occviewport.*`, and the two adapters combine numerical robot state with presentation.

`src/network/devicehub.*` owns ControlIO and GeneralIO threads. FTS and RSI share ControlIO; PLC uses GeneralIO. `DeviceRunner` and its `QQmlPropertyMap` live on the GUI thread; requests are queued to `AbstractDevice`. Socket/timer ownership, queued state, stop-before-thread-quit, worker joins, and asynchronous copied-input FTS saving remain mandatory. `src/network/plc/plcmessagemanager.*` is currently a QObject codec using QVariant dictionaries. `src/network/fts/ftsdevice.*` publishes the last batch sample when packet processing observes at least 16 ms elapsed; 7000 Hz is its timestamp assumption, not measured traffic.

The destination has six authored source owners: `src/geometry`/RoboCrapGeometry, `src/kinematics`/RoboCrapKinematics, `src/trajectories`/RoboCrapTrajectories, `src/devices`/RoboCrapDevices, `src/viewport`/RoboCrapViewport, and `src/application`/robocrap. Application privately assembles the other five. Kinematics uses Geometry; Trajectories uses Kinematics and Geometry; Viewport uses Geometry for numerical/drawing values. Application exports no C++ include directory. OCCT remains private to Viewport. Geometry publicly supplies Eigen because published numerical types contain Eigen values. Frozen RSI code temporarily keeps Devices dependencies on Trajectories/Geometry/Kinematics and legacy numerical header exposure. Components/Styles remain in `qml/Modules` and are excluded from refactoring.

### Binding behavior during every milestone


Preserve XYZABC order, millimeters/degrees, and Z-Y-X Euler composition. Keep KR10 position tolerance 1e-4 mm, rotation-entry tolerance 1e-6, wrist singularity threshold 1e-9, seeded branch selection, and continuous winding; never wrap saved joints modulo 360 degrees. Numerical results return through values, not output parameters. Initialize persistent vectors to zero and transforms to identity. Add no C++ lambdas. Handle exceptions only from external-library calls, preserving necessary recovery/cleanup; bare rethrow may propagate the same external exception. Define no project exception types and throw no new exceptions from project code. Functions have at most four parameters; any three/four-parameter function needs a responsibility-based explanation.

Fix animation points and times before IK. For a constant-acceleration LIN span, use `N = max(1, ceil(T * sqrt(a / (8 * epsilon))))`, where `N` is interval count, `T` is span duration in seconds, `a` is acceleration magnitude in mm/s², and fixed `epsilon` is 0.01 mm; cruise has one interval. Central counts come from existing curvature/time bounds. A hypothetical 0.2 s ramp with default acceleration 20 mm/s² gives four intervals and five endpoint-inclusive points, of which four are appended because the first is shared. Preserve exact phase endpoints. No extra points, quarter-time IK checks, or automatic retiming may follow an IK/rate rejection.

Keep joint position/speed, polynomial extrema, finite derivatives, and central XYZ acceleration checks. Per-joint acceleration magnitude rejection remains disabled in central preview, LIN including terminal stops, and spline checks; HOME timing still uses configured acceleration. Preserve three independent clamped spline preparations, their endpoint velocities, and immutable publication. The chord criterion does not bound orientation or actual TCP error between saved points after joint interpolation.

Keep Generate separate from Dry Run. Saved motions survive source deletion; pending raw-edge Generate still needs its source plane and cylinder. Model/TCP changes invalidate playback; unchanged calibration does not. Selection must not replace an active motion. Preserve pause/resume, instantaneous Stop at HOME when presentation is available, completion at HOME, and cleanup on readiness loss/mode change/path deletion. Robot graphics readiness still gates Generate and Dry Run. Keep Browse/Retry, basename-priority valid BREP loading, serialized CAD parsing, camera-gesture fitting policy, and decorative exclusion from picking/fit.

The frozen RSI implementation includes `src/network/rsi/`, `src/pathgeneration/rsi/`, `src/pathgeneration/chamfer/chamferjob.*`, `chamfercartesiansampler.*`, `rsiposeencoder.*`, `rsicorrectionlimits.*`, `src/scene/scenemachiningpreparation.*`, `src/scene/chamfershareddata.*`, and `resources/files/rsi/`. The user's complete Logger-removal request permits one recorded mechanical exception: eleven RsiDevice logging statements and its QDebug include were changed in Step 03. All other frozen implementation content remains unchanged; future work does not gain permission to alter RSI behavior. Do not repair the dormant demo, change correction records, add execution, change sampling, or run preparation. Keep existing TCP-returning motion evaluation, central getters, `ChamferSamplingParameters`, prepared-job metatypes/delivery, and current compatibility reads. Shared non-RSI types may be extended without forcing RSI callers to change. Update CMake references around frozen files instead of relocating their bodies.

## Plan of Work


Implement each numbered milestone as a coherent change in the current layout before enforcing the final folder/target layout. Read the named producer, consumer, QML definitions/custom bases, and call sites again before an API edit. After each milestone inspect the diff and update this plan and affected architectural documentation to the implemented state. Priorities describe user impact and architecture dependencies, not measured timing. Apply the resolved Step 02 exception policy when editing library failure handling.

### 01 — Completed prerequisite: capture source/configuration and frozen interfaces


Record HEAD, Git status, active source ownership, QML imports, application commands, and numerical compatibility consumers in `Artifacts and Notes`. Preserve existing user edits and deletion. Search active `src`, `qml`, `resources`, `cmake`, and root CMake, excluding build/vendor/python/old directories from routine source inspection. The user's executable request specifically permits read-only inspection of the existing build's executable/cache/object metadata; it does not authorize building. Record that the raw-edge path still checks both sources and that saved paths use immutable snapshots.

Save executable/configuration identity, source and frozen-RSI hashes, declarations/call sites, and unchanged copies of the existing XP/XC geometry and robot JSON under `codex_plans/architecture_baseline/`. Label those files as source snapshots, not accepted animation requests. The user explicitly skipped TCP/settings input and numerical success/failure/result capture. Do not fabricate calibration, recreate a deleted fixture, or run a profiler/debugger motion capture for this step.

Record the current file and source-line counts before implementation using the exact scope and counting procedure in `Artifacts and Notes`. Keep this original baseline and add later measurements alongside it; moving a file between owners must not be reported as removing its code.

Acceptance: a future implementer can identify the exact source/configuration state, executable identity, captured interfaces, immutable project input copies, and frozen compatibility consumers. Verify the recorded inputs/user edits are unchanged. The source/configuration snapshot completes the agreed step; numerical equivalence is unverified and the user-skipped numerical activity is not a pending checkbox. Freeze excluded RSI APIs before changing shared headers.

### 02 — Completed prerequisite: permit external-library exceptions and preserve recovery


The inspected handlers are in `src/3d/occcontroller.cpp`, `viewportassets.cpp`, `occt/cadloadworker.cpp`, `cachedshapeloader.cpp`, `occviewer.cpp`, `occscene.cpp`, `occviewwindow.cpp`, and `occviewport.cpp`. The recount confirms 13 try blocks in eight files. The user explicitly permits handling exceptions from external libraries such as OCCT; the corresponding `AGENTS.md` rule has been updated. Existing C++ handlers remain unchanged.

Preserve the current operations during later refactoring. `CachedShapeLoader::loadStpWithCache` falls back from an unreadable/invalid cached shape or a BRepTools::Read exception to STEP import; a BRepTools::Write exception warns while retaining the successful imported shape. `OccScene::addShapePartWithId` removes the partially inserted part after display failure and rethrows the same exception. `OccController::applyPose` restores the last committed robot presentation and returns false when presentation fails; it commits the candidate only after presentation succeeds. `OccViewWindow::event` and `initializeViewport` release the failed viewport, update failure/readiness state, and report the error; robot attachment failure reports an error without using the viewport-initialization teardown path. Existing worker/load/solve/font-initialization error reporting and viewer-release/redraw warnings also remain.

The selected policy allows try/catch only for external-library exceptions, including standard C++ exceptions raised by those calls. Bare throw may propagate the same external exception after cleanup. Do not define project exception types or throw a new exception from project code, even if its type belongs to a library. Expected project failures still use optional/result return values. Keep compiler exception support and third-party code unchanged. This permission already comes from the user; do not request it again.

Acceptance met: the rule and Decision Log record the user's permission and the concrete recovery for invalid CAD, partial display installation, failed pose presentation, and failed native-window operations. No handler body or application behavior changed. Future edits still inspect the affected operation/callers and preserve its recovery.

### 03 — Completed: replace Logger with Qt console diagnostics


The user replaced this milestone's former operation-result/UI scope with console diagnostics and complete removal of the Logger mechanism. `src/network/abstractdevice.cpp`, `plc/plcdevice.cpp`, `fts/ftsdevice.cpp`, and `rsi/rsidevice.cpp` now contain direct `qInfo().noquote()` calls at the same 5/14/20/11 message sites. They retain the original conditions, text expressions, category labels, and objectName source. The sole message-format change is replacing the FTS connection HTML with plain local/peer addresses and ports. These calls add no GUI queue, copied logging payload, or replacement logging class.

`src/main.cpp` uses Qt's message pattern for timestamps and retains a small named `logViewportMessage` function for the existing text/error signal. That function writes `[ERROR]` or `[INFO]`, source `3D`, and the original unescaped text. The connection uses QApplication as its lifetime context. Both hot-reload branches use this common setup. Removed files are `src/logger.h`, `src/logger.cpp`, and `src/network/common/loggermessage.h`. Removed integration is AbstractDevice::logMessage, LoggerMessage metatype registration, the three device relay connections, the QML logger context property/QQmlContext include, and the three owning CMake source entries.

The former DeviceOperationResult contract, GUI runner status/error properties, and QML error displays are deferred at the user's request. Do not implement them automatically as remaining Step 03 work or restore Logger. Device commands, state publication, confirmed PLC output behavior, asynchronous FTS saves, worker lifetimes, and queued prepared-job delivery retain their existing logic. RsiDevice changes are limited to console logging, not RSI preparation/execution.

Acceptance at the source level: no active source/QML/CMake reference to Logger, LoggerMessage, logAdded, or the device logMessage signal remains; every former device message site writes through qInfo with its original source/category/text; the viewport connection uses no removed object; no deleted source remains in CMake. Inspect non-logging device code for changes and compare remaining frozen RSI hashes. Diff/content checks are required. Builds and runtime checks remain unperformed unless separately requested; the existing executable cannot validate new code. No hardware action is part of this milestone.

### 04 — Major repeated computation: share central preparation per Generate


Change `MotionCompiler` in `src/pathgeneration/chamfer/chamfermotion.cpp` and stage results in `chamfercentralvalidation.*`/`chamfercentralmotion.*`. Create one private prepared-central value for this Generate call. Lazily construct the three phase arc maps, prescribed timing, and fixed schedule when the first starting configuration reaches central validation. Return that value/error through a cohesive result. Seed-dependent validation receives the prepared value, immutable robot setup, and the preceding LIN seed; stop returning a redundant robot copy.

Cache analytical targets only as their scheduled indices are reached. If a target fails, retain that failure at its original index. For each candidate, solve/check its own joints against the cached prefix. Do not evaluate a later target before an earlier IK failure that previously ended the attempt. Keep configuration order, early LIN failures, first-failure precedence, caps, schedule, winding, and complete-motion publication.

Change diagnostics in `chamferpreparationdiagnostics.h` and the result assembly so shared preparation time is counted once, while each candidate retains its own attempted-stage/IK measurements. Three candidates reaching all three maps currently construct nine maps; after this step the job constructs three. This is a call count, not a measured speedup, and the target cache adds payload.

Acceptance: accepted cases retain their schedule, joint solutions, and rejection behavior; multiple reached candidates share exactly one prepared central value. A candidate failing before central work must not pay for it. Failed attempts remain in diagnostics, and shared milliseconds are not duplicated in totals.

### 05 — Major kinematic interface: return already verified transforms


In `src/3d/robot/kinematics/kr10kinematics.*`, extend `IkSolution` with the candidate's ordered link transforms while retaining existing `q`, `status`, `turn`, and optional failure shape for frozen consumers. Calculate the six-step candidate FK once, apply the existing target-relative flange checks to that array, and return it on success. Replace the wrist decomposition's six-step FK with a helper that performs the same first three joint multiplications in the same order. Preserve the six-step seed calculation used to select the shoulder branch.

Update `ChamferProfiledKinematics` without pretending its explicit FK count includes FK internal to IK. In `src/3d/robot/robotpreviewstate.cpp`, form the accepted RobotPose from the returned array rather than calling forward again; retain finite-joint/limit/transform checks. Replace QString output arguments with cohesive pose/error return values when this state API changes. Update `OccController::loadRobot`, `solve`, and `applyPlaybackPose` to consume that result in the same change, retaining their current commit policy until Steps 08/10 replace its owner. Preserve model-reference lifetime and prevent copying/moving the owner while its solver borrows the member.

Acceptance: same seed produces the same branch, winding, q/status/turn and flange residual decision. The successful manual path performs six seed steps, three wrist-prefix steps, and six candidate steps, totaling 15 rather than 24. Existing RSI callers can still read the original solution fields without source edits.

### 06 — Major assembly cost: retain accepted FK TCP at each point


Extend `ChamferCentralJointSample` in `src/pathgeneration/chamfer/chamfercentralvalidation.h` with the accepted actual TCP matrix. In the validator's sample operation, compose it from the verified flange transforms and saved flange-to-TCP transform, perform the current BASE-coordinate TCP residual check, and return it together with time, joints, and the analytical target. The analytical target remains separate because central XYZ acceleration currently uses it.

In `MotionCompiler::inverse`, LIN construction, and central assembly in `chamfermotion.cpp`, return and append accepted joints/TCP together instead of discarding transforms and calling FK again. Keep the solver's relative flange check and the caller's TCP check: a tool offset makes these different conditions. Do not remove HOME display points, which the renderer uses.

Acceptance: appending an accepted central or LIN point requires no additional full FK. Stored TCP remains FK-derived; analytical targets are not substituted for accepted FK values. Point times/counts, phase boundaries, TCP tolerances, and candidate rejection remain unchanged.

### 07 — Major calibration boundary: cache accepted-list reconstruction


In `src/scene/sceneendeffectors.*`, keep accepted XYZABC lists and their exact equality behavior, and add a cached optional flange-to-TCP matrix reconstructed from the accepted list using the existing Euler conversion. Update it atomically with accepted calibration before emitting signals. Do not make the raw composed matrix from `applySpindlePoses` canonical: current code extracts Euler coordinates and consumers reconstruct them, so skipping that reconstruction changes numerics.

Use the cached matrix in `scenemachining.cpp`, profile CLI, and robot pose/TCP conversion operations. Preserve `FLANGE_ER * ER_TCP` order, transform tolerance, exact no-op handling, original ER coordinates from the read-only UI, and spindleTcpChanged on ER changes even when the final flange TCP is equivalent. Do not convert displayed rounded coordinates back into calibration.

Acceptance: unchanged Apply does not invalidate paths; a real ER/TCP change commits all coupled values before observers run. Accepted coordinate lists, reconstructed matrix, and compatibility revision behavior match the baseline. Internal consumers stop rebuilding the same matrix on each call.

### 08 — Major owner change: application robot configuration and accepted pose


Refactor `src/3d/robot/model/kr10model.*` into the application's `RobotDefinition` implementation and reuse `robotpreviewstate.*` as `RobotController`, initially with owning CMake declarations updated in the current layout. The loader returns a kinematic model and neutral graphics records: source basename, RGB/style, and ordered link description. It must not import `OccPartProps`, `gp_Trsf`, or `Quantity_Color`. Construct those OCCT values inside the renderer.

RobotController owns the stable model/solver and one committed numerical pose on the GUI thread. It exposes loadRobot, manual FK/IK/TCP IK, and playback pose operations, returning cohesive results. Preserve current HOME-versus-model checks and all manual operations even where current QML has no caller. Application owns CAD source choices and model/readiness revisions; renderer retains CAD worker requests/results, parsing serialization, generation checks, and drawing restoration values.

Update `src/main.cpp`, `occcontroller.*`, `occt/occviewwindow.*`, `occt/occviewport.*`, `adapters/occrobotadapter.*`, `qml/Views/Viewport3D/RobotViewport.qml`, and Main's robot/tool commands together. During the transition preserve graphics-ready gating and keep one accepted state owner; do not create synchronized copies in both Application and Viewport. Step 02 is required before modifying existing failure handlers.

Acceptance: robot JSON parsing produces the same numerical model and graphics; manual operations still work; no complete Kr10Model/RobotPreviewState is required by the renderer interface. Native-surface recreation replays renderer-local cached transforms without replacing the application model or revision.

### 09 — Major renderer boundary: accept drawing records, not domain objects


Define concrete viewport records in the existing public controller header: stable application ID with an analytic plane/cylinder patch, circle/ellipse, or existing motion phase polylines; style; bounded diagnostic marker/normal positions; draft axis; ordered robot/tool transforms; and neutral tool identity. Use C++17 alternatives for the supported geometry forms rather than a generic scene framework. The graphics description supplies link count; the renderer must not import KR10 LinkCount.

Move projection from `SceneObject`/`ChamferMotion`/`ChamferPathParameters` into existing application document/robot implementation files. Use analytical dimensions for surfaces and saved FK TCP positions for motion polylines; do not fit or resample inside Viewport. Preserve 256 marker/128 normal-position caps, 64 ellipse markers, and one circle normal. Draft axis receives only center, signed unit direction, and display length.

Replace `OccSceneAdapter::synchronizeObject` and the window/controller forwarding methods with insertion/removal, visibility, diagnostics, selection, and draft-axis operations on these values. Remove the unused reverse diagnostic setter chain through controller/window/viewport/adapter; UI diagnostics still change SceneModel. Remove SceneModel/Machining/EndEffectors and motion imports from renderer public and internal interfaces after adapting all callers.

Acceptance: rendering code cannot inspect document objects, motion internals, feed settings, current joints, or application calibration. Imported analytic surfaces, overlays, saved seven-phase polylines, picking IDs, Browse/Retry, and surface-replay behavior remain correct. Failed insertion preserves the previously accepted document/presentation state.

### 10 — Major playback boundary: one FK and an explicit acceptance result


Add `ChamferMotion::evaluateJoints(seconds)` returning joints/phase without FK. Preserve `evaluate(seconds)` as a TCP-returning compatibility operation for frozen consumers. Share interval/phase selection rather than duplicating it, and retain finite-time rejection, finite out-of-range clamping, exact saved knots, quintic HOME progress, and prepared LIN/central spline evaluation.

Change `SceneMachining::presentPlayback` to call RobotController directly with the evaluated joints and the saved motion's flange-to-TCP matrix. RobotController computes one pose, checks all link transforms and the saved-tool TCP composition for finiteness, sends already composed display transforms to the renderer, and commits only after returned AIS-transform acceptance. Remove the QList joint round trip, playbackPoseRequested connection, isSignalConnected guard, and renderer callback into failPlayback. Return failure to SceneMachining, which stops and reports it before elapsed/phase state advances. Adding RobotController as a collaborator gives SceneMachining three responsibilities at construction: document, calibration, and robot pose service; any QObject parent is the fourth parameter.

Acceptance: one interior Dry Run update calculates FK once. There is no IK, integration, target preparation, or new spline during playback. Pause/resume, end at HOME, immediate Stop to HOME, source deletion survival, active-path deletion, graphics loss, mode change, and calibration invalidation preserve their existing policies. Accepted transforms do not mean drawing has completed.

### 11 — Major application operation owner: WorkspaceController


Add `src/application/workspacecontroller.h/.cpp`. It owns application mode/submode/selection decisions and has three actual collaborators: document, machining session, and RobotController. RobotController coordinates active-tool calibration/presentation, so WorkspaceController does not need a fourth service parameter just to reach the end effectors. An optional QObject parent is allowed as parameter four with that ownership justification.

Move select/toggle, delete selected, intersect selected/select result, mode switch, and shared selection-to-machining-input logic out of `qml/Main.qml`. Preserve stable IDs and zero meaning no input. Stop playback using existing HOME semantics before switching away from Machining; then update active tool and selected machining input. Send resulting selection changes to the renderer through typed application connections. Keep selected input distinct from pending Generate source and active playback path.

Keep panel visibility, focus, draft text, editor capture/Cancel, and group expansion in QML. Update WorkflowPanel, ScenePanel, MachiningPanel, RobotViewport, and Main creators/properties together; preserve hot-reload reset to Measuring and empty selection. Read definitions and required delegate roles before edits, and do not refactor Components/Styles.

Acceptance: import selects the inserted object, additive picking toggles selection, Delete removes only selected application objects, intersection selects new edges, and mode changes preserve tool/editor/playback behavior. Selection does not swap a running motion. Main no longer independently sequences document, machining, and robot mutations.

### 12 — Major retained renderer work: update only changed selections/live parts


In `src/3d/occt/occscene.*`, replace vector tombstones with a live associative store and a separate monotonically increasing PartId counter. Removed parts are erased; IDs are never reused or reset on clear; handle counter exhaustion through the existing optional/result failure path. Internal PartId zero remains valid, unlike application object ID zero. Adapt lookups, remove/visibility/transform operations, and rollback paths together after Step 02.

Change selection reconciliation to compare installed visible part identities. Remove outlines for leaving/hidden/replaced parts and construct outlines only for entering/replaced parts; retain active OCCT selection and face-boundary styling. The same application-ID list can require reconciliation after visibility or presentation replacement, so list equality alone cannot suppress work. Remove Main's second visibility-triggered full selection resend once the application/renderer path covers it.

Acceptance: hide/show the same selected object, modify unrelated diagnostics, replace/remove a presentation, and repeatedly update a draft axis. Selection/picking remains correct; unchanged selected faces do not get new outlines; retained part entries equal live parts rather than all historical insertions. No universal process-memory saving is claimed for the associative store.

### 13 — Major device interface: typed PLC codec and pending records


In `src/network/plc/plcmessagemanager.*`, replace the stateless QObject codec with plain typed operations. At the QML/Runner boundary decode module/index/enabled once into OutputChange. Validate exact input categories/ranges, calculate WriteIo masks, encode bytes/error, decode a typed response alternative, and publish QVariantHash only when updating GUI state. In `plcdevice.*`, remove manager allocation/lifecycle and store command-specific typed correlation in pending records.

Preserve module 1/2, bit 0-7, numeric-string/bool/fractional rejection, every supported raw/register/variable request, six-byte header, 32-byte response extraction, pair-byte swapping, magic/version/length checks, and response-field matching. Keep pending entries until matching response, failed/partial write, disconnect, or stop as now. Do not add expiry/retry/ID recycling policy. Preserve direct Qt diagnostics for encoding rejection, incomplete/complete queued writes, matched errors, and exhaustion. The separate operation-result/UI contract remains deferred.

Preserve PlcMessage enum names through application foreign registration when removing QObject from the codec; do not silently drop an exported enum because no current QML reader was found. Keep named queued invocations and confirmed output-state behavior.

Acceptance: baseline valid requests produce identical bytes; malformed input returns the same rejection category; mismatched replies do not complete pending entries or update LEDs. No manager QObject allocation remains, and internal header/pending/response fields are not repeatedly extracted by dictionary key. Use recorded/offline inputs only; no live PLC actions.

### 14 — Device callback reduction: one pending FTS sample


In `src/network/fts/ftsdevice.*`, replace `m_batch` with one optional last accepted sample. Every accepted packet still receives its sequence-based timestamp and emits `dataSampleHFReady`, preserving the existing direct force delivery without changing RSI. `processBatch` consumes the pending last sample for the existing low-frequency recording and UI publication and clears it. Preserve the packet-driven elapsed-time gate of at least 16 ms, not a new periodic publication timer.

Trace every start/stop/bias/disconnect/timeout/log/save path before replacing batch clears/flushes. Keep each final-sample rule, first all-axis publication, deadband, recording cap, copied-input asynchronous save, completion state/error, and shutdown wait. Remove unconsumed dataBatchReady/streamReset only after checking all signal/QML/string callers; these are deliberate API retirements. Do not remove the live high-frequency signal or stopLogRecording lifecycle calls.

Acceptance: the high-frequency sequence is unchanged and low-frequency/log selection still uses the same last sample at each gate/flush. Under hypothetical uniform 7000/s input, about 112 pending samples per 16 ms are replaced by one; neither input rate nor latency improvement is claimed as measured.

### 15 — Enforce six target owners and intentional application registration


Perform 15a, 15b, and 15c as coherent migrations with updated includes/source lists at each stopping point. Do not count a folder move as an eliminated dependency. Root CMake becomes orchestration, each capability folder declares its target, and only `include/<capability>/...` is publicly exported. Use PRIVATE links for implementation dependencies and PUBLIC only for published values requiring them. Keep C++17/Qt6.8/CMake3.16 and both hot-reload paths.

For 15a, move the KR10 solver/kinematic model into `src/kinematics`, leave numerical surface/math code in `src/geometry`, and move animation path/arc/timing/validation/motion/profiling helpers into `src/trajectories`. Add their target-local CMake files and published headers, updating their consumers. Keep numerical code free of scene/device/renderer calls. Move profile CLI and JSON selection/reading into private Application. `BoundedPlane`/`BoundedCylinder` file reading migrates to real application callers; the Plane legacy file factory and numerical utility readers needed by frozen RSI remain explicit compatibility exceptions. Do not retire getters/types required by frozen consumers merely to make the header tree look complete.

For 15b, move non-RSI worker/codec files into `src/devices`, non-domain OCCT/window/CAD/presentation code into `src/viewport`, and document/calibration/machining, RobotController/RobotDefinition, DeviceHub/Runner/ProfileModel, asset-path configuration, and main into `src/application`. The removed Logger files/type must not be recreated or migrated. Add `src/devices/CMakeLists.txt` and `src/application/CMakeLists.txt`; move `src/3d/CMakeLists.txt` to `src/viewport/CMakeLists.txt`. The application file declares robocrap and its source/module ownership; root CMake retains project/options, dependency discovery orchestration, subdirectories, and deployment/install assembly. Remove the aggregate `src/CMakeLists.txt` after all declarations have real owners. Remove Scene/Backend target ownership after their sources are absorbed into the proper owners. Application has no public C++ include directory; Viewport links Geometry and private OCCT, with no Application/Trajectories dependencies. Devices no longer owns QQmlPropertyMap or QML registration.

Keep frozen RSI bodies at their current paths and explicitly list them in the new owning targets: wire/device code in Devices, offline numerical code in Trajectories, GUI preparation/shared-file coordination in Application. If moved non-RSI headers break a frozen include, retain a minimal forwarding header at that exact legacy path or retain the needed header there for this phase; it forwards to the new published type without adding behavior. Limit any temporary root-src include to PRIVATE compilation of legacy consumers, document each remaining compatibility include/link, and never export all of src PUBLIC. Legacy prepared-job/correction types can still require PUBLIC numerical dependencies; label them retained rather than removed. Remove those exceptions only in separately reopened RSI work.

For 15c, compile `backendqmltypes.h` into the existing `robocrap_qml_module` in both hot-reload modes and remove RoboCrapBackend/plugin links. Register `rdtResponse` through an Application foreign wrapper; keep RDTResponse as the same queued worker value without its QML macro. Preserve Hub C++ ownership, GUI affinity, engine/lifetime checks, and typed application robot/workspace exposure. Keep Viewport's module URI `RoboCrap.Viewport3D`. Update Backend imports/aliases in Main, ScenePanel, PropertiesPanel, MachiningPanel, Network, ConnectionsTable, DeltaPanel, DeltaModuleAP, FtsPanel, FtsBars, and retained KukaPanel; the latter is import compatibility only, not RSI implementation.

Move application QML under `src/application/qml` and update root/per-target source lists, hot_reload_config.json, qml/config.json, resource base/prefix handling, and DeployOcct references together. Keep resource URLs and deployed CAD/cache directory semantics; leave Components/Styles source paths/qmldir untouched. After moving hot-reload configuration, the embedded config URL must stay valid in the ON branch; OFF must load Main from the same application URI. Review both branches even when only one can be run.

Acceptance: five libraries plus robocrap own the non-RSI code described above, the intended graph has no reverse Viewport/Application edge, broad PUBLIC src exports are gone, every moved include/import/resource resolves, and the same QML element names remain available under the documented new Application URI. Explicitly list the frozen-RSI exceptions and retained numerical interface exposure; do not claim the unrestricted nine-edge graph or pure Devices endpoint. Requested build/QML registration verification must cover both configuration paths when the required kit/Felgo installation is available.

### 16 — Smaller stored payload: omit saved velocity and temporary coefficients


In `chamfermotion.h/.cpp`, verify that no saved consumer reads `ChamferJointPoint::velocity`. Keep the HOME derivative calculation and finiteness check local to timing, then omit the saved field. This removes 48 bytes of raw payload per point, up to 19.2 MB at the existing 400000-point cap before alignment/container effects. Preserve HOME display points and existing timing/failure context.

Have `prepareJointSpline` write each checked polynomial into the already allocated final range in the unpublished candidate. Remove the second interval-coefficient array and its copy-back pass. A six-joint interval's four coefficient vectors have 192 bytes of raw payload; remove temporary storage, not required final coefficients. Any failed pivot/extremum/finite/rate check still discards the whole candidate before shared publication.

Acceptance: saved times, joints, TCP, spline coefficients and extrema decisions match the baseline; only unused/temporary payload disappears. State raw payload arithmetic separately from measured resident memory.

### 17 — Smaller arc work: reuse exact values without widening acceptance


In `chamferarclength.cpp`, pass the already computed first/last differentials and original full-interval integral into inverse accuracy checking. Represent the measured interval as one internal numerical value containing its two ArcPoint endpoints, original Integral, and stored distance width; pass that interval and its polynomial to the check. These fields describe the same measured curve interval, not an arbitrary parameter bag, and avoid adding a fifth function parameter. Reuse the first endpoint, zero-distance case, and full integral at the last endpoint, retaining all nine check fractions, positive/monotone inverse checks, residual/derivative tolerances, and cumulative stored-distance rounding check. Do not substitute recomputed or differently rounded distance widths.

Maintain two per-operation counters: actual differential evaluations and the logical evaluations that the old algorithm would have attempted. Charge the original logical calls at their original positions, including cached endpoint/full-integral reads, and reject at the same MaximumEvaluations=800000 boundary. Merely lowering the actual counter would accept work previously rejected and is not equivalent. Keep the counter distinction visible in diagnostics.

Acceptance: intervals reaching all nine checks use 127 rather than 144 actual evaluations while retaining the old logical budget; knots, polynomials, early errors, and work-limit rejection remain the same. Failure partway through a check charges only the work the old path would have reached.

### 18 — Smaller curve work: avoid discarded rotation with its guards intact


In `chamferpath.cpp/.h`, factor a private machining position/radial calculation for use by the lead evaluator, and cache immutable trigonometric constants when the curve is created. Calculate only the needed final lead orientation where equivalence is established. No new public helper class is required.

The discarded machining rotation currently enforces sign/basis conditions. Retain those checks explicitly, or record a derivation from factory invariants that covers the same failure cases before removing them. Preserve depth positivity, signed outward direction, analytical endpoint positions/rotations, and final lead basis checks. If equivalence cannot be established, keep the existing operation and record this optimization deferred; do not weaken acceptance to complete a checkbox.

Acceptance: selected machining/lead progress values and shared endpoints return the same accepted poses/errors; numerical position/orientation conventions and derivatives remain unchanged. The reduction is demonstrated by removed operations, not an assumed time saving.

### 19 — Lower priority: remove verified unused code and deployment payload


Search creation, QML/invoke strings, CMake/resource/deploy references immediately before deletion. Remove QmlChartBridge .h/.cpp, NavigationPanel.qml, and empty files.qrc together with their source/resource entries; remove QtCharts and QtXml requirements only after verifying remaining source dependencies. Merge the sole-used socketstateutils conversion into AbstractDevice's unnamed namespace; moved lines are not counted as deleted work.

Retire uncalled ChamferPath::sample and its result-only types, ChamferArcLength::distanceAt, reverse diagnostic forwarding already replaced by Step 09, unused warningString/loadedSource, OccViewer::redraw, uncalled OccScene helpers/OccPart accessors, PLC string converters, and unused DeltaModuleAP refresh helpers only after the latest consumer search. Keep `ChamferPathSample`, `ChamferPathResult`, `ChamferSamplingParameters`, active world/TCP/ER axes, selection outlines, stopLogRecording, all frozen RSI code, and the now-used diagnostic sink.

Retire per-part trihedron branches only where no producer enables them and after Step 02 covers any touched failure cleanup. Preserve originals of the CAD authoring files; remove only their `.m3d` deployment pattern. Do not delete or restore user assets. Do not inline unrelated application QML wrappers or modify Components/Styles for cosmetic line-count reductions. Treat settings equality as a separate small operation preserving error clearing/Cancel semantics, and leave legacy timing fields/getters needed by frozen consumers available.

Acceptance: no dangling source list, import, resource URL, invokable name, or registration remains; removed APIs are explicitly documented. Package reduction does not masquerade as numerical code/memory reduction. Retained compatibility types still compile through unchanged RSI consumers when build verification is requested.

### 20 — Conditional numerical redesign: analytical cylinder Jacobian


This changes fitting numerics and is not required for the six-owner architecture. Keep it separate from all moves and behavior-preserving reductions. In `src/geometry/boundedcylinder.cpp`, the Jacobian is the matrix of sample residual derivatives with respect to the five fit parameters. Its current implementation perturbs each parameter and traverses all samples for each column. Replace it only after numerical comparison has suitable accepted and degenerate/rank-border cases for all six starting axis choices, called seeds. A rank-border case is near a loss of independently recoverable fit parameters. Otherwise record it deferred and retain finite differences.

For one fixed centered sample \(\bar{p}\) in mm, unit axis \(\bar{a}\), transverse center \(\bar{c}\) in mm, and radius \(R\) in mm, define \(z(\bar{p},\bar{a},\bar{c})=(\bar{p}-\bar{c})\cdot\bar{a}\), \(\bar{r}(\bar{p},\bar{a},\bar{c})=\bar{p}-\bar{c}-z(\bar{p},\bar{a},\bar{c})\bar{a}\), and \(\rho(\bar{p},\bar{a},\bar{c})=\|\bar{r}(\bar{p},\bar{a},\bar{c})\|\). The residual is \(f(\bar{p},\bar{a},\bar{c},R)=\rho(\bar{p},\bar{a},\bar{c})-R\). For each current transverse unit basis vector \(\bar{b}_j\), where \(j=1,2\), the proposed axis column is \(-z(\bar{p},\bar{a},\bar{c})(\bar{r}(\bar{p},\bar{a},\bar{c})\cdot\bar{b}_j)/\rho(\bar{p},\bar{a},\bar{c})\); the transverse-center column is \(-(\bar{r}(\bar{p},\bar{a},\bar{c})\cdot\bar{b}_j)/\rho(\bar{p},\bar{a},\bar{c})\); the radius column is -1. Verify these columns against the actual displacement parametrization before use and explicitly handle near-zero radial distance.

Keep seed order, damping (the stabilization term in the fit update), trial residual evaluation, sample compensation, root-mean-square residual acceptance, and rank/observability handling (the checks that the measurements determine the fitted parameters). Acceptance compares resulting axis/radius/residual and accepted/rejected states, including the existing 0.01 observability boundary, through the existing import/profile outputs or an extension of their production diagnostics. Record any deliberate changed fit behavior and evidence; no iteration/time benefit is assumed. Do not add project test logic to obtain the comparison.

### 21 — Conditional scheduling change: asynchronous surface import


Only implement if a disconnected runtime demonstrates a read/fit pause worth addressing. Current `src/scene/scenesurfaceimporter.cpp` reads, decodes, fits, and inserts before returning; busyChanged is synchronously observable but normal rendering waits. First record the operation and computation duration; do not call it a measured bottleneck before measurement.

If justified, the importer owns one copied request/future containing source URL, selected surface kind, destination identity, and request generation. The worker reads/decodes/fits numerical values and returns fitted geometry/name/error; GUI acceptance validates the destination and token and calls SceneModel insertion. Preserve raw-circle versus compensated-plane/cylinder behavior and units. Reject concurrent imports with the existing busy behavior rather than invent a queue; destination destruction or shutdown prevents insertion and joins the worker. The accepted import alone emits loaded/selects the object. No worker receives a mutable SceneModel pointer or populates a shared result object.

Acceptance: the same input returns the same fit/error, busy can paint while work proceeds, destination loss produces no insertion, and no stale worker changes selection. Record this changed scheduling policy separately from any fitting algorithm change; retain synchronous import if the condition is not met.

### 22 — Scoped completion: verify ownership, behavior, and documentation


Audit the six source owners, published include paths, Qt dependencies, registration/imports, and resources. Verify no Viewport include/link reaches application document, calibration, motion, or kinematics state. Verify private Application headers are not exported. List every retained RSI source-path/header/link/API exception with its frozen consumer and future removal condition; no RSI implementation or preparation is a completion requirement here. Confirm Components/Styles and controller workflows were not refactored.

Update `docs/ARCHITECTURE.md` and only affected architecture constraints in `AGENTS.md` to the implemented state, including intentional API/URI changes, result contracts, real timing counters, readiness, and scoped exceptions. Update this plan's progress, findings, decisions, and retrospective. Do not copy stale file counts from the review or resurrect deleted documentation. Record optional 20/21 as implemented with evidence or explicitly deferred.

Acceptance requires the per-step source/numerical evidence and the verification actually requested by the user. If build/runtime evidence was not authorized or unavailable, label it pending rather than claiming a verified executable. Report skipped checks and pre-existing failures separately. The finish condition is eliminated calculations/interfaces with preserved supported behavior, not a clean folder tree alone.

## Concrete Steps


For every milestone, begin in the repository root and inspect status. The following read-only commands are available without build authorization; `safe.directory` is scoped to each Git invocation because this workspace reports ownership mismatch. Do not change global Git configuration.

    Set-Location -LiteralPath 'E:\Qt\QtProjects\RoboCrap'
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap status --short
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap rev-parse --short HEAD
    rg -n 'target_link_libraries|target_include_directories|qt_add_qml_module' CMakeLists.txt src/CMakeLists.txt src/3d/CMakeLists.txt
    rg -n 'ChamferMotion|validateChamferCentral|solveIK|solveFK' src/scene src/pathgeneration/chamfer src/3d/robot -g '*.h' -g '*.cpp'
    rg -n 'RoboCrap.Backend|OccController|invoke\(' qml -g '*.qml'
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --stat

Adapt paths in searches after Step 15 moves them. Inspect the actual final diff and new/untracked file contents; `git diff` does not include untracked contents. Never stage a plan just to make it visible in a diff. Exclude build/libs/python/old directories from routine source searches. No Git mutation command is part of this plan.

When, and only when, the user explicitly requests build verification, discover existing CMake caches and inspect the chosen cache's CMAKE_HOME_DIRECTORY, compiler, generator, Qt6_DIR, build type, USE_HOT_RELOAD, and FAST_QML_BUILD. Build-directory inspection is then explicitly relevant. Discover the installed matching Qt/MinGW kit rather than inventing SDK paths. Preserve the user's build configuration; use a separate directory when different options are required. Record the resolved absolute paths in this section before execution.

The variable `$architectureBuildDirectory` below must be set to that verified directory, not a guessed path. For a separate configured directory, `$architectureKitArguments` is the recorded array of matching Qt/compiler/generator options obtained from that discovery; do not execute configure with an unresolved kit. These are conditional command templates, not a claim that a build directory/kit has been selected during plan authoring.

    cmake -S . -B "$architectureBuildDirectory" @architectureKitArguments -DUSE_HOT_RELOAD=OFF -DFAST_QML_BUILD=OFF
    cmake --build "$architectureBuildDirectory" --target help
    cmake --build "$architectureBuildDirectory" --target robocrap --parallel
    cmake --build "$architectureBuildDirectory" --target robocrap_qmllint

Use configure only for the separately selected configuration or a matching authorized cache. Build the changed owning target and robocrap; verify actual target names before using new RoboCrapKinematics/Trajectories/Viewport names. Run robocrap_qmllint for changed application QML/registration. Components/Styles lint is unnecessary unless an integration check actually affects them. Repeat the ON branch only with installed Felgo/FelgoHotReload and requested verification; report inability rather than installing or changing requirements automatically.

If the user later explicitly requests preview numerical verification and an accepted input is then supplied, resolve `$architectureExecutable` to the verified executable. No such input or report was created in Step 01 because that activity was skipped. Before invoking the existing profiler, inspect its input flag:

    $architectureInput = Get-Content -Raw -LiteralPath 'codex_plans/architecture_preview_input.json' | ConvertFrom-Json
    if ($architectureInput.previewOnly -ne $true) { Write-Error 'Preview input must explicitly set previewOnly to true.'; return }
    & $architectureExecutable --profile-chamfer 'codex_plans/architecture_preview_input.json' 'codex_plans/architecture_preview_after.json'

Only a separately requested numerical comparison can create before/after reports or point payload. Expected changes are named counters/payload reductions; physical movement duration and prescribed point schedule remain unchanged in equivalent reductions. Do not claim existing numerical evidence from the Step 01 source snapshot or reopen its skipped activity automatically. Never run profiling without explicit previewOnly=true, click RSI Sampling, invoke generateTrajectory, or start controller execution.

For requested disconnected UI verification, start only the verified executable in a session with hardware endpoints disconnected. Exercise local imports, calibration, Generate, Dry Run, selection, resize, and native-view recreation. Do not connect PLC/FTS/RSI, send output changes, Bias a sensor, or start hardware motion without separate explicit authorization covering that action. Local application preview is separate from controller readiness.

## Validation and Acceptance


Validation is required for each milestone at the level authorized by the user. Source checks and diffs run routinely; build-backed and runtime/numerical checks follow the conditions above. No new project tests/harnesses are permitted. A future implementation report must distinguish inspected code, calculations of expected operation counts, executed numerical comparisons, disconnected UI observations, and hardware behavior not checked.

When numerical comparison is separately requested, compare an accepted input and failing input with their recorded original outputs: fixed sample count/times, exact shared endpoints, q/status/turn and winding, FK TCP, candidate/error order, position/speed/XYZ acceleration and spline rejection. Step 01 contains no such numerical outputs; source inspection alone must not be described as that comparison. Retain current tolerances; do not introduce a broader comparison tolerance to hide a changed result. Record actual changed floating-point behavior if any; behavior-preserving steps do not silently become a numerical redesign. Verify no IK/integration/spline construction occurs during Dry Run by inspecting its complete call chain and available existing diagnostics.

For UI/lifetime steps, observe Generate draft commit/Cancel, repeated unchanged Apply, changed calibration invalidation, imported object selection, additive picking, hidden selection, intersection/deletion, playback pause/resume/Stop/end, selection during playback, readiness loss, and shutdown during Generate/CAD/FTS save. No dangling worker or native window may access a destroyed owner. Presentation failure must not commit a new robot pose or advance displayed elapsed time.

For devices, compare recorded request/response bytes and malformed-input handling without hardware. Confirm worker requests remain queued, state/results/property maps are mutated on the GUI thread, sockets/timers stay on their I/O thread, shutdown stops devices before quitting threads, and FTS high-frequency delivery remains intact. Do not infer response-loss safety from a past working-protocol report; the pending-ID lifetime behavior remains a documented deferred concern.

For architecture, read all published headers and owning CMake files, search cross-owner includes (including relative paths), resolve QML imports and resources, and trace current creation/call sites. PRIVATE links alone do not prove independence. Completion leaves no non-RSI renderer/document cycle and no broad PUBLIC src export, while naming all deferred RSI exceptions. Intended operation-count evidence includes three central maps per reached Generate job, one accepted candidate FK reused by consumers, and one interior playback FK. Optional analytical fitting/import changes have their own acceptance and do not invalidate these comparisons.

## Idempotence and Recovery


Re-read status and each touched file before editing; earlier milestone names may have moved but the current owning CMake lists are authoritative. Keep a coherent working implementation at every stopping point and record partial progress. If a candidate preparation fails, discard its unpublished values; if a presentation update fails, preserve/replay the last accepted renderer state according to the resolved policy. Do not publish partial jobs or replace an old scene path before successful insertion of its replacement.

If a source migration fails, repair the last affected source list/include/import incrementally using the recorded diff; do not reset, stash, discard, or restore unrelated user files. Any rollback that discards changes requires an explicit user request. Never recursively move/delete a computed Windows path without checking its resolved target stays in the intended workspace. Preserve source CAD, user IDE settings, and deleted user files. Never edit generated build outputs or vendored dependencies to make a check pass.

Keep compatibility declarations until every non-frozen consumer has migrated. A missing external consumer is not proven by a repository search; document intended public API retirements and preserve the frozen ones. Retrying a failed compile or accepting a later worker result must not skip stale-token checks, replay old commands, or restart hardware workflows.

## Artifacts and Notes


### Before-implementation file and line counts


Captured on 2026-10-14 at HEAD `3709a12ca3726858f2116608f4dd3dc2c01a336d`, including current working-tree edits, before architecture implementation. Overall project scope is every existing tracked or non-ignored untracked file under `src/`, `qml/`, `resources/`, and `cmake/`, plus root `CMakeLists.txt` and `hot_reload_config.json`. Exclude any directory named build, libs, or python, or starting with old. Documentation/plans outside those roots, Git metadata, IDE user settings, and generated build output are excluded. Deleted files count as zero. Resource contracts/examples inside the selected roots remain included. Existing frozen RSI inputs remain included in size totals, without authorizing changes to them.

Count one physical text line for every element returned by .NET `ReadAllLines`: blank and comment lines count, a final unterminated line counts once, and LF/CRLF endings give the same count. A nonblank line contains at least one non-whitespace character; comment-only lines still count. CAD/font extensions `.stp`, `.step`, `.brep`, `.m3d`, `.ttf`, and `.otf` count as files with zero text lines, including text-encoded CAD. These are explicit physical source/text counts, not comment-stripped code counts.

| Input category | Files | Physical text/source lines | Nonblank lines |
|---|---:|---:|---:|
| C++ `.h` / `.cpp` | 129 | 17,584 | 14,881 |
| All QML `.qml`, including Components/Styles | 37 | 4,198 | 3,526 |
| CMakeLists.txt / `.cmake` | 6 | 518 | 471 |
| Other text inputs: JSON, SVG, qrc, qmldir, controller examples/contracts | 51 | 1,745 | 1,699 |
| CAD/font assets | 26 | 0 | 0 |
| Overall project total | 249 | 24,045 | 20,577 |

Components/Styles account for 20 files, 844 physical text lines, and 693 nonblank lines, including their CMake/qmldir inputs. Excluding them gives the architecture scope: 229 files, 23,201 physical text lines, and 19,884 nonblank lines. Its C++ and application QML subset is 150 files, 21,017 physical source lines, and 17,784 nonblank lines: 129 C++ files plus 21 application QML files. For reference, all C++/QML including the excluded reusable controls is 166 files, 21,782 physical source lines, and 18,407 nonblank lines. The source-hash manifest's 193 present text inputs use a different recorded selection and are not the project file total.

Repeat the following read-only PowerShell procedure from `E:\Qt\QtProjects\RoboCrap` for an overall count. It includes existing non-ignored untracked inputs as well as tracked inputs and removes duplicates. Retain the original numbers above when recording an after-implementation comparison. If application QML moves under `src/application`, classify it by the `.qml` extension, not its former folder.

    $architectureFiles = @(git -c safe.directory=E:/Qt/QtProjects/RoboCrap ls-files --cached --others --exclude-standard -- src qml resources cmake CMakeLists.txt hot_reload_config.json | Sort-Object -Unique | Where-Object { $_ -notmatch '(^|/)(build|libs|python|old[^/]*)(/|$)' -and (Test-Path -LiteralPath $_ -PathType Leaf) })
    $architectureCounts = @($architectureFiles | ForEach-Object {
        $extension = [System.IO.Path]::GetExtension($_).ToLowerInvariant()
        $lines = @()
        if ($extension -notin @('.stp', '.step', '.brep', '.m3d', '.ttf', '.otf')) {
            $lines = [System.IO.File]::ReadAllLines((Join-Path (Get-Location).Path $_))
        }
        [pscustomobject]@{ Path = $_; PhysicalLines = $lines.Count; NonblankLines = @($lines | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count }
    })
    $architectureCounts.Count
    $architectureCounts | Measure-Object PhysicalLines,NonblankLines -Sum
    $architectureScope = @($architectureCounts | Where-Object { $_.Path -notmatch '^qml/Modules/(Components|Styles)/' })
    $architectureScope.Count
    $architectureScope | Measure-Object PhysicalLines,NonblankLines -Sum
    $architectureCode = @($architectureScope | Where-Object { [System.IO.Path]::GetExtension($_.Path).ToLowerInvariant() -in @('.h', '.cpp', '.qml') })
    $architectureCode.Count
    $architectureCode | Measure-Object PhysicalLines,NonblankLines -Sum

At the original baseline capture, overall output was 249 files, PhysicalLines sum 24045, NonblankLines sum 20577; architecture scope was 229 / 23201 / 19884; its C++/QML subset was 150 / 21017 / 17784. Keep these original numbers; later measurements appear below. The baseline recount changed only the plan and did not run a build, application, profiler, or RSI operation.

### Step 01 evidence


Step 01 evidence (2026-10-14): `codex_plans/architecture_baseline/manifest.json`, `interfaces.json`, `SOURCE_CONTRACTS.md`, and `inputs/XP.json`, `inputs/XC.json`, `inputs/kr10.json`. HEAD is `3709a12ca3726858f2116608f4dd3dc2c01a336d`. The existing executable is `build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/robocrap.exe`, SHA256 `7C88C80F983A1156865021B1C2161408EB35E93B4FBF244E5CE115D93537A609`, Debug, USE_HOT_RELOAD=OFF, FAST_QML_BUILD=ON. Cache/object timestamps and symbols are consistent with the inspected sources; no embedded source-revision proof is asserted. At Step 01 completion the captured 193 present tracked text inputs remained unchanged and 31 excluded RSI paths were clean relative to HEAD. Initial user edits/deletion were preserved. Numerical input/results were explicitly skipped. GDB inspected symbols/types only; neither the executable nor the profiler was started. No build or hardware action occurred. The OCCT policy was unresolved at Step 01 completion and was subsequently resolved in Step 02; Step 03 implementation is recorded separately below. Do not create empty evidence files or fabricate example success transcripts.

Step 01 checks passed: source/frozen-RSI/executable hashes, preserved pre-existing user edits/deletion, three exact JSON copies, captured interface source locations, JSON parsing, required plan sections/heading spacing/progress placement, new-file whitespace, and `git diff --check`. Six baseline files exist; there are no numerical result files.

Step 02 evidence (2026-10-14): the user explicitly allowed external-library exceptions and kept other exceptions banned. Only `AGENTS.md` and this plan changed. The eight handler files still contain 13 try blocks; inspected cache, part insertion, pose update, and native-window branches retain their original recovery. Existing source hashes remain unchanged. No build, runtime, profiler, RSI, or hardware operation ran.

### Step 03 evidence and size after Logger removal


On 2026-10-14, source comparison verified all 50 former device payloads and category labels, with only the FTS connection HTML deliberately replaced by plain text. Removing diagnostic statements and the added QDebug include from before/after versions leaves identical non-logging content in AbstractDevice, PlcDevice, FtsDevice, and RsiDevice. The other 30 frozen RSI inputs retain their baseline SHA256 hashes; RsiDevice's authorized logging-only revision has SHA256 `9EDBE709CF493EF0B74BD21F726DC95F78D34106EB18F21FAFBBDC6F47556325`. The 183 other present tracked text inputs and unrelated pre-existing user edits/deletion remain unchanged. Searches found no active Logger, LoggerMessage, logAdded, or logMessage reference in source/QML/CMake/resources. `git diff --check` passed. The executable was not rebuilt or run, so console behavior is verified through source paths, not a runtime observation.

Using the unchanged counting rules above, the source after Step 03 has the following sizes. The original baseline is retained alongside them.

| Scope | Before files | After files | Before physical lines | After physical lines |
|---|---:|---:|---:|---:|
| Overall project | 249 | 246 | 24,045 | 23,854 |
| Architecture scope, excluding Components/Styles | 229 | 226 | 23,201 | 23,010 |
| C++ and application QML in architecture scope | 150 | 147 | 21,017 | 20,829 |

After-removal nonblank counts are 20,404 overall, 19,711 in the architecture scope, and 17,614 in its C++/application-QML subset. Three deleted C++ files account for the file reduction. Overall physical text decreases by 191 lines; C++/application-QML physical source decreases by 188 lines and CMake by three lines. These are source-size differences, not measured runtime or memory improvements. Additional operation-result/QML error work remains deferred by the user.

For each completed milestone record a short evidence entry with date, source paths/functions, before/after operation or interface, actual checks/output, and pending limitations. For example, when actually observed, a central-preparation entry can state a job reached three candidates, built three maps once, retained the baseline points/times, and preserved a named rejected attempt. Do not use that example as an already measured result.

## Interfaces and Dependencies


Use the existing concrete owners, Qt types, Eigen values, optional/results, and futures; add no Core library, interface target, event bus, generic presenter base, or strategy framework. New public values describe the actual operation, not an entire application object graph. The following names define the intended contracts; preserve legacy compatibility fields/getters until frozen consumers can be migrated separately.

`Kr10Kinematics::IkSolution` retains q/status/turn and adds the verified ordered numerical transforms; `solveIK(target, seed)` keeps its optional result shape. A private first-three-joint calculation returns a matrix. Pose construction returns a `RobotPoseResult` containing optional RobotPose and error, replacing QString& outputs. Arrays of persistent transforms are explicitly filled with identity.

`ChamferMotion::evaluateJoints(seconds)` returns optional `ChamferPlaybackJoints` containing six joints and phase. Existing `evaluate(seconds)` returns the existing TCP-bearing compatibility result. Shared central preparation is a private numerical value/result, and seed-dependent validation returns accepted samples without an input robot copy. New application consumers use the completed motion, saved settings/display values, and evaluator instead of individual preparation stages.

`RobotController::applyPlayback(joints, savedFlangeToTcp)` returns `RobotOperationResult` containing accepted/error status after numerical pose calculation and renderer acceptance. The saved calibration is explicit input so playback does not silently use a different current tool. Manual FK/IK/TCP commands remain application API. RobotController owns its borrowed model's lifetime and commits its own state through a named operation. Its application collaborators are SceneEndEffectors for accepted calibration/active-tool choice and the concrete viewport for returned transform acceptance; SceneEndEffectors remains the calibration owner.

`OccController::applyRobotTransforms(drawingPose)` returns `ViewportOperationResult` containing acceptance/error. DrawingPose contains ordered link transforms and already composed tool/axis transforms, with no robot model, joint list, SceneObject, or ChamferMotion. Other renderer methods receive one concrete object/diagnostic/selection/CAD request value. Renderer-local replay storage holds those accepted drawing values and worker results; it does not retain application QObject pointers.

`WorkspaceController` has document, machining, and RobotController collaborators, typed selection/mode notifications, and explicit select/toggle/delete/intersect/mode operations. QML drafts and local panel state remain in QML. Diagnostics use Qt console output directly. The proposed DeviceOperationResult/operationResultReady GUI contract is deferred; no custom logging payload or GUI-owned shared result object is introduced.

At the scoped endpoint, Geometry publishes Eigen and QtCore; Kinematics publishes its numerical model/results and Geometry requirements; Trajectories publishes completed animation interfaces and explicit legacy compatibility; Viewport publishes numerical drawing records and Qt GUI/QML window requirements with private OCCT; non-RSI Devices uses QtCore/Network and copied operation/protocol values. Application owns all QML adaptation/registration and assembly. Devices' remaining numerical links and any legacy Qt/header exposure required by frozen RSI are explicitly exceptional. They cannot be declared removed by relocating headers or changing PUBLIC to PRIVATE alone.

## Revision note


2026-10-14: created this ExecPlan from the current architecture review and inspected source interfaces. Ordered implementation by impact/dependencies, added granular progress marks and acceptance evidence, excluded RSI implementation, and recorded the conditional policies/compatibility needed to make the scoped destination honest. No source implementation or build configuration was changed while writing it.

2026-10-14: completed only Step 01 after explicit authorization to use the existing executable without a build. Recorded source/configuration and frozen interfaces; narrowed the step to exclude numerical input/result capture at the user's explicit request. Updated progress, acceptance, decisions, findings, commands, and evidence consistently. No later implementation was started.

2026-10-14: added the user-requested before-implementation file/line baseline, category breakdown, precise scope, and repeatable read-only counting procedure. Updated progress, findings, decisions, outcomes, and Step 01 accordingly. The recount accounts for the deleted 67-line JSON README and distinguishes physical/nonblank lines from comment-stripped code. Only this plan changed.

2026-10-14: completed Step 02 after the user explicitly allowed exceptions originating in external libraries such as OCCT while keeping other exceptions banned. Updated `AGENTS.md`, recorded the existing recovery and cleanup/rethrow, and removed the resolved policy conflict from current plan instructions. No C++ code or size baseline changed; Steps 03 onward remain pending.

2026-10-14: completed the user-revised Step 03 by replacing all former Logger messages with direct Qt console diagnostics and removing its class, payload, signal, metatype, context property, relay wiring, and source-list entries. Deferred the previous operation-result/QML proposal; recorded the logging-only RsiDevice exception without reopening RSI work. Updated current architecture/instruction documents and retained the original before-implementation baseline. Build/runtime checks were not requested.
