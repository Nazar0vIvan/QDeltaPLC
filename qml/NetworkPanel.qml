import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 74

  title: qsTr("Network")

  ColumnLayout {
    id: layout

    anchors.fill: parent
    anchors.topMargin: -20
    spacing: 10

    QxField { // local address

      id: laField
      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PC IP :"

      QxTextField {
        id: la

        height: parent.height; width: root.fieldWidth
        text:"192.168.1.1"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField { // local port
      id: lpField

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PC Port :"

      QxTextField {
        id: lp

        height: parent.height; width: root.fieldWidth
        text: "3333"
        validator: IntValidator{ bottom: 0; top: 65535; }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField { // peer address

      id: paField
      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PLC IP :"

      QxTextField {
        id: pa

        height: parent.height; width: root.fieldWidth
        text: "192.168.1.2"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField { // peer port
      id: ppField

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PLC Port :"

      QxTextField {
        id: pp

        height: parent.height; width: root.fieldWidth
        text: "5051"
        validator: IntValidator{ bottom: 0; top: 65535; }
      }
    }

    QxField { // message
      id: msgField

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "Message"

      QxTextField {
        id: msg

        height: parent.height; width: root.fieldWidth
        validator: IntValidator{ bottom: 0; top: 65535; }
      }
    }

    QxField { // connection status
      id: statusField

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "Status :"

      Text {
        text: plcRunner && plcRunner.socketState === 3 ?  "Connected" : "Disconnected"
        color: btnConnect.checked ? Styles.minColor : Styles.maxColor
        anchors.verticalCenter: parent.verticalCenter
      }
    }

    Item { Layout.fillHeight: true } // spacer

    RowLayout {
      id: btnlayout

      Layout.fillWidth: true
      Layout.preferredHeight: 30

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
}
