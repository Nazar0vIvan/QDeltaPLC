import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  implicitWidth: leftPadding + rl.implicitWidth + rightPadding
  implicitHeight: topPadding + rl.implicitHeight + bottomPadding

  RowLayout {
    id: rl

    Control {
      id: rsiPanel

      contentItem: RowLayout {
        RsiPosition {
          id: cartesianPosition

          tags: ["X","Y","Z","A","B","C"]
          dimension: "mm"
        }
        RsiPosition {
          id: jointPosition

          tags: ["A1","A2","A3","A4","A5","A6"]
          dimension: "deg"
        }
      }

      background: Rectangle {
        color: "transparent"
        border{width: 1; color: Styles.background.dp12}
      }

      Label {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        leftPadding: 10; rightPadding: 10
        topPadding: 6; bottomPadding: 6

        background: Rectangle {
          color: Styles.background.dp01
          border{width: 1; color: Styles.background.dp12}
        }

        textFormat: Text.RichText
        text: "Robot Sensor Interface"

        color: Styles.foreground.medium
        font{pixelSize: 12}
      }
    }
  }
}
