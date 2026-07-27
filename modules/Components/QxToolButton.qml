import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

ToolButton {
  id: root

  required property string imageSource
  property int radius: 4

  padding: 5
  opacity: enabled ? (pressed ? 0.9 : hovered ? 1.0 : 0.9) : 0.4

  contentItem: Image {
    fillMode: Image.PreserveAspectFit
    source: root.imageSource
    mipmap: true
    smooth: true
  }
  background: Rectangle {
    color: Styles.background.dp04
    radius: root.radius
    border {
      width: 1
      color: Styles.background.dp12
    }
  }
}
