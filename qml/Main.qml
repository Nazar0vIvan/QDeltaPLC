import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import "NavigationPanel"
import "MenuBar"
import "Views/Dashboard"
import "Views/Network"
import "Views/Viewport3D"

import Styles 1.0
import Components 1.0

ApplicationWindow {
  id: root

  property int navPanelWidth: 44

  width: 1000
  height: 800
  visible: true
  title: qsTr("RoboCrap")
  color: Colors.background.dp00

  menuBar: MainMenuBar {
    id: mainMenuBar
  }

  RowLayout {
    id: rl

    anchors.fill: parent
    spacing: 0

    NavigationPanel {
      id: navPanel

      Layout.fillHeight: true
      Layout.preferredWidth: root.navPanelWidth
      Layout.leftMargin: Metrics.sp4
      Layout.rightMargin: Metrics.sp0
      Layout.topMargin: Metrics.sp4
      Layout.bottomMargin: Metrics.sp4
    }

    SplitView {
      id: sv

      Layout.fillWidth: true;
      Layout.fillHeight: true
      Layout.margins: Metrics.sp4

      orientation: Qt.Vertical

      StackLayout {
        id: sl

        SplitView.fillWidth: true;
        SplitView.fillHeight: true

        currentIndex: navPanel.currentIndex

        RobotViewport { id: viewport }

        Dashboard { id: dashboard }

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
