import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Switch {
  id: root

  implicitWidth: implicitIndicatorWidth
  implicitHeight: implicitIndicatorHeight

  property bool displayOnly: false
  property bool available: true
  property alias displayonly: root.displayOnly
  property alias imageSource: image.source
  property bool isOn: false
  property int barWidth: Metrics.sz36
  property int barHeight: Metrics.sz20

  checkable: false
  padding: Metrics.sp0
  spacing: Metrics.sp8

  contentItem: Text {
    verticalAlignment: Text.AlignVCenter
    text: root.text
    color: Colors.foreground.high
    font: Fonts.body
  }

  indicator: Rectangle {
    implicitWidth: root.barWidth
    implicitHeight: root.barHeight
    radius: height / 2
    color: root.available && root.isOn ? Colors.secondary.base : Colors.background.dp06

    Rectangle {
      width: parent.height - 4 * Metrics.w1
      height: width
      radius: width / 2
      anchors.verticalCenter: parent.verticalCenter
      x: !root.available ? (parent.width - width) / 2
                   : root.isOn ? parent.width - width - Metrics.w1
                   : 2 * Metrics.w1
      color: root.available ? Colors.foreground.high : Colors.foreground.disabled
      Behavior on x {
        NumberAnimation {
          duration: Metrics.animationFast
        }
      }

      Image {
        id: image

        anchors.centerIn: parent
        width: parent.width - 6
        fillMode: Image.PreserveAspectFit
        visible: root.available && root.displayOnly
      }
    }
  }
}
