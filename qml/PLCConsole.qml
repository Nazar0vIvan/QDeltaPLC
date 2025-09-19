import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2

import Styles 1.0
import Components 1.0

Rectangle {
  id: root

  color: "transparent"

  ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 10

    Text {
      text: qsTr("PLC AS332T-A")
      color: Styles.foreground.high
      font{pixelSize: 20; bold: true}
    }

    RowLayout {
      id: rl

      Layout.fillWidth: true

      LedPanel {
        id: ledPanel

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitHeight
      }

      NetworkTcpPanel {
        id: networkTcpPanel

        title: qsTr("Network")
        Layout.preferredWidth: 300
        Layout.preferredHeight: ledPanel.height
      }

      PLCControlPanel {
        id: controlPanel

        title: qsTr("Controls")
        Layout.preferredWidth: 300;
        Layout.preferredHeight: ledPanel.height
      }

      Item { Layout.fillWidth: true }
    }
  }
}
