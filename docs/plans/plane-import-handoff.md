# Simplify plane ownership and import handoff

This living plan follows PLANS.md. Builds and commits are not authorized.

## Purpose / Big Picture

A bounded import has one authoritative coefficient set in BoundedPlane and one scene-owned PlaneGeometry wrapper. QML requests import into a scene and receives a SceneObject, without transferring geometry or creating scene entries itself.

## Progress

- [x] (2026-09-22) Read producer, scene ownership, QML caller and regression fixtures.
- [x] (2026-09-22) Remove redundant coefficient storage and intermediate result wrapper.
- [x] (2026-09-22) Migrate callers, documentation and regression cases; inspect diffs.

## Context and Orientation

src/geometry/planeimporter.cpp computes an immutable BoundedPlane on a worker, but currently wraps it for QML before SceneModel wraps it again. src/scene/planegeometry.h copies bounded coefficients. Main.qml and tests forward importer.result to addBoundedPlane. SceneModel already owns all scene objects on the GUI thread.

## Decision Log

Decision (2026-09-22): retain coefficient-only addPlane compatibility, storing its coefficients in an optional value that is empty for bounded geometry. Bounded getters read the shared BoundedPlane directly.

Decision (2026-09-22): change PlaneImporter.load to accept the destination SceneModel and emit loaded(SceneObject*). Capture a QPointer to the destination for the duration of the job. The worker retains only numerical data; insertion happens in its GUI-thread completion callback. Remove the QML result property and make addBoundedPlane a C++ method accepting immutable numerical data. This implements the explicitly requested handoff change without a new service class.

## Plan of Work

First change PlaneGeometry's coefficient storage and getters. Then change importer completion to insert into the captured model, reporting failure if it has been destroyed. Keep busy true through insertion and completion notifications to prevent overlapping reentrant loads. Update Main and tests to pass the model and consume the created object. Retain numerical fitting and OCCT code unchanged.

## Concrete Steps

From E:/Qt/QtProjects/RoboCrap run git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check and search all callers of load, loaded and addBoundedPlane. Check new files separately. When build verification is authorized, follow tests/qml/README.md to build and run sceneobjecttests and boundedplanetests and build robocrap plus its QML lint target.

## Validation and Acceptance

Source checks verify a single wrapper allocation per import, no geometry transfer through QML, immutable shared data, and destination lifetime checking. Regression sources should check valid/repeated imports, failure without scene mutation, and destination destruction during import. No compiled test results can be claimed without a requested build.

## Idempotence and Recovery

Preserve existing user edits, IDs, ownership and geometry values. Do not touch hardware or build outputs. Failed import and destroyed destinations must not create partial objects.

## Interfaces and Dependencies

PlaneImporter.load(QUrl, SceneModel*) replaces the one-argument entry point; loaded(SceneObject*) replaces coefficient result notification. SceneModel.addBoundedPlane(QUrl, QString, shared_ptr<const BoundedPlane>) becomes C++ only. Existing Qt Core/Qml dependencies suffice.

## Surprises & Discoveries

The current source exposes busy=false before result insertion. This cleanup keeps it true until completion delivery so signal handlers cannot start a second request during the first insertion.

## Outcomes & Retrospective

Implemented single authoritative bounded coefficient storage, one scene-owned wrapper per import, and direct C++ insertion. Updated Main and test callers for load(url, scene) and loaded(object), removing the result property and QML bounded insertion API. Added failed/null-destination cases and a deterministic C++ destruction-before-completion check plus coefficient-reference identity checks. Source and whitespace checks performed; executable tests/build/lint not run because build verification was not requested. No rendering changes are included.

## Artifacts and Notes

The coefficient-only insertion API remains available for existing fixtures. It is separate from the real bounded JSON import flow.

Revision: initial design and verification constraints recorded.

Revision: implementation completed and regressions updated. Destination destruction is checked synchronously in C++ to avoid a race between QML deferred deletion and worker completion.
