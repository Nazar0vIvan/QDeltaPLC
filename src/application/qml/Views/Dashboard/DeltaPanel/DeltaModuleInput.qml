import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias tag: tag.text
  property int ledSize: Metrics.sz12
  property bool plugged: false
  property bool isOn: false

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  enabled: root.plugged

  RowLayout {
    id: rl

    spacing: Metrics.sp8

    Text {
      id: label

      color: root.enabled ? Colors.foreground.high : Colors.foreground.disabled
      font: Fonts.body
    }
    QxLed {
      id: led

      Layout.alignment: Qt.AlignVCenter
      diameter: root.ledSize
      ledColor: !root.enabled ? Colors.foreground.disabled
                : root.isOn ? Colors.minColor : Colors.maxColor
      isOn: root.enabled
    }
    Text {
      id: tag

      color: root.enabled ? Colors.foreground.high : Colors.foreground.disabled
      font: Fonts.body
    }
  }
}
