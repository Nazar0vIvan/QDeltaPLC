import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs as Dialogs
import RoboCrap.Backend 1.0 as Backend

import "MenuBar"
import "Views/Viewport3D"
import "Views/Workspace"

import Styles 1.0
import Components 1.0

ApplicationWindow {
  id: root

  property int workflowMode: WorkflowPanel.Measuring
  property int measurementSubmode: WorkflowPanel.Rough

  // UI display preferences; scene visibility will be connected in a later step.
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

  Backend.PlaneImporter {
    id: planeImporter

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
    onAccepted: planeImporter.load(planeFileDialog.selectedFile, root.sceneModel)
  }

  Dialogs.MessageDialog {
    id: importError
    title: qsTr("Plane import failed")
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
  }

  footer: ToolBar {
    padding: Metrics.sp8

    background: Rectangle {
      color: Colors.background.dp01
      border.color: Colors.background.dp12
      border.width: Metrics.w1
    }

    contentItem: Label {
      text: planeImporter.busy ? qsTr("Importing plane…")
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
        onVisibilityRequested: (object, visible) => root.sceneModel.setObjectVisible(object, visible)
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
        planeImportAvailable: !planeImporter.busy && !planeFileDialog.visible
        onPlaneImportRequested: planeFileDialog.open()
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
      onVisibilityRequested: (object, visible) => root.sceneModel.setObjectVisible(object, visible)
    }
  }
}
