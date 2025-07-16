import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls

import AppStyles 1.0

GroupBox {
  id: root

  property string groupBoxTitle: "untitled"

  background: Rectangle {
    anchors.fill: parent
    color: "transparent"
    border{width: 1; color: Styles.foreground.high}
  }

  label: Rectangle {
    x: title.font.pixelSize
    anchors.bottom: parent.top
    anchors.bottomMargin: -height/2
    anchors.leftMargin: 20
    width: title.paintedWidth + 16; height: title.font.pixelSize + 8
    color: Styles.background.dp00
    border{width: 1; color: Styles.foreground.high}
    Text {
      id: title
      anchors.centerIn: parent
      text: qsTr(root.groupBoxTitle)
      color: Styles.foreground.high
      font.pixelSize: 16
    }
  }
}
