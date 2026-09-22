# Move scene ownership into C++

This living ExecPlan follows PLANS.md. Git commits are not authorized. Build commands are not authorized by the current request.

## Purpose / Big Picture

Imported planes will be owned by application C++ objects, while Scene and Properties continue to share the same observable object and stable ID. Import, rename, visibility, selection, and sample rows must behave as before. Geometry fitting stays in src/geometry. CAD rendering, bounds, trajectories, and UI redesign are outside this change.

## Progress

- [x] (2026-09-22) Read current models, import producer, panel consumers, registration, application lifetime, and build lists.
- [x] (2026-09-22) Implement C++ objects and read-only collection with application lifetime and immutable IDs.
- [x] (2026-09-22) Migrate QML consumers and tests; remove obsolete QML model definitions and packaging entries.
- [x] (2026-09-22) Review source contracts, installed Qt 6.8.3 list-property constructor, and diff; document the optional test runner.
- [x] (2026-09-22) Record that build, lint and executable regression tests remain unrun because build verification was not requested.

## Surprises & Discoveries

The current tests mutate the public object list and destroy individual QML-created objects. These operations must be replaced by fresh model fixtures because ownership will belong to C++. Existing local edits include PropertiesPanel and QxCheckBox; preserve their presentation.

The backend is static. A stock qmltestrunner cannot exercise the migrated types on its own. An optional sceneobjecttests executable links the actual backend and embeds the icon resources; its source JSON import test exercises the real PlaneImporter rather than only a synthetic QML signal. The installed Qt 6.8.3 qqmllist.h confirms the count/at-only QQmlListProperty constructor used to keep collection mutations in C++.

## Decision Log

Decision: retain the objects list interface rather than introducing QAbstractListModel and changing browser grouping. The current browser consumes shared object pointers, so a read-only QQmlListProperty with collection notification supplies the needed interface without a presentation refactor. Date: 2026-09-22.

Decision: application main owns an ApplicationScene (a SceneModel subclass) exposed as Backend.Scene; SceneModel remains independently creatable for isolated tests. The subclass permits separate singleton and ordinary type registrations without registering one C++ type under competing names. Date: 2026-09-22.

Decision: expose immutable ID, kind, classification, source and geometry properties. Rename and visibility mutations go through the model and notify existing references. Keep addPlane and metadata insertion in C++; fit mathematics remain in PlaneImporter/Plane. Date: 2026-09-22.

Decision: add ROBOCRAP_BUILD_SCENE_TESTS defaulting to OFF and require QuickTest only within that option. This preserves normal build dependencies while providing a runner that can load the migrated static backend. No CTest registration is introduced. Date: 2026-09-22.

## Outcomes & Retrospective

Implemented the ownership migration and retained the nine sample rows, ID-based selection, panel edit signals and coefficient presentation. The application scene outlives the QML engine; each imported object owns an immutable coefficient presentation. No geometry fitting, hardware, viewport, workflow or layout behavior was added. Reviewed source and packaging references; executable verification is still required once a build is requested. Public QML creation of SceneObject/PlaneGeometry and direct list/property assignment are intentionally replaced by SceneModel methods so UI code cannot bypass ownership.

## Context and Orientation

qml/Main.qml currently creates qml/Models/SceneModel.qml and sample SceneObject instances. PlaneImporter emits a QVariantList of normalized coefficients. ScenePanel filters the objects list into fixed UI groups; PropertiesPanel reads the selected pointer and casts its geometry to PlaneGeometry. src/network/backendqmltypes.h exposes foreign C++ types through RoboCrap.Backend, with src/CMakeLists.txt listing source inputs. src/main.cpp creates application services before the QML engine so services outlive the UI.

## Plan of Work

Create src/scene/planegeometry.h, sceneobject.h/.cpp, scenemodel.h/.cpp, and applicationscene.h. PlaneGeometry exposes immutable coefficients, SceneObject owns its geometry, and SceneModel owns every object. Add the sources and foreign registrations. main.cpp creates the application scene and seeds the existing nine sample rows. Replace directory imports with RoboCrap.Backend and replace Main's declarative model with a reference to Backend.Scene. Remove the three obsolete QML definitions and their root CMake entries. Migrate tests to temporary C++ model instances and retain panel interaction checks.

## Concrete Steps

Run source inspection commands from E:/Qt/QtProjects/RoboCrap. After edits run:

    git -c safe.directory=E:/Qt/QtProjects/RoboCrap diff --check
    git -c safe.directory=E:/Qt/QtProjects/RoboCrap status --short
    rg -n 'qml/Models|../../Models|Array.from\(coefficients\)' qml src tests CMakeLists.txt

The last search should find no stale model import or conversion workaround. Inspect new untracked source files separately. Do not compile merely to run tests; the current request does not authorize build verification.

## Validation and Acceptance

Source review must verify immutable identity, QObject parenting, explicit C++ ownership, collection notifications, per-object rename/visibility notifications, foreign-pointer rejection, unique repeated import IDs, and QVariantList acceptance without JS conversion. The migrated Qt Quick tests exercise shared identity, edits, invalid inputs and repeated import. Once build verification is requested, use the matching Qt 6.8+ MinGW kit, build robocrap and robocrap_qmllint with hot reload OFF, and run the dedicated test runner described with the tests. In a disconnected application, import resources/json/rough-plane-sample.json and verify a new selected Scene row and coefficient values in Properties, then rename/hide it. No plane is expected in CAD yet.

## Idempotence and Recovery

Only source files are changed. Do not discard pre-existing user edits, stage, or commit. Temporary test models own and destroy their own objects; the application scene persists across QML component recreation. Preserve module URIs and minimum Qt/CMake versions.

## Artifacts and Notes

The original importer emits normalized [nx, ny, nz, d]. Its worker does not access the scene; results are inserted on the GUI thread. No fit algorithm changes are needed.

## Interfaces and Dependencies

Use Qt Core and Qml already linked by RoboCrapBackend. SceneModel exposes objects, findObject(QString), addPlane(QUrl, QString, QVariantList), addObject(QString, QString, SceneObject::Kind, SceneObject::Classification), renameObject(SceneObject*, QString), and setObjectVisible(SceneObject*, bool). addObject creates metadata-only placeholders; addPlane validates finite normalized numeric coefficients and creates owned PlaneGeometry. No Eigen types appear in the public Qt presentation headers.

Revision: initial plan records scope, lifetime, API migration and verification constraints.

Revision: implementation complete; recorded the optional linked runner, actual importer regression, read-only API changes, and outstanding executable verification. See tests/qml/README.md for exact runner commands after build authorization.
