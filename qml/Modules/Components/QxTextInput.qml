import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

TextField {
  id: control

  property bool confirmed: false
  property alias placeholder: defaultTxt.text
  property int radius: 4

  leftPadding: 8
  color: Styles.foreground.high
  selectionColor: Styles.primary.highlight
  selectedTextColor: Styles.foreground.high
  selectByMouse: true
  readOnly: false
  verticalAlignment: TextInput.AlignVCenter

  background: Rectangle {
    color: "transparent"
    radius: control.radius
    border {
      width: control.readOnly ? 0 : 1
      color: control.activeFocus ? Styles.primary.base : control.confirmed ? "green" : Styles.background.dp12
    }
  }

  onFocusChanged: {
    if (focus) selectAll()
    control.confirmed = false
  }

  Text {
    id: defaultTxt

    anchors.fill: parent
    verticalAlignment: Text.AlignVCenter
    color: Styles.foreground.medium
    font: Styles.fonts.body
    visible: !(control.activeFocus || control.text)
  }
}
