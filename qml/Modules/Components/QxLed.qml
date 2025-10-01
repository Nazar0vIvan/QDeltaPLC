import QtQuick
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property alias tag: tag.text
  property color ledColor: led.color
  property int size: 20
  property bool isOn: false

  implicitWidth: root.size
  implicitHeight: tag.height + root.size + cl.spacing

  ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 10

    Text {
      id: tag

      Layout.alignment: Qt.AlignHCenter
      color: Styles.foreground.high
    }

    Rectangle {
      id: led

      Layout.preferredWidth: root.size
      Layout.preferredHeight: root.size
      radius: parent.width/2
      color: root.isOn ? Qt.lighter(root.ledColor, 1.4) : Qt.darker(root.ledColor, 1.4)

      border{ color: Qt.darker(root.ledColor, 1.8); width: 3 }

      Behavior on color { ColorAnimation { duration: 150 } }
    }

  }
}
