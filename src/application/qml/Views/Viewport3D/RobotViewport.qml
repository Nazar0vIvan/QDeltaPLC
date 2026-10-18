import QtQuick

import Components 1.0
import Styles 1.0
import RoboCrap.Viewport3D 1.0 as Viewport3D

Item {
  id: root

  // Shows the busy indicator over the scene, in addition to the startup scene loading.
  property bool busy: false

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

  // The OCCT view is a native child window that covers Qt Quick items, and Windows cannot make
  // child windows transparent, so the indicator is a transparent top-level window (transient for
  // the main window). It is not stays-on-top, which would float it over every application.
  Window {
    id: loadingOverlay

    // Set once the robot is shown; closing the view clears `ready` again, which is not loading.
    property bool robotShown: false
    // On-screen viewport centre; the view window is positioned in the main window's client area.
    readonly property point viewportCenter: loadingOverlay.transientParent && container.window
        ? Qt.point(loadingOverlay.transientParent.x + container.window.x + container.window.width / 2,
                   loadingOverlay.transientParent.y + container.window.y + container.window.height / 2)
        : Qt.point(0, 0)

    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    color: "transparent"
    x: loadingOverlay.viewportCenter.x - loadingOverlay.width / 2
    y: loadingOverlay.viewportCenter.y - loadingOverlay.height / 2
    width: indicator.implicitWidth
    height: indicator.implicitHeight
    // While busy, and from startup until the robot is first shown (OccController.loading covers
    // only CAD parsing); only once the view window is laid out and the main window is active,
    // so the indicator appears at the centre and activation does not bring the main window over it.
    visible: (root.busy
              || !loadingOverlay.robotShown && Viewport3D.OccController.errorString.length === 0)
             && container.window?.height > 0 && (loadingOverlay.transientParent?.active ?? false)

    Connections {
      target: Viewport3D.OccController

      function onReadyChanged() { if (Viewport3D.OccController.ready) loadingOverlay.robotShown = true }
    }

    QxBusyIndicator {
      id: indicator

      anchors.fill: parent
      diameter: 48
      running: loadingOverlay.visible
    }
  }
}
