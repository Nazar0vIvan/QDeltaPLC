import QtQuick 2.12
import QtQuick.Layouts
import QtQuick.Controls 2.12
import QtQuick.Controls.Basic

import Styles 1.0
import Components 1.0

Window {
  width: 1000
  height: 600
  visible: true
  title: qsTr("QDeltaPLC")
  color: Styles.background.dp00

  ColumnLayout {
    id: rl

    anchors.fill: parent
    spacing: 0

    RowLayout {
      id: cl

      Layout.fillHeight: true; implicitHeight: 50
      Layout.fillWidth: true;
      Layout.alignment: Qt.AlignTop
      Layout.margins: 10

      GridView {
          id: ledsView

          readonly property int cols: 3
          readonly property int rows: 2
          readonly property int cellSpacing: 10

          clip: true
          interactive: false
          cellWidth: 100 + cellSpacing
          cellHeight: 200
          implicitWidth:  cols*cellWidth  + (cols-1)*cellSpacing
          implicitHeight: rows*cellHeight + (rows-1)*cellSpacing

          Layout.preferredWidth: implicitWidth
          Layout.preferredHeight: implicitHeight
          Layout.alignment: Qt.AlignTop
          Layout.topMargin: 20

          model: ListModel {
              ListElement { label: "Mains"; color: "yellow"; isOn: true;  w: 100; h: 200 }
              ListElement { label: "Y1.6";  color: "green";  isOn: false; w: 100; h: 200 }
              ListElement { label: "Y1.7";  color: "green";  isOn: false; w: 100; h: 200 }
              ListElement { label: "Y2.5";  color: "red";    isOn: false; w: 70;  h: 140 }
              ListElement { label: "Y2.6";  color: "red";    isOn: false; w: 70;  h: 140 }
              ListElement { label: "Y2.7";  color: "red";    isOn: false; w: 70;  h: 140 }
          }

          delegate: Item {
              width: ledsView.cellWidth + ledsView.cellSpacing
              height: ledsView.cellHeight

              QxLed {
                anchors.centerIn: parent
                width: model.w; height: model.h
                labelText: model.label
                ledColor: model.color
                isOn: model.isOn
              }
          }
      }

      QxGroupBox {
        id: gb_net

        property int fieldHeight: 28
        property int fieldWidth: 120
        property int labelWidth: 74

        title: qsTr("Network")
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 300
        Layout.preferredHeight: 370
        Layout.topMargin: 20

        ColumnLayout {
          id: gb_net_lyt

          anchors.fill: parent
          anchors.topMargin: -20
          spacing: 20

          ListView {
            id: listView_net

            spacing: 10
            Layout.preferredHeight: 4*gb_net.fieldHeight + 3*gb_net.spacing
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            model: ListModel {
                ListElement { labelText: "PC IP :";    text: "192.168.1.1"; }
                ListElement { labelText: "PC Port :";  text: "3333"; }
                ListElement { labelText: "PLC IP :";   text: "192.168.1.5";}
                ListElement { labelText: "PLC Port :"; text: "2324"; }
            }

            delegate: QxField {
              width: listView_net.width
              height: gb_net.fieldHeight
              labelText: model.labelText
              labelWidth: gb_net.labelWidth

              TextField {
                  height: gb_net.fieldHeight
                  width: gb_net.fieldWidth
                  readOnly: false
                  text: model.text
                  color: Styles.foreground.high
                  background: Rectangle {
                      color: Styles.background.dp04
                      radius: 4
                  }
              }
            }
          }

          QxField {
            labelText: "Status :"
            labelWidth: 74
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Text {
              text: btn_connect.checked ?  "Connected" : "Disconnected"
              color: btn_connect.checked ? Styles.minColor : Styles.maxColor
            }
          }

          Item { Layout.fillHeight: true }

          QxToggleButton {
            id: btn_connect

            Layout.alignment: Qt.AlignTop
            textOn: "Disconnect"
            textOff: "Connect"
            onTurnedOn: ledsView.model.setProperty(1, "isOn", true)
            onTurnedOff: ledsView.model.setProperty(1, "isOn", false)
          }

        }
      }

      QxGroupBox {
        id: gb_ctrl

        property int fieldHeight: 22
        property int fieldWidth: 42
        property int labelWidth: 28

        title: qsTr("Controls")

        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 300;
        Layout.preferredHeight: 370
        Layout.topMargin: 20

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
              labelWidth: gb_ctrl.labelWidth
              height: gb_ctrl.fieldHeight
              width: listView_Y1.width

              QxSwitch {
                text: model.tag
                width: gb_ctrl.fieldWidth
                height: gb_ctrl.fieldHeight
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
              labelWidth: gb_ctrl.labelWidth
              height: gb_ctrl.fieldHeight
              width: listView_Y2.width

              QxSwitch {
                text: model.tag
                width: gb_ctrl.fieldWidth
                height: gb_ctrl.fieldHeight
              }
            }
          }
        }


      }

      Item { Layout.fillWidth: true }
    }
    Logger {
      id: logger

      Layout.fillHeight: true; implicitHeight: 50
      Layout.fillWidth: true;
      Layout.alignment: Qt.AlignTop
    }
  }
}
