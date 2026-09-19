import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias tag: tag.text
  property int fieldWidth: Metrics.w80
  property real value: 0.00

  signal increment()
  signal decrement()

  implicitWidth: row.implicitWidth
  implicitHeight: row.implicitHeight

  RowLayout {
    id: row

    spacing: Metrics.sp4

    Text {
      id: tag

      Layout.preferredWidth: Metrics.w24
      color: Colors.foreground.high
      font: Fonts.body
    }

    QxStepButton {
      fieldWidth: root.fieldWidth
      text: root.value.toFixed(2)

      onDecrement: root.decrement()
      onIncrement: root.increment()
    }
  }
}
