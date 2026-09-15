import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

ScrollView {
  id: root

  property int pageLeftPadding: UiMetrics.pageMargin
  property int pageRightPadding: UiMetrics.pageMargin
  property int pageTopPadding: UiMetrics.pageMargin
  property int pageBottomPadding: UiMetrics.pageMargin
  default property alias pageContent: layout.data

  spacing: UiMetrics.spacingMedium

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
