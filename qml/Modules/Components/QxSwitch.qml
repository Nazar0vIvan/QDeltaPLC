import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls 2.12

import Styles 1.0

Switch {
  id: control

  implicitWidth: 50
  implicitHeight: 26

  contentItem: Text {
    text: control.text
    verticalAlignment: Text.AlignVCenter
    anchors.left: indicator.right
    anchors.leftMargin: 8
    color: control.checked ? "#4caf50" : "black"
  }
  indicator: Rectangle {
      implicitWidth: 60
      implicitHeight: 30
      radius: height/2
      color: control.checked ? "#4caf50" : "#999"

      Rectangle {
          width: 26; height: 26
          radius: width/2
          anchors.verticalCenter: parent.verticalCenter
          x: control.checked ? parent.width - width - 2 : 2
          color: "white"
          Behavior on x { NumberAnimation { duration: 150 } }
      }
  }
}
