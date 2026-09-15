import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property string title: ""
  default property alias panelContent: body.data

  spacing: UiMetrics.spacingMedium

  topPadding: header.implicitHeight + UiMetrics.spacingMedium
  bottomPadding: UiMetrics.panelPadding
  leftPadding: UiMetrics.panelPadding
  rightPadding: UiMetrics.panelPadding

  contentItem: ColumnLayout {
    id: body

    spacing: root.spacing
  }

  background: Rectangle {
    implicitWidth: header.implicitWidth
    color: "transparent"
    border {
      width: UiMetrics.borderWidth
      color: Styles.background.dp12
    }
  }

  Label {
    id: header

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }

    leftPadding: UiMetrics.panelPadding
    rightPadding: UiMetrics.panelPadding
    topPadding: UiMetrics.spacingXSmall
    bottomPadding: UiMetrics.spacingXSmall

    text: root.title
    textFormat: Text.RichText
    color: Styles.foreground.medium
    font: Styles.fonts.body

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: UiMetrics.borderWidth
        color: Styles.background.dp12
      }
    }
  }
}
