import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

import Styles 1.0

Switch {
  id: root

  implicitWidth: implicitIndicatorWidth
  implicitHeight: implicitIndicatorHeight

  property bool displayOnly: false
  property alias displayonly: root.displayOnly
  property alias imageSource: image.source
  property bool isOn: false
  property int barWidth: UiMetrics.indicatorSizeLarge
  property int barHeight: UiMetrics.indicatorSizeMedium

  checkable: false
  padding: 0
  spacing: UiMetrics.spacingSmall

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
      width: parent.height - 4 * UiMetrics.borderWidth
      height: width
      radius: width / 2
      anchors.verticalCenter: parent.verticalCenter
      x: root.isOn ? parent.width - width - UiMetrics.borderWidth
                   : 2 * UiMetrics.borderWidth
      color: Styles.foreground.high
      Behavior on x {
        NumberAnimation {
          duration: UiMetrics.animationFast
        }
      }

      Image {
        id: image

        anchors.centerIn: parent
        width: parent.width - 6
        fillMode: Image.PreserveAspectFit
        visible: root.displayOnly
      }
    }
  }
}
