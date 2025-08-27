import QtQuick 2.15
import QtQuick.Layouts

import Styles 1.0

Rectangle {
  id: control

  property alias labelText: label_txt.text
  property alias ledColor: led.color

  color: Styles.background.dp24
  radius: 6

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    spacing: 10

    Rectangle {
      id: label

      Layout.fillHeight: true
      Layout.fillWidth: true
      implicitHeight: 50
      color: Styles.foreground.high
      radius: 4

      Text {
        id: label_txt

        anchors.centerIn: parent
        color: Styles.background.dp00
      }
    }
    Rectangle {
      id: led

      Layout.fillWidth: true
      Layout.preferredHeight: width
      radius: parent.width/2
    }
  }
}
