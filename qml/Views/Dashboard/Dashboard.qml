import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import "DeltaPanel"
import "FtsPanel"
import "KukaPanel"

Item {
  id: root

  ColumnLayout {
    id: cl

    anchors {
      fill: parent
      topMargin: 40; bottomMargin: 20
      leftMargin: 20; rightMargin: 20
    }
    spacing: 20

    RowLayout {
      id: rl

      Layout.fillWidth: true
      spacing: 20

      DeltaPanel {
        id: deltaPanel

        title: 'PLC AS332T-A'
      }

      FtsPanel {
        id: ftsPanel

        title: qsTr("FTS Delta-IP68-SI-660-60")
        Layout.alignment: Qt.AlignTop
      }
    }

    Rectangle {
      id: separator2

      Layout.fillWidth: true
      Layout.preferredHeight: 1
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    KukaPanel {
      id: kukaPanel

      title: qsTr("Robot Sensor Interface")

      Layout.topMargin: 14
    }

    Item { Layout.fillHeight: true }

  }
}
