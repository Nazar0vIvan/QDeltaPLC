import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property int imageSize: 6
  property int fieldWidth: 50
  property int fieldHeight: 24
  property alias dimension: dimension.text
  property real value: 0.000

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    spacing: 0

    Image {
      id: imgStep

      Layout.preferredHeight: root.fieldHeight
      Layout.rightMargin: 14
      fillMode: Image.PreserveAspectFit
      source: "qrc:/assets/pics/step.svg"
    }

    Button { // left arrow
      id: btnDecrement

      Layout.preferredWidth: 14
      Layout.preferredHeight: root.fieldHeight
      topPadding: 6; bottomPadding: 6
      rightPadding: 4; leftPadding: 4

      contentItem: Image {
        source: "qrc:/assets/pics/arrow_left.svg"
        mipmap: true
        smooth: true
      }
      background: Rectangle {
        topLeftRadius: 4; bottomLeftRadius: 4
        color: Styles.background.dp04
        border{ width: 1; color: Styles.background.dp24 }
        opacity:  btnDecrement.pressed ? 0.7 : btnDecrement.hovered ? 1.0 : 0.7
      }

      onClicked: { root.value = root.value - 0.001 }
    }

    TextField {
      id: valueField

      Layout.preferredHeight: root.fieldHeight
      Layout.preferredWidth: root.fieldWidth
      color: Styles.foreground.high
      text: root.value.toFixed(3)

      background: Rectangle {
        color: "transparent"
        border{width: 1; color: Styles.background.dp04}
      }
    }

    Button { // right arrow
      id: btnIncrement

      Layout.preferredWidth: 14
      Layout.preferredHeight: root.fieldHeight
      topPadding: 6; bottomPadding: 6
      rightPadding: 4; leftPadding: 4

      contentItem: Image {
        source: "qrc:/assets/pics/arrow_right.svg"
        mipmap: true
        smooth: true
      }
      background: Rectangle {
        topRightRadius: 4; bottomRightRadius: 4
        color: Styles.background.dp04
        border{ width: 1; color: Styles.background.dp24 }
        opacity:  btnIncrement.pressed ? 0.7 : btnIncrement.hovered ? 1.0 : 0.7
      }

      onClicked: { root.value = root.value + 0.001 }
    }

    Text {
      id: dimension

      color: Styles.foreground.high
      Layout.leftMargin: 8
    }
  }
}
