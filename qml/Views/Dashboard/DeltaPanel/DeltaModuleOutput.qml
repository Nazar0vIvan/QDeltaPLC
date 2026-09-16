import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root


  property int switchWidth: Metrics.indicatorSizeLarge
  property int switchHeight: Metrics.indicatorSizeMedium
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

    spacing: Metrics.spacingSmall

    Text {
      id: label

      color: Colors.foreground.high
      font: Fonts.body
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

      color: Colors.foreground.high
      font: Fonts.body
    }
  }
}
