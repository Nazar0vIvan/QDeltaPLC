import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs as Dialogs
import robocrap_qml_module 1.0 as App
import RoboCrap.Viewport3D 1.0 as Viewport3D

import "MenuBar"
import "Views/Dashboard/DeltaPanel"
import "Views/Dashboard/FtsPanel"
import "Views/Network"
import "Views/Viewport3D"
import "Views/Workspace"

import Styles 1.0
import Components 1.0

ApplicationWindow {
  id: root

  readonly property int workflowMode: App.WorkspaceController.mode
  property bool dryScanActive: false
  property bool dryScanRunning: false
  onMeasurementSubmodeChanged: { root.dryScanActive = false; root.dryScanRunning = false }
  readonly property int measurementSubmode: App.WorkspaceController.submode

  onWorkflowModeChanged: {
    root.dryScanActive = false
    root.dryScanRunning = false
    root.toolSetupOpen = false
    if (root.chamferEditorOpen) machiningPanel.cancelEditing()
  }

  function openToolSetup() {
    if (root.chamferEditorOpen) machiningPanel.cancelEditing()
    root.toolSetupOpen = true
  }

  // Editor state stays local; application selection belongs to WorkspaceController.
  property bool chamferEditorOpen: false
  property bool toolSetupOpen: false
  readonly property list<real> selectedObjectIds: App.WorkspaceController.selectedObjectIds
  readonly property App.SceneObject selectedSceneObject: App.WorkspaceController.selectedObject

  Shortcut {
    sequence: "Del"
    context: Qt.WindowShortcut
    autoRepeat: false
    enabled: root.selectedObjectIds.length > 0
             && !(root.activeFocusItem instanceof TextInput)
             && !(root.activeFocusItem instanceof TextEdit)
    onActivated: App.WorkspaceController.deleteSelected()
  }

  onSelectedSceneObjectChanged: {
    root.toolSetupOpen = false
    if (root.chamferEditorOpen) machiningPanel.cancelEditing()
  }

  Connections {
    target: App.Machining
    function onGenerated(objectId) {
      root.chamferEditorOpen = false
      App.WorkspaceController.selectObject(objectId, false)
    }
  }
  Component.onCompleted: App.WorkspaceController.reset()

  App.PlaneImporter {
    id: surfaceImporter

    onLoaded: object => App.WorkspaceController.selectObject(object.objectId, false)
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
    title: qsTr("Geometry import failed")
    buttons: Dialogs.MessageDialog.Ok
  }

  readonly property bool geometryImportAvailable: !surfaceImporter.busy && !planeFileDialog.visible
                                                 && !cylinderFileDialog.visible
  readonly property App.SceneModel sceneModel: App.Scene
  readonly property bool intersectionAvailable: App.WorkspaceController.intersectionAvailable

  Connections {
    target: root.sceneModel
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
    onFtsPanelRequested: {
      ftsPanelWindow.show()
      ftsPanelWindow.raise()
      ftsPanelWindow.requestActivate()
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
    id: ftsPanelWindow

    title: qsTr("FTS Panel")
    transientParent: root
    flags: Qt.Tool
    color: Colors.background.dp00
    width: minimumWidth
    height: minimumHeight
    minimumWidth: ftsPanel.implicitWidth + 2 * Metrics.sp16
    minimumHeight: ftsPanel.implicitHeight + 2 * Metrics.sp16
    maximumWidth: minimumWidth
    maximumHeight: minimumHeight

    FtsPanel {
      id: ftsPanel

      anchors.fill: parent
      anchors.margins: Metrics.sp16
      title: qsTr("FTS Delta-IP68-SI-660-60")
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
    id: statusBar

    readonly property App.DeviceRunner rsi: App.Hub.device("rsi")
    // Until the robot is shown; OccController.loading covers only CAD parsing.
    readonly property bool sceneLoading: !Viewport3D.OccController.ready
                                         && Viewport3D.OccController.errorString.length === 0
    readonly property bool busy: App.Machining.calculating || statusBar.sceneLoading
    readonly property string message: {
      if (surfaceImporter.busy) return qsTr("Importing surface…")
      if (App.Machining.calculating) return qsTr("Calculating trajectory…")
      if (statusBar.sceneLoading) return qsTr("Loading…")
      if (root.workflowMode !== WorkflowPanel.Machining || root.chamferEditorOpen) return ""
      const preparation = App.MachiningPreparation
      // While sampling, unavailableReason reports progress.
      const status = preparation.error || preparation.summary
                     || preparation.unavailableReason || qsTr("Ready for RSI Sampling.")
      return statusBar.rsi && statusBar.rsi.isConnected
             ? status + "\n" + qsTr("Controller: %1").arg(statusBar.rsi.data.rsiState ?? "idle") : status
    }

    padding: Metrics.sp8
    implicitHeight: Metrics.h40

    background: Rectangle {
      color: Colors.background.dp01
      border.color: Colors.background.dp12
      border.width: Metrics.w1
    }

    contentItem: QxHField {
      spacing: Metrics.sp16
      fixedLabelWidth: true
      labelWidth: Metrics.w200
      color: Colors.foreground.medium
      labelText: root.workflowMode === WorkflowPanel.Machining
                 ? qsTr("Mode: Machining") : qsTr("Mode: Measuring / %1").arg(
                     root.measurementSubmode === WorkflowPanel.Rough ? qsTr("Rough") : qsTr("Precise"))

      Label {
        id: statusMessage

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: statusBar.message.replace(/[\r\n]+/g, " · ")
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
        color: statusBar.message.length > 0 && !surfaceImporter.busy && !statusBar.busy
               && App.MachiningPreparation.error.length > 0 ? Colors.maxColor : Colors.foreground.medium
        font: Fonts.body
        Accessible.name: statusBar.message

        // Inline before the right-aligned text; not part of the layout, so nothing moves.
        QxBusyIndicator {
          id: busyIndicator

          x: statusMessage.width - statusMessage.contentWidth - busyIndicator.width - Metrics.sp6
          anchors.verticalCenter: statusMessage.verticalCenter
          diameter: Metrics.sz20
          running: statusBar.busy
        }
      }
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
        onModeRequested: mode => App.WorkspaceController.mode = mode
        onSubmodeRequested: submode => App.WorkspaceController.submode = submode
      }

      ScenePanel {
        Layout.fillWidth: true
        Layout.fillHeight: true

        sceneModel: root.sceneModel
        selectedObjectIds: root.selectedObjectIds
        onSelectionRequested: (objectId, additive) => App.WorkspaceController.selectObject(objectId, additive)
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
        planeImportAvailable: root.geometryImportAvailable
        cylinderImportAvailable: root.geometryImportAvailable
        playbackAvailable: App.Machining.canPlay
        playing: App.Machining.playing
        playbackActive: App.Machining.playbackActive
        preparationAvailable: !root.chamferEditorOpen && App.MachiningPreparation.canPrepare
        preparing: App.MachiningPreparation.preparing
        onPreparationRequested: App.MachiningPreparation.prepare()
        onPauseRequested: App.Machining.pause()
        scanActive: root.dryScanActive
        scanRunning: root.dryScanRunning
        scanAvailable: root.selectedSceneObject !== null
                       && (root.selectedSceneObject.kind === App.SceneObject.Plane
                           || root.selectedSceneObject.kind === App.SceneObject.Cylinder
                           || root.selectedSceneObject.kind === App.SceneObject.Cone)
        onScanToggleRequested: {
          root.dryScanActive = true
          root.dryScanRunning = !root.dryScanRunning
        }
        onScanStopRequested: { root.dryScanActive = false; root.dryScanRunning = false }
        onDryRunRequested: App.Machining.dryRun()
        onStopRequested: App.Machining.stop()
        generationAvailable: !App.Machining.playbackActive && root.selectedSceneObject !== null
                             && (root.selectedSceneObject.kind === App.SceneObject.Edge
                                 || root.selectedSceneObject.kind === App.SceneObject.MachiningPath)
        onToolSetupRequested: root.openToolSetup()
        onGenerationRequested: {
          root.toolSetupOpen = false
          if (!root.chamferEditorOpen) machiningPanel.beginEditing()
          root.chamferEditorOpen = true
          machiningPanel.focusEditor()
        }
        intersectionAvailable: root.intersectionAvailable
        onIntersectionRequested: App.WorkspaceController.intersectSelected()
        onPlaneImportRequested: planeFileDialog.open()
        onCylinderImportRequested: cylinderFileDialog.open()
      }

      RobotViewport {
        id: viewport

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 0
        Layout.minimumHeight: 0
      }
    }

    Item {
      SplitView.preferredWidth: 330
      SplitView.minimumWidth: 280
      SplitView.maximumWidth: 330
      PropertiesPanel {
        anchors.fill: parent
        visible: !root.chamferEditorOpen && !root.toolSetupOpen
        selectionCount: root.selectedObjectIds.length
        selectedObject: root.selectedSceneObject
        showPoints: root.selectedSceneObject ? root.selectedSceneObject.showPoints : false
        showNormals: root.selectedSceneObject ? root.selectedSceneObject.showNormals : false
        onPointsToggled: checked => {
          if (root.selectedSceneObject)
            root.sceneModel.setObjectDiagnosticOverlays(root.selectedSceneObject, checked, root.selectedSceneObject.showNormals)
        }
        onNormalsToggled: checked => {
          if (root.selectedSceneObject)
            root.sceneModel.setObjectDiagnosticOverlays(root.selectedSceneObject, root.selectedSceneObject.showPoints, checked)
        }
        onRenameRequested: (object, name) => root.sceneModel.renameObject(object, name)
        onVisibilityRequested: (object, visible) => root.sceneModel.setObjectVisible(object, visible)
      }
      EndEffectorPanel {
        anchors.fill: parent
        visible: root.toolSetupOpen && root.workflowMode === WorkflowPanel.Measuring
        onCloseRequested: root.toolSetupOpen = false
        measuring: true
        measuringName: App.EndEffectors.measuringName
        ballDiameter: App.EndEffectors.rubyBallDiameter
        stylusLength: App.EndEffectors.stylusLength
        colletPose: App.EndEffectors.spindleColletPose
        tcp: App.EndEffectors.spindleTcp
        tcpVisible: false
        colletVisible: false
      }
      EndEffectorPanel {
        anchors.fill: parent
        visible: root.toolSetupOpen && root.workflowMode === WorkflowPanel.Machining
        onCloseRequested: root.toolSetupOpen = false
        measuring: false
        measuringName: App.EndEffectors.measuringName
        ballDiameter: App.EndEffectors.rubyBallDiameter
        stylusLength: App.EndEffectors.stylusLength
        colletPose: App.EndEffectors.spindleColletPose
        tcp: App.EndEffectors.spindleTcpInCollet
        tcpVisible: Viewport3D.OccController.showSpindleTcp
        colletVisible: Viewport3D.OccController.showSpindleCollet
        onTcpVisibilityRequested: visible => Viewport3D.OccController.showSpindleTcp = visible
        onColletVisibilityRequested: visible => Viewport3D.OccController.showSpindleCollet = visible
        onPosesRequested: (colletValues, tcpValues) => {
          if (App.EndEffectors.applySpindlePoses(colletValues, tcpValues))
            Viewport3D.OccController.showSpindleTcp = true
        }
      }
      MachiningPanel {
        id: machiningPanel
        anchors.fill: parent
        visible: root.chamferEditorOpen
        coordinator: App.Machining
        inputName: root.selectedSceneObject ? root.selectedSceneObject.name : ""
        savedPath: root.selectedSceneObject && root.selectedSceneObject.kind === App.SceneObject.MachiningPath
                   ? root.selectedSceneObject.geometry as App.MachiningPath : null
        onGenerateRequested: App.Machining.generate()
        onCancelRequested: root.chamferEditorOpen = false
      }
    }
  }
}
