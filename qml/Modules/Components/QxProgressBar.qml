import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Styles 1.0

Item {
  id: root

  property alias labelText: label.text

  property int barWidth: 100
  property alias from: pb.from
  property alias to: pb.to
  property alias color: fill.color
  property alias value: pb.value

  implicitWidth: rl.implicitWidth

  RowLayout {
    id: rl

    height: parent.height
    spacing: 10

    Label {
      id: label

      verticalAlignment: Text.AlignVCenter
      Layout.alignment: Qt.AlignVCenter
      color: Styles.foreground.high
    }

    ProgressBar {
      id: pb

      Layout.preferredWidth: root.barWidth
      Layout.preferredHeight: parent.height
      Layout.alignment: Qt.AlignVCenter

      background: Rectangle {
        color: Styles.background.dp04
        border{width: 1; color: Styles.background.dp12}
      }

      contentItem: Item {
        id: host

        // Calculate the position and width for both positive and negative ranges
        readonly property real rangeMax: Math.max(Math.abs(pb.from), Math.abs(pb.to))
        readonly property real valueClamped: Math.min(Math.max(pb.value, pb.from), pb.to)
        readonly property real positiveWidth: (Math.max(0, valueClamped) / rangeMax) * (width / 2)
        readonly property real negativeWidth: (Math.abs(Math.min(0, valueClamped)) / rangeMax) * (width / 2)

        Rectangle {
          id: fill
          width: positiveWidth + negativeWidth
          height: parent.height

          // Apply gradient across the entire range
          gradient: Gradient {
            GradientStop { position: 0.0; color: "#4ed964" }  // Start color (near zero)
            GradientStop { position: 0.5; color: "#ffff00" }  // Mid color (center of range)
            GradientStop { position: 1.0; color: "#ff3a31" }  // End color (far end of range)
          }

          x: negativeWidth  // Offset the start of the fill for negative range
          visible: width > 0.5
        }
      }
    }

    Text {
      id: valueText

      text: pb.value.toFixed(3)
      color: Styles.foreground.high
      Layout.alignment: Qt.AlignVCenter
    }
  }
}


