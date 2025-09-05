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

  signal connectPressed()
  signal disconnectPressed()

  title: qsTr("Network")

  ColumnLayout {
    id: layout

    anchors.fill: parent
    anchors.topMargin: -20
    spacing: 10

    QxField {
      id: laField // local address

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PC IP :"

      QxTextField {
        id: la

        height: parent.height; width: root.fieldWidth
        text:"192.168.2.1"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField {
      id: lpField // local port

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


    QxField {
      id: paField // peer address

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PLC IP :"

      QxTextField {
        id: pa

        height: parent.height; width: root.fieldWidth
        text: "192.168.2.5"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField {
      id: pp // peer port

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "PLC Port :"

      QxTextField {
        id: ppText

        height: parent.height; width: root.fieldWidth
        text: "502"
        validator: IntValidator{ bottom: 0; top: 65535; }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField {
      id: statusField // connection status

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "Status :"

      Text {
        text: btnConnect.checked ?  "Connected" : "Disconnected"
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
        id: btnSubmit

        checkable: false
        text: "Submit"
        enabled: !btnConnect.enabled

        onClicked: {
          plcRunner.setConfig({
            localAddress: la.text,
            localPort:    Number(lp.text),
            peerAddress:  pa.text,
            peerPort:     Number(pp.text)
          })
        }
      }

      QxButton {
        id: btnConnect

        checkable: true
        enabled: false
        textOn: "Disconnect"
        textOff: "Connect"
        onTurnedOn: root.connectPressed()
        onTurnedOff: root.disconnectPressed()
      }
    }
  }
}
