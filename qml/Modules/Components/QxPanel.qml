import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property string title: ""
  default property alias panelContent: body.data

  spacing: Metrics.sp12

  topPadding: header.implicitHeight + Metrics.sp12
  bottomPadding: Metrics.sp10
  leftPadding: Metrics.sp10
  rightPadding: Metrics.sp10

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

    leftPadding: Metrics.sp10
    rightPadding: Metrics.sp10
    topPadding: Metrics.sp4
    bottomPadding: Metrics.sp4

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
