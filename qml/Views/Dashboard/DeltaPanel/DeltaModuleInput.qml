import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias tag: tag.text
  property int ledSize: Metrics.indicatorSizeSmall
  property bool isOn: false

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  opacity: enabled ? 1.0 : 0.5

  RowLayout {
    id: rl

    spacing: Metrics.spacingSmall

    Text {
      id: label

      color: Colors.foreground.high
      font: Fonts.body
    }
    QxLed {
      id: led

      Layout.alignment: Qt.AlignVCenter
      diameter: root.ledSize
      ledColor: Colors.minColor
      isOn: root.enabled && root.isOn
    }
    Text {
      id: tag

      color: Colors.foreground.high
      font: Fonts.body
    }
  }
}
