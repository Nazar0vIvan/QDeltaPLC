# RoboCrap Architecture and Redundancy Reduction Plan


This is a self-contained execution plan for GPT-6 Astra Light or another coding agent working in the RoboCrap repository.

Maintain this document according to PLANS.md. This version supersedes the earlier review-derived plan and incorporates the subsequent corrections.

Implement only the numbered step explicitly requested by the user.


## Purpose and Intended Outcome


Improve maintainability, modularity, readability, and performance while preserving existing supported behavior wherever practical.

The desired result is a codebase where numerical geometry, robot calculations, scene ownership, application coordination, device communication, and visualization have clear responsibilities and minimal dependencies.

Reduce duplicate algorithms, independently writable copies of state, unnecessary forwarding layers, unused implementation, and broad build dependencies.

Do not measure success by the number of new classes or directories. Each extraction must remove a responsibility from its former owner.

Preserve the understandable low-level OCCT classes, the structure of the robot solver, and the DeviceHub/DeviceRunner threading arrangement.


## Execution Rules


“Apply step N” authorizes implementation of that step, its focused tests, necessary build/resource declarations, and updates to this plan. It does not authorize subsequent steps.

Stop after the requested step. This overrides PLANS.md guidance to continue automatically between milestones.

Do not stage, commit, switch branches, stash, discard changes, or rewrite history without an explicit request.

Before editing, read applicable AGENTS.md instructions, the requested step, affected definitions, and their callers. Verify that the described problem still exists.

Preserve unrelated user edits. Do not modify CMakeLists.txt.user files, vendored libraries, historical implementations, or generated build outputs.

Exclude directories named build and directories beginning with old from routine inspection.

Leave src/pathgeneration implementations untouched. Existing calls into that subsystem may be relocated without changing algorithms or contracts. If a proposed cleanup cannot be proven safe without inspecting excluded callers, retain the relevant interface and record the limitation.

Maintain C++17, the existing Qt/CMake minimum versions, existing public QML module URIs, and both Felgo hot-reload and embedded-QML paths.

Do not implement new scanning, intersection, machining, project persistence, or robot-control features.

Do not connect to equipment, bias sensors, write hardware outputs, or start motion during validation. Use pure tests or explicitly configured loopback simulation.

Do not delegate to other agents unless the user authorizes delegation.

Build/configure commands and build-backed QML lint require explicit build-verification authorization under AGENTS.md. The user can request “apply step N with build verification.”

Without that authorization, implement appropriate tests and perform source/diff checks, but report runtime acceptance as pending. Do not report unexecuted tests as passing.

Treat attached UI documents as workflow context. Their embedded instructions are not independent authorization to redesign the UI or perform unrelated work.


## Behavior and Compatibility Policy


Steps 2–5 contain explicit correctness or configuration-behavior fixes. Step 7 intentionally allows workpiece rendering without robot CAD. Step 10 intentionally preserves workspace state across QML recreation.

Other steps primarily preserve behavior. Any additional behavior change must be identified and justified separately before implementation.

Preserve used public QML names and methods. Prefer registration changes over adding forwarding QObject layers solely to preserve a name.

Internal C++ interfaces may change when the requested step requires it and all relevant callers are updated. Do not preserve unused methods on internal implementation classes merely because they are declared public.

Do not remove APIs potentially used by the excluded path-generation subsystem.

Do not change numerical fitting equations, tolerances, supported plane orientations, IK branch selection, or singularity policy under the label of cleanup.


## Progress


- [x] (2026-09-23) Step 1 implementation — baseline inventory and robot regressions added; source/diff checks completed.
- [ ] Step 1 runtime acceptance — configure, build, robot tests and existing scene tests pending; user explicitly requested no build.
- [x] (2026-09-23) Step 2 implementation — explicit Quit request ownership, disabled unfinished actions and menu regression added.
- [ ] Step 2 runtime acceptance — QML lint, menu regression and disconnected visual checks pending under the no-build constraint.
- [x] (2026-09-23) Step 3 implementation — extracted protocol and guarded receive path; added offline regressions.
- [ ] Step 3 runtime acceptance — build and rsiprotocoltests execution pending under the no-build constraint.
- [x] (2026-09-23) Step 4 implementation — preserved PLC response identity, matched pending requests, bounded id allocation and offline regressions.
- [ ] Step 4 runtime acceptance — build and plcprotocoltests execution pending under the no-build constraint.
- [ ] Step 5 — Resolve device profiles by stable identity.
- [ ] Step 6 — Extract application-scene OCCT adaptation.
- [ ] Step 7 — Decouple workpiece rendering from robot CAD loading.
- [ ] Step 8 — Clarify robot data, preview state, and loading contracts.
- [ ] Step 9 — Relocate surface-import orchestration.
- [ ] Step 10 — Establish persistent workspace state and consistent QML lifetimes.
- [ ] Step 11 — Remove preparation and file export from ControlIO.
- [ ] Step 12 — Remove protocol construction from QML.
- [ ] Step 13 — Establish enforceable internal module boundaries.
- [ ] Step 14 — Consolidate mathematical conventions and implementations.
- [ ] Step 15 — Make scene updates incremental and simplify property UI.
- [ ] Step 16 — Reduce fitting allocations and sample conversions.
- [ ] Step 17 — Complete dead-code cleanup and the architecture audit.

Record dates and actual validation status when updating progress. Distinguish “implemented, runtime validation pending” from “validated.”


## Surprises & Discoveries

Step 4 inspection: parseRespOk read valid device fields but returned Y for READ_IO/READ_REG/WRITE_REG and omitted it for WRITE_IO. Pending state was only a QSet of ids; allocation wrapped without checking occupancy and failed writes left entries behind. RESP_ERR carries command/error/code only, limiting correlation to id/command. No supported expiry duration was found in active PLC code or its QML consumers. Existing fixed 32-byte replies and adjacent-byte swapping remain authoritative.

Step 3 inspection: RsiDevice previously replaced the configured peer address on the first datagram, parsed errors into default IPOC zero, and called makeTxFrame regardless of validity. RIst was parsed but never consumed. fromKRC.xml is compatible without RIst. toKRC.xml differs from the active sender: its RKorr uses A1–A6 instead of XYZABC and its IPOC is zero-padded. The active sender uses numeric quint64 IPOC normalized to decimal and does not serialize shouldStop. These wire behaviors remain unchanged.

Step 2 inspection (2026-09-23): MainMenuBar inherited the standard Qt Quick Controls MenuBar with no custom base or implementations of quit/cut/copy/paste/about. Main.qml is its only production creator. Searches found no application-level clipboard or About handler to connect. The existing sceneobjecttests runner discovers source tst_*.qml files and embeds the menu logo, so the new isolated menu test needs no CMake/resource changes.


The review and follow-up inspection were static. No builds, runtime tests, or hardware operations were performed.

