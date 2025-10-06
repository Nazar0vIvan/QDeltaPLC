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

      QxTextInput {
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
      labelWidth: root.labelWidth
      labelText: "PC Port :"

      QxTextInput {
        id: lp

        height: root.fieldHeight
        width: root.fieldWidth
        text: "59152"
        validator: IntValidator{ bottom: 0; top: 65535; }
      }
    }

    QxField { // peer address

      id: paField
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "FTS IP :"

      QxTextInput {
        id: pa

        height: root.fieldHeight
        width: root.fieldWidth
        text: "192.168.1.3"
        validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }
      }
    }

    QxField { // peer port
    id: ppField

    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: root.fieldHeight
    labelWidth: root.labelWidth
    labelText: "FTS Port :"

    QxTextInput {
      id: pp

      height: root.fieldHeight
      width: root.fieldWidth
      text: "49152"
      readOnly: true
    }
  }

    QxField { // streaming status
      id: statusField

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: root.fieldHeight
      labelWidth: root.labelWidth
      labelText: "Status :"

      Text {
        text: ftsRunner && ftsRunner.isStreaming ?  "Streaming" : "Idle"
        color: text === "Streaming" ? Styles.minColor : Styles.maxColor
        anchors.verticalCenter: parent.verticalCenter
      }
    }

    Rectangle { // separator
      Layout.preferredHeight: 1
      Layout.fillWidth: true
      color: Styles.background.dp12
    }

    QxButton {
      id: btnStart

      Layout.preferredHeight: root.fieldHeight
      checked: ftsRunner && ftsRunner.isStreaming
      enabled: ftsRunner
      text: checked ? "Stop" : "Start"
      onClicked: {
        if (!ftsRunner) return
        if (checked) {
            ftsRunner.stopStreaming()
        } else {
            ftsRunner.startStreaming({
              peerAddress:  pa.text,
              peerPort:     Number(pp.text)
            })
        }
      }
    }
  }
}
