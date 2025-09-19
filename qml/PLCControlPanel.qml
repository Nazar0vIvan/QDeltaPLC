import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 0

    RowLayout {
      id: outputs

      property int swHeight: 22
      property int swWidth: 42
      property int swLabelWidth: 28

      Layout.preferredHeight: childrenRect.height
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignTop

      ListView {
        id: listView_Y1

        spacing: 6
        Layout.preferredHeight: childrenRect.height
        Layout.preferredWidth: 150

        header: Text {
          height: 26
          textFormat: Text.RichText
          text: 'AS16AP11<font color="orange">P</font>-A'
          font.weight: 600
          font.pixelSize: 12
          color: Styles.foreground.high
        }

        model: ListModel {
            ListElement { terminal: "Y1.0"; tag: "N/D" }
            ListElement { terminal: "Y1.1"; tag: "N/D" }
            ListElement { terminal: "Y1.2"; tag: "N/D" }
            ListElement { terminal: "Y1.3"; tag: "N/D" }
            ListElement { terminal: "Y1.4"; tag: "N/D" }
            ListElement { terminal: "Y1.5"; tag: "N/D" }
            ListElement { terminal: "Y1.6"; tag: "N/D" }
            ListElement { terminal: "Y1.7"; tag: "N/D" }
        }

        delegate: QxField {
          labelText: model.terminal
          labelWidth: outputs.swLabelWidth
          height: outputs.swHeight
          width: listView_Y1.width

          QxSwitch {
            text: model.tag
            width: outputs.swWidth
            height: outputs.swHeight
          }
        }
      }

      ListView {
        id: listView_Y2

        spacing: 6
        Layout.preferredHeight: childrenRect.height
        Layout.preferredWidth: 150

        header: Text {
          height: 26
          textFormat: Text.RichText
          text: 'AS16AP11<font color="orange">T</font>-A'
          font.weight: 600
          font.pixelSize: 12
          color: Styles.foreground.high
        }

        model: ListModel {
          ListElement { terminal: "Y2.0"; tag: "N/D" }
          ListElement { terminal: "Y2.1"; tag: "N/D" }
          ListElement { terminal: "Y2.2"; tag: "N/D" }
          ListElement { terminal: "Y2.3"; tag: "N/D" }
          ListElement { terminal: "Y2.4"; tag: "N/D" }
          ListElement { terminal: "Y2.5"; tag: "N/D" }
          ListElement { terminal: "Y2.6"; tag: "N/D" }
          ListElement { terminal: "Y2.7"; tag: "N/D" }
        }

        delegate: QxField {
          labelText: model.terminal
          labelWidth: outputs.swLabelWidth
          height: outputs.swHeight
          width: listView_Y2.width

          QxSwitch {
            text: model.tag
            width: outputs.swWidth
            height: outputs.swHeight
          }
        }
      }
    }

    RowLayout {
      id: msgCtrl

      property int msgHeight: 28
      property int msgWidth: 100

      Layout.preferredHeight: msgCtrl.msgHeight
      Layout.fillWidth: true
      spacing: 14

      QxField { // message
        id: msgField

        Layout.preferredHeight: msgCtrl.msgHeight
        Layout.preferredWidth: labelWidth + msg.width + spacing
        labelText: "Message:"

        QxTextField {
          id: msg

          height: parent.height; width: msgCtrl.msgWidth
          validator: IntValidator{ bottom: 0; top: 65535; }
        }
      }

      QxButton {
        id: btnSend

        Layout.preferredHeight: msgCtrl.msgHeight
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
