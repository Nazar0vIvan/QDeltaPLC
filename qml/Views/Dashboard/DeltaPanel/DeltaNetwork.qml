import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Control {
  id: root

  property int fieldHeight: 24
  property int fieldWidth: 112

  property alias title: header.text

  topPadding: 40
  bottomPadding: 10
  leftPadding: 10
  rightPadding: 10

  background: Rectangle {
    color: "transparent"
    border {
      width: 1
      color: Styles.background.dp12
    }
  }

  contentItem: ColumnLayout {

    spacing: 20

    RowLayout {
      id: rl1

      QxField {
        // message
        id: msgField

        Layout.preferredHeight: root.fieldHeight
        labelText: "Message:"

        QxTextInput {
          id: msg

          width: root.fieldWidth
          height: root.fieldHeight
        }
      }

      QxToolButton {
        id: btnSend

        // enabled: plcRunner && plcRunner.socketState === 3
        imageSource: "qrc:/assets/pics/send.svg"
        Layout.preferredWidth: 22
        Layout.preferredHeight: 22

        // onClicked: {
        //   if (!msg.text)
        //     return
        //   plcRunner.writeMessage(msg.text)
        // }
      }
    }

    RowLayout {
      id: rl2

      QxButton {
        id: btnConnect

        enabled: plcRunner
        checkable: true
        checked: plcRunner.socketState === 3
        text: plcRunner && checked ? "Disconnect" : "Connect"
        onClicked: {
          if (!plcRunner)
            return
          if (checked) {
            plcRunner.invoke("connectToHost")
          } else {
            plcRunner.invoke("disconnectFromHost")
          }
        }
      }
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: 1
        color: Styles.background.dp12
      }
    }

    textFormat: Text.RichText

    color: Styles.foreground.medium
    font {
      pixelSize: 12
    }
  }
}
