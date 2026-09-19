import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property int imageSize: Metrics.sz12
  property int fieldWidth: Metrics.w80
  property alias text: valueField.text
  property alias validator: valueField.validator

  signal increment()
  signal decrement()

  padding: Metrics.sp0

  background: Rectangle {
    color: "transparent"
    border{width: Metrics.w1; color: Colors.background.dp04}
    radius: Metrics.r4
  }

  contentItem: RowLayout {
    id: rl

    spacing: 0

    Button {
      id: btnDecrement

      padding: Metrics.sp8

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        sourceSize.height: root.imageSize
        mipmap: true
        smooth: true
        source: "qrc:/pics/minus.svg"
      }

      background: Rectangle {
        color: btnDecrement.hovered ? Colors.background.dp04 : "transparent"
        topLeftRadius: Metrics.r4
        bottomLeftRadius: Metrics.r4
      }

      onClicked: root.decrement()
    }

    QxTextInput {
      id: valueField

      Layout.preferredWidth: root.fieldWidth
    }

    Button {
      id: btnIncrement

      padding: Metrics.sp8

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        sourceSize.width: root.imageSize
        sourceSize.height: root.imageSize
        mipmap: true
        smooth: true
        source: "qrc:/pics/plus.svg"
      }
      background: Rectangle {
        color: btnIncrement.hovered ? Colors.background.dp04 : "transparent"
        topRightRadius: Metrics.r4
        bottomRightRadius: Metrics.r4
      }

      onClicked: root.increment()
    }
  }
}
