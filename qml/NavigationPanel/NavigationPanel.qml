import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Rectangle {
  id: root

  property int currentIndex: lv.currentIndex

  ListView {
    id: lv

    anchors{fill: parent; margins: 8}
    spacing: 0

    model: ListModel {
      ListElement { iconPath: "qrc:/assets/pics/dashboard.svg" }
      ListElement { iconPath: "qrc:/assets/pics/settings.svg" }
    }

    delegate: NavigationMenuDelegate {
      width: lv.width
      height: lv.width
      iconPath: model.iconPath
      selected: lv.currentIndex === index

      MouseArea{
        anchors.fill: parent
        onClicked: lv.currentIndex = index
      }
    }
    highlight: Rectangle {
      y: lv.currentItem ? lv.currentItem.y : 0
      width: lv.currentItem.width
      height: lv.currentItem.height
      radius: 4
      color: Styles.primary.base
      opacity: 0.5
      Behavior on y { NumberAnimation { duration: 100 } }
    }
  }
}
