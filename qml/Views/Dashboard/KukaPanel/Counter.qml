import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property int fieldWidth: UiMetrics.fieldWidthSmall
  property alias dimension: dimension.text
  property real value: 0.000

  implicitWidth: row.implicitWidth
  implicitHeight: row.implicitHeight

  RowLayout {
    id: row

    spacing: UiMetrics.spacingSmall

    Image {
      Layout.preferredWidth: UiMetrics.iconSizeLarge
      Layout.preferredHeight: UiMetrics.iconSizeLarge
      fillMode: Image.PreserveAspectFit
      source: "qrc:/pics/step.svg"
      mipmap: true
      smooth: true
    }

    QxStepButton {
      fieldWidth: root.fieldWidth
      text: root.value.toFixed(3)

      onDecrement: root.value -= 0.001
      onIncrement: root.value += 0.001
    }

    Text {
      id: dimension

      color: Styles.foreground.high
      font: Styles.fonts.body
    }
  }
}
