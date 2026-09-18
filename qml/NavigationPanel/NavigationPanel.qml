pragma ComponentBehavior: Bound

import QtQuick

import Styles 1.0

Rectangle {
  id: root

  property int currentIndex: lv.currentIndex

  border{width: 1; color: Colors.background.dp04}
  color: Colors.background.dp01
  radius: 4

  ListView {
    id: lv

    highlightMoveDuration: 0

    anchors {
      fill: parent
      margins: Metrics.sp6
    }
    spacing: Metrics.sp8

    model: ListModel {
      ListElement { sourcePath: "qrc:/pics/3d.svg" }
      ListElement { sourcePath: "qrc:/pics/dashboard.svg" }
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
