import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias tag: tag.text
  property int switchWidth: 20
  property int switchHeight: 10
  property bool isOn: false

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    Text {
      id: label

      color: Styles.foreground.high

    }
    QxSwitch {
      id: led

      Layout.preferredWidth: root.switchWidth
      Layout.preferredHeight: root.switchHeight
    }
    Text {
      id: tag

      color: Styles.foreground.high
    }
  }
}
