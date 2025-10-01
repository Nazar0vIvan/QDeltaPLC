import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Switch {
  id: control

  signal turnedOn()
  signal turnedOff()

  checkable: true

  contentItem: Text {
    anchors.left: indicator.right
    anchors.verticalCenter: indicator.verticalCenter
    anchors.leftMargin: 6
    verticalAlignment: Text.AlignVCenter
    text: control.text
    color: Styles.foreground.high
  }

  indicator: Rectangle {
      width: control.width
      height: control.height
      radius: control.height/2
      color: control.checked ? Styles.secondary.base : Styles.background.dp06

      Rectangle {
          width: control.height - 6;
          height: control.height - 6
          radius: width/2
          anchors.verticalCenter: parent.verticalCenter
          x: control.checked ? parent.width - width - 2 : 2
          color: Styles.foreground.high
          Behavior on x { NumberAnimation { duration: 150 } }
      }
  }
  onCheckedChanged: {
      if (checked) turnedOn();
      else turnedOff();
  }
}
