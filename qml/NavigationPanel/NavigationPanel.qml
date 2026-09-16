pragma ComponentBehavior: Bound

import QtQuick
import Styles 1.0

Rectangle {
  id: root

  property int currentIndex: lv.currentIndex

  ListView {
    id: lv

    anchors {
      fill: parent
      margins: Metrics.spacingSmall
    }
    spacing: 0

    model: ListModel {
      ListElement { sourcePath: "qrc:/pics/dashboard.svg" }
      // ListElement { sourcePath: "qrc:/pics/settings.svg" }
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

      MouseArea {
        anchors.fill: parent
        onClicked: lv.currentIndex = delegate.index
      }
    }
    highlight: Rectangle {
      y: lv.currentItem ? lv.currentItem.y : 0
      width: lv.width
      height: lv.width
      radius: Metrics.radiusSmall
      color: Colors.primary.base
      opacity: 0.5
      Behavior on y {
        NumberAnimation { duration: Metrics.animationFast }
      }
    }
  }
}
