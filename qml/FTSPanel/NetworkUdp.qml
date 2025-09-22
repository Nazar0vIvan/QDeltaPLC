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
        text: "59152"
        validator: IntValidator{ bottom: 0; top: 65535; }

        onEditingFinished: btnSubmit.enabled = true
      }
    }

    QxField { // peer address

      id: paField
      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "FTS IP :"

      QxTextField {
        id: pa

        height: parent.height; width: root.fieldWidth
        text: "192.168.1.3"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }
      }
    }

    QxField { // peer port
    id: ppField

    Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
    labelWidth: root.labelWidth
    labelText: "FTS Port :"

    TextEdit {
      id: pp

      height: parent.height; width: root.fieldWidth
      color: Styles.foreground.high
      text: "49152"
      verticalAlignment: Qt.AlignVCenter
      readOnly: true
      selectByMouse: true
      selectionColor: Styles.primary.highlight
    }
  }

    QxField { // streaming status
      id: statusField

      Layout.fillWidth: true; Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "Status :"

      Text {
        text: ftsRunner && ftsRunner.socketState === 3 ?  "Streaming" : "Idle"
        color: text === "Streaming" ? Styles.minColor : Styles.maxColor
        anchors.verticalCenter: parent.verticalCenter
      }
    }

    Item { Layout.fillHeight: true } // spacer

    QxButton {
      id: btnStart

      Layout.preferredHeight: root.fieldHeight
      checked: ftsRunner && ftsRunner.socketState === 3
      enabled: ftsRunner && (
          ftsRunner.socketState === 0 ||
          ftsRunner.socketState === 3
      )
      text: checked ? "Stop" : "Start"
      onClicked: {
        if (!ftsRunner) return
        if (checked) {
            ftsRunner.stopStreaming()
        } else {
            ftsRunner.startStreaming()
        }
      }
    }
  }
}