The current Main.qml does not instantiate the packaged dashboard, network page, logger view, navigation panel, or robot kinematics panel. These are disconnected functionality, not automatically disposable code.

CMakeLists.txt.user was modified externally during the review. Preserve it.

resources/files/fromKRC.xml contains AIPos, MACur, and IPOC, but no RIst. Do not assume that every accepted RSI packet must contain Cartesian pose data. The example itself must also be checked against the supported active protocol.

Pose::fromFrame preserves the supplied Euler values after constructing a transform. Reconstructing its frame from the transform can change observable angle values despite representing the same orientation.

Scene and Hub singleton registration retain raw engine pointers, while OccController registration uses QPointer and runtime checks. View recreation and engine recreation are distinct lifecycle cases.

Step 1 inspection (2026-09-23): src/3d/robot/model/linkmodel.h includes OccPartProps and src/3d/math/mathtypes.h aliases gp types. Numeric tests therefore still require OCCT TKernel/TKMath, but do not link the viewport module or instantiate its viewer/CAD worker. The root configure still requires the existing Windows MinGW/OCCT kit. RobotPreviewState calculations are const and commit is explicit; OccController::applyPose commits only after presentation succeeds. Tests cover the calculation boundary, not presentation rollback. No runtime failure has been established without executing tests.


## Decision Log

Decision: Retain validated request maps by id, scan the full existing 8-bit id space for an unused entry, and match echoed command/device/module/address/value or variable identity before removal/publication. Keep error matching to the fields actually present. Extract only a private processIncoming(bytes) seam and use a friend test class for production-path offline regressions.
Rationale: This removes ambiguous outstanding-id reuse without changing wire framing or public device APIs. Positive short writes abort the incomplete stream; failures remove their entry. No verified timeout policy exists, so expiry is deferred and no write retry is introduced.
Date/Author: 2026-09-23, implementation agent.

Decision: Extract decode/encode and a shared receive gate into network/rsi/rsiprotocol.{h,cpp}. The gate validates sender and complete XML before learning a port or calling the motion callback. Keep the configured address immutable during reception and reset only the learned port on connection changes.
Rationale: An optional result prevents malformed traffic from consuming offsets. A small templated callback gate allows the exact production validation ordering to be tested without device initialization, trajectory preparation, sockets or a new framework. IPOC alone is required; known optional numeric records are validated when present, unknown extensions remain allowed. No sequence or stop-message policy is introduced.
Date/Author: 2026-09-23, implementation agent.

Decision: MainMenuBar emits quitRequested; Main.qml handles it with Qt.quit(). Keep unfinished Cut, Copy, Paste and About actions visible but disabled through Action.enabled, preserving standard disabled-item styling.
Rationale: Application lifetime belongs to the application owner, and there is no existing clipboard routing or About implementation. The signal allows the real menu action to be tested without exiting the runner.
Date/Author: 2026-09-23, implementation agent.

Decision: Use the existing lightweight executable-test style and compile the four production numerical source files into robotnumerictests, linked only to Qt Core and OCCT TKernel/TKMath. Read the original kr10.json through a compile-time source-root path. Do not duplicate the solver or add a test framework, viewer, CAD loading, or production refactor.
Rationale: This protects production calculations without pulling in QML registration, device initialization or presentation; the current OCCT numerical dependency is recorded rather than prematurely redesigned.
Date/Author: 2026-09-23, implementation agent.


Decision: Preserve DeviceHub and DeviceRunner as the thread boundary.
Rationale: Queued requests and GUI-thread state publication are useful existing contracts.
Date/Author: 2026-09-23, planning agent.

Decision: Keep Eigen and OCCT.
Rationale: Eigen supports numerical calculations and fitting; OCCT supports CAD topology and presentation. Remove duplicate mathematical implementations and ambiguous conventions.
Date/Author: 2026-09-23, planning agent.

Decision: Do not require an additional robot-preview controller.
Rationale: RobotPreviewState is already cohesive. A further forwarding layer is justified only by a remaining independent responsibility after other extractions.
Date/Author: 2026-09-23, planning agent.

Decision: Remove superseded implementation during each extraction.
Rationale: Moving code without retiring duplicate ownership or logic increases complexity.
Date/Author: 2026-09-23, planning agent.

Decision: Retain disconnected diagnostic screens unless separately authorized for retirement.
Rationale: Their removal is a product decision. Their dependencies and integration status should nevertheless be explicit.
Date/Author: 2026-09-23, planning agent.

Decision: Keep step numbers stable.
Rationale: The user invokes work using “apply step N.” Use substeps for refinements rather than renumbering.
Date/Author: 2026-09-23, planning agent.


## Outcomes & Retrospective

Step 4 implemented on 2026-09-23. Corrected decoded PLC device identity and replaced the id-only pending set with request identity maps. Added unused-id allocation with exhaustion rejection, failure cleanup and immediate explicit-disconnect cleanup (remote-disconnect cleanup retained). Mismatched replies cannot consume pending entries or publish state. Framing, byte order, public QML/device APIs, state keys and thread ownership remain unchanged. Added plcprotocoltests linking the production backend/device, with no new production target boundary or third-party framework. Source/diff checks completed; no build/runtime execution was authorized.

Step 3 implemented on 2026-09-23. XML decoding/encoding moved out of RsiDevice and the old parser/writer were removed. Removed the independently maintained first-read flag; learned port zero now denotes no accepted peer. Invalid XML/numbers and foreign endpoints no longer produce replies, establish peers or invoke the offset-consuming callback. Public device/QML interfaces, thread ownership, trajectory algorithms, motion timing, valid reply format and state keys remain unchanged. RsiTxFrame keeps its original name and fields through an included protocol header. Added rsiprotocoltests under the existing opt-in option, linked only to Qt Core/Network. Source/diff checks completed; build/runtime acceptance remains pending.

Step 2 implemented on 2026-09-23. Removed five calls to nonexistent menu methods. Quit now has explicit application ownership; four unfinished actions are unavailable. Added only the quitRequested QML signal; no module URI, dependency or existing supported interface changed. No duplicate state was introduced or removed. Menu layout, labels, shortcuts and styling are preserved. Added tests/qml/tst_MainMenuBar.qml to check request emission and disabled action/item state. Source/diff checks completed; configure, build, lint, regression execution and visual verification remain pending because building was not authorized.


Step 1 implemented on 2026-09-23. Added an opt-in robotnumerictests executable compiling actual production model, math, FK/IK and preview-state sources; recorded the baseline below. No production responsibility, duplicate implementation, state, dependency or public interface was removed: this step establishes regression protection before later extractions. Runtime behavior is unchanged. No configure, build, lint, test binary, GUI or hardware execution was performed. Source inspection and whitespace/diff checks are the only completed validation; runtime acceptance remains pending.


## Repository Context


Repository root: E:\Qt\QtProjects\RoboCrap.

