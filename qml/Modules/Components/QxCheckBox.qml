import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

CheckBox {
  id: root

  implicitWidth: Metrics.sz20
  implicitHeight: Metrics.sz20

  padding: Metrics.sp0
  spacing: Metrics.sp0

  contentItem: Item { }

  indicator: Rectangle {
    implicitWidth: Metrics.sz20
    implicitHeight: Metrics.sz20
    anchors.centerIn: parent

    radius: Metrics.r4
    color: root.checked ? Colors.secondary.base
                        : Colors.background.dp00

    border {
      width: Metrics.w1
      color: root.checked ? Colors.background.dp00
                          : Colors.foreground.medium
    }

    Text {
      anchors.centerIn: parent
      visible: root.checked

      text: "\u2713"
      color: Colors.background.dp00
      font {
        family: "Segoe UI Symbol"
        pixelSize: 14
        bold: true
      }
    }
  }
}
