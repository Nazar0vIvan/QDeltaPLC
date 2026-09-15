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
  property int radius: UiMetrics.radiusLarge

  spacing: UiMetrics.spacingSmall

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
      width: UiMetrics.borderWidth
      color: Styles.background.dp24
    }
  }

  Label {
    id: header

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }

    leftPadding: UiMetrics.spacingLarge
    rightPadding: UiMetrics.spacingLarge
    topPadding: UiMetrics.panelPadding
    bottomPadding: UiMetrics.panelPadding

    text: root.title
    color: Styles.foreground.high
    font: Styles.fonts.subtitle

    background: Rectangle {
      color: Styles.background.dp03
      topLeftRadius: root.radius
      topRightRadius: root.radius
    }
  }
}
