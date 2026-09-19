import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Styles 1.0

ScrollView {
  id: root

  ScrollBar.vertical.policy: ScrollBar.AlwaysOn

  TextArea {
    id: textArea

    width: root.availableWidth
    height: root.availableHeight
    color: Colors.foreground.high
    readOnly: true
    selectByMouse: true
    selectionColor: Colors.primary.transparent
    placeholderTextColor: Colors.foreground.high
    textFormat: TextEdit.RichText
    background: Rectangle {
      color: Colors.background.dp04
      border {
        color: Colors.foreground.high
        width: Metrics.w1
      }
    }

    Connections {
      // logger is the intentional context property registered in main.cpp.
      // qmllint disable unqualified
      target: logger
      // qmllint enable unqualified
      function onLogAdded(message){
        const colors = {
            0: "#ff3a31",
            1: "#4ed964",
            2: "white",
            3: "#fdb050",
            4: "pink",
        }
        const col = colors[Number(message.type)] || Colors.foreground.high
        textArea.insert(textArea.length, `<span style="color:${col}">${message.text}</span><br/>`)
        root.ScrollBar.vertical.position = 1.0 - root.ScrollBar.vertical.size
      }
    }
  }
}




