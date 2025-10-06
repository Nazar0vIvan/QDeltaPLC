import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import "DeltaPanel"
import "FTSPanel"
import "NavigationPanel"
import "Views/Options"
import "Views/Dashboard"

import Styles 1.0
import Components 1.0

Window {
  id: root

  property int navPanelWidth: 60

  width: 1000
  height: 900
  visible: true
  title: qsTr("QDeltaPLC")
  color: Styles.background.dp00

  RowLayout {
    id: rl

    anchors.fill: parent
    spacing: 0

    NavigationPanel {
      id: navPanel

      Layout.fillHeight: true
      Layout.preferredWidth: root.navPanelWidth

      color: Styles.background.dp00
    }

    Rectangle {
      id: separator

      Layout.preferredWidth: 2
      Layout.fillHeight: true
      color: Styles.background.dp04
    }

    SplitView {
      id: sv

      Layout.fillWidth: true; Layout.fillHeight: true
      orientation: Qt.Vertical

      StackLayout {
        id: sl

        SplitView.fillHeight: true; SplitView.fillWidth: true
        currentIndex: navPanel.currentIndex

        Dashboard { id: dashboard }

        Options { id: settingsView }
      }

      Logger {
        id: loggerPanel

        SplitView.fillWidth: true
        SplitView.preferredHeight: 100
      }
    }
  }
}

/*
  SplitView {
    id: sv

    orientation: Qt.Vertical
    anchors.fill: parent
    anchors.topMargin: 10

    ColumnLayout {
      id: cl

      SplitView.fillWidth: true
      SplitView.fillHeight: true
      spacing: 20

      DeltaPanel {
        id: deltaPanel

        Layout.fillWidth: true
        Layout.leftMargin: 20
      }

      Rectangle {
        id: separator

        Layout.fillWidth: true
        Layout.preferredHeight: 2
        color: Styles.background.dp04
      }

      FTSPanel {
        id: ftsPanel

        Layout.fillWidth: true
        Layout.leftMargin: 20
      }

      Item { Layout.fillHeight: true }
    }

    Logger {
      id: loggerPanel

      SplitView.fillWidth: true
      SplitView.preferredHeight: 100
    }
  }
}
*/
