import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property int imageSize: 10
  property int fieldWidth: 50

  signal increment()
  signal decrement()

  padding: 0

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp04}
    radius: 4
  }

  contentItem: RowLayout {
    id: rl

    spacing: 0

    Button {
      id: btnDecrement

      leftPadding: 6; rightPadding: 6
      topPadding: 10; bottomPadding: 10

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        width: root.imageSize
        mipmap: true
        smooth: true
        source: "minus.svg"
      }

      background: Rectangle {
        color: btnDecrement.hovered ? Styles.background.dp04 : "transparent"
        topLeftRadius: 4
        bottomLeftRadius: 4
      }
    }

    QxTextInput {
      id: separator

      Layout.preferredWidth: root.fieldWidth
      Layout.preferredHeight: btnDecrement.implicitHeight
    }

    Button {
      id: btnIncrement

      leftPadding: 6; rightPadding: 6
      topPadding: 5; bottomPadding: 5

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        width: root.imageSize
        mipmap: true
        smooth: true
        source: "plus.svg"
      }
      background: Rectangle {
        color: btnIncrement.hovered ? Styles.background.dp04 : "transparent"
        topRightRadius: 4
        bottomRightRadius: 4
      }
    }
  }
}
