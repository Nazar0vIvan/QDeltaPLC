import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

GroupBox {
  id: control

  label: Label {
    x: Metrics.spacingXLarge
    y: -height / 2
    leftPadding: Metrics.panelPadding
    rightPadding: Metrics.panelPadding
    topPadding: Metrics.spacingXSmall
    bottomPadding: Metrics.spacingXSmall
    textFormat: Text.RichText
    text: control.title
    color: Colors.foreground.high
    font: Fonts.body
    background: Rectangle {
      color: Colors.background.dp04
      border {
        width: Metrics.borderWidth
        color: Colors.foreground.high
      }
    }
  }
  background: Rectangle {
    border {
      width: Metrics.borderWidth
      color: Colors.foreground.high
    }
    color: Colors.background.dp00
  }
}
