import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Switch {
  id: root

  property bool displayonly: false
  property alias imageSource: image.source
  property bool isOn: false
  property int barWidth: 36
  property int barHeight: 20

  checkable: false
  padding: 0
  spacing: 6

  contentItem: Text {
    verticalAlignment: Text.AlignVCenter
    text: root.text
    color: Styles.foreground.high
    font: Styles.fonts.body
  }

  indicator: Rectangle {
    implicitWidth: root.barWidth
    implicitHeight: root.barHeight
    radius: height / 2
    color: root.isOn ? Styles.secondary.base : Styles.background.dp06

    Rectangle {
      width: parent.height - 6
      height: parent.height - 6
      radius: width / 2
      anchors.verticalCenter: parent.verticalCenter
      x: root.isOn ? parent.width - width - 2 : 2
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
}