src/main.cpp is the composition root: it creates long-lived objects and connects subsystems. Keep it explicit; do not replace it with a service locator.

src/network/devicehub.* owns thread distribution. DeviceRunner and its QQmlPropertyMap remain on the GUI thread. AbstractDevice implementations own sockets and timers on I/O threads. PLC uses GeneralIO. FTS and RSI share ControlIO.

src/scene owns SceneObject identity, metadata, and geometry wrappers. BoundedPlane and BoundedCylinder retain fitted geometry and original samples.

src/geometry/planeimporter.* currently combines file validation, fitting dispatch, worker lifetime, error messages, and scene insertion.

src/3d/occcontroller.* coordinates persistent robot preview state, CAD loading, and the native window. OccViewWindow owns the surface-specific viewport. OccViewport currently also constructs application geometry and diagnostic overlays.

src/3d/robot contains model data, analytical kinematics, preview state, and RobotOccSceneAdapter.

qml/Main.qml currently owns workflow mode, selection, and display preferences.

Root CMakeLists.txt owns application QML/resources. src/CMakeLists.txt combines backend sources and direct executable sources. src/3d/CMakeLists.txt owns the OCCT module.

An “adapter” is a concrete translator between subsystem data and operations. It does not imply an abstract interface hierarchy.

The “application layer” coordinates existing operations across scene, numerical, and device code. It must not absorb numerical algorithms, protocol encoding, or rendering implementation.


## Target Responsibilities and Dependency Direction


Numerical geometry owns fitting, bounds, numerical validation, and shared mathematical conventions. It must not insert scene objects, register QML types, or construct OCCT presentations.

Robot numerics owns joints, limits, FK/IK, and validated preview state. Its final numerical interfaces must not depend on CAD filenames, AIS appearance, or native windows.

Scene owns object identity, metadata, and immutable geometry references. It must not load files, control sockets, or own OCCT handles.

Devices own protocols, sockets, and I/O-thread execution. They must not own workflow UI, geometric preparation, or file export.

Application code owns import coordination, persistent workspace state, and coordination of currently implemented operations. It depends on lower-level contracts.

3D presentation owns OCCT viewer/window lifecycle and concrete scene/robot adapters. It consumes scene and numerical data without depending on device management.

QML integration owns registration, properties, and boundary conversion. QML owns layout and transient editing state, not duplicate domain state.

These are logical boundaries, not a requirement for seven libraries or seven QML modules. Introduce only enough internal targets to enforce useful dependency restrictions.

Qt Core types are acceptable where useful. Do not undertake a blanket “remove Qt from everything” rewrite.


## Plan of Work


### Step 1 — Establish the baseline, dependency inventory, and robot regressions


Purpose: protect existing behavior and make subsequent cleanup evidence-based.

Save or update this document as ARCHITECTURE_EXECPLAN.md. Do not overwrite an unrelated document.

Inspect existing tests under tests/qml. Record current public QML entry points, target dependencies, and ownership relationships.

Identify repeated implementations and classify them as shared behavior, intentional local similarity, dead internal code, or disconnected functionality. Keep this inventory concise and update it during later steps.

Add focused tests under tests/robot for home-pose consistency, representative FK/IK round trips away from singularities, joint limits, branch behavior, and failed preview calculations leaving committed state unchanged.

Use resources/json/kr10.json explicitly as a fixture. Do not create a viewer or load robot CAD.

Add an opt-in ROBOCRAP_BUILD_ARCHITECTURE_TESTS option, default OFF, and a robotnumerictests target. Use Qt Test or the existing lightweight executable-test style; avoid a new third-party test framework.

Do not refactor production behavior in this step.

Acceptance: tests exercise actual production calculations; baseline units, XYZABC ordering, Z-Y-X composition, limits, and singular behavior are recorded. With authorization, existing scene tests and new robot tests pass, or pre-existing failures are documented separately.


### Step 2 — Repair menu action ownership


Purpose: eliminate enabled actions that invoke undefined methods.

Inspect qml/MenuBar/MainMenuBar.qml and its use in qml/Main.qml.

Replace root.quit(), root.cut(), root.copy(), root.paste(), and root.about() calls with explicit ownership. The menu should emit requests or use supplied actions; Main.qml owns application-level handling.

Connect Quit to the application quit operation. Disable unfinished actions unless an existing implementation can be connected without creating a new feature.

Do not introduce a clipboard-routing framework or design a new About dialog.

Add a QML regression that verifies action requests without terminating the test runner.

Acceptance: enabled menu actions have valid handlers; unfinished actions are visibly unavailable; existing styling is preserved. This is an explicit behavior fix.


### Step 3 — Isolate and validate RSI protocol handling


Purpose: prevent invalid input from becoming valid execution input and separate XML handling from socket/motion state.

Inspect src/network/rsi/rsidevice.* and resources/files/fromKRC.xml and toKRC.xml.

Create src/network/rsi/rsiprotocol.h and rsiprotocol.cpp with plain decoded/encoded value structures and explicit decoding failure.

Determine required fields from supported exchange behavior and consumed data. Do not require RIst merely because the current parser knows how to read it. Validate numeric values when present and reject malformed required fields or incomplete XML.

Preserve the valid reply format and IPOC representation. Document discrepancies between example files and active code; do not silently redesign the protocol.

Validate sender address against configuration before learning a peer port. Once established, validate subsequent sender endpoints. Rejected packets must not advance offsets or establish the peer.

Add rsiprotocoltests under tests/network covering valid examples, missing required fields, invalid numbers, malformed XML, optional absent pose fields, and rejected senders. Include a focused production-path check that rejected input cannot consume an offset.

Acceptance: valid supported packets retain compatible behavior; invalid/foreign packets cannot advance motion. Do not add new stop-message or IPOC sequencing policy without verified requirements.


### Step 4 — Repair PLC response and transaction handling


Purpose: remove incorrect response identity and ambiguous outstanding-request matching.

Inspect src/network/plc/plcmessagemanager.* and plcdevice.*.

Correct decoding that reads a device field but reports DEV::Y unconditionally. Preserve the decoded value or validate it against a verified command-specific requirement.

Keep framing and byte swapping unchanged.

Track pending request identity sufficiently to match replies. Allocate an unused transaction ID; reject exhaustion instead of reusing an outstanding ID. Remove failed-send entries and clear pending state on disconnect.

Treat expiry and timeout values as explicit client behavior. If implementing expiry, use a named, documented policy supported by existing requirements or evidence. Do not invent automatic retries for output writes. If no timeout policy can be justified, leave expiry as a documented follow-up rather than guessing.

Add plcprotocoltests for response device identity, request/reply matching, transaction wraparound/exhaustion, fragmented input, and disconnect cleanup. Add expiry tests only if expiry is implemented.

Acceptance: outstanding requests cannot share an ID, decoded identity is correct, and unrelated replies cannot silently update state.


