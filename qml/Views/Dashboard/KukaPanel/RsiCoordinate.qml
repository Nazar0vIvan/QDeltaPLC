import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias tag: tag.text
  property int imageSize: 10
  property int fieldWidth: 80
  property int fieldHeight: 24
  property real value: 0.00

  signal increment()
  signal decrement()

  implicitWidth: cl.implicitWidth
  implicitHeight: cl.implicitHeight

  ColumnLayout {
    id: cl

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

        Layout.preferredWidth: 20
        Layout.preferredHeight: root.fieldHeight
        padding: 4

        contentItem: Image {
          fillMode: Image.PreserveAspectFit
          source: "qrc:/assets/pics/minus.svg"
          mipmap: true
          smooth: true
        }

        background: Rectangle {
          topLeftRadius: 4; bottomLeftRadius: 4
          color: Styles.background.dp04
          border{width: 1; color: Styles.background.dp24}
          opacity:  btnDecrement.pressed ? 0.7 : btnDecrement.hovered ? 1.0 : 0.7
        }
      }

      TextField {
        id: valueField

        Layout.preferredWidth: root.fieldWidth
        Layout.preferredHeight: root.fieldHeight
        text: root.value.toFixed(2)
        verticalAlignment: TextInput.AlignVCenter
        horizontalAlignment: TextInput.AlignHCenter
        color: Styles.foreground.high

        background: Rectangle {
          border{width: 1; color: Styles.background.dp04}
          color: "transparent"
        }
      }

      Button {
        id: btnIncrement

        Layout.preferredWidth: 20
        Layout.preferredHeight: root.fieldHeight
        padding: 4

        contentItem: Image {
          fillMode: Image.PreserveAspectFit
          source: "qrc:/assets/pics/plus.svg"
          mipmap: true
          smooth: true
        }
        background: Rectangle {
          topRightRadius: 4; bottomRightRadius: 4
          color: Styles.background.dp04
          border{width: 1; color: Styles.background.dp24}
          opacity:  btnIncrement.pressed ? 0.7 : btnIncrement.hovered ? 1.0 : 0.7
        }
      }
    }
  }
}
