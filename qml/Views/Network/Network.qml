import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Components 1.0
import Styles 1.0

Item  {
  id: root

  implicitWidth: 700
  implicitHeight: 500

  ColumnLayout {
    id: contentLayout

    anchors.fill: parent
    anchors.margins: 14

    spacing: 30

    Label {
      Layout.preferredHeight: 10
      text: qsTr("Network")
      font: Styles.fonts.title
      color: Styles.foreground.high
    }

    QxPanel {
      id: configPanel

      title: "Configuration"

      Rectangle {
        Layout.preferredWidth: 300
        Layout.preferredHeight: 100
        color: "transparent"
      }
    }

    QxPanel {
      id: connePanel

      title: "Connections"

      Rectangle {
        Layout.preferredWidth: 300
        Layout.preferredHeight: 100
        color: "transparent"
      }
    }

    Item { Layout.fillHeight: true }
  }
}
