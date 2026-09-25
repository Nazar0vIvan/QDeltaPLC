import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property int switchWidth: Metrics.sz36
  property int switchHeight: Metrics.sz20
  property bool plugged: false
  property bool displayOnly: false
  property alias labelText: label.text
  property alias tag: tag.text
  property alias isOn: sw.isOn

  signal clicked()

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
    QxSwitch {
      id: sw

      barWidth: root.switchWidth
      barHeight: root.switchHeight
      imageSource: "qrc:/pics/lock.svg"
      enabled: root.plugged && !root.displayOnly
      available: root.enabled
      displayOnly: root.displayOnly

      onClicked: root.clicked()
    }
    Text {
      id: tag

      color: root.enabled ? Colors.foreground.high : Colors.foreground.disabled
      font: Fonts.body
    }
  }
}