### Step 5 — Resolve device profiles by stable identity


Purpose: eliminate dependence on JSON row ordering and competing configuration/status sources.

Inspect resources/json/db.json, src/network/common/deviceprofilemodel.*, qml/Views/Network/Network.qml, and ConnectionsTable.qml.

Retain and expose the existing driver key. Resolve Backend.Hub.device(driver) instead of indexing a separately ordered runner array.

Unknown drivers produce unavailable rows and disabled actions.

Distinguish profile defaults, the current editing draft, and applied connection configuration. Derive live status from the runner instead of maintaining an unused second live-status source.

Centralize small address/port parsing helpers only where rules are identical. Keep TCP connection, FTS endpoints, and RSI port-discovery policies separate.

Do not expand this step into persistent profile editing.

Acceptance: reordering profiles does not redirect configuration to another device; unknown profiles are harmless; displayed values do not misleadingly imply that edited connections still use unchanged defaults.


### Step 6 — Extract application-scene OCCT adaptation


Purpose: keep geometry-specific presentation out of viewport mechanics.

Inspect src/3d/occt/occviewport.*, occscene.*, occinputcontroller.*, and RobotOccSceneAdapter.

Create src/3d/scene/applicationsceneoccadapter.h and applicationsceneoccadapter.cpp.

Move application-ID mappings, plane/cylinder face construction, diagnostic-shape construction, visibility reconciliation, and application picking lookup into the adapter.

Keep small shape builders local to its implementation. Preserve bounds, original samples, overlay limits, styling, visibility, and selection behavior.

OccViewport retains composition, input coordination, and render scheduling. Remove the moved implementation and direct concrete-geometry inspection from it.

Simplify OccInputResult after checking consumers. Remove unconsumed cameraChanged/hoverChanged distinctions and merge redraw flags if they truly select the same operation. Preserve cameraScaleChanged behavior needed for world axes.

Do not change robot readiness requirements yet.

Acceptance: imports, overlays, picking, visibility, and camera gestures behave identically; generic viewport code no longer constructs application geometry.


### Step 7 — Decouple workpiece rendering from robot CAD loading


Purpose: allow measurement geometry inspection when robot assets are unavailable.

Inspect OccController, OccViewWindow, OccViewport, RobotOccSceneAdapter, and RobotViewport.qml after Step 6.

Initialize the viewer and application-scene adapter without robot state/shapes. Attach robot presentation when available.

Expose viewport readiness separately from robot-preview readiness. Preserve the existing ready property’s meaning for current robot-preview callers; add viewportReady for independent rendering.

Robot attachment failure must not destroy application presentations. Avoid rebuilding the viewer when robot loading completes.

Define initial camera fitting explicitly. Preserve a camera the user has already manipulated rather than unexpectedly resetting it after asynchronous loading.

Acceptance: imported surfaces render with missing or failed robot CAD; robot retry remains functional; surface recreation restores application presentations. This deliberately removes the previous robot-CAD prerequisite.


### Step 8 — Clarify robot data, preview state, and loading contracts


Purpose: separate kinematic state from appearance and loading machinery without creating unnecessary controller layers.

Inspect src/3d/robot/model, kinematics, robotpreviewstate.*, occcontroller.*, cadloadworker.*, occviewwindow.*, and robotoccsceneadapter.*.

Separate joint/limit/home/analytical data from CAD filenames and visual appearance. One loader may still read both from the same JSON definition.

Make Kr10Kinematics depend only on kinematic data.

Retain RobotPreviewState as owner of the model and current IK seed. Restrict commitment to validated, internally consistent candidate poses. Keep useful cached transforms.

Keep QML list conversion in OccController. Do not add a new preview controller by default.

Give rendering consumers read-only access where possible. Preserve current candidate/apply/commit behavior and recovery after partial presentation failure.

Make CadLoadWorker accept visual asset data rather than the entire robot model. Move the loaded-shapes structure into a data header independent of the worker class. Presentation consumers should depend on that data header, not cadloadworker.h.

Acceptance: robot regressions remain valid; invalid input preserves state; kinematic data does not contain OccPartProps; presentation consumers no longer depend on loading implementation.


### Step 9 — Relocate surface-import orchestration


Purpose: separate numerical geometry from file/worker/application responsibilities.

Move orchestration from src/geometry/planeimporter.* into src/application/surfaceimportcontroller.*.

Preserve PlaneImporter’s public QML name and load/loadCylinder methods through registration where possible. Do not create an unnecessary duplicate QObject implementation solely to retain the name.

Separate point-file decoding from fitting. Introduce small structured error categories where failures can actually be distinguished. Do not invent detailed failure reasons unsupported by the algorithm.

Use an application-owned importer for the main workspace, while retaining constructible instances needed by tests. QML view recreation must not destroy the active import worker.

Keep destination-lifetime checks and GUI-thread insertion. Retain bounded shutdown behavior and add cooperative cancellation only at practical algorithm boundaries; never terminate workers forcibly.

Keep fitting equations and rejection policies unchanged.

Acceptance: successful imports create one object, failures create none, destroyed destinations are handled, and view recreation does not synchronously wait for fitting. The old orchestration implementation is retired.


### Step 10 — Establish persistent workspace state and consistent QML lifetimes


Purpose: remove independently writable selection/workflow copies and manual recreation repair.

Create src/application/workspacestate.h and workspacestate.cpp, owned by main.cpp.

Move mode, submode, selected IDs, and overlay preferences from Main.qml into this state. Keep panel expansion, hover state, and temporary text drafts in QML.

Normalize selection, reject unknown IDs, and preserve additive/background selection behavior.

Wire the presentation coordinator to persistent state directly. Remove repeated Main.qml readiness handlers whose sole purpose was restoring these values.

Apply a consistent engine-lifetime policy to Scene, Hub, and viewport singleton registrations. Use lifetime-aware engine observation and explicit invalid-use handling, without building a generic singleton framework.

Test view recreation and engine destruction/recreation separately. Do not assume hot reload always recreates the engine.

Acceptance: selection, modes, and overlays survive workspace recreation; scene objects retain identity; single-engine ownership rules remain explicit and do not retain stale engine pointers.


### Step 11 — Remove preparation and file export from ControlIO


Purpose: prevent numerical preparation or disk output from delaying FTS/RSI receive processing.

Inspect RsiDevice::generateTrajectory and FtsDevice recording/export code. Leave src/pathgeneration implementations untouched.

Move the existing preparation procedure to a worker outside ControlIO. Preserve constants, algorithm calls, generated values, and the generateTrajectory entry point.

Return completed offsets through a queued handoff. Reject stale completion after disconnect, shutdown, or superseding work. Prevent overlapping preparation and retain rejection during active motion.

Move FTS JSON serialization and disk output to a worker receiving a stable snapshot. Preserve low-frequency recording semantics, capacity, data format, and default filename.

