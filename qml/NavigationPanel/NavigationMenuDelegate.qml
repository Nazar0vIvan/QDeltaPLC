import QtQuick
import QtQuick.Window

import Styles 1.0

Item {
  id: root

  signal clicked()

  property alias iconPath: image.source
  property bool selected: false

  readonly property real dpr: Screen.devicePixelRatio

  Rectangle {
    anchors.fill: parent

    radius: Metrics.radiusSmall
    color: Colors.foreground.high
    opacity: 0.2
    visible: root.selected || mouseArea.containsMouse
  }

  Image {
    id: image

    width: 22
    height: 22

    x: Math.round((parent.width - width) / 2 * root.dpr) / root.dpr
    y: Math.round((parent.height - height) / 2 * root.dpr) / root.dpr

    opacity: root.selected ? 1.0 : 0.7

    sourceSize: Qt.size(
      Math.round(width * root.dpr),
      Math.round(height * root.dpr)
    )

    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: false
  }

  MouseArea {
    id: mouseArea

    anchors.fill: parent
    hoverEnabled: true

    onClicked: root.clicked()
  }
}
