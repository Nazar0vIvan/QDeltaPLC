pragma ComponentBehavior: Bound

import QtQuick

import Styles 1.0
import Components 1.0

import RoboCrap.Backend 1.0 as Backend

QxPanel {
  id: root

  required property Backend.DeviceRunner fts

  readonly property int barWidth: 160

  spacing: Metrics.sp8

  Repeater {
    model: ListModel {
      ListElement { tag: "Fx"; axis: "fx"; minValue: -1980.0; maxValue: 1980.0 }
      ListElement { tag: "Fy"; axis: "fy"; minValue: -660.0; maxValue: 660.0 }
      ListElement { tag: "Fz"; axis: "fz"; minValue: -660.0; maxValue: 660.0 }
      ListElement { tag: "Tx"; axis: "tx"; minValue: -60.0; maxValue: 60.0 }
      ListElement { tag: "Ty"; axis: "ty"; minValue: -60.0; maxValue: 60.0 }
      ListElement { tag: "Tz"; axis: "tz"; minValue: -60.0; maxValue: 60.0 }
    }

    delegate: QxProgressBar {
      id: bar

      required property string tag
      required property string axis
      required property real minValue
      required property real maxValue

      barWidth: root.barWidth
      from: bar.minValue
      to: bar.maxValue
      labelText: bar.tag
      color: Colors.secondary.base
      value: root.fts && root.fts.data.streaming
             ? +Number(root.fts.data[bar.axis]).toFixed(3)
             : 0
    }
  }
}
