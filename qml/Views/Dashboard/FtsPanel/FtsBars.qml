import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Control {
  id: root

  property alias title: header.text
  readonly property real countFactor: 1000000
  readonly property int viewWidth: 240
  readonly property int rowHeight: 20
  readonly property int barWidth: 160

  topPadding: 40; bottomPadding: 10
  leftPadding: 10; rightPadding: 10

  contentItem: ListView {
    id: lv

    spacing: 10
    implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentItem.childrenRect.height

    model: ListModel {
        ListElement { tag: "Fx"; from: -1980.0; to: 1980.0; idx: 3 }
        ListElement { tag: "Fy"; from: -660.0;  to: 660.0;  idx: 4 }
        ListElement { tag: "Fz"; from: -660.0;  to: 660.0;  idx: 5 }
        ListElement { tag: "Tx"; from: -60.0;   to: 60.0;   idx: 6 }
        ListElement { tag: "Ty"; from: -60.0;   to: 60.0;   idx: 7 }
        ListElement { tag: "Tz"; from: -60.0;   to: 60.0;   idx: 8 }
    }

    delegate: QxProgressBar {
      id: bar

      height: root.rowHeight
      width: implicitWidth
      barWidth: root.barWidth

      from: model.from
      to: model.to
      labelText: model.tag
      color: Styles.secondary.base

      value: ftsRunner.isStreaming && ftsRunner.lastReading.length !== 0 ?
            Number((ftsRunner.lastReading[model.idx])/root.countFactor).toFixed(3) : 0.000
    }
  }

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp12}
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    leftPadding: 10; rightPadding: 10
    topPadding: 6; bottomPadding: 6

    background: Rectangle {
      color: Styles.background.dp01
      border{width: 1; color: Styles.background.dp12}
    }

    textFormat: Text.RichText
    color: Styles.foreground.medium
    font{pixelSize: 12}
  }
}
