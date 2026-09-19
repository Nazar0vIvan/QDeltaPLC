import QtQuick

import Styles 1.0

Rectangle {
  id: root

  property bool isOn: false
  property color ledColor: "green"
  property real diameter: Metrics.sz20

  implicitWidth: root.diameter
  implicitHeight: root.diameter

  radius: Math.min(width, height) / 2

  color: root.isOn
         ? Qt.lighter(root.ledColor, 1.6)
         : Qt.darker(root.ledColor, 2.0)

  border {
    width: Metrics.w1
    color: Qt.darker(root.ledColor, 1.8)
  }

  Behavior on color {
    ColorAnimation {
      duration: Metrics.animationFast
    }
  }
}
