import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Styles 1.0

GroupBox {
  id: control

  label: Label {
      x: 20; y: -height/2
      leftPadding: 10; rightPadding: 10
      topPadding: 6; bottomPadding: 6
      text: control.title
      color: Styles.foreground.high
      background: Rectangle {
        color: Styles.background.dp04
        border{ width: 1; color: Styles.foreground.high }
      }
  }
  background: Rectangle {
    border{ width: 1; color: Styles.foreground.high }
    color: Styles.background.dp00
  }
}
