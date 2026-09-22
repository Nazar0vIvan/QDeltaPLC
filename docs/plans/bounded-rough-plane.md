# Define bounded imported rough planes

This living ExecPlan follows PLANS.md. No build or git commit is authorized.

## Purpose / Big Picture

Importing a point JSON will retain its samples and expose a finite plane rectangle: center origin, unit normal, two tangent axes, width, height and point count. Properties displays these read-only. CAD drawing remains outside this step.

## Progress

- [x] (2026-09-22) Inspect fitter, importer, scene ownership, Properties and base controls, and test callers.
- [x] (2026-09-22) Implement bounded geometry and immutable worker result transfer, retaining coefficient-only APIs.
- [x] (2026-09-22) Display bounded fields in scrollable Properties and extend offline regressions.
- [x] (2026-09-22) Review source, diff and sample reference calculations; executable tests deferred without build authorization.

## Context and Orientation

src/geometry/plane.cpp fits z as a function of x and y, emitting a normalized implicit equation. src/geometry/planeimporter.cpp does this work in a QThread, then emits coefficients on the GUI thread. src/scene/PlaneGeometry exposes immutable data owned by a SceneObject, whose owner is SceneModel. qml/Main.qml forwards import results. PropertiesPanel uses QxPanel (a Control with a ColumnLayout default content) and QxHField (a labeled RowLayout). Existing edits from scene ownership and user checkbox changes must remain.

## Decision Log

Decision (2026-09-22): use a center origin and tangent U obtained by projecting world X, falling back to world Y when the normal is almost parallel to X. V = normal cross U. Bounds are extrema of orthogonal projections in this frame, not a minimum-area rectangle. This is deterministic, independent of point order apart from numerical rounding, and suitable for future rendering.

Decision (2026-09-22): keep existing Plane fitting and coefficient-only APIs; introduce plain bounded geometry data and an immutable shared result on PlaneGeometry. Importer exposes its last successful geometry via a result property before its existing loaded signal. SceneModel clones the presentation wrapper while sharing immutable numerical data. Workers never create QObjects or touch the scene. Existing coefficient-only addPlane remains explicitly unbounded.

## Surprises & Discoveries

The Properties layout has no scrolling; additional geometry fields require a ScrollView so narrow/short panels remain usable. The existing fitter does not support vertical planes and uses global normal equations. This task preserves that fitter rather than changing numerical fitting behavior.

## Plan of Work

First add geometry/boundedplane.h/.cpp with immutable-by-ownership values, original point samples, frame and bounds. Extract shared JSON reading into geometry/utils so the old Plane::fromJsonFile and bounded importer parse identically. Bounds and fitting are computed on the import worker. Next extend scene/planegeometry.h, importer result exposure and SceneModel::addBoundedPlane; switch Main to that method. Finally wrap Properties content in a vertical ScrollView, display origin/dimensions/count, and extend tests with numerical containment, frame and input retention checks.

## Concrete Steps

Work from E:/Qt/QtProjects/RoboCrap. Run git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check and inspect all new files separately for whitespace and content. Do not configure or compile unless explicitly requested. Once authorized, enable ROBOCRAP_BUILD_SCENE_TESTS in a verified matching build, build sceneobjecttests and boundedplanetests, and run both; see tests/qml/README.md. Also build robocrap and its QML lint target with hot reload OFF.

## Validation and Acceptance

The sample has five points on z = 0.5x - 0.25y + 10. Expect center (0,0,10), unit perpendicular right-handed axes, positive dimensions and five unchanged samples. All projected samples must lie inside half-width/half-height around the center. Tests cover noisy points, horizontal planes, reordered samples, invalid/collinear inputs, GUI fields, and repeated imports surviving importer result replacement. Runtime/build tests require new C++ compilation and remain deferred without build authorization.

## Idempotence and Recovery

Preserve stable IDs, sample rows, previous edits and module URIs. No hardware or generated build outputs are touched. Import failure must not insert a partial object. Shared immutable data keeps imported geometry alive after the importer replaces or destroys its result.

## Interfaces and Dependencies

BoundedPlane is a non-QObject data value using arrays of doubles and a vector of original samples, with fromPoints and fromJsonFile factory functions returning optional results. Public scene headers do not expose Eigen. PlaneGeometry retains existing coefficients and adds hasBounds, originX/Y/Z, axisU/axisV lists, width, height, pointCount, and a C++ boundedPlane accessor. PlaneImporter.result is a read-only PlaneGeometry pointer notified on replacement. SceneModel.addBoundedPlane accepts that result and gives the scene its own wrapper. Existing dependencies suffice; optional numerical tests link the backend.

## Outcomes & Retrospective

Implemented the bounded numerical value, shared JSON parser, worker-side construction and GUI-owned result wrapper. Scene imports retain a separate wrapper and shared immutable samples. Properties now scrolls and displays origin, dimensions and count for bounded planes. Added C++ numerical and QML reimport regression coverage. Source/diff checks and an independent Python calculation of sample extents were performed. No C++ build, QML lint, numerical test executable or GUI run was performed; these require build verification authorization. CAD rendering remains unimplemented.

## Artifacts and Notes

Rectangle corners are origin +/- width/2 * U +/- height/2 * V. Coordinates and dimensions use the input units (the application convention is millimeters). Raw samples are retained; bounds describe their projections.

Revision: initial scope and conventions recorded before implementation.

Revision: completed source implementation and regression sources. Independent sample calculation yielded width 222.28757209048453 and height 189.1777875283397 in the documented frame. Build-backed verification remains pending.
