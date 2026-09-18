pragma ComponentBehavior: Bound

import QtQuick

import Components 1.0
import Styles 1.0

QxPanel {
  id: root

  required property var tags
  property string dimension: ""

  opacity: enabled ? 1.0 : 0.5
  spacing: Metrics.sp12

  Repeater {
    model: root.tags.length

    delegate: RsiCoordinate {
      required property int index

      tag: root.tags[index]
    }
  }

  Counter {
    id: step

    dimension: root.dimension
  }
}