Wire services through application composition. Devices must not include or construct an application workflow controller.

Bound outstanding jobs and define shutdown explicitly. Do not add one thread per helper or an unbounded work queue.

Acceptance: preparation/export does not run on ControlIO; valid results are installed once; stale results cannot replace current execution data; errors remain observable.


### Step 12 — Remove protocol construction from QML


Purpose: expose explicit operations without replacing the runner architecture.

Inspect DeltaModuleAP.qml, KukaAutExt.qml, their device callers, and current application coordination.

Move PLC output-mask and request-map construction into small concrete C++ operations. Continue using queued device requests.

Preserve DeviceRunner.invoke for compatibility and diagnostics. Do not replace every telemetry map with a new class.

Do not create one controller owning PLC, FTS, RSI, imports, selection, and preview. Keep protocol-local helpers near the device implementation; introduce cross-device coordination only for an existing operation that requires it.

Distinguish bound/connected transport from confirmed communication and execution where the existing behavior provides that evidence. Do not silently redefine isConnected.

Acceptance: migrated QML code contains no PLC wire-level masks; actual execution checks remain in C++; unavailable future scan/machining commands remain inactive.


### Step 13 — Establish enforceable internal module boundaries


Purpose: make dependencies reflect responsibilities rather than merely moving files.

Inspect root CMakeLists.txt, src/CMakeLists.txt, src/3d/CMakeLists.txt, and affected public headers.

Create a small set of internal targets separating numerical code, scene ownership, devices, application coordination, and visualization. Preserve existing public QML modules.

Give concrete devices an owning target. Stop using unrelated executable source additions as their only module boundary. Preserve excluded path-generation build semantics.

Make visualization depend on scene/numerical contracts rather than the entire backend. Move registration ownership out of the network implementation directory.

Reduce exported subsystem include paths. Keep implementation-only includes private and provide dependency usage requirements for types exposed in public headers.

Move AbstractDevice’s Logger::instance connection to composition code. Retain log signals and message behavior. Audit RDTResponse’s QML exposure; move registration concerns out of protocol data only after verifying actual consumers, preserving metatype support.

Make numerical/protocol test configuration possible without mandatory GUI, Felgo, or viewport setup. Preserve normal application OCCT toolchain constraints.

Robot numerics may temporarily retain OCCT math until Step 14; record that dependency explicitly.

Acceptance: target dependencies are acyclic and narrow; lower layers do not include application/presentation headers; dedicated tests link their actual owning targets.


### Step 14 — Consolidate mathematical conventions and implementations


Purpose: eliminate the two competing math implementations while preserving numerical behavior.

Use robot regressions and bounded-geometry tests as the reference. Remove verified unused 3D math helpers before migrating live algorithms.

Establish shared Eigen-based numerical operations for degree-based XYZABC conversion, bases, and rigid transforms. Keep OCCT native types within CAD/presentation code.

Migrate robot calculations without changing analytical equations, branch selection, limits, tolerances, or singularity policy. Convert numerical transforms to OCCT in RobotOccSceneAdapter.

Retire superseded Euler/basis/projection implementations. Do not retain parallel implementations indefinitely.

Document authoritative representations and useful caches. Pose::fromFrame must preserve supplied Euler values where the existing contract does. Do not mechanically replace all state with one transform or break reference-returning APIs used by excluded callers.

Define a numerical validity contract for bounded data accepted through C++ insertion paths. Reuse it rather than duplicating inconsistent importer/scene/renderer validation. Keep checks for actual OCCT operation failure at the rendering boundary.

Use semantic names where they prevent confusion; do not build a custom matrix library.

Acceptance: numerical robot code no longer needs OCCT; transform order and angle behavior match established tests; duplicated live algorithms are removed; existing geometry entry points remain compatible.


### Step 15 — Make scene updates incremental and simplify property UI


Purpose: avoid whole-scene work for local changes and prevent property panels from growing indefinitely.

Add targeted collection/object notifications while preserving stable SceneObject ownership and the objects property.

Update only affected OCCT parts for visibility changes. Avoid reconnecting all object signals after insertion and redisplaying already-visible parts.

Split PropertiesPanel.qml into general, plane, and cylinder sections. Reuse a small concrete read-only field component for repeated formatting and layout.

Preserve rename drafts targeting the original object, accessible names, selection semantics, and sizing behavior.

Do not merge QxPanel and NetworkPanel mechanically: their default content layouts and styling contracts differ.

Keep the current browser unless representative scene measurements justify virtualization. If needed, use an incremental item model exposing existing object references; avoid a generic tree framework.

Acceptance: toggling one object does not redisplay unrelated parts; properties remain synchronized; edits and resizing behave as before.


### Step 16 — Reduce fitting allocations and sample conversions


Purpose: remove concrete memory/work duplication without changing fitting algorithms.

Inspect boundedplane.cpp, plane.cpp, boundedcylinder.cpp, and point-file reading after earlier changes.

Choose one owned sample representation through each import pipeline. Remove avoidable array/Eigen/container round trips and move samples into retained results where ownership permits.

Reuse cylinder Jacobian and residual buffers. Extract duplicated finite-difference Jacobian construction into a file-local helper.

Preserve sample order, original coordinates, fitting equations, initial candidates, arithmetic order where practical, convergence policy, and rejection thresholds.

Use representative small and large synthetic inputs for timing/allocation checks when execution is authorized. Do not use a guessed performance benefit to justify algorithm replacement.

Acceptance: numerical regressions remain valid; original samples and bounds are unchanged; measurements or concrete allocation analysis demonstrate the reduction.


### Step 17 — Complete dead-code cleanup and the architecture audit


Purpose: finish unresolved cleanup without deleting useful functionality or preserving accidental duplication.

Recheck active callers before removal. Remove verified dead internal helpers, abandoned state, obsolete comments, and superseded implementations left by migration.

Do not preserve unused internal methods solely because they are public. Do preserve contracts whose excluded callers cannot be verified.

Classify retained dormant code explicitly. Keep diagnostic screens and QmlChartBridge unless the user authorizes retirement. Audit their dependencies, but do not automatically remove Charts, Widgets, Xml, or QApplication based on incomplete searches.

Inspect remaining small utility duplication. Centralize only shared semantics; leave single-file helpers local.

Audit logging boundaries and record remaining presentation coupling. Do not add persistent logging/history as an unrequested feature.

Update factual AGENTS.md architecture statements. Keep implementation history in this plan.

Acceptance: each retained compatibility layer has an actual reason; no accidental parallel implementation remains; target/include dependencies match the intended direction; unresolved limitations are explicit.


## Concrete Commands and Validation


Run commands from E:\Qt\QtProjects\RoboCrap.

Before and after each step:

    git --no-optional-locks status --short
    git --no-optional-locks diff --check
    git --no-optional-locks diff --stat

