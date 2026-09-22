import QtQuick

import Styles 1.0
import RoboCrap.Viewport3D 1.0 as Viewport3D

Item {
  id: root

  Rectangle {
    anchors.fill: parent

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

  Component.onCompleted: {
    if (!Viewport3D.OccController.ready
        && !Viewport3D.OccController.loading) {
      Viewport3D.OccController.loadRobot()
    }
  }
}
