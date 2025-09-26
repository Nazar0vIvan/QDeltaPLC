import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Styles 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias from: pb.from
  property alias to: pb.to
  property alias color: fill.color

  property real value
  property color trackColor:"#2B2B2B"
  property color centerLine:"#666666"

  RowLayout {
    id: rl

    anchors.fill: parent

    Label {
        id: label

        text: "Name"
        color: Styles.foreground.high
        verticalAlignment: Text.AlignVCenter
        Layout.alignment: Qt.AlignVCenter
    }

    ProgressBar {
      id: pb

      Layout.fillWidth: true
      Layout.preferredHeight: root.height
      Layout.alignment: Qt.AlignVCenter

      from: 0
      to: 100

      value: root.value

      background: Rectangle {
          radius: height / 2
          color: root.trackColor
          border.color: Qt.rgba(1, 1, 1, 0.07)

          Rectangle { // zero line
              width: 2
              height: parent.height
              color: root.centerLine
              anchors.centerIn: parent
          }
      }

      contentItem: Item {
        id: host

        // Clamp into [from, to]
        readonly property real v: Math.max(Math.min(pb.value, pb.to), pb.from)
        // Ranges for each side
        readonly property real posRange: Math.max(0, pb.to)
        readonly property real negRange: Math.abs(Math.min(0, pb.from))
        readonly property real halfW: width / 2

        Rectangle {
            id: fill

            color: root.barColor
            radius: parent.height / 2
            height: parent.height

            // Width grows from the center toward the side
            width: host.v >= 0
                   ? (host.posRange === 0 ? 0 : (host.v / host.posRange) * host.halfW)
                   : (host.negRange === 0 ? 0 : (Math.abs(host.v) / host.negRange) * host.halfW)

            // Position from center
            x: host.v >= 0 ? host.halfW : host.halfW - width

            // Hide at (or near) zero to avoid any tiny sliver
            visible: width > 0.5
        }
      }
    }
    Text {
      id: valText

      text: Number(root.value).toFixed(2)
      color: Styles.foreground.high
      Layout.alignment: Qt.AlignVCenter
    }
  }
}


