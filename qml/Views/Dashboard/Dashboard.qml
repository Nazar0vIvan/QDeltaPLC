import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Styles 1.0
import Components 1.0

import "DeltaPanel"
import "FtsPanel"
import "KukaPanel"

ScrollView {
  id: root

  clip: true

  contentWidth: availableWidth
  contentHeight: content.implicitHeight

  ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
  ScrollBar.vertical.policy: ScrollBar.AsNeeded

  Item {
    id: content

    width: root.availableWidth
    implicitHeight: cl1.implicitHeight + 40 + 20

    ColumnLayout {
      id: cl1

      anchors {
        top: parent.top
        left: parent.left
        right: parent.right
        topMargin: 40
        leftMargin: 20
        rightMargin: 20
      }

      spacing: 20

      RowLayout {
        id: rl1

        Layout.fillWidth: true
        spacing: 20

        DeltaPanel {
          id: deltaPanel

          title: "PLC AS332T-A"
        }
      }

      Rectangle {
        id: separator2

        Layout.fillWidth: true
        Layout.preferredHeight: 1

        gradient: Gradient {
          orientation: Gradient.Horizontal

          GradientStop {
            position: 0.0
            color: Styles.secondary.dark
          }

          GradientStop {
            position: 1.0
            color: Styles.background.dp00
          }
        }
      }

      RowLayout {
        id: rl2

        Layout.fillWidth: true
        spacing: 20

        KukaPanel {
          id: kukaPanel

          title: qsTr("Robot Sensor Interface")

          Layout.alignment: Qt.AlignTop
          Layout.topMargin: 14
        }

        FtsPanel {
          id: ftsPanel

          title: qsTr("FTS Delta-IP68-SI-660-60")

          Layout.alignment: Qt.AlignTop
          Layout.topMargin: 14
        }
      }
    }
  }
}
