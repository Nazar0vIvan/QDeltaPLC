import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Components 1.0
import Styles 1.0

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

      QxComboBox {

          label: qsTr("Device")

          model: [
              "PLC AT332T",
              "PLC AS332T",
              "PLC AS228T"
          ]

          onActivated: index => {
              console.log(index, currentText)
          }
      }
    }

    QxPanel {
      title: "Connections"
    }

    Item { Layout.fillHeight: true }
  }
}
