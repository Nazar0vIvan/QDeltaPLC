import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Styles 1.0

Item {
  id: root
  property alias labelText: label.text
  property int barWidth: Metrics.fieldWidthMedium
  property alias from: pb.from
  property alias to: pb.to
  property alias color: fill.color
  property alias value: pb.value

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    spacing: Metrics.spacingSmall

    Label {
      id: label
      verticalAlignment: Text.AlignVCenter
      Layout.alignment: Qt.AlignVCenter
      color: Colors.foreground.high
      font: Fonts.body
    }

    ProgressBar {
      id: pb
      Layout.preferredWidth: root.barWidth
      Layout.preferredHeight: Metrics.controlHeightCompact
      Layout.alignment: Qt.AlignVCenter
      background: Rectangle {
        color: Colors.background.dp04
        border {
          width: Metrics.borderWidth
          color: Colors.background.dp12
        }
      }

      contentItem: Item {
        id: host

        readonly property real v: Math.max(Math.min(pb.value, pb.to), pb.from)
        readonly property real posRange: Math.max(0, pb.to)
        readonly property real negRange: Math.abs(Math.min(0, pb.from))
        readonly property real halfW: width / 2

        Rectangle {
          id: fill
          height: parent.height
          width: host.v >= 0 ? (host.posRange === 0 ? 0 : (host.v / host.posRange) * host.halfW) : (host.negRange === 0 ? 0 : (Math.abs(host.v) / host.negRange) * host.halfW)
          x: host.v >= 0 ? host.halfW : host.halfW - width
          visible: width > 0.5
        }
      }
    }

    Text {
      id: valueText

      Layout.alignment: Qt.AlignVCenter
      text: pb.value.toFixed(3)
      color: Colors.foreground.high
      font: Fonts.body
    }
  }
}
