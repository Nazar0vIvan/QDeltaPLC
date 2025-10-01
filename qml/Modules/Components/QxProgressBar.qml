import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Styles 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias barWidth: pb.width

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

      Layout.preferredWidth: 200
      Layout.preferredHeight: root.height
      Layout.alignment: Qt.AlignVCenter

      from: 0
      to: 100

      value: root.value

      background: Rectangle {
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

            height: parent.height
            width: host.v >= 0
                   ? (host.posRange === 0 ? 0 : (host.v / host.posRange) * host.halfW)
                   : (host.negRange === 0 ? 0 : (Math.abs(host.v) / host.negRange) * host.halfW)

            x: host.v >= 0 ? host.halfW : host.halfW - width
            visible: width > 0.5
        }
      }
    }
    Text {
      id: valText

      text: root.value.toFixed(3)
      color: Styles.foreground.high
      Layout.alignment: Qt.AlignVCenter
    }
  }
}


