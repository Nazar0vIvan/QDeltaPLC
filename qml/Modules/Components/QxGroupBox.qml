import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

GroupBox {
  id: control

  label: Label {
    x: Metrics.sp24
    y: -height / 2
    leftPadding: Metrics.sp10
    rightPadding: Metrics.sp10
    topPadding: Metrics.sp4
    bottomPadding: Metrics.sp4
    textFormat: Text.RichText
    text: control.title
    color: Colors.foreground.high
    font: Fonts.body
    background: Rectangle {
      color: Colors.background.dp04
      border {
        width: Metrics.w1
        color: Colors.foreground.high
      }
    }
  }
  background: Rectangle {
    border {
      width: Metrics.w1
      color: Colors.foreground.high
    }
    color: Colors.background.dp00
  }
}
