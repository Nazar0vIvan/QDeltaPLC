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

      from: -Math.max(Math.abs(root.from), Math.abs(root.to))
      to: Math.max(Math.abs(root.from), Math.abs(root.to))

      background: Rectangle {
        color: Styles.background.dp04
        border { width: 1; color: Styles.background.dp12 }
      }

      contentItem: Item {
        // Positive progress
        Rectangle {
          visible: value > 0
          x: parent.width / 2
          width: (value / to) * (parent.width / 2)
          height: parent.height
          radius: 3

          gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
              position: 0.0;
              color: Qt.rgba(0.305, 0.850, 0.392, 1) // #4ed964
            }
            GradientStop {
              position: 0.5;
              color: interpolateColor(
                Qt.rgba(0.305, 0.850, 0.392, 1),
                Qt.rgba(1.0, 1.0, 0.0, 1),
                Math.min(width / (parent.width / 4), 1)
              )
            }
            GradientStop {
              position: 1.0;
              color: interpolateColor(
                Qt.rgba(0.305, 0.850, 0.392, 1),
                Qt.rgba(1.0, 0.227, 0.192, 1),
                Math.min(width / (parent.width / 2), 1)
              )
            }
          }
        }

        // Negative progress
        Rectangle {
          visible: value < 0
          x: (parent.width / 2) - width
          width: (Math.abs(value) / Math.abs(from)) * (parent.width / 2)
          height: parent.height
          radius: 3

          gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
              position: 0.0;
              color: interpolateColor(
                Qt.rgba(0.305, 0.850, 0.392, 1),
                Qt.rgba(1.0, 0.227, 0.192, 1),
                Math.min(width / (parent.width / 2), 1)
              )
            }
            GradientStop {
              position: 0.5;
              color: interpolateColor(
                Qt.rgba(0.305, 0.850, 0.392, 1),
                Qt.rgba(1.0, 1.0, 0.0, 1),
                Math.min(width / (parent.width / 4), 1)
              )
            }
            GradientStop {
              position: 1.0;
              color: Qt.rgba(0.305, 0.850, 0.392, 1) // #4ed964
            }
          }
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

  function interpolateColor(color1, color2, ratio) {
    return Qt.rgba(
      color1.r + (color2.r - color1.r) * ratio,
      color1.g + (color2.g - color1.g) * ratio,
      color1.b + (color2.b - color1.b) * ratio,
      1
    )
  }
}
