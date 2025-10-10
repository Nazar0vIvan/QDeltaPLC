import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

Control {
  id: root

  required property var tags
  property string title: ""

  topPadding: 40; bottomPadding: 10
  leftPadding: 10; rightPadding: 10

  contentItem: ListView {
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
}
