import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  required property string title
  default property alias panelContent: contentLayout.data

  property double contentHorizontalMargin: 0
  property double contentVerticalMargin: 0
  property int radius: Metrics.r10

  spacing: Metrics.sp8

  topPadding: header.implicitHeight + root.contentVerticalMargin
  bottomPadding: root.contentVerticalMargin
  leftPadding: root.contentHorizontalMargin
  rightPadding: root.contentHorizontalMargin

  contentItem: RowLayout {
    id: contentLayout

    spacing: root.spacing
  }

  background: Rectangle {
    implicitWidth: header.implicitWidth
    color: "transparent"
    radius: root.radius
    border {
      width: Metrics.w1
      color: Colors.background.dp24
    }
  }

  Label {
    id: header

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }

    leftPadding: Metrics.sp16
    rightPadding: Metrics.sp16
    topPadding: Metrics.sp10
    bottomPadding: Metrics.sp10

    text: root.title
    color: Colors.foreground.high
    font: Fonts.subtitle

    background: Rectangle {
      color: Colors.background.dp03
      topLeftRadius: root.radius
      topRightRadius: root.radius
    }
  }
}
