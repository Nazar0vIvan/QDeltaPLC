import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs as Dialogs
import RoboCrap.Backend 1.0 as Backend
import RoboCrap.Viewport3D 1.0 as Viewport3D

import "MenuBar"
import "Views/Dashboard/DeltaPanel"
import "Views/Network"
import "Views/Viewport3D"
import "Views/Workspace"

import Styles 1.0
import Components 1.0

ApplicationWindow {
  id: root

  property int workflowMode: WorkflowPanel.Measuring
  property int measurementSubmode: WorkflowPanel.Rough

  onWorkflowModeChanged: root.syncEndEffector()

  function syncEndEffector() {
    Backend.EndEffectors.setActiveTool(root.workflowMode === WorkflowPanel.Machining
                                      ? Backend.EndEffectors.Spindle : Backend.EndEffectors.Measuring)
  }

  // UI display preferences for the imported rough-surface presentations.
  property bool showPoints: false
  property bool showNormals: false
  property bool showScanPath: false
  property bool showMachiningPath: false
  property list<real> selectedObjectIds: []

  readonly property Backend.SceneObject selectedSceneObject: root.selectedObjectIds.length === 1
                                                            ? root.sceneModel.findObject(root.selectedObjectIds[0]) : null

  function selectSceneObject(objectId, additive) {
    if (!additive) {
      root.selectedObjectIds = [objectId]
      return
    }
    const selected = root.selectedObjectIds.slice()
    const index = selected.indexOf(objectId)
    if (index >= 0)
      selected.splice(index, 1)
    else
      selected.push(objectId)
    root.selectedObjectIds = selected
  }

  function deleteSelectedObjects() {
    const ids = root.selectedObjectIds.slice()
    root.selectedObjectIds = []
    root.sceneModel.removeObjects(ids)
  }

  Shortcut {
    sequence: "Del"
    context: Qt.WindowShortcut
    autoRepeat: false
    enabled: root.selectedObjectIds.length > 0
             && !(root.activeFocusItem instanceof TextInput)
             && !(root.activeFocusItem instanceof TextEdit)
    onActivated: root.deleteSelectedObjects()
  }

  function intersectSelected() {
    const createdIds = root.sceneModel.intersect(root.selectedObjectIds)
    if (createdIds.length > 0)
      root.selectedObjectIds = createdIds
  }

  function syncViewportSelection() {
    Viewport3D.OccController.setSelectedObjects(root.selectedObjectIds)
  }

  function syncViewportOverlays() {
    Viewport3D.OccController.setDiagnosticOverlays(root.showPoints, root.showNormals)
  }

  function setSceneObjectVisible(object, visible) {
    if (root.sceneModel.setObjectVisible(object, visible))
      root.syncViewportSelection()
  }

  onSelectedObjectIdsChanged: root.syncViewportSelection()
  onShowPointsChanged: root.syncViewportOverlays()
  onShowNormalsChanged: root.syncViewportOverlays()
  Component.onCompleted: {
    root.syncEndEffector()
    root.syncViewportSelection()
    root.syncViewportOverlays()
  }

  Connections {
    target: Viewport3D.OccController

    function onDeleteSelectionRequested() { root.deleteSelectedObjects() }

    function onViewportReadyChanged() {
      if (Viewport3D.OccController.viewportReady) {
        root.syncViewportSelection()
        root.syncViewportOverlays()
      }
    }

    function onApplicationSelectionRequested(objectId, additive) {
      if (objectId === 0)
        root.selectedObjectIds = []
      else
        root.selectSceneObject(objectId, additive)
    }
  }

  Backend.PlaneImporter {
    id: surfaceImporter

    onLoaded: object => root.selectedObjectIds = [object.objectId]
    onFailed: message => {
      importError.text = message
      importError.open()
    }
  }

  Dialogs.FileDialog {
    id: planeFileDialog
    title: qsTr("Import rough plane")
    fileMode: Dialogs.FileDialog.OpenFile
    nameFilters: [qsTr("JSON point files (*.json)")]
    onAccepted: surfaceImporter.load(planeFileDialog.selectedFile, root.sceneModel)
  }

  Dialogs.FileDialog {
    id: circleFileDialog
    title: qsTr("Import rough circle")
    fileMode: Dialogs.FileDialog.OpenFile
    nameFilters: [qsTr("JSON point files (*.json)")]
    onAccepted: surfaceImporter.loadCircle(circleFileDialog.selectedFile, root.sceneModel)
  }

  Dialogs.FileDialog {
    id: cylinderFileDialog
    title: qsTr("Import rough cylinder")
    fileMode: Dialogs.FileDialog.OpenFile
    nameFilters: [qsTr("JSON point files (*.json)")]
    onAccepted: surfaceImporter.loadCylinder(cylinderFileDialog.selectedFile, root.sceneModel)
  }

  Dialogs.MessageDialog {
    id: importError
    title: qsTr("Geometry import failed")
    buttons: Dialogs.MessageDialog.Ok
  }

  readonly property bool geometryImportAvailable: !surfaceImporter.busy && !planeFileDialog.visible
                                                 && !circleFileDialog.visible && !cylinderFileDialog.visible
  readonly property Backend.SceneModel sceneModel: Backend.Scene
  // Reading the collection also reevaluates eligibility after model insertions.
  readonly property bool intersectionAvailable: root.sceneModel.objects.length >= 2
                                                && root.sceneModel.canIntersect(root.selectedObjectIds)

  Connections {
    target: root.sceneModel
    function onObjectsChanged() {
      root.selectedObjectIds = root.selectedObjectIds.filter(id => root.sceneModel.findObject(id) !== null)
    }
    function onIntersectionFailed(message) {
      intersectionError.text = message
      intersectionError.open()
    }
  }

  Dialogs.MessageDialog {
    id: intersectionError
    title: qsTr("Surface intersection failed")
    buttons: Dialogs.MessageDialog.Ok
  }

  Dialogs.FileDialog {
    id: endEffectorFileDialog
    property bool measuring: true
    title: endEffectorFileDialog.measuring ? qsTr("Choose measuring end-effector CAD") : qsTr("Choose spindle end-effector CAD")
    fileMode: Dialogs.FileDialog.OpenFile
    nameFilters: [qsTr("STEP files (*.step *.stp *.STEP *.STP)")]
    onAccepted: {
      const currentSource = endEffectorFileDialog.measuring
                            ? Backend.EndEffectors.measuringCadSource : Backend.EndEffectors.spindleCadSource
      if (currentSource.toString() === endEffectorFileDialog.selectedFile.toString()) {
        Viewport3D.OccController.reloadEndEffector(endEffectorFileDialog.measuring
                                                 ? Backend.EndEffectors.Measuring : Backend.EndEffectors.Spindle)
      } else if (endEffectorFileDialog.measuring)
        Backend.EndEffectors.setMeasuringCadSource(endEffectorFileDialog.selectedFile)
      else
        Backend.EndEffectors.setSpindleCadSource(endEffectorFileDialog.selectedFile)
    }
  }

  width: 1366
  height: 768
  minimumWidth: 960
  minimumHeight: 600
  visible: true
  title: qsTr("RoboCrap")
  color: Colors.background.dp00

  menuBar: MainMenuBar {
    id: mainMenuBar
    onQuitRequested: Qt.quit()
    onSettingsRequested: {
      settingsWindow.show()
      settingsWindow.raise()
      settingsWindow.requestActivate()
    }
    onPlcPanelRequested: {
      plcPanelWindow.show()
      plcPanelWindow.raise()
      plcPanelWindow.requestActivate()
    }
  }

  Window {
    id: plcPanelWindow

    title: qsTr("PLC Panel")
    transientParent: root
    flags: Qt.Tool
    color: Colors.background.dp00
    width: minimumWidth
    height: minimumHeight
    minimumWidth: plcPanel.implicitWidth + 2 * Metrics.sp16
    minimumHeight: plcPanel.implicitHeight + 2 * Metrics.sp16
    maximumWidth: minimumWidth
    maximumHeight: minimumHeight

    DeltaPanel {
      id: plcPanel

      anchors.fill: parent
      anchors.margins: Metrics.sp16
      title: qsTr("PLC AS332T-A")
    }
  }

  Window {
    id: settingsWindow

    title: qsTr("Settings")
    transientParent: root
    flags: Qt.Tool
    color: Colors.background.dp00
    width: Math.ceil(networkSettings.contentWidth)
    height: Math.ceil(networkSettings.contentHeight)
    minimumWidth: 640
    minimumHeight: 360

    Network {
      id: networkSettings

      anchors.fill: parent
    }
  }

  footer: ToolBar {
    padding: Metrics.sp8

    background: Rectangle {
      color: Colors.background.dp01
      border.color: Colors.background.dp12
      border.width: Metrics.w1
    }

    contentItem: Label {
      text: surfaceImporter.busy ? qsTr("Importing surface…")
            : root.workflowMode === WorkflowPanel.Machining
            ? qsTr("Mode: Machining")
            : qsTr("Mode: Measuring / %1").arg(
                root.measurementSubmode === WorkflowPanel.Rough
                ? qsTr("Rough") : qsTr("Precise"))
      color: Colors.foreground.medium
      font: Fonts.body
    }
  }

  SplitView {
    id: workspace

    anchors.fill: parent
    anchors.margins: Metrics.sp4
    orientation: Qt.Horizontal

    handle: Rectangle {
      implicitWidth: Metrics.sp4
      color: SplitHandle.pressed ? Colors.primary.base
             : SplitHandle.hovered ? Colors.background.dp12
             : Colors.background.dp00
    }

    ColumnLayout {
      SplitView.preferredWidth: 265
      SplitView.minimumWidth: Math.max(200, workflowPanel.implicitWidth)

      spacing: Metrics.sp4

      WorkflowPanel {
        id: workflowPanel

        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(194, implicitHeight)

        mode: root.workflowMode
        submode: root.measurementSubmode
        onModeRequested: mode => root.workflowMode = mode
        onSubmodeRequested: submode => root.measurementSubmode = submode
      }

      ScenePanel {
        Layout.fillWidth: true
        Layout.fillHeight: true

        sceneModel: root.sceneModel
        selectedObjectIds: root.selectedObjectIds
        onSelectionRequested: (objectId, additive) => root.selectSceneObject(objectId, additive)
        onVisibilityRequested: (object, visible) => root.setSceneObjectVisible(object, visible)
        onRenameRequested: (object, name) => root.sceneModel.renameObject(object, name)
      }
    }

    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumWidth: 400

      spacing: Metrics.sp4

      ModeToolBar {
        Layout.fillWidth: true
        Layout.minimumHeight: implicitHeight
        Layout.maximumHeight: implicitHeight

        mode: root.workflowMode
        submode: root.measurementSubmode
        showPoints: root.showPoints
        showNormals: root.showNormals
        showScanPath: root.showScanPath
        showMachiningPath: root.showMachiningPath
        planeImportAvailable: root.geometryImportAvailable
        circleImportAvailable: root.geometryImportAvailable
        cylinderImportAvailable: root.geometryImportAvailable
        intersectionAvailable: root.intersectionAvailable
        onIntersectionRequested: root.intersectSelected()
        onPlaneImportRequested: planeFileDialog.open()
        onCircleImportRequested: circleFileDialog.open()
        onCylinderImportRequested: cylinderFileDialog.open()
        onPointsToggled: checked => root.showPoints = checked
        onNormalsToggled: checked => root.showNormals = checked
        onScanPathToggled: checked => root.showScanPath = checked
        onMachiningPathToggled: checked => root.showMachiningPath = checked
      }

      RobotViewport {
        id: viewport

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 0
        Layout.minimumHeight: 0
      }
    }

    SplitView {
      orientation: Qt.Vertical
      SplitView.preferredWidth: 330
      SplitView.minimumWidth: 280

      PropertiesPanel {
        SplitView.fillHeight: true
        SplitView.minimumHeight: 140
        selectionCount: root.selectedObjectIds.length
        selectedObject: root.selectedSceneObject
        onRenameRequested: (object, name) => root.sceneModel.renameObject(object, name)
        onVisibilityRequested: (object, visible) => root.setSceneObjectVisible(object, visible)
      }

      EndEffectorPanel {
        id: endEffectorPanel
        SplitView.preferredHeight: Math.min(implicitHeight, 370)
        SplitView.minimumHeight: 140
        measuring: root.workflowMode === WorkflowPanel.Measuring
        cadSource: endEffectorPanel.measuring ? Backend.EndEffectors.measuringCadSource : Backend.EndEffectors.spindleCadSource
        loading: endEffectorPanel.measuring ? Viewport3D.OccController.measuringCadLoading : Viewport3D.OccController.spindleCadLoading
        loadError: endEffectorPanel.measuring ? Viewport3D.OccController.measuringCadError : Viewport3D.OccController.spindleCadError
        measuringName: Backend.EndEffectors.measuringName
        ballDiameter: Backend.EndEffectors.rubyBallDiameter
        stylusLength: Backend.EndEffectors.stylusLength
        tcp: Backend.EndEffectors.spindleTcp
        onTcpRequested: values => Backend.EndEffectors.setSpindleTcp(values)
        onRetryRequested: Viewport3D.OccController.reloadEndEffector(
                            endEffectorPanel.measuring ? Backend.EndEffectors.Measuring : Backend.EndEffectors.Spindle)
        onBrowseRequested: {
          endEffectorFileDialog.measuring = endEffectorPanel.measuring
          endEffectorFileDialog.open()
        }
      }
    }
  }
}
