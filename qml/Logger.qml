import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2

import Styles 1.0

ScrollView {
  id: root

  ScrollBar.vertical.policy: ScrollBar.AlwaysOn

  TextArea {
    id: textArea

    width: root.availableWidth
    height: root.availableHeight
    color: Styles.foreground.high
    readOnly: true
    selectByMouse: true
    selectionColor: Styles.primary.transparent
    placeholderTextColor: Styles.foreground.high
    textFormat: TextEdit.RichText
    background: Rectangle {
      color: Styles.background.dp04
      border{ color: Styles.foreground.high; width: 1 }
    }

    Connections {
      target: logger
      function onLogAdded(message){
        const colors = {
            0: "red",
            1: "green",
            2: "white",
            3: "orange",
            4: "pink",
        }
        const col = colors[Number(message.type)] || Styles.foreground.high
        textArea.insert(textArea.length, `<span style="color:${col}">${message.text}</span><br/>`)
      }
    }
  }
}




