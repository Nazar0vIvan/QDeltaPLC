import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0

Control {
  id: root

  property int imageSize: UiMetrics.iconSizeSmall
  property int fieldWidth: UiMetrics.fieldWidthSmall
  property alias text: valueField.text
  property alias validator: valueField.validator

  signal increment()
  signal decrement()

  padding: 0

  background: Rectangle {
    color: "transparent"
    border{width: UiMetrics.borderWidth; color: Styles.background.dp04}
    radius: UiMetrics.radiusSmall
  }

  contentItem: RowLayout {
    id: rl

    spacing: 0

    Button {
      id: btnDecrement

      padding: UiMetrics.spacingSmall

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        sourceSize.width: root.imageSize
        sourceSize.height: root.imageSize
        mipmap: true
        smooth: true
        source: "qrc:/pics/minus.svg"
      }

      background: Rectangle {
        color: btnDecrement.hovered ? Styles.background.dp04 : "transparent"
        topLeftRadius: UiMetrics.radiusSmall
        bottomLeftRadius: UiMetrics.radiusSmall
      }

      onClicked: root.decrement()
    }

    QxTextInput {
      id: valueField

      Layout.preferredWidth: root.fieldWidth
    }

    Button {
      id: btnIncrement

      padding: UiMetrics.spacingSmall

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        sourceSize.width: root.imageSize
        sourceSize.height: root.imageSize
        mipmap: true
        smooth: true
        source: "qrc:/pics/plus.svg"
      }
      background: Rectangle {
        color: btnIncrement.hovered ? Styles.background.dp04 : "transparent"
        topRightRadius: UiMetrics.radiusSmall
        bottomRightRadius: UiMetrics.radiusSmall
      }

      onClicked: root.increment()
    }
  }
}
