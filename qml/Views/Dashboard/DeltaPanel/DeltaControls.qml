import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  property int fieldHeight: 24
  property int fieldWidth: 120

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  ColumnLayout {
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
        id: connectBnt

        Layout.preferredHeight: root.fieldHeight
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
        id: sendBtn

        Layout.preferredHeight: root.fieldHeight
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
}
