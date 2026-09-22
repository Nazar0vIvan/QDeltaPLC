import QtQuick
import QtTest
import "../../qml/Models"
import "../../qml/Views/Workspace"

TestCase {
  id: testCase

  name: "SceneObjects"
  when: windowShown
  visible: true
  width: 700
  height: 440

  property list<string> selectedIds: ["plane"]
  readonly property SceneObject selectedObject: testCase.selectedIds.length === 1
                                                ? scene.findObject(testCase.selectedIds[0]) : null
  // A second consumer of the same instance, independent of PropertiesPanel.
  readonly property string browserName: plane.name
  readonly property bool browserVisible: plane.visible

  SceneModel {
    id: scene
    SceneObject { id: plane; objectId: "plane"; name: "Plane P1"; kind: SceneObject.Plane; classification: SceneObject.Rough }
    SceneObject { id: cylinder; objectId: "cylinder"; name: "Cylinder C1"; kind: SceneObject.Cylinder; classification: SceneObject.Precise }
  }

  SceneObject { id: foreign; objectId: "plane"; name: "Foreign"; kind: SceneObject.Plane }

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
    scene.objects = [plane, cylinder]
    scene.renameObject(plane, "Plane P1")
    scene.renameObject(cylinder, "Cylinder C1")
    scene.setObjectVisible(plane, true)
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

  function test_validationAndForeignObjects() {
    verify(!scene.renameObject(plane, "   "))
    compare(plane.name, "Plane P1")
    verify(!scene.renameObject(foreign, "Wrong object"))
    verify(!scene.setObjectVisible(foreign, false))
    verify(!scene.renameObject(null, "Missing"))
    compare(scene.findObject("unknown"), null)
  }

  function test_selectionSurvivesRenameAndReorder() {
    verify(scene.renameObject(plane, "Other name"))
    scene.objects = [cylinder, plane]
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
    compare(imported.kind, SceneObject.Plane)
    compare(imported.classification, SceneObject.Rough)
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

    scene.objects = [plane, cylinder]
    imported.destroy()
    repeated.destroy()
  }

  function test_invalidPlaneResultDoesNotChangeScene() {
    compare(scene.addPlane("file:///bad.json", "Bad", [1, 2, 3]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 0, 0]), null)
    compare(scene.addPlane("file:///bad.json", "Bad", [0, 0, 1, NaN]), null)
    compare(scene.addPlane("", "Bad", [0, 0, 1, 0]), null)
    compare(scene.objects.length, 2)
    compare(testCase.selectedObject, plane)
  }
}
