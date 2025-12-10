import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

Control {
  id: root

  required property var tags
  property string dimension: ""
  property alias title: header.text
  property int imageSize: 10

  topPadding: 40
  bottomPadding: 10
  leftPadding: 10
  rightPadding: 10

  opacity: enabled ? 1.0 : 0.5

  contentItem: ColumnLayout {
    spacing: 14

    ListView {
      id: lv

      implicitWidth: contentItem.childrenRect.width
      implicitHeight: contentItem.childrenRect.height
      spacing: 10

      interactive: false
      boundsBehavior: Flickable.StopAtBounds
      clip: true

      model: 6

      delegate: RsiCoordinate {
        tag: root.tags[index]
      }
    }

    Counter {
      id: step

      dimension: root.dimension
    }
  }

  background: Rectangle {
    color: "transparent"
    border {
      width: 1
      color: Styles.background.dp12
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top

    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    font: Styles.fonts.body
    textFormat: Text.RichText
    color: Styles.foreground.medium

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: 1
        color: Styles.background.dp12
      }
    }
  }
}
