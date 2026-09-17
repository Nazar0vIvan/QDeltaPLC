pragma ComponentBehavior: Bound

import QtQuick

import Styles 1.0

Rectangle {
  id: root

  property int currentIndex: lv.currentIndex

  ListView {
    id: lv

    highlightMoveDuration: 0

    anchors {
      fill: parent
      margins: Metrics.spacingXSmall
    }
    spacing: Metrics.spacingSmall

    model: ListModel {
      ListElement { sourcePath: "qrc:/pics/dashboard.svg" }
      ListElement { sourcePath: "qrc:/pics/3d.svg" }
      ListElement { sourcePath: "qrc:/pics/network.svg" }
    }

    delegate: NavigationMenuDelegate {
      id: delegate

      required property int index
      required property string sourcePath

      width: ListView.view.width
      height: width

      iconPath: delegate.sourcePath
      selected: ListView.isCurrentItem

      onClicked: lv.currentIndex = delegate.index
    }
  }
}
