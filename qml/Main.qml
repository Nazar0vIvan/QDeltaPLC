import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import "NavigationPanel"
import "Views/Dashboard"
import "Views/Network"

import Styles 1.0
import Components 1.0

ApplicationWindow {
  id: root

  property int navPanelWidth: 40

  width: 1000
  height: 800
  visible: true
  title: qsTr("RoboCrap")
  color: Colors.background.dp00

  RowLayout {
    id: rl

    anchors.fill: parent
    spacing: 0

    NavigationPanel {
      id: navPanel

      Layout.fillHeight: true
      Layout.preferredWidth: root.navPanelWidth

      color: Colors.background.dp00
    }

    Rectangle {
      id: separator

      Layout.preferredWidth: Metrics.separatorWidth
      Layout.fillHeight: true
      color: Colors.background.dp04
    }

    SplitView {
      id: sv

      Layout.fillWidth: true;
      Layout.fillHeight: true

      orientation: Qt.Vertical

      StackLayout {
        id: sl

        SplitView.fillWidth: true;
        SplitView.fillHeight: true

        currentIndex: navPanel.currentIndex

        Dashboard { id: dashboard }

        Item { id: cam }

        Network { id: network }
      }

      Logger {
        id: loggerPanel

        SplitView.fillWidth: true
        SplitView.preferredHeight: 100
        SplitView.minimumHeight: 50
      }
    }
  }
}
