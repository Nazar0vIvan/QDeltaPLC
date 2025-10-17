import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Switch {
  id: root

  property bool displayonly: false
  property alias imageSource: image.source

  signal turnedOn
  signal turnedOff

  checkable: true

  contentItem: Text {
    anchors.left: indicator.right
    anchors.verticalCenter: indicator.verticalCenter
    anchors.leftMargin: 6
    verticalAlignment: Text.AlignVCenter
    text: root.text
    color: Styles.foreground.high
    font: Styles.fonts.body
  }

  indicator: Rectangle {
    width: root.width
    height: root.height
    radius: root.height / 2
    color: root.checked ? Styles.secondary.base : Styles.background.dp06

    Rectangle {
      width: root.height - 6
      height: root.height - 6
      radius: width / 2
      anchors.verticalCenter: parent.verticalCenter
      x: root.checked ? parent.width - width - 2 : 2
      color: Styles.foreground.high
      Behavior on x {
        NumberAnimation {
          duration: 150
        }
      }

      Image {
        id: image

        anchors.centerIn: parent
        width: parent.width - 8
        fillMode: Image.PreserveAspectFit
        visible: root.displayonly
      }
    }
  }
  onCheckedChanged: {
    if (checked)
      turnedOn()
    else
      turnedOff()
  }
}
