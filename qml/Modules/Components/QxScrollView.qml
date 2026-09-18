import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

ScrollView {
  id: root

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
    implicitWidth: layout.implicitWidth + 2 * Metrics.sp20
    implicitHeight: layout.implicitHeight + 2 * Metrics.sp20

    ColumnLayout {
      id: layout

      x: Metrics.sp20
      y: Metrics.sp20
      spacing: root.spacing
    }
  }
}