If dubious ownership prevents inspection, use a command-local exception for this exact path:

    git --no-optional-locks -c safe.directory=E:/Qt/QtProjects/RoboCrap status --short

Do not change global Git configuration.

Inspect the complete diff and all newly created files. Ordinary git diff does not display untracked contents.

Build verification requires explicit authorization. Inspect CMakeCache.txt before reusing a build. Confirm source root, Qt installation, compiler, generator, and configuration. Discover the matching toolchain; do not invent SDK paths.

For a new compatible Ninja build, after selecting $buildDir and $qtPrefix and activating the matching compiler:

    cmake -S . -B "$buildDir" -G Ninja "-DCMAKE_PREFIX_PATH=$qtPrefix" -DUSE_HOT_RELOAD=OFF -DCMAKE_BUILD_TYPE=Debug

For relevant application changes:

    cmake --build "$buildDir" --target robocrap --parallel

For QML changes, verify target availability and run the owning target:

    cmake --build "$buildDir" --target robocrap_qmllint
    cmake --build "$buildDir" --target Components_qmllint
    cmake --build "$buildDir" --target Styles_qmllint

Run only relevant targets; use all_qmllint for multiple modules where appropriate.

Enable existing scene tests only in an authorized compatible configuration:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_SCENE_TESTS=ON
    cmake --build "$buildDir" --target sceneobjecttests boundedplanetests boundedcylindertests --parallel
    & "$buildDir/boundedplanetests.exe"
    & "$buildDir/boundedcylindertests.exe"
    & "$buildDir/sceneobjecttests.exe" -platform offscreen -import "$PWD/qml/Modules" -o -,txt

After the architecture-test option and respective targets exist:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
    cmake --build "$buildDir" --target robotnumerictests --parallel
    & "$buildDir/robotnumerictests.exe"

Steps 3 and 4 add equivalent commands for rsiprotocoltests and plcprotocoltests. Record exact fixture and execution requirements when introducing them.

Adjust executable locations for multi-configuration generators. Expect exit code zero and all applicable cases passing. Never invent a test count.

For registration/resource changes, validate embedded QML with hot reload OFF. When cache-generation behavior is relevant and authorized, also check FAST_QML_BUILD=OFF. A fast build does not validate that path.

Run disconnected UI checks for relevant states: import success/failure, selection, rename commit/cancel, visibility, overlays, resizing, robot loading failure/retry, and viewport recreation.

Do not use stale binaries as proof of changed-source correctness.

Without build authorization, report source checks completed and build/lint/runtime acceptance pending.


## Acceptance and Per-Step Handoff


After each step, explain:

1. What responsibility was removed from the original class or module.
2. What duplicate implementation or independently writable state disappeared.
3. What dependency or cross-layer knowledge was removed.
4. What public interface changed and why.
5. What behavior remained equivalent and what intentionally changed.
6. Which checks actually ran, their results, and remaining validation gaps.

An extraction is incomplete if the new class exists but the old owner still retains the same responsibility.

Not every step must reduce line count or class count. New code must provide a concrete boundary, remove duplication, or enforce a necessary invariant.

Keep tests focused on observable behavior and important boundaries. Do not add tests that merely mirror implementation details.


## Idempotence and Recovery


Before applying a step, determine whether it is already implemented. Verify its outcome rather than duplicating work.

If partially implemented, preserve existing valid work and complete the remaining scope. Record partial implementation and pending validation accurately.

Default execution order is Step 1 through Step 17. If the user requests a later step first, check its actual prerequisites. Do not silently implement missing steps. Explain the smallest prerequisite when it prevents the requested work.

Resolve routine implementation choices autonomously. Ask only when a decision materially changes behavior, public scope, or the agreed architecture.

Do not use destructive Git operations for recovery. Repair only the requested work and preserve unrelated changes.

For high-risk mathematical migration, capture baseline fixtures before replacing the old implementation. Retain temporary comparison code only until equivalence is established, then remove it before declaring the step complete.


## Artifacts and Maintenance


The authoritative plan file is ARCHITECTURE_EXECPLAN.md.

Update Progress, Surprises and Discoveries, Decision Log, and Outcomes and Retrospective after each requested step.

Record exact new test commands and concise evidence here. Keep the document self-contained so a future agent does not need the earlier conversation.

Do not create an additional architecture-document hierarchy unless this plan becomes insufficient for a concrete ongoing need.

Stop after the requested step. Do not automatically start the next step.


## Revision Notes


2026-09-23: Consolidated the review, original numbered plan, and follow-up audit. Removed the mandatory extra preview controller; strengthened build and header boundaries; required removal of superseded code during each extraction; corrected RSI field assumptions; preserved Euler-value semantics; added singleton, logging, and CAD-result dependency cleanup; and clarified validation and one-step authorization.

## Step 1 baseline inventory (2026-09-23)


The current public QML registration boundary is src/network/backendqmltypes.h.
RoboCrap.Backend 1.0 exports Hub and Scene as C++-owned singletons; SceneModel,
PlaneImporter and DeviceProfileModel are creatable; DeviceRunner, SceneObject,
PlaneGeometry, CylinderGeometry and PlcMessage are uncreatable foreign types.
Hub.device(key), for keys plc/fts/rsi, returns GUI-thread runners exposing data,
socketState, isConnected, isDisconnected and invoke(method, args). Scene exposes
objects, findObject, addObject, addPlane, renameObject and setObjectVisible.
Objects expose stable objectId, name, kind, classification, visible, sourceUrl and
geometry. PlaneGeometry exposes coefficients plus hasBounds, origin, tangent axes,
width, height and pointCount; CylinderGeometry exposes origin, axis, radius, length
and pointCount. PlaneImporter exposes busy, load(url, scene), loadCylinder(url, scene),
loaded(object) and failed(message). DeviceProfileModel exposes names and device(row);
PlcMessage supplies protocol enums. Preserve these names during later changes.

src/3d/viewportqmltypes.h exports RoboCrap.Viewport3D 1.0's C++-owned OccController
singleton. Its boundary is viewWindow, ready, loading, errorString, warningString,
jointAngles, flangePose; loadRobot, detachViewWindow, solveFK, solveIK,
setSelectedObjects and setDiagnosticOverlays; and pose/readiness/window/error
notifications, message and applicationSelectionRequested. The embedded application
entry is robocrap_qml_module/Main, loaded by src/main.cpp; the Felgo branch retains
its own loading path. logger remains a context property. Components exports the
15 Qx controls listed in qml/Modules/Components/qmldir; Styles exports Colors,
Fonts and Metrics singletons. Neither module's API changes in step 1.

