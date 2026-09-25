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

  // UI display preferences for the imported rough-surface presentations.
  property bool showPoints: false
  property bool showNormals: false
  property bool showScanPath: false
  property bool showMachiningPath: false
  property list<string> selectedObjectIds: []

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
    root.syncViewportSelection()
    root.syncViewportOverlays()
  }

  Connections {
    target: Viewport3D.OccController

    function onReadyChanged() {
      if (Viewport3D.OccController.ready) {
        root.syncViewportSelection()
        root.syncViewportOverlays()
      }
    }

    function onApplicationSelectionRequested(objectId, additive) {
      if (objectId.length === 0)
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
    id: cylinderFileDialog
    title: qsTr("Import rough cylinder")
    fileMode: Dialogs.FileDialog.OpenFile
    nameFilters: [qsTr("JSON point files (*.json)")]
    onAccepted: surfaceImporter.loadCylinder(cylinderFileDialog.selectedFile, root.sceneModel)
  }

  Dialogs.MessageDialog {
    id: importError
    title: qsTr("Surface import failed")
    buttons: Dialogs.MessageDialog.Ok
  }

  readonly property Backend.SceneModel sceneModel: Backend.Scene

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
        planeImportAvailable: !surfaceImporter.busy && !planeFileDialog.visible && !cylinderFileDialog.visible
        cylinderImportAvailable: !surfaceImporter.busy && !planeFileDialog.visible && !cylinderFileDialog.visible
        onPlaneImportRequested: planeFileDialog.open()
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

    PropertiesPanel {
      SplitView.preferredWidth: 272
      SplitView.minimumWidth: 220

      selectionCount: root.selectedObjectIds.length
      selectedObject: root.selectedSceneObject
      onRenameRequested: (object, name) => root.sceneModel.renameObject(object, name)
      onVisibilityRequested: (object, visible) => root.setSceneObjectVisible(object, visible)
    }
  }
}
