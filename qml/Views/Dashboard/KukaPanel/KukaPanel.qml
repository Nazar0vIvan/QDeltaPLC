import QtQuick
import QtQuick.Layouts

import Styles 1.0

import "DeltaPanel"
import "FTSPanel"

Item {
  id: root

  ColumnLayout {
    id: cl

    anchors{ fill: parent; margins: 20 }
    spacing: 20

    RowLayout {
      id: rl

      Layout.fillWidth: true
      spacing: 20

      DeltaPanel { id: deltaPanel }

      Rectangle {
        id: separator1

        Layout.preferredWidth: 2
        Layout.preferredHeight: deltaPanel.height
        color: Styles.background.dp04
      }

      FTSPanel {
        id: ftsPanel
      }

      Item { Layout.fillWidth: true }
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

    Item { Layout.fillHeight: true }

  }
}
