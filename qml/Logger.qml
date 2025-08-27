import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2

import Styles 1.0

TextArea {
  id: textArea

  color: Styles.foreground.high
  readOnly: true
  selectByMouse: true
  selectionColor: Styles.primary.transparent
  background: Rectangle {
    color: Styles.background.dp04
    border{color: Styles.foreground.high; width: 1}
  }
}


