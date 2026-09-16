import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

ScrollView {
  id: root

  property int pageLeftPadding: Metrics.pageMargin
  property int pageRightPadding: Metrics.pageMargin
  property int pageTopPadding: Metrics.pageMargin
  property int pageBottomPadding: Metrics.pageMargin
  default property alias pageContent: layout.data

  spacing: Metrics.spacingMedium

  clip: true
  contentWidth: page.implicitWidth
  contentHeight: page.implicitHeight

  ScrollBar.horizontal.policy: ScrollBar.AsNeeded
  ScrollBar.vertical.policy: ScrollBar.AsNeeded

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
