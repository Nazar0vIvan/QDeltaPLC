import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
  id: root

  required property string title
  default property alias content: cl.data

  property int frameWidth: 6

  padding: frameWidth

  background: Rectangle {
    color: "transparent"
    border {
      width: root.frameWidth
      color: "orange"
    }
  }

  contentItem: ColumnLayout {
    id: cl
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
      color: Styles.foreground.medium
      font.pixelSize: 12

      background: Rectangle {
        color: "red"
        border {
          width: 1
          color: Styles.background.dp12
        }
      }
    }
  }
}