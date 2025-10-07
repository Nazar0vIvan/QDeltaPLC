import QtQuick
import QtQuick.Layouts

import Styles 1.0

import "DeltaPanel"
import "FTSPanel"

Item {
  id: root

  ColumnLayout {
    id: cl

    anchors{fill: parent; topMargin: 20}
    spacing: 20

    DeltaPanel {
      id: deltaPanel

      Layout.fillWidth: true
      Layout.leftMargin: 20
    }

    Rectangle {
      id: separator

      Layout.fillWidth: true
      Layout.preferredHeight: 1
      Layout.leftMargin: 20
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    FTSPanel {
      id: ftsPanel

      Layout.fillWidth: true
      Layout.leftMargin: 20
    }

    Item { Layout.fillHeight: true }

  }
}
