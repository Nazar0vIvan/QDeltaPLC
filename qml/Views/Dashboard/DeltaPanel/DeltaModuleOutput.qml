import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property int switchWidth: 20
  property int switchHeight: 10
  property bool plugged: false
  property bool displayonly: false
  property alias labelText: label.text
  property alias tag: tag.text
  property alias isOn: sw.isOn

  signal clicked();

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  enabled: plugged && !displayonly
  opacity: plugged ? 1.0 : 0.5

  RowLayout {
    id: rl

    Text {
      id: label

      color: Styles.foreground.high

    }
    QxSwitch {
      id: sw

      Layout.preferredWidth: root.switchWidth
      Layout.preferredHeight: root.switchHeight
      imageSource: "qrc:/assets/pics/lock.svg"
      enabled: root.enabled
      displayonly: root.displayonly

      onClicked: root.clicked()
    }
    Text {
      id: tag

      color: Styles.foreground.high
    }
  }
}