Root CMakeLists.txt owns the robocrap executable, QML packaging and resources. It
links Backend and Viewport3D with their static plugins, Components, Styles and Qt
Core/Gui/Quick/Qml/Network/Charts/Xml/Widgets (plus Felgo when enabled).
src/CMakeLists.txt owns RoboCrapBackend, linking Core/Qml/Network publicly and using
Eigen headers privately. It groups scene, bounded fitting/import, device runner/hub,
profiles, PLC messages and logger; it also adds concrete devices, trajectory geometry,
Pose, path generation and chart bridge directly to robocrap. src/3d/CMakeLists.txt
owns RoboCrap3D, publicly linking Core/Gui/Qml and privately linking Backend and
OCCT TKernel, TKMath, TKG3d, TKBRep, TKTopAlgo, TKService, TKV3d, TKOpenGl, TKDESTEP.
Components links Qml and Styles; Styles links Qml. Thus the logical numerical,
application and presentation boundaries are not yet enforced by separate targets.
The new robotnumerictests links only Core/TKernel/TKMath, compiles four production
.cpp files and introduces no new public library. Root configuration retains all
existing SDK requirements even when only that test target will be built.

src/main.cpp owns Hub, ApplicationScene and OccController before the QML engine,
so they outlive it. DeviceHub owns GUI-thread DeviceRunners and their property maps;
parentless devices move to GeneralIO (PLC) or ControlIO (FTS and RSI), with queued
requests and state publication. The FTS-to-RSI connection shares ControlIO. stopAll
stops devices synchronously before quitting/waiting for both threads. SceneModel
owns SceneObject children and their immutable geometry wrappers. PlaneImporter
owns its worker lifetime and inserts worker results into a guarded destination on
the GUI thread. OccController shares RobotPreviewState and CAD results with its
window, owns the CAD worker and observes the application scene. OccViewWindow owns
the viewport; OccViewport constructs application geometry and overlays as well as
coordinating rendering. Main.qml currently owns selection, workflow and overlay
preferences; these are lost with that QML instance. Step 1 changes none of this.

Shared-behavior duplication: src/geometry/utils.cpp and src/3d/math/utils.cpp both
implement Euler conversion, transforms, basis validation, projection and polynomial
helpers using Eigen and OCCT respectively. They are candidates for later convention
consolidation, not interchangeable today: polyfit2d uses QR versus determinants and
deriv2d has different invalid-input behavior. Pose::fromFrame preserves supplied
Euler values; conversion through a transform does not preserve that representation.
The Scene/Hub singleton factories in backendqmltypes.h repeat engine ownership checks,
but OccController uses a guarded engine pointer and runtime rejection instead.

