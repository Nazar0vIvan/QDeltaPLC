import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

TextField {
  id: control

  leftPadding: 5
  color: control.readOnly ? Styles.foreground.disabled : Styles.foreground.high
  selectionColor: Styles.primary.highlight
  selectByMouse : true
  readOnly: false

  background: Rectangle {
    color: Styles.background.dp04
    radius: 4
    border {
      width: control.readOnly ? 0 : control.activeFocus ? 2 : control.hovered ? 0 : 1
      color: control.activeFocus ? Styles.primary.base : Styles.background.dp12
    }
  }

  onAccepted: { editingFinished(); focus = false }
  onFocusChanged: if(focus) selectAll()
}
