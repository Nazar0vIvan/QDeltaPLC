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
  property int ledSize: 10
  property bool isOn: false

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  opacity: enabled ? 1.0 : 0.5

  RowLayout {
    id: rl

    Text {
      id: label

      color: Styles.foreground.high

    }
    Rectangle {
      id: led

      Layout.preferredWidth: root.ledSize
      Layout.preferredHeight: root.ledSize
      color: root.enabled ? Styles.foreground.disabled : root.isOn ? "green" : Styles.background.dp04
      border{width: 1; color: Styles.background.dp12}
    }
    Text {
      id: tag

      color: Styles.foreground.high
    }
  }
}
