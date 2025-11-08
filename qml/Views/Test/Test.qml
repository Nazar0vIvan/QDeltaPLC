import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Rectangle {
  id: root

  property int fieldHeight: 23
  property int fieldWidth: 100

  color: Styles.background.dp00

  ColumnLayout {
    id: cl

    anchors.fill: parent
    anchors.margins: 20

    spacing: 20

    RowLayout {
      id: rl

      spacing: 20

      TextArea {
        id: ta

        Layout.fillHeight: true
        Layout.preferredWidth: 500
        color: Styles.foreground.high
        selectByMouse: true
        selectionColor: Styles.primary.transparent
        placeholderTextColor: Styles.foreground.high
        textFormat: TextEdit.RichText
        background: Rectangle {
          color: Styles.background.dp04
          border{ color: Styles.foreground.high; width: 1 }
        }
      }

      ColumnLayout {
        id: cl1

        Layout.fillHeight: true
        Layout.fillWidth: true
        spacing: 20

        QxField {
          id: ipocField

          labelWidth: 40
          Layout.preferredHeight: root.fieldHeight
          labelText: "IPOC:"

          QxTextInput {
            id: ipoc

            height: root.fieldHeight
            width: root.fieldWidth
          }
        }

        QxButton {
          id: readIPOC

          text: "test"

          onClicked: {
            rsiRunner.invoke("test")
          }
        }
        Item { Layout.fillHeight: true }
      }
    }
  }
}
