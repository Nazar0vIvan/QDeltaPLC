import QtQuick

import Styles 1.0

Rectangle {
  id: root

  property bool isOn: false
  property color ledColor: "green"

  implicitWidth: UiMetrics.ledSize
  implicitHeight: UiMetrics.ledSize

  radius: width / 2

  color: root.isOn
         ? Qt.lighter(root.ledColor, 1.6)
         : Qt.darker(root.ledColor, 2.0)

  border {
    width: 3
    color: Qt.darker(root.ledColor, 1.8)
  }

  Behavior on color {
    ColorAnimation {
      duration: 150
    }
  }
}
