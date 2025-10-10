import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias tag: tag.text
  property int imageSize: 10
  property int fieldWidth: 80
  property real value: 0.00

  signal increment()
  signal decrement()

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    spacing: 0

    Text {
      id: tag

      color: Styles.foreground.high
      Layout.preferredWidth: 26
    }

    Button {
      id: btnDecrement

      leftPadding: 8; rightPadding: 8
      topPadding: 14; bottomPadding: 14

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        width: root.imageSize
        mipmap: true
        smooth: true
        source: "minus.svg"
      }

      background: Rectangle {
        topLeftRadius: 4; bottomLeftRadius: 4
        color: Styles.background.dp04
        border{width: 1; color: Styles.background.dp04}
        opacity: btnDecrement.hovered ? 1.0 : 0.8
      }
    }

    TextField {
      id: valueField

      Layout.preferredWidth: root.fieldWidth
      Layout.preferredHeight: btnDecrement.implicitHeight
      text: root.value.toFixed(2)
      verticalAlignment: TextInput.AlignVCenter
      horizontalAlignment: TextInput.AlignHCenter

      background: Rectangle {
        border{width: 1; color: Styles.background.dp04}
        color: "transparent"
      }
    }

    Button {
      id: btnIncrement

      leftPadding: 8; rightPadding: 8
      topPadding: 10; bottomPadding: 10

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        width: root.imageSize
        mipmap: true
        smooth: true
        source: "plus.svg"
      }
      background: Rectangle {
        topRightRadius: 4; bottomRightRadius: 4
        color: Styles.background.dp04
        border{width: 1; color: Styles.background.dp04}
        opacity: btnIncrement.hovered ? 1.0 : 0.8
      }
    }
  }
}
