import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root


  property int switchWidth: UiMetrics.indicatorSizeLarge
  property int switchHeight: UiMetrics.indicatorSizeMedium
  property bool plugged: false
  property bool displayOnly: false
  property alias labelText: label.text
  property alias tag: tag.text
  property alias isOn: sw.isOn

  signal clicked();

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  enabled: plugged && !displayOnly
  opacity: plugged ? 1.0 : 0.5

  RowLayout {
    id: rl

    spacing: UiMetrics.spacingSmall

    Text {
      id: label

      color: Styles.foreground.high
      font: Styles.fonts.body
    }
    QxSwitch {
      id: sw

      barWidth: root.switchWidth
      barHeight: root.switchHeight
      imageSource: "qrc:/pics/lock.svg"
      enabled: root.enabled
      displayOnly: root.displayOnly

      onClicked: root.clicked()
    }
    Text {
      id: tag

      color: Styles.foreground.high
      font: Styles.fonts.body
    }
  }
}
