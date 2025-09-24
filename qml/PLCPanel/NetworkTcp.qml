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

  ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 10

    QxField { // local address

      id: laField
      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PC IP :"

      TextEdit {
        id: la

        height: parent.height; width: root.fieldWidth
        color: Styles.foreground.high
        text: "192.168.1.1"
        verticalAlignment: Qt.AlignVCenter
        readOnly: true
        selectByMouse: true
        selectionColor: Styles.primary.highlight
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

    Item{ Layout.fillHeight: true }

    RowLayout {
      id: btnlayout

      Layout.fillWidth: true
      Layout.preferredHeight: root.fieldHeight

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
    }
  }
}
