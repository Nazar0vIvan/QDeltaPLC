import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

ToolBar {
  id: root

  required property int mode
  required property int submode
  required property bool showPoints
  required property bool showNormals
  required property bool showScanPath
  required property bool showMachiningPath
  property bool planeImportAvailable: false

  signal planeImportRequested()
  signal pointsToggled(bool checked)
  signal normalsToggled(bool checked)
  signal scanPathToggled(bool checked)
  signal machiningPathToggled(bool checked)

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
      icon.color: button.foregroundColor
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

      spacing: Metrics.sp16

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
          text: qsTr("Cylinder from JSON")
          iconSource: "qrc:/pics/cylinder.svg"
        }

        Tool {
          text: qsTr("Cone from JSON")
          iconSource: "qrc:/pics/cone.svg"
        }
      }

      Tool {
        visible: root.precise
        text: qsTr("Intersect Selected")
        iconSource: "qrc:/pics/intersec.svg"
      }

      Row {
        visible: !root.measuring
        spacing: Metrics.sp16

        Tool {
          text: qsTr("Generate Path")
          iconSource: "qrc:/pics/generate_path.svg"
        }

        Tool {
          text: qsTr("Dry Run")
          iconSource: "qrc:/pics/dry_run.svg"
        }

        Tool {
          text: qsTr("Start Machining")
          iconSource: "qrc:/pics/run.svg"
        }
      }

      Tool {
        visible: root.precise
        text: qsTr("Scan")
        iconSource: "qrc:/pics/scan.svg"
      }

      Tool {
        visible: !root.rough
        text: qsTr("Stop")
        iconSource: "qrc:/pics/stop.svg"
      }

      Rectangle {
        width: Metrics.w1
        height: Metrics.sz36
        y: Metrics.sp12
        color: Colors.background.dp12
      }

      Tool {
        visible: root.measuring
        text: qsTr("Show Points")
        iconSource: "qrc:/pics/points.svg"
        available: true
        checkable: true
        checked: root.showPoints
        onToggled: checked => root.pointsToggled(checked)
      }

      Tool {
        visible: root.measuring
        text: qsTr("Show Normals")
        iconSource: "qrc:/pics/normals.svg"
        available: true
        checkable: true
        checked: root.showNormals
        onToggled: checked => root.normalsToggled(checked)
      }

      Tool {
        visible: !root.rough
        text: qsTr("Show Path")
        iconSource: "qrc:/pics/show_path.svg"
        available: true
        checkable: true
        checked: root.measuring ? root.showScanPath : root.showMachiningPath
        onToggled: checked => {
          if (root.measuring)
            root.scanPathToggled(checked)
          else
            root.machiningPathToggled(checked)
        }
      }
    }
  }
}
