import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs as Dialogs
import RoboCrap.Backend 1.0 as Backend
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

  property int workflowMode: WorkflowPanel.Measuring
  property bool dryScanActive: false
  property bool dryScanRunning: false
  onMeasurementSubmodeChanged: { root.dryScanActive = false; root.dryScanRunning = false }
  property int measurementSubmode: WorkflowPanel.Rough

  onWorkflowModeChanged: {
    root.dryScanActive = false
    root.dryScanRunning = false
    if (root.workflowMode !== WorkflowPanel.Machining) Backend.Machining.stop()
    if (root.workflowMode === WorkflowPanel.Machining) {
      if (measuringSetupWindow) measuringSetupWindow.hide()
    } else if (spindleSetupWindow) spindleSetupWindow.hide()
    if (root.chamferEditorOpen) machiningPanel.cancelEditing()
    root.syncEndEffector()
    root.syncMachiningInput()
  }

  function syncEndEffector() {
    Backend.EndEffectors.setActiveTool(root.workflowMode === WorkflowPanel.Machining
                                      ? Backend.EndEffectors.Spindle : Backend.EndEffectors.Measuring)
  }

  function openToolSetup() {
    const window = root.workflowMode === WorkflowPanel.Machining ? spindleSetupWindow : measuringSetupWindow
    window.show()
    window.raise()
    window.requestActivate()
  }

  // Editor state and selected application objects.
  property bool chamferEditorOpen: false
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

  function syncMachiningInput() {
    const object = root.selectedSceneObject
    Backend.Machining.inputId = root.workflowMode === WorkflowPanel.Machining && object
        && (object.kind === Backend.SceneObject.Edge || object.kind === Backend.SceneObject.MachiningPath)
        ? object.objectId : 0
  }

  onSelectedSceneObjectChanged: {
    if (root.chamferEditorOpen) machiningPanel.cancelEditing()
    root.syncMachiningInput()
  }

  Connections {
    target: Backend.Machining
    function onGenerated(objectId) {
      root.chamferEditorOpen = false
      root.selectedObjectIds = [objectId]
    }
  }
  function syncViewportSelection() {
    Viewport3D.OccController.setSelectedObjects(root.selectedObjectIds)
  }

  function setSceneObjectVisible(object, visible) {
    if (root.sceneModel.setObjectVisible(object, visible))
      root.syncViewportSelection()
  }

  onSelectedObjectIdsChanged: root.syncViewportSelection()
  Component.onCompleted: {
    if (root.workflowMode !== WorkflowPanel.Machining) Backend.Machining.stop()
    root.syncEndEffector()
    root.syncMachiningInput()
    root.syncViewportSelection()
  }

  Connections {
    target: Viewport3D.OccController

    function onDeleteSelectionRequested() { root.deleteSelectedObjects() }

    function onViewportReadyChanged() {
      if (Viewport3D.OccController.viewportReady) {
        root.syncViewportSelection()
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
    onFtsPanelRequested: {
      ftsPanelWindow.show()
      ftsPanelWindow.raise()
      ftsPanelWindow.requestActivate()
    }
  }

  Window {
    id: calculationWindow

    title: qsTr("Calculating trajectory")
    transientParent: root
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint
    visible: Backend.Machining.calculating
    color: Colors.background.dp01
    width: 360
    height: 110
    minimumWidth: 360
    maximumWidth: 360
    minimumHeight: 110
    maximumHeight: 110
    x: root.x + (root.width - width) / 2
    y: root.y + (root.height - height) / 2
    onClosing: close => close.accepted = !Backend.Machining.calculating

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: Metrics.sp16
      spacing: Metrics.sp12

      Label {
        Layout.fillWidth: true
        text: qsTr("Calculating trajectory…")
        color: Colors.foreground.high
        font: Fonts.body
      }
      ProgressBar {
        Layout.fillWidth: true
        indeterminate: Backend.Machining.calculating
        palette.highlight: Colors.primary.base
      }
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

  Window {
    id: measuringSetupWindow

    title: qsTr("Measuring end effector")
    transientParent: root
    flags: Qt.Tool
    color: Colors.background.dp00
    width: 440
    height: 360
    minimumWidth: 360
    minimumHeight: 280

    EndEffectorPanel {
      anchors.fill: parent
      anchors.margins: Metrics.sp8
      measuring: true
      cadSource: Backend.EndEffectors.measuringCadSource
      loading: Viewport3D.OccController.measuringCadLoading
      loadError: Viewport3D.OccController.measuringCadError
      measuringName: Backend.EndEffectors.measuringName
      ballDiameter: Backend.EndEffectors.rubyBallDiameter
      stylusLength: Backend.EndEffectors.stylusLength
      colletPose: Backend.EndEffectors.spindleColletPose
      tcp: Backend.EndEffectors.spindleTcp
      tcpVisible: false
      colletVisible: false
      onRetryRequested: Viewport3D.OccController.reloadEndEffector(Backend.EndEffectors.Measuring)
      onBrowseRequested: {
        endEffectorFileDialog.measuring = true
        endEffectorFileDialog.open()
      }
    }
  }

  Window {
    id: spindleSetupWindow

    title: qsTr("Spindle EE")
    transientParent: root
    flags: Qt.Tool
    color: Colors.background.dp00
    width: 440
    height: 640
    minimumWidth: 360
    minimumHeight: 400

    EndEffectorPanel {
      anchors.fill: parent
      anchors.margins: Metrics.sp8
      measuring: false
      cadSource: Backend.EndEffectors.spindleCadSource
      loading: Viewport3D.OccController.spindleCadLoading
      loadError: Viewport3D.OccController.spindleCadError
      measuringName: Backend.EndEffectors.measuringName
      ballDiameter: Backend.EndEffectors.rubyBallDiameter
      stylusLength: Backend.EndEffectors.stylusLength
      colletPose: Backend.EndEffectors.spindleColletPose
      tcp: Backend.EndEffectors.spindleTcpInCollet
      tcpVisible: Viewport3D.OccController.showSpindleTcp
      colletVisible: Viewport3D.OccController.showSpindleCollet
      onTcpVisibilityRequested: visible => Viewport3D.OccController.showSpindleTcp = visible
      onColletVisibilityRequested: visible => Viewport3D.OccController.showSpindleCollet = visible
      onPosesRequested: (colletValues, tcpValues) => {
        if (Backend.EndEffectors.applySpindlePoses(colletValues, tcpValues))
          Viewport3D.OccController.showSpindleTcp = true
      }
      onRetryRequested: Viewport3D.OccController.reloadEndEffector(Backend.EndEffectors.Spindle)
      onBrowseRequested: {
        endEffectorFileDialog.measuring = false
        endEffectorFileDialog.open()
      }
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
        planeImportAvailable: root.geometryImportAvailable
        circleImportAvailable: root.geometryImportAvailable
        cylinderImportAvailable: root.geometryImportAvailable
        playbackAvailable: Backend.Machining.canPlay
        playing: Backend.Machining.playing
        playbackActive: Backend.Machining.playbackActive
        onPauseRequested: Backend.Machining.pause()
        scanActive: root.dryScanActive
        scanRunning: root.dryScanRunning
        scanAvailable: root.selectedSceneObject !== null
                       && (root.selectedSceneObject.kind === Backend.SceneObject.Plane
                           || root.selectedSceneObject.kind === Backend.SceneObject.Cylinder
                           || root.selectedSceneObject.kind === Backend.SceneObject.Cone)
        onScanToggleRequested: {
          root.dryScanActive = true
          root.dryScanRunning = !root.dryScanRunning
        }
        onScanStopRequested: { root.dryScanActive = false; root.dryScanRunning = false }
        onDryRunRequested: Backend.Machining.dryRun()
        onStopRequested: Backend.Machining.stop()
        generationAvailable: !Backend.Machining.playbackActive && root.selectedSceneObject !== null
                             && (root.selectedSceneObject.kind === Backend.SceneObject.Edge
                                 || root.selectedSceneObject.kind === Backend.SceneObject.MachiningPath)
        onToolSetupRequested: root.openToolSetup()
        onGenerationRequested: {
          if (!root.chamferEditorOpen) machiningPanel.beginEditing()
          root.chamferEditorOpen = true
          machiningPanel.focusEditor()
        }
        intersectionAvailable: root.intersectionAvailable
        onIntersectionRequested: root.intersectSelected()
        onPlaneImportRequested: planeFileDialog.open()
        onCircleImportRequested: circleFileDialog.open()
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
        visible: !root.chamferEditorOpen
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
        onVisibilityRequested: (object, visible) => root.setSceneObjectVisible(object, visible)
      }
      MachiningPanel {
        id: machiningPanel
        anchors.fill: parent
        visible: root.chamferEditorOpen
        coordinator: Backend.Machining
        inputName: root.selectedSceneObject ? root.selectedSceneObject.name : ""
        savedPath: root.selectedSceneObject && root.selectedSceneObject.kind === Backend.SceneObject.MachiningPath
                   ? root.selectedSceneObject.geometry as Backend.MachiningPath : null
        onApplyRequested: Backend.Machining.apply()
        onCancelRequested: root.chamferEditorOpen = false
      }
    }
  }
}
