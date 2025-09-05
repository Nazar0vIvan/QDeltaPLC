import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  property int fieldHeight: 22
  property int fieldWidth: 42
  property int labelWidth: 28

  title: qsTr("Controls")

  RowLayout {

    anchors.fill: parent
    anchors.topMargin: -20

    ListView {
      id: listView_Y1

      spacing: 6
      Layout.fillHeight: true
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
        labelWidth: root.labelWidth
        height: root.fieldHeight
        width: listView_Y1.width

        QxSwitch {
          text: model.tag
          width: root.fieldWidth
          height: root.fieldHeight
        }
      }
    }

    ListView {
      id: listView_Y2

      spacing: 6
      Layout.fillHeight: true
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
        labelWidth: root.labelWidth
        height: root.fieldHeight
        width: listView_Y2.width

        QxSwitch {
          text: model.tag
          width: root.fieldWidth
          height: root.fieldHeight
        }
      }
    }
  }
}
