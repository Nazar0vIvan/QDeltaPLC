pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

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
      ListElement { label: "3D" }
      ListElement { label: "Dashboard" }
      ListElement { label: "Network" }
    }

    delegate: ItemDelegate {
      id: delegate

      required property int index
      required property string label

      width: ListView.view.width
      height: width

      text: delegate.label
      highlighted: ListView.isCurrentItem

      onClicked: lv.currentIndex = delegate.index
    }
  }
}
