import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0
import RoboCrap.Viewport3D 1.0 as Viewport3D

Item {
  id: root

  RowLayout {
    anchors.fill: parent
    spacing: Metrics.sp12

    ColumnLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 0

      spacing: Metrics.sp8

      ToolBar {
        id: viewportToolBar

        Layout.fillWidth: true

        padding: Metrics.sp4

        background: Rectangle {
          color: Colors.background.dp01

          border {
            width: Metrics.w1
            color: Colors.background.dp12
          }
        }

        RowLayout {
          anchors.fill: parent
          spacing: Metrics.sp4

          QxToolButton {
            imageSize: Metrics.sz22
            imageSource: "qrc:/pics/plane.svg"
          }

          QxToolButton {
            imageSize: Metrics.sz22
            imageSource: "qrc:/pics/cylinder.svg"
          }

          Item {
            Layout.fillWidth: true
          }
        }
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 0
        Layout.minimumHeight: 0

        color: Colors.background.dp01
        border.color: Colors.background.dp12
        border.width: Metrics.w1

        WindowContainer {
          id: container

          anchors.fill: parent
          anchors.margins: Metrics.w1

          focus: false
          activeFocusOnTab: false
          window: Viewport3D.OccController.viewWindow

          Component.onDestruction: {
            Viewport3D.OccController.detachViewWindow(container.window)
            container.window = null
          }
        }
      }
    }

    ScrollView {
      id: controls

      Layout.preferredWidth: panel.implicitWidth + Metrics.sp20
      Layout.maximumWidth: root.width * 0.5
      Layout.minimumWidth: 0
      Layout.fillHeight: true

      clip: true

      ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

      RobotKinematicsPanel {
        id: panel

        width: controls.availableWidth
      }
    }
  }

  Component.onCompleted: {
    if (!Viewport3D.OccController.ready
        && !Viewport3D.OccController.loading) {
      Viewport3D.OccController.loadRobot()
    }
  }
}
