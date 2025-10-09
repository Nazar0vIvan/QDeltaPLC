import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Control {
  id: root

  property int fieldHeight: 24
  property int fieldWidth: 120

  property string title: ""
  property alias isHeaderVisible: header.visible

  topPadding: isHeaderVisible ? 40 : 10; bottomPadding: 10
  leftPadding: 10; rightPadding: 10

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp12}
  }

  contentItem: ColumnLayout {
    id: cl

    spacing: 20

    QxField { // message
      id: msgField

      Layout.preferredHeight: root.fieldHeight
      labelText: "Message:"

      QxTextInput {
        id: msg

        width: root.fieldWidth
        height: root.fieldHeight
      }
    }

    RowLayout {
      id: rl

      QxButton {
        id: btnConnect

        checked: plcRunner && plcRunner.socketState === 3
        enabled: plcRunner && (
          plcRunner.socketState === 0 ||
          plcRunner.socketState === 3
        )
        text: checked ? "Disconnect" : "Connect"
        onClicked: {
          if (!plcRunner) return
          if (checked) {
              plcRunner.disconnectFromHost()
          } else {
            plcRunner.connectToHost({
              localAddress: la.text,
              localPort:    Number(lp.text),
              peerAddress:  pa.text,
              peerPort:     Number(pp.text)
            })
          }
        }
      }

      QxButton {
        id: btnSend

        checkable: false
        text: "Send"
        enabled: plcRunner && plcRunner.socketState === 3

        onClicked: {
          if (!msg.text) return
          plcRunner.writeMessage(msg.text)
        }
      }
    }
  }

  Control {
    id: header

    anchors.right: parent.right
    anchors.top: parent.top
    leftPadding: 10; rightPadding: 10
    topPadding: 6; bottomPadding: 6

    background: Rectangle {
      color: "transparent"
      border{width: 1; color: Styles.background.dp12}
    }

    contentItem: Text {
      text: root.title
      color: Styles.foreground.medium
    }
  }

}
