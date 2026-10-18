import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

// Ring of rounded bars fading behind a dark head. The ring turns with an Animator,
// which runs on the render thread and keeps turning while the GUI thread is busy.
BusyIndicator {
  id: root

  property color color: Colors.secondary.dark
  property real diameter: Metrics.sz36
  property int barCount: 12
  property int period: 1000 // ms per revolution

  implicitWidth: root.diameter
  implicitHeight: root.diameter
  padding: Metrics.sp0

  contentItem: Item {
    id: ring

    readonly property real size: Math.min(ring.width, ring.height)
    readonly property real barWidth: ring.size * 0.09
    readonly property real barLength: ring.size * 0.26
    readonly property real fade: 0.85 // Opacity lost from the head to the last bar.

    opacity: root.running ? 1 : 0

    Behavior on opacity {
      OpacityAnimator {
        duration: Metrics.animationFast
      }
    }

    RotationAnimator on rotation {
      from: 0
      to: 360
      duration: root.period
      loops: Animation.Infinite
      running: root.running && root.visible
    }

    Repeater {
      model: root.barCount

      delegate: Rectangle {
        id: bar

        required property int index
        // The ring turns clockwise, so the last bar leads and lower indices trail it.
        readonly property int lag: root.barCount - 1 - bar.index

        x: (ring.width - bar.width) / 2
        y: (ring.height - ring.size) / 2
        width: ring.barWidth
        height: ring.barLength
        radius: bar.width / 2
        color: root.color
        opacity: 1 - ring.fade * bar.lag / root.barCount

        transform: Rotation {
          origin.x: bar.width / 2
          origin.y: ring.height / 2 - bar.y
          angle: bar.index * 360 / root.barCount
        }
      }
    }
  }
}
