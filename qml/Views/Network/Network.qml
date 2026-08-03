import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Components 1.0

Item  {
  id: root

  ColumnLayout {

    anchors.fill: parent
    anchors.margins: 14

    spacing: 20

    Label {
      Layout.preferredHeight: 30
      text: qsTr("Network")
      font: Styles.fonts.title
      color: Styles.foreground.high
    }

    QxPanel {
      title: "Configuration"

      Rectangle {
        Layout.preferredWidth: 300
        Layout.preferredHeight: 100
        color: "green"
      }

      Rectangle {
        Layout.preferredWidth: 300
        Layout.preferredHeight: 100
        color: "blue"
      }
    }

    QxPanel {
      title: "Connections"
    }

    Item { Layout.fillHeight: true }
  }
}
