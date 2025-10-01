import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 74

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding  +  cl.implicitHeight + bottomPadding

  ColumnLayout {
    id: cl

    spacing: 10

    QxField { // local address
      id: laField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "PC IP :"

      QxTextField {
        id: la

        height: root.fieldHeight
        width: root.fieldWidth
        text: "192.168.1.1"
        readOnly: true
      }
    }

    QxField { // local port
      id: lpField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "PC Port :"

      QxTextField {
        id: lp

        width: root.fieldWidth
        height: root.fieldHeight
        text: "3333"
        validator: IntValidator{ bottom: 0; top: 65535; }
      }
    }

    QxField { // peer address

      id: paField
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "PLC IP :"

      QxTextField {
        id: pa

        width: root.fieldWidth
        height: root.fieldHeight
        text: "192.168.1.2"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }
      }
    }

    QxField { // peer port
      id: ppField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "PLC Port :"

      QxTextField {
        id: pp

        width: root.fieldWidth
        height: root.fieldHeight
        text: "5051"
        validator: IntValidator{ bottom: 0; top: 65535; }
      }
    }

    QxField { // connection status
      id: statusField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "Status :"

      Text {
        text: plcRunner && plcRunner.socketState === 3 ?  "Connected" : "Disconnected"
        color: btnConnect.checked ? Styles.minColor : Styles.maxColor
        anchors.verticalCenter: parent.verticalCenter
      }
    }

    QxField { // message
      id: msgField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      slotWidth: root.fieldWidth
      labelWidth: root.labelWidth
      labelText: "Message:"

      QxTextField {
        id: msg

        width: root.fieldWidth
        height: root.fieldHeight
      }
    }

    Rectangle { // separator
      Layout.preferredHeight: 1
      Layout.fillWidth: true
      color: Styles.background.dp12
    }

    RowLayout {
      id: btn_rl

      QxButton {
        id: btnConnect

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
        id: btnSend

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
