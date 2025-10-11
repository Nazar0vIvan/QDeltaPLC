import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Button {
  id: control

  checkable: false
  opacity: enabled ? 1.0 : 0.4

  background: Rectangle {
    radius: 6
    color: control.checked ? Styles.background.dp04 : Styles.secondary.base
    border.color: control.checked ? Styles.foreground.high : Styles.secondary.dark
    opacity: control.pressed ? 0.9 : control.hovered ? 1.0 : 0.95
  }
  contentItem: Text {
    anchors.fill: parent
    text: control.text
    color: control.checked ? Styles.foreground.high : Styles.background.dp00
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
}
