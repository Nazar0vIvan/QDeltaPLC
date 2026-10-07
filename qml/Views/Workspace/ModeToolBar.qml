import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

ToolBar {
  id: root

  required property int mode
  required property int submode
  property bool planeImportAvailable: false
  property bool circleImportAvailable: false
  property bool cylinderImportAvailable: false
  property bool intersectionAvailable: false
  property bool generationAvailable: false
  property bool preparationAvailable: false
  property bool preparing: false
  property bool playbackAvailable: false
  property bool playing: false
  property bool playbackActive: false
  property bool scanAvailable: false
  property bool scanRunning: false
  property bool scanActive: false
  signal scanToggleRequested()
  signal scanStopRequested()
  signal pauseRequested()
  signal dryRunRequested()
  signal stopRequested()

  signal planeImportRequested()
  signal circleImportRequested()
  signal cylinderImportRequested()
  signal intersectionRequested()
  signal generationRequested()
  signal preparationRequested()
  signal toolSetupRequested()

  readonly property bool measuring: root.mode === WorkflowPanel.Measuring
  readonly property bool rough: root.measuring && root.submode === WorkflowPanel.Rough
  readonly property bool precise: root.measuring && root.submode === WorkflowPanel.Precise

  implicitHeight: 83
  padding: Metrics.sp8

  onModeChanged: tools.contentX = 0
  onSubmodeChanged: tools.contentX = 0

  background: Rectangle {
    color: Colors.background.dp01
    radius: Metrics.r4
    border.width: Metrics.w1
    border.color: Colors.background.dp12
  }

  component Tool: Item {
    id: tool

    property alias text: button.text
    property alias iconSource: button.icon.source
    property alias checkable: button.checkable
    property alias checked: button.checked
    property bool available: false
    property bool preserveIconColors: false

    signal toggled(bool checked)
    signal triggered()

    implicitWidth: Math.max(100, button.implicitWidth)
    implicitHeight: 60
    width: implicitWidth
    height: implicitHeight

    ToolButton {
      id: button

      readonly property color foregroundColor: !button.enabled ? Colors.foreground.disabled
                                               : button.checked ? "black" : "white"

      anchors.fill: parent
      enabled: tool.available
      padding: Metrics.sp8
      spacing: Metrics.sp6
      font: Fonts.caption
      display: AbstractButton.TextUnderIcon
      icon.width: Metrics.sz24
      icon.height: Metrics.sz24
      icon.color: button.enabled && tool.preserveIconColors ? "transparent" : button.foregroundColor
      palette.buttonText: button.foregroundColor
      palette.highlight: button.foregroundColor
      palette.disabled.buttonText: Colors.foreground.disabled

      background: Rectangle {
        radius: Metrics.r4
        color: button.checked ? Colors.secondary.base
               : button.down ? Colors.background.dp12
               : button.hovered ? Colors.background.dp04 : "transparent"
        border.width: button.checked || button.visualFocus ? Metrics.w2 : Metrics.w1
        border.color: button.visualFocus ? Colors.primary.light
                      : button.checked ? Colors.secondary.dark : Colors.background.dp12
      }

      onClicked: {
        tool.triggered()
        tool.toggled(button.checked)
      }
    }
  }

  contentItem: Flickable {
    id: tools

    clip: true
    contentWidth: toolRow.implicitWidth
    contentHeight: height
    flickableDirection: Flickable.HorizontalFlick
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentWidth > width

    ScrollBar.horizontal: ScrollBar {
      policy: ScrollBar.AsNeeded
    }

    Row {
      id: toolRow

      spacing: Metrics.sp10

      Tool {
        text: root.measuring ? qsTr("Measuring EE") : qsTr("Machining EE")
        iconSource: "qrc:/pics/settings.svg"
        available: true
        onTriggered: root.toolSetupRequested()
      }

      Rectangle {
        width: Metrics.w1
        height: Metrics.sz36
        y: Metrics.sp12
        color: Colors.background.dp12
      }

      Row {
        visible: root.rough
        spacing: Metrics.sp16

        Tool {
          text: qsTr("Plane from JSON")
          iconSource: "qrc:/pics/plane.svg"
          available: root.planeImportAvailable
          onTriggered: root.planeImportRequested()
        }

        Tool {
          text: qsTr("Circle from JSON")
          iconSource: "qrc:/pics/circ.svg"
          available: root.circleImportAvailable
          onTriggered: root.circleImportRequested()
        }

        Tool {
          text: qsTr("Cylinder from JSON")
          iconSource: "qrc:/pics/cylinder.svg"
          available: root.cylinderImportAvailable
          onTriggered: root.cylinderImportRequested()
        }

        Tool {
          text: qsTr("Cone from JSON")
          iconSource: "qrc:/pics/cone.svg"
        }
      }

      Tool {
        visible: root.measuring
        text: qsTr("Intersect Selected")
        iconSource: "qrc:/pics/intersec.svg"
        available: root.intersectionAvailable
        onTriggered: root.intersectionRequested()
      }

      Tool {
        visible: !root.measuring
        text: qsTr("Generate Path")
        iconSource: "qrc:/pics/generate_path.svg"
        available: root.generationAvailable
        onTriggered: root.generationRequested()
      }

      Tool {
        visible: !root.measuring
        text: root.preparing ? qsTr("Preparing…") : qsTr("Prepare")
        iconSource: "qrc:/pics/generate_path.svg"
        available: root.preparationAvailable
        onTriggered: root.preparationRequested()
      }

      Rectangle {
        visible: !root.rough
        width: Metrics.w1
        height: Metrics.sz36
        y: Metrics.sp12
        color: Colors.background.dp12
      }

      Tool {
        visible: !root.rough
        text: (root.precise ? root.scanRunning : root.playing) ? qsTr("Pause")
              : root.precise ? qsTr("Dry Scan") : qsTr("Dry Run")
        iconSource: (root.precise ? root.scanRunning : root.playing) ? "qrc:/pics/pause.svg" : "qrc:/pics/dry.svg"
        preserveIconColors: true
        available: root.precise ? root.scanActive || root.scanAvailable : root.playing || root.playbackAvailable
        onTriggered: {
          if (root.precise) root.scanToggleRequested()
          else if (root.playing) root.pauseRequested()
          else root.dryRunRequested()
        }
      }

      Tool {
        visible: !root.rough
        text: qsTr("Stop")
        iconSource: "qrc:/pics/stop.svg"
        preserveIconColors: true
        available: root.precise ? root.scanActive : root.playbackActive
        onTriggered: {
          if (root.precise) root.scanStopRequested()
          else root.stopRequested()
        }
      }

      Rectangle {
        visible: !root.rough
        width: Metrics.w1
        height: Metrics.sz36
        y: Metrics.sp12
        color: Colors.background.dp12
      }

      Tool {
        visible: !root.rough
        text: root.precise ? qsTr("Scan") : qsTr("Start Machining")
        iconSource: root.precise ? "qrc:/pics/scan.svg" : "qrc:/pics/run.svg"
        preserveIconColors: true
      }
    }
  }
}
