import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias tag: tag.text
  property int fieldWidth: UiMetrics.fieldWidthSmall
  property real value: 0.00

  signal increment()
  signal decrement()

  implicitWidth: row.implicitWidth
  implicitHeight: row.implicitHeight

  RowLayout {
    id: row

    spacing: UiMetrics.spacingXSmall

    Text {
      id: tag

      Layout.preferredWidth: UiMetrics.controlHeightCompact
      color: Styles.foreground.high
      font: Styles.fonts.body
    }

    QxStepButton {
      fieldWidth: root.fieldWidth
      text: root.value.toFixed(2)

      onDecrement: root.decrement()
      onIncrement: root.increment()
    }
  }
}
