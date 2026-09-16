import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property string title: ""
  default property alias panelContent: body.data

  spacing: Metrics.spacingMedium

  topPadding: header.implicitHeight + Metrics.spacingMedium
  bottomPadding: Metrics.panelPadding
  leftPadding: Metrics.panelPadding
  rightPadding: Metrics.panelPadding

  contentItem: ColumnLayout {
    id: body

    spacing: root.spacing
  }

  background: Rectangle {
    implicitWidth: header.implicitWidth
    color: "transparent"
    border {
      width: Metrics.borderWidth
      color: Colors.background.dp12
    }
  }

  Label {
    id: header

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }

    leftPadding: Metrics.panelPadding
    rightPadding: Metrics.panelPadding
    topPadding: Metrics.spacingXSmall
    bottomPadding: Metrics.spacingXSmall

    text: root.title
    textFormat: Text.RichText
    color: Colors.foreground.medium
    font: Fonts.body

    background: Rectangle {
      color: Colors.background.dp01
      border {
        width: Metrics.borderWidth
        color: Colors.background.dp12
      }
    }
  }
}
