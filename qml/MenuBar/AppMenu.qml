import QtQuick 2.12
import QtQuick.Controls 2.15

import Styles 1.0

Menu {
  delegate: MenuItem {
    id: menuItem

    implicitWidth: 180; implicitHeight: 35
    leftPadding: 0

    indicator: Item {
      implicitWidth: 28; implicitHeight: implicitWidth
      anchors.verticalCenter: parent.verticalCenter
      Image {
        width: 15; height: width
        anchors.centerIn: parent
        source: menuItem.icon.source
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
      }

    }
    contentItem: Text {
      leftPadding: menuItem.icon.source ? menuItem.indicator.width : 0
      verticalAlignment: Text.AlignVCenter
      text: menuItem.text
      font: Styles.fonts.caption
      color: parent.enabled ? Styles.foreground.high : Styles.foreground.disabled
    }
    background: Rectangle {
      color: menuItem.hovered ? Styles.primary.transparent : Styles.background.dp00
    }
  }

  background: Rectangle {
    implicitWidth: 180; implicitHeight: 30
    color: Styles.background.dp00
    border{ width: 1; color: "gray" }
  }
}
