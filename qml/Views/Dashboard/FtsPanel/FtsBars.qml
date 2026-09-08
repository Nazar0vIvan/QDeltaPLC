import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import QDelta.Backend 1.0 as Backend

Control {
  id: root

  required property Backend.DeviceRunner fts

  property alias title: header.text
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
      ListElement { tag: "Fx"; axis: "fx"; from: -1980.0; to: 1980.0 }
      ListElement { tag: "Fy"; axis: "fy"; from: -660.0;  to: 660.0 }
      ListElement { tag: "Fz"; axis: "fz"; from: -660.0;  to: 660.0 }
      ListElement { tag: "Tx"; axis: "tx"; from: -60.0;   to: 60.0 }
      ListElement { tag: "Ty"; axis: "ty"; from: -60.0;   to: 60.0 }
      ListElement { tag: "Tz"; axis: "tz"; from: -60.0;   to: 60.0 }
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

      value: root.fts && root.fts.data.streaming
             ? +Number(root.fts.data[model.axis]).toFixed(3)
             : 0
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
