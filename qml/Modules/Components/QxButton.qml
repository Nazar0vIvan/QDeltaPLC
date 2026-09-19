import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Button {
  id: control

  checkable: false

  leftPadding: Metrics.sp12
  rightPadding: Metrics.sp12
  topPadding: Metrics.sp8
  bottomPadding: Metrics.sp8

  background: Rectangle {
    radius: Metrics.r6
    color: control.checked ? Colors.background.dp04 : Colors.secondary.base
    border.color: control.checked ? Colors.foreground.high : Colors.secondary.dark
    opacity: enabled ? (control.pressed ? 0.9 : control.hovered ? 1.0 : 0.9) : 0.4
  }
  contentItem: Text {
    text: control.text
    color: control.checked ? Colors.foreground.high : Colors.background.dp00
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font: Fonts.body
  }
}
