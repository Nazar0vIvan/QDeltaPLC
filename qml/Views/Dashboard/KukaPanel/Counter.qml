import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property int imageSize: 10
  property alias imageSource: image.source
  property alias dimension: dimension.text
  property alias value: value.text

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    Image {
      id: image

      Layout.preferredWidth: root.imageSize
      fillMode: Image.PreserveAspectFit
    }

    Button {
      id: btnLeftArrow

      contentItem: Image {
        width: 10
        source: "arrow_left"
        fillMode: Image.PreserveAspectFit
      }
      background: Rectangle {
        color: "transparent"
      }
    }

    TextField {
      id: value

      color: Styles.foreground.high
    }

    Text {
      id: dimension

      color: Styles.foreground.high
    }

    Button {
      id: btnRightArrow

      contentItem: Image {
        width: 10
        source: "arrow_right"
        fillMode: Image.PreserveAspectFit
      }
      background: Rectangle {
        color: "transparent"
      }
    }
  }
}
