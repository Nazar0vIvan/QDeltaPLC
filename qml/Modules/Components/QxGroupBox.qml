import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

GroupBox {
  id: control

  label: Label {
    x: UiMetrics.spacingXLarge
    y: -height / 2
    leftPadding: UiMetrics.panelPadding
    rightPadding: UiMetrics.panelPadding
    topPadding: UiMetrics.spacingXSmall
    bottomPadding: UiMetrics.spacingXSmall
    textFormat: Text.RichText
    text: control.title
    color: Styles.foreground.high
    font: Styles.fonts.body
    background: Rectangle {
      color: Styles.background.dp04
      border {
        width: UiMetrics.borderWidth
        color: Styles.foreground.high
      }
    }
  }
  background: Rectangle {
    border {
      width: UiMetrics.borderWidth
      color: Styles.foreground.high
    }
    color: Styles.background.dp00
  }
}
