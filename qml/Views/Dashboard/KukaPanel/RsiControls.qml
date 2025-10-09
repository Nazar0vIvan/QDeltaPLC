import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

Control {
  id: root

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp12}
  }

  contentItem: RowLayout {
    spacing: 30

    ListView {
      id: xyz_lb

      implicitWidth: contentItem.childrenRect.width
      implicitHeight: contentItem.childrenRect.height
      spacing: 10

      interactive: false
      boundsBehavior: Flickable.StopAtBounds
      clip: true

      model: ListModel {
        ListElement { tag: "X" }
        ListElement { tag: "Y" }
        ListElement { tag: "Z" }
        ListElement { tag: "A" }
        ListElement { tag: "B" }
        ListElement { tag: "C" }
      }

      delegate: RsiCoordinate {
        tag: model.tag
      }
    }
  }
}
