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

      LedPanel {
        id: ledPanel

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 20
      }

      NetworkPanel {
        id: networkPanel

        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 300
        Layout.preferredHeight: 370
        Layout.topMargin: 20

        onConnectPressed: plcSocket.slotConnect();
        onDisconnectPressed: plcSocke.slotDisconnect();
      }

      ControlPanel {
        id: controlPanel

        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 300;
        Layout.preferredHeight: 370
        Layout.topMargin: 20
      }

      Item { Layout.fillWidth: true }
    }
    Logger {
      Layout.fillHeight: true; implicitHeight: 50
      Layout.fillWidth: true;
      Layout.alignment: Qt.AlignTop
    }
  }
}
