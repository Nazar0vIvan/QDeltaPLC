import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import "NavigationPanel"
import "Views/Options"
import "Views/Dashboard"
import "Views/Test"

import Styles 1.0
import Components 1.0

Window {
  id: root

  property int navPanelWidth: 60

  width: 1300
  height: 1000
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


        SplitView.fillWidth: true; SplitView.fillHeight: true
        currentIndex: navPanel.currentIndex

        Dashboard { id: dashboard }

        Options { id: options }

        Test { id: test }
      }

      Logger {
        id: loggerPanel

        SplitView.fillWidth: true
        SplitView.preferredHeight: 100
      }
    }
  }
}
