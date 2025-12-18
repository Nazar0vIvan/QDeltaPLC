import QtQuick
import QtQuick.Effects

import Styles 1.0

Item {
  id: root

  property alias iconPath: image.source
  property bool selected: false

  Image {
    id: image

    anchors{fill: parent; margins: 8}
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
  }

  MultiEffect {
  id: effect

  visible: root.selected
  anchors.fill: image
  source: image
  colorization: 1.0
  colorizationColor: Styles.primary.base
  }
}