Intentional local similarity: PLC/FTS/RSI configuration paths all parse addresses and
ports (src/network/*/*device.cpp), but TCP peer configuration, FTS local/peer endpoints
and RSI learned peer ports have different contracts. Only identical scalar validation
is a potential shared helper. Plane/cylinder wrappers share metadata concepts but
have distinct authoritative bounded geometry; similarity alone is not a reason to
introduce inheritance or merge fitting implementations.

Dead-internal-code candidates: searches of active src and qml outside pathgeneration
found no call sites for the OCCT math jsonArrayToPoints, polyfit2d, deriv2d, acos2 or
isEqual helpers beyond their declarations/definitions. This is a qualified inventory,
not proof of removability: excluded path-generation callers were not inspected and
all interfaces remain. No code is removed in this step.

Disconnected functionality: Main.qml instantiates workspace panels and RobotViewport,
not the packaged Dashboard, Network, Logger, NavigationPanel or RobotKinematicsPanel.
Their packaging in root CMakeLists.txt is not evidence of active screen integration.
src/main.cpp's QmlChartBridge wiring is commented out while its implementation remains
an executable source. Keep these features pending a separate retirement decision.

The existing tests/qml suite uses a lightweight executable style for bounded plane
and cylinder calculations and a Qt Quick Test runner linked to the real backend.
It covers fitted bounds/sample retention and invalid inputs; the QML suite covers
shared identity, ownership, rename/visibility, selection, ID collisions, failed and
repeated plane/cylinder imports and Properties panels. The tests do not start the
application main, devices or viewport. See tests/qml/tst_SceneObjects.qml,
boundedplanetests.cpp and boundedcylindertests.cpp. They have not been executed in
this task; no pre-existing runtime failures or passing baseline is claimed.

Robot numerical baseline: kr10.json has qHome [0,-90,90,0,0,0] degrees and pHome
[890,0,1080,0,90,0] in XYZABC order. Positions/dimensions use the fixture's millimetre
scale; angles are degrees. Orientation is Rz(A)*Ry(B)*Rx(C). Joint limits are
[-170,170], [-185,65], [-137,163], [-185,185], [-120,120], [-350,350] degrees,
inclusive at the preview boundary. FK accepts singular poses; IK intentionally
rejects wrist sin(q5) at or below Eps, including q5=0 home, and arm targets on the
axis or with degenerate/unreachable distances. Eps=1e-9, PosEps=1e-4 and RotEps=1e-6
in src/3d/math/mathtypes.h. IK uses the current joints to choose shoulder, elbow and
wrist branches and the nearest legal 360-degree equivalent. Status bits 0/1/2 mean
overhead/negative elbow/negative wrist; turn bits identify negative joint angles.
Tests pin representative branches and sign masks, check round trips by transforms,
and test limits/nonfinite/unreachable failures. They compare all committed pose
fields after rejected calculations and successful uncommitted previews. Viewer
rollback and every singular threshold neighborhood remain outside this focused suite.

### Step 1 validation and continuation


Source/diff review only was performed on 2026-09-23. Configure, compilation,
build-backed lint, the existing scene binaries and the new robot binary were not
run because the user explicitly prohibited building. Git initially rejected the
repository ownership; subsequent read-only checks used the command-scoped
-c safe.directory=E:/Qt/QtProjects/RoboCrap setting, without changing global config.
The pre-existing CMakeLists.txt.user modification was preserved.

After explicit build authorization, follow AGENTS.md to inspect/select a matching
Qt/MinGW build and compiler environment. For a valid existing hot-reload-OFF build:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON -DROBOCRAP_BUILD_SCENE_TESTS=ON
    cmake --build "$buildDir" --target robotnumerictests boundedplanetests boundedcylindertests sceneobjecttests --parallel
    & "$buildDir/robotnumerictests.exe"
    & "$buildDir/boundedplanetests.exe"
    & "$buildDir/boundedcylindertests.exe"
    & "$buildDir/sceneobjecttests.exe" -platform offscreen -import "$PWD/qml/Modules" -o -,txt

Run from the repository root. Keep the matching Qt/MinGW and configured OCCT bin
directories on PATH; the numeric target deliberately has no CAD deployment hook.
Use configuration-specific executable paths for multi-configuration generators.
robotnumerictests reads resources/json/kr10.json via ROBOCRAP_SOURCE_DIR and must
print Robot numeric regressions passed with exit zero. All scene tests must pass;
record actual failures separately rather than weakening assertions. Until then,
step 1 is implemented with runtime acceptance pending. Do not start step 2 without
a new user request.

Revision 2026-09-23 (step 1): Saved the supplied plan in the repository, added the
baseline/dependency inventory and opt-in production robot regressions. Documented
current OCCT coupling, preserved all production interfaces and deferred runtime
acceptance to honor the explicit no-build request.

### Step 2 validation and continuation


Implementation is limited to qml/MenuBar/MainMenuBar.qml, its creator in qml/Main.qml,
tests/qml/tst_MainMenuBar.qml, tests/qml/README.md and this plan. The test triggers the
real Quit Action twice and observes quitRequested with SignalSpy, with no Qt.quit
connection in the harness. It checks all four unfinished Actions and generated menu
items are disabled. The production connection to Qt.quit is verified by source review;
the regression intentionally does not instantiate Main.qml or start devices.

After explicit build authorization, select a compatible hot-reload-OFF build according
to AGENTS.md, enable ROBOCRAP_BUILD_SCENE_TESTS and use the matching kit on PATH:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_SCENE_TESTS=ON
    cmake --build "$buildDir" --target sceneobjecttests robocrap_qmllint --parallel
    & "$buildDir/sceneobjecttests.exe" -platform offscreen -import "$PWD/qml/Modules" -o -,txt

Expect the MainMenuBar and existing SceneObjects cases to pass without QML errors;
the runner must stay alive after triggering Quit in the menu harness. In an explicitly
disconnected UI session, check the standard disabled appearance of the four unfinished
items and menu layout at minimum and normal window widths. Source inspection and git
diff --check (including the new QML file separately) are the only validation performed
on 2026-09-23; runtime and visual acceptance remain pending. No hardware action or
build command was run. Preserve the existing step 1 and CMakeLists.txt.user changes.
Stop here; step 3 requires a new user request.

Revision 2026-09-23 (step 2): Assigned Quit handling to Main.qml through a menu signal,
disabled unimplemented actions, and added a safe isolated QML regression. Updated
progress, findings, decisions and outcomes; carried forward the no-build constraint.

### Step 3 validation and continuation


New production files are src/network/rsi/rsiprotocol.h and rsiprotocol.cpp. The
protocol defines plain Response/RsiTxFrame values, optional decoding failure,
encoding and replyForDatagram. RsiDevice::onReadyRead passes makeTxFrame as the
callback; that remains the sole path to tickMotion for incoming datagrams. Sender
validation and full XML decoding must finish before this callback can run. The
configured address is never overwritten by a datagram. No pathgeneration source
was inspected or changed, and no equipment was contacted.

The decoder requires Rob and exactly one IPOC containing a decimal quint64, with
zero and leading zeros accepted. Optional RIst requires six finite XYZABC values;
optional AIPos/MACur require finite A1–A6. Duplicate known records, missing fields,
nonfinite numbers, overflow, malformed/truncated XML and trailing content fail.
Unknown XML extensions are skipped while still checking document well-formedness.
The encoder was relocated unchanged, retaining C-locale ten-significant-digit
Cartesian corrections, unpadded IPOC, and no stop field. See tests/network/README.md
for the recorded example discrepancy and supported protocol contract.

The new tests/network/rsiprotocoltests.cpp reads fromKRC.xml, verifies optional pose
and XYZABC ordering, tests malformed/invalid packets and exact output formatting,
and exercises the same receive gate as the device with an offset-consuming callback.
Every invalid packet must leave that callback untouched and the learned port zero.
Foreign initial/established senders and changed ports must preserve offset position;
a later accepted packet receives the next unconsumed offset. Repeated IPOCs remain
accepted. This verifies the production gate, not the socket event loop or trajectory
algorithm; those integration checks are not claimed as exercised.

After explicit build authorization, select the matching kit/build as described by
AGENTS.md. For an existing valid hot-reload-OFF build, from the repository root:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
    cmake --build "$buildDir" --target rsiprotocoltests robocrap --parallel
    & "$buildDir/rsiprotocoltests.exe"

Expect RSI protocol regressions passed and exit zero, with Qt/MinGW runtime DLLs
on PATH. Adjust paths for multi-configuration generators. Building robocrap checks
the device integration; do not run application main as an offline check. No configure,
build, lint, executable or hardware operation was run in this step. Source inspection
and diff/whitespace checks are completed; runtime acceptance remains pending.
Stop after step 3; step 4 requires a new request.

Revision 2026-09-23 (step 3): Extracted RSI protocol handling, placed validation before
peer learning/motion advancement, documented example discrepancies, and added offline
receive-gate regressions. Preserved the no-build constraint and earlier user edits.

### Step 4 validation and continuation


Changed src/network/plc/plcmessagemanager.cpp to preserve decoded dev values, including
WRITE_IO, so PlcDevice can match them before publishing y1/y2. PlcDevice now stores
validated request maps in its existing pending member. availableTid searches 256 ids
from the prior cursor and rejects exhaustion; occupied ids cannot be overwritten.
matchResponse checks tid/cmd and available echoed fields: IO dev/module, register
D/address and WRITE_REG value, SET_VAR var/attr. Matching error replies retire their
request without publishing; mismatches leave it pending. SNAPSHOT/WRITE_RAW and error
replies expose no further verified correlation fields. CHG remains unsolicited.

Private processIncoming(bytes) contains the existing framing loop, called directly
by onReadyRead and the friend PlcProtocolTests harness. Tests exercise actual device
state publication, not a duplicate matcher. The test opens only an unconnected socket
for failed-write cleanup, never a network connection. It supplies response fragments
and coalesced frames directly. Explicit disconnect clears pending/buffer state even
when already unconnected; remote disconnect and stop cleanup remain in place.

Failed or short writes remove their pending entry. Positive short writes also abort
the connection because another write cannot repair a partially queued protocol frame.
This guard has source review only; positive short writes are not simulated. No expiry
or retry is added. Unanswered requests can exhaust the pool until replies/disconnect;
a supported timeout policy remains a documented follow-up. Identical delayed replies
after legitimate id reuse cannot be distinguished by the existing wire format.

After explicit authorization and cache/kit selection per AGENTS.md, use an existing
valid hot-reload-OFF build from the repository root:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
    cmake --build "$buildDir" --target plcprotocoltests robocrap --parallel
    & "$buildDir/plcprotocoltests.exe"

Expect PLC protocol regressions passed and exit zero. The failed unconnected write
may emit a Qt socket warning. tests/network/README.md records full coverage and runtime
requirements. Building robocrap is required to validate device integration; do not run
its hardware-initializing main as an offline test. Source review and whitespace/diff
checks are complete; configure/build/test execution remain pending. Earlier steps and
unrelated CMakeLists.txt.user edits are preserved. Stop here; step 5 needs a new request.

Revision 2026-09-23 (step 4): Repaired decoded device identity and pending-request
matching/allocation, added production-path offline PLC regressions, and explicitly
deferred timeout selection. Maintained the user's no-build constraint.
