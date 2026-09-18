import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

ScrollView {
  id: root

  property int pageLeftPadding: Metrics.sp20
  property int pageRightPadding: Metrics.sp20
  property int pageTopPadding: Metrics.sp20
  property int pageBottomPadding: Metrics.sp20
  default property alias pageContent: layout.data

  spacing: Metrics.sp12

  clip: true
  contentWidth: page.implicitWidth
  contentHeight: page.implicitHeight

  ScrollBar.horizontal.policy: ScrollBar.AsNeeded
  ScrollBar.vertical.policy: ScrollBar.AsNeeded

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Colors.background.dp04}
    radius: 4
  }

  Item {
    id: page

    width: root.contentWidth
    implicitWidth: layout.implicitWidth
                   + root.pageLeftPadding
                   + root.pageRightPadding
    implicitHeight: layout.implicitHeight
                    + root.pageTopPadding
                    + root.pageBottomPadding

    ColumnLayout {
      id: layout

      x: root.pageLeftPadding
      y: root.pageTopPadding
      spacing: root.spacing
    }
  }
}
