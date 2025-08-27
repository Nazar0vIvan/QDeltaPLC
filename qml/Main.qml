import QtQuick 2.12
import QtQuick.Layouts
import QtQuick.Controls 2.12
import QtQuick.Controls.Basic

import Styles 1.0
import Components 1.0

Window {
  width: 800
  height: 480
  visible: true
  title: qsTr("QDeltaPLC")
  color: Styles.background.dp00

  ColumnLayout {
    id: rl

    anchors.fill: parent

    RowLayout {
      id: cl

      Layout.fillHeight: true;
      Layout.fillWidth: true; implicitWidth: 50
      Layout.alignment: Qt.AlignTop
      Layout.margins: 10

      GridView {
          id: ledsView

          readonly property int columns: 3
          readonly property int rows: 2
          readonly property int cellSpacing: 10

          clip: true
          interactive: false
          cellWidth: 100 + cellSpacing
          cellHeight: 200
          implicitWidth:  columns * cellWidth  + (columns - 1)*cellSpacing
          implicitHeight: rows    * cellHeight + (rows    - 1)*cellSpacing

          Layout.preferredWidth: implicitWidth
          Layout.preferredHeight: implicitHeight
          Layout.alignment: Qt.AlignTop

          model: ListModel {
              ListElement { label: "Mains"; color: "yellow"; w: 100; h: 200 }
              ListElement { label: "RUN";   color: "green";  w: 100; h: 200 }
              ListElement { label: "G2";    color: "green";  w: 100; h: 200 }
              ListElement { label: "R1";    color: "red";    w: 70;  h: 140 }
              ListElement { label: "R2";    color: "red";    w: 70;  h: 140 }
              ListElement { label: "R3";    color: "red";    w: 70;  h: 140 }
          }

          delegate: Item {
              width: ledsView.cellWidth + ledsView.cellSpacing
              height: ledsView.cellHeight

              QxLed {
                  width: model.w
                  height: model.h
                  labelText: model.label
                  ledColor: model.color
                  anchors.centerIn: parent
              }
          }
      }
      QxGroupBox {
        id: plc_network

        title: qsTr("Network")
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 300; Layout.preferredHeight: 400

        ColumnLayout {

          anchors.fill: parent
          anchors.topMargin: 20
          spacing: 20

          ListView {
            id: listView

            property int fieldHeight: 26
            property int fieldWidth: 100
            property int labelWidth: 60

            spacing: 10
            Layout.preferredHeight: 4 * fieldHeight + 3 * spacing
            Layout.fillWidth: true

            model: ListModel {
                ListElement { labelText: "PC IP :";    text: "192.168.1.1"; }
                ListElement { labelText: "PC Port :";  text: "3333"; }
                ListElement { labelText: "PLC IP :";   text: "192.168.1.5";}
                ListElement { labelText: "PLC Port :"; text: "2324"; }
            }

            delegate: QxField {
              width: listView.width
              height: listView.fieldHeight
              labelText: model.labelText
              labelWidth: listView.labelWidth

              TextField {
                  id: txtField

                  height: listView.fieldHeight
                  width: listView.fieldWidth
                  readOnly: false
                  text: model.text
                  color: Styles.foreground.high
                  background: Rectangle {
                      color: Styles.background.dp04
                  }
              }
            }
          }

          QxField {
            id: field

            labelText: "Status :"
            labelWidth: 60
            Layout.preferredHeight: 20
            Layout.fillWidth: true

            RowLayout {

              spacing: 4

              Text {
                text: "Disconnected"
                color: Styles.foreground.high
              }

              Rectangle {
                width: 20; height: 20
                radius: width / 2
                color: btn_connect.checked ? "limegreen" : "firebrick"
                border.color: Qt.darker(color, 1.4)
                border.width: 2

                Behavior on color {
                  ColorAnimation { duration: 250 }
                }
              }
            }


          }

          QxToggleButton {
            id: btn_connect

            textOn: "Disconnect"
            textOff: "Connect"
            onTurnedOn: console.log("Started")
            onTurnedOff: console.log("Stopped")
          }

          Item { Layout.fillHeight: true }
        }
      }
      Item { Layout.fillWidth: true }
    }
    Logger {
      id: logger

      Layout.fillHeight: true;
      Layout.fillWidth: true; implicitWidth: 50
      Layout.alignment: Qt.AlignTop
    }
  }
}
