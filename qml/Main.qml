import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQml.Models

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

  // A lookup into the sample model, not a separate copy of the selected object.
  readonly property var selectedSceneObject: root.selectedObjectIds.length === 1
                                            ? root.findSceneObject(root.selectedObjectIds[0]) : null

  function findSceneObject(objectId) {
    for (let groupIndex = 0; groupIndex < sampleScene.count; ++groupIndex) {
      const group = sampleScene.get(groupIndex)
      for (let objectIndex = 0; objectIndex < group.objects.count; ++objectIndex) {
        if (group.objects.get(objectIndex).objectId === objectId)
          return { objects: group.objects, index: objectIndex, kind: group.kind }
      }
    }
    return null
  }

  function updateSceneObject(objectId, role, value) {
    const entry = root.findSceneObject(objectId)
    if (entry)
      entry.objects.setProperty(entry.index, role, value)
  }

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

  // Temporary browser data only; these objects do not exist in the 3D scene.
  ListModel {
    id: sampleScene

    ListElement {
      name: qsTr("Rough Surfaces"); kind: "rough"; badge: "R"; expanded: true
      objects: [
        ListElement { objectId: "rough-plane"; name: "Plane P1"; typeName: qsTr("Plane"); iconSource: "qrc:/pics/plane.svg"; objectVisible: true },
        ListElement { objectId: "rough-cylinder"; name: "Cylinder C1"; typeName: qsTr("Cylinder"); iconSource: "qrc:/pics/cylinder.svg"; objectVisible: true },
        ListElement { objectId: "rough-cone"; name: "Cone K1"; typeName: qsTr("Cone"); iconSource: "qrc:/pics/cone.svg"; objectVisible: true }
      ]
    }
    ListElement {
      name: qsTr("Precise Surfaces"); kind: "precise"; badge: "P"; expanded: true
      objects: [
        ListElement { objectId: "precise-plane"; name: "Plane P1"; typeName: qsTr("Plane"); iconSource: "qrc:/pics/plane.svg"; objectVisible: true },
        ListElement { objectId: "precise-cylinder"; name: "Cylinder C1"; typeName: qsTr("Cylinder"); iconSource: "qrc:/pics/cylinder.svg"; objectVisible: true }
      ]
    }
    ListElement {
      name: qsTr("Edges"); kind: "edges"; badge: ""; expanded: true
      objects: [
        ListElement { objectId: "edge-1"; name: "Edge E1"; typeName: qsTr("Edge"); iconSource: "qrc:/pics/edge.svg"; objectVisible: true },
        ListElement { objectId: "edge-2"; name: "Edge E2"; typeName: qsTr("Edge"); iconSource: "qrc:/pics/edge.svg"; objectVisible: true }
      ]
    }
    ListElement {
      name: qsTr("Scan Paths"); kind: "scanPaths"; badge: ""; expanded: true
      objects: [
        ListElement { objectId: "scan-1"; name: "Scan S1"; typeName: qsTr("Path"); iconSource: "qrc:/pics/path.svg"; objectVisible: true }
      ]
    }
    ListElement {
      name: qsTr("Machining Paths"); kind: "machiningPaths"; badge: ""; expanded: true
      objects: [
        ListElement { objectId: "path-1"; name: "Path P1"; typeName: qsTr("Path"); iconSource: "qrc:/pics/path.svg"; objectVisible: true }
      ]
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
  }

  footer: ToolBar {
    padding: Metrics.sp8

    background: Rectangle {
      color: Colors.background.dp01
      border.color: Colors.background.dp12
      border.width: Metrics.w1
    }

    contentItem: Label {
      text: root.workflowMode === WorkflowPanel.Machining
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

        sceneModel: sampleScene
        selectedObjectIds: root.selectedObjectIds
        onExpansionRequested: (groupIndex, expanded) => sampleScene.setProperty(groupIndex, "expanded", expanded)
        onSelectionRequested: (objectId, additive) => root.selectSceneObject(objectId, additive)
        onVisibilityRequested: (groupIndex, objectIndex, objectVisible) =>
          sampleScene.get(groupIndex).objects.setProperty(objectIndex, "objectVisible", objectVisible)
        onRenameRequested: (groupIndex, objectIndex, name) =>
          sampleScene.get(groupIndex).objects.setProperty(objectIndex, "name", name)
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
      objectId: root.selectedObjectIds.length === 1 ? root.selectedObjectIds[0] : ""
      selectedName: root.selectedSceneObject
                  ? root.selectedSceneObject.objects.get(root.selectedSceneObject.index).name : ""
      objectType: root.selectedSceneObject
                  ? root.selectedSceneObject.objects.get(root.selectedSceneObject.index).typeName : ""
      classification: !root.selectedSceneObject ? ""
                      : root.selectedSceneObject.kind === "rough" ? qsTr("Rough")
                      : root.selectedSceneObject.kind === "precise" ? qsTr("Precise")
                      : qsTr("Not applicable")
      objectVisible: root.selectedSceneObject
                     ? root.selectedSceneObject.objects.get(root.selectedSceneObject.index).objectVisible : false
      onRenameRequested: (objectId, name) => root.updateSceneObject(objectId, "name", name)
      onVisibilityRequested: (objectId, objectVisible) => root.updateSceneObject(objectId, "objectVisible", objectVisible)
    }
  }
}
