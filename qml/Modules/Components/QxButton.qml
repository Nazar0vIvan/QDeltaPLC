import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Button {
  id: control

  checkable: false

  background: Rectangle {
    radius: 6
    color: control.checked ? Styles.background.dp04 : Styles.secondary.base
    border.color: control.checked ? Styles.foreground.high : Styles.secondary.dark
    opacity: enabled ? (control.pressed ? 0.9 : control.hovered ? 1.0 : 0.9) : 0.4
  }
  contentItem: Text {
    text: control.text
    color: control.checked ? Styles.foreground.high : Styles.background.dp00
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    font: Styles.fonts.body
  }
}
