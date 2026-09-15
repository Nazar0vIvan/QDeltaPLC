import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias tag: tag.text
  property int ledSize: UiMetrics.indicatorSizeSmall
  property bool isOn: false

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  opacity: enabled ? 1.0 : 0.5

  RowLayout {
    id: rl

    spacing: UiMetrics.spacingSmall

    Text {
      id: label

      color: Styles.foreground.high
      font: Styles.fonts.body
    }
    QxLed {
      id: led

      Layout.alignment: Qt.AlignVCenter
      diameter: root.ledSize
      ledColor: Styles.minColor
      isOn: root.enabled && root.isOn
    }
    Text {
      id: tag

      color: Styles.foreground.high
      font: Styles.fonts.body
    }
  }
}
