import QtQuick
import QtTest
import RoboCrap.Backend 1.0 as Backend
import "../../qml/Views/Workspace"

TestCase {
  id: testCase

  name: "SceneObjects"
  when: windowShown
  visible: true
  width: 700
  height: 440

  property list<string> selectedIds: []
  readonly property Backend.SceneObject selectedObject: testCase.scene && testCase.selectedIds.length === 1
                                                        ? testCase.scene.findObject(testCase.selectedIds[0]) : null
  // A second consumer of the same instance, independent of PropertiesPanel.
  readonly property string browserName: testCase.plane ? testCase.plane.name : ""
  readonly property bool browserVisible: testCase.plane ? testCase.plane.visible : false

  // Exercise the separate coefficient-only insertion API.
  signal planeLoaded(url sourceUrl, string name, list<var> coefficients)
  property Backend.SceneObject importedResult: null
  onPlaneLoaded: (sourceUrl, name, coefficients) => {
    testCase.importedResult = scene.addPlane(sourceUrl, name, coefficients)
  }

  Backend.SceneModel { id: emptyScene }
  Component { id: sceneFactory; Backend.SceneModel {} }
  property Backend.SceneModel scene: emptyScene
  property Backend.SceneObject plane: null
  property Backend.SceneObject cylinder: null
  property Backend.SceneObject foreign: null

  Backend.PlaneImporter {
    id: importer
    onLoaded: object => testCase.importedResult = object
  }
  SignalSpy { id: loadedSpy; target: importer; signalName: "loaded" }
  SignalSpy { id: failedSpy; target: importer; signalName: "failed" }

  PropertiesPanel {
    id: panel
    width: 272
    height: 400
    selectedObject: testCase.selectedObject
    selectionCount: testCase.selectedIds.length
    onRenameRequested: (object, name) => scene.renameObject(object, name)
    onVisibilityRequested: (object, visible) => scene.setObjectVisible(object, visible)
  }

  ScenePanel {
    id: browser
    x: 300
    width: 265
    height: 400
    sceneModel: scene
    selectedObjectIds: testCase.selectedIds
    onRenameRequested: (object, name) => scene.renameObject(object, name)
    onVisibilityRequested: (object, visible) => scene.setObjectVisible(object, visible)
  }

  function editor() {
    return findChild(panel, "objectNameEditor")
  }

  function init() {
    failOnWarning(/TypeError|ReferenceError|invalid nullptr|coerced to void/)
    editor().modified = false
    editor().focus = false
    testCase.scene = createTemporaryObject(sceneFactory, testCase)
    verify(testCase.scene !== null)
    testCase.plane = scene.addObject("plane", "Plane P1", Backend.SceneObject.Plane, Backend.SceneObject.Rough)
    testCase.cylinder = scene.addObject("cylinder", "Cylinder C1", Backend.SceneObject.Cylinder, Backend.SceneObject.Precise)
    const otherScene = createTemporaryObject(sceneFactory, testCase)
    testCase.foreign = otherScene.addObject("plane", "Foreign", Backend.SceneObject.Plane, Backend.SceneObject.Unclassified)
    testCase.selectedIds = ["plane"]
    panel.width = 272
    waitForPolish(panel)
  }

  function test_sharedIdentityAndNotifications() {
    compare(panel.selectedObject, plane)
    compare(findChild(browser, "sceneObject-plane").modelData, plane)
    compare(findChild(browser, "sceneObject-cylinder").modelData, cylinder)
    compare(browser.objectsForGroup("rough")[0], plane)
    compare(browser.objectsForGroup("precise")[0], cylinder)
    verify(scene.renameObject(plane, "  Renamed plane  "))
    compare(testCase.browserName, "Renamed plane")
    compare(editor().text, "Renamed plane")
    verify(scene.setObjectVisible(plane, false))
    compare(testCase.browserVisible, false)
    compare(findChild(panel, "objectVisibilityCheckBox").checked, false)
  }

  function cleanup() {
    editor().modified = false
    editor().focus = false
    testCase.selectedIds = []
    testCase.scene = emptyScene
    testCase.plane = null
    testCase.cylinder = null
    testCase.foreign = null
    testCase.importedResult = null
    // Qt Quick Test destroys each temporary model and its C++-owned children.
  }

  function test_validationAndForeignObjects() {
    verify(!scene.renameObject(plane, "   "))
    compare(plane.name, "Plane P1")
    verify(!scene.renameObject(foreign, "Wrong object"))
    verify(!scene.setObjectVisible(foreign, false))
    verify(!scene.renameObject(null, "Missing"))
    compare(scene.findObject("unknown"), null)
  }

  function test_selectionSurvivesRenameAndInsertion() {
    verify(scene.renameObject(plane, "Other name"))
    verify(scene.addObject("edge", "Edge", Backend.SceneObject.Edge, Backend.SceneObject.Unclassified) !== null)
    compare(testCase.selectedObject, plane)
    compare(panel.selectedObject.objectId, "plane")
    testCase.selectedIds = ["plane", "cylinder"]
    compare(panel.selectedObject, null)
    compare(panel.selectionCount, 2)
    testCase.selectedIds = []
    compare(panel.selectedObject, null)
    compare(panel.selectionCount, 0)
  }

  function test_renameThroughEditor() {
    editor().forceActiveFocus()
    keyClick(Qt.Key_A, Qt.ControlModifier)
    keyClick(Qt.Key_X)
    keyClick(Qt.Key_Return)
    compare(plane.name, "x")
    compare(testCase.browserName, "x")
    editor().forceActiveFocus()
    keyClick(Qt.Key_A, Qt.ControlModifier)
    keyClick(Qt.Key_Z)
    keyClick(Qt.Key_Escape)
    compare(plane.name, "x")
    compare(editor().text, "x")
  }

  function test_pendingRenameTargetsOriginalObject() {
    editor().forceActiveFocus()
    keyClick(Qt.Key_A, Qt.ControlModifier)
    keyClick(Qt.Key_X)
    testCase.selectedIds = ["cylinder"]
    compare(plane.name, "x")
    compare(cylinder.name, "Cylinder C1")
    compare(editor().text, "Cylinder C1")
  }

  function test_visibilityThroughProperties() {
    const checkbox = findChild(panel, "objectVisibilityCheckBox")
    waitForPolish(panel)
    mouseClick(checkbox)
    compare(plane.visible, false)
    compare(testCase.browserVisible, false)
    compare(checkbox.checked, false)
  }

  function test_editsThroughSceneRow() {
    const row = findChild(browser, "sceneObject-plane")
    row.beginRename()
    keyClick(Qt.Key_A, Qt.ControlModifier)
    keyClick(Qt.Key_Y)
    keyClick(Qt.Key_Return)
    compare(plane.name, "y")
    compare(editor().text, "y")
    waitForPolish(browser)
    mouseClick(findChild(row, "sceneVisibilityButton"))
    compare(plane.visible, false)
    compare(findChild(panel, "objectVisibilityCheckBox").checked, false)
    compare(testCase.selectedIds.length, 1)
    compare(testCase.selectedIds[0], "plane")
  }

  function test_importedPlaneSharedByBrowserAndProperties() {
    const scale = Math.sqrt(21)
    const coefficients = [-2 / scale, 1 / scale, 4 / scale, -40 / scale]
    const source = "qrc:/json/rough-plane-sample.json"
    const imported = scene.addPlane(source, "Sample plane", coefficients)
    verify(imported !== null)
    compare(scene.objects.length, 3)
    compare(imported.kind, Backend.SceneObject.Plane)
    compare(imported.classification, Backend.SceneObject.Rough)
    compare(imported.sourceUrl.toString(), source)
    compare(imported.geometry.normalX, coefficients[0])
    compare(imported.geometry.normalY, coefficients[1])
    compare(imported.geometry.normalZ, coefficients[2])
    compare(imported.geometry.offset, coefficients[3])

    testCase.selectedIds = [imported.objectId]
    waitForPolish(panel)
    compare(panel.selectedObject, imported)
    compare(panel.planeGeometry, imported.geometry)
    compare(findChild(panel, "objectSourceField").text, source)
    compare(findChild(panel, "planeOffsetValue").text, coefficients[3].toPrecision(6))
    compare(findChild(browser, "sceneObject-" + imported.objectId).modelData, imported)
    panel.width = 220
    waitForPolish(panel)
    const offsetLabel = findChild(panel, "planeOffsetValue")
    verify(offsetLabel.width > 0)
    verify(offsetLabel.mapToItem(panel, 0, offsetLabel.height).y <= panel.height)
    verify(scene.renameObject(imported, "Renamed import"))
    compare(editor().text, "Renamed import")
    verify(scene.setObjectVisible(imported, false))
    compare(findChild(panel, "objectVisibilityCheckBox").checked, false)

    const repeated = scene.addPlane(source, "Sample plane", coefficients)
    verify(repeated !== null && repeated !== imported)
    verify(repeated.objectId !== imported.objectId)
    verify(repeated.geometry !== imported.geometry)
    compare(panel.selectedObject, imported)
    testCase.selectedIds = ["cylinder"]
    compare(panel.planeGeometry, null)
    compare(findChild(panel, "planeOffsetValue").visible, false)

  }

  function test_invalidPlaneResultDoesNotChangeScene() {
    compare(scene.addPlane("file:///bad.json", "Bad", [1, 2, 3]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 0, 0]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 1, NaN]), null)
    compare(scene.addPlane("", "Bad", [0, 0, 1, 0]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 1, Infinity]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 1, "0"]), null)
    compare(scene.objects.length, 2)
    compare(testCase.selectedObject, plane)
  }

  function test_duplicateIdentityRejected() {
    compare(scene.addObject("plane", "Duplicate", Backend.SceneObject.Plane, Backend.SceneObject.Rough), null)
    compare(scene.objects.length, 2)
    compare(scene.findObject("plane"), plane)
  }

  function test_importSkipsExistingIdentity() {
    verify(scene.addObject("imported-plane-1", "Existing", Backend.SceneObject.Edge,
                           Backend.SceneObject.Unclassified) !== null)
    const imported = scene.addPlane("file:///plane.json", "  ", [0, 0, 1, 0])
    verify(imported !== null)
    compare(imported.objectId, "imported-plane-2")
    verify(imported.name.length > 0)
    compare(scene.findObject(imported.objectId), imported)
  }

  function test_importerVariantListBoundary() {
    const scale = Math.sqrt(21)
    const coefficients = [-2 / scale, 1 / scale, 4 / scale, -40 / scale]
    testCase.planeLoaded("file:///rough-plane-sample.json", "Sample plane", coefficients)
    const imported = testCase.importedResult
    verify(imported !== null)
    compare(scene.objects.length, 3)
    compare(imported.geometry.normalX, coefficients[0])
    compare(imported.geometry.normalY, coefficients[1])
    compare(imported.geometry.normalZ, coefficients[2])
    compare(imported.geometry.offset, coefficients[3])
    testCase.importedResult = null
  }

  function test_failedImportLeavesSceneUnchanged() {
    loadedSpy.clear()
    failedSpy.clear()
    importer.load(Qt.resolvedUrl("missing-plane-file.json"), testCase.scene)
    tryCompare(importer, "busy", false, 10000)
    compare(loadedSpy.count, 0)
    compare(failedSpy.count, 1)
    compare(scene.objects.length, 2)
    compare(testCase.importedResult, null)
  }

  function test_missingDestinationRejected() {
    loadedSpy.clear()
    failedSpy.clear()
    importer.load(Qt.resolvedUrl("../../resources/json/rough-plane-sample.json"), null)
    compare(importer.busy, false)
    compare(loadedSpy.count, 0)
    compare(failedSpy.count, 1)
    compare(scene.objects.length, 2)
  }

  function test_sampleJsonThroughCppImporter() {
    loadedSpy.clear()
    failedSpy.clear()
    importer.load(Qt.resolvedUrl("../../resources/json/rough-plane-sample.json"), testCase.scene)
    tryCompare(importer, "busy", false, 10000)
    compare(failedSpy.count, 0)
    compare(loadedSpy.count, 1)
    const imported = testCase.importedResult
    verify(imported !== null)
    compare(scene.objects.length, 3)
    const scale = Math.sqrt(21)
    verify(Math.abs(imported.geometry.normalX + 2 / scale) < 1e-10)
    verify(Math.abs(imported.geometry.normalY - 1 / scale) < 1e-10)
    verify(Math.abs(imported.geometry.normalZ - 4 / scale) < 1e-10)
    verify(Math.abs(imported.geometry.offset + 40 / scale) < 1e-10)
    testCase.selectedIds = [imported.objectId]
    waitForPolish(panel)
    compare(panel.selectedObject, imported)
    compare(findChild(browser, "sceneObject-" + imported.objectId).modelData, imported)
    verify(imported.geometry.hasBounds)
    compare(imported.geometry.pointCount, 5)
    verify(Math.abs(imported.geometry.originX) < 1e-10)
    verify(Math.abs(imported.geometry.originY) < 1e-10)
    verify(Math.abs(imported.geometry.originZ - 10) < 1e-10)
    verify(imported.geometry.width > 0 && imported.geometry.height > 0)
    compare(findChild(panel, "planePointCountValue").text, "5")
    compare(findChild(panel, "planeWidthValue").text, imported.geometry.width.toPrecision(6))
    compare(findChild(panel, "planeHeightValue").text, imported.geometry.height.toPrecision(6))
    const firstGeometry = imported.geometry
    loadedSpy.clear()
    importer.load(Qt.resolvedUrl("../../resources/json/rough-plane-sample.json"), testCase.scene)
    tryCompare(importer, "busy", false, 10000)
    compare(loadedSpy.count, 1)
    compare(imported.geometry, firstGeometry)
    compare(firstGeometry.pointCount, 5)
    verify(testCase.importedResult.objectId !== imported.objectId)
    compare(firstGeometry.width, testCase.importedResult.geometry.width)
  }
}
