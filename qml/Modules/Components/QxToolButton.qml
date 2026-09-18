import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

ToolButton {
  id: root

  required property string imageSource
  property int radius: Metrics.radiusSmall

  padding: Metrics.sp4
  opacity: enabled ? (pressed ? 0.9 : hovered ? 1.0 : 0.9) : 0.4

  contentItem: Image {
    fillMode: Image.PreserveAspectFit
    source: root.imageSource
    mipmap: true
    smooth: true
  }
  background: Rectangle {
    color: Colors.background.dp04
    radius: root.radius
    border {
      width: Metrics.borderWidth
      color: Colors.background.dp12
    }
  }
}
