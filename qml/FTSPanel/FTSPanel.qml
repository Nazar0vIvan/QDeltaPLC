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
    spacing: 30

    Text {
      id: title

      text: qsTr("Schunk FTS Delta-IP68-SI-660-60")
      color: Styles.foreground.high
      font{pixelSize: 20; bold: true}
    }

    RowLayout {
      id: rl

      Layout.fillHeight: true
      Layout.fillWidth: true;

      NetworkUdp {
        id: networkUdpPanel

        title: qsTr("Network")
        Layout.preferredWidth: 300
        Layout.preferredHeight: 300
      }
    }
  }
}
