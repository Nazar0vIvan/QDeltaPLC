import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  Component.onCompleted: {
    console.log(
      "QxPanel loaded from:",
      Qt.resolvedUrl(".")
    )
  }

  required property string title
  default property alias content: cl.data

  padding: 1

  background: Rectangle {
    color: "red"
    border {width: 1; color: Styles.background.dp12}
  }

  contentItem: ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 0

    Label {
      id: header

      Layout.fillWidth: true
      Layout.preferredHeight: 28

      leftPadding: 10
      rightPadding: 10
      topPadding: 6
      bottomPadding: 6

      text: root.title
      textFormat: Text.RichText
      color: Styles.foreground.high
      font.pixelSize: 12

      background: Rectangle {
        color: Styles.background.dp06
        border{ width: 1; color: Styles.background.dp12 }
      }
    }
  }
}
