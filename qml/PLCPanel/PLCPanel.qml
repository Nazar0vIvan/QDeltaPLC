import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ColumnLayout {
  id: rool

  spacing: 30

  Text {
    text: qsTr("PLC AS332T-A")
    color: Styles.foreground.high
    font{pixelSize: 20; bold: true}
  }

  RowLayout {
    id: rl

    Layout.fillWidth: true
    Layout.preferredHeight: childrenRect.height
    spacing: 20

    NetworkTcp {
      id: networkTcpPanel

      title: qsTr("Network")
      Layout.preferredWidth: 300
      Layout.preferredHeight: ledPanel.implicitHeight
      Layout.alignment: Qt.AlignTop
    }

    PLCControls {
      id: controlPanel

      title: qsTr("Controls")
      Layout.preferredWidth: 300;
      Layout.preferredHeight: ledPanel.implicitHeight
      Layout.alignment: Qt.AlignTop
    }

    PLCLeds {
      id: ledPanel

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight
      Layout.alignment: Qt.AlignTop
    }

    Item { Layout.fillWidth: true }
  }
}

