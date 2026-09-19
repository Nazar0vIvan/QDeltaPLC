pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

QxPanel {
  id: root

  required property list<string> labels
  required property list<string> units
  required property list<string> values

  signal valueEdited(int index, string value)

  readonly property bool valid: {
    if (root.values.length !== 6)
      return false
    for (let i = 0; i < 6; ++i) {
      if (root.values[i].trim().length === 0 || !isFinite(Number(root.values[i])))
        return false
    }
    return true
  }

  function numbers() {
    const result = []
    for (let i = 0; i < root.values.length; ++i)
      result.push(Number(root.values[i]))
    return result
  }

  spacing: Metrics.sp8

  Repeater {
    model: 6

    delegate: QxHField {
      id: field

      required property int index

      Layout.fillWidth: true
      labelWidth: Metrics.sp24
      labelText: root.labels[field.index] ?? ""

      QxTextInput {
        id: input

        Layout.preferredWidth: Metrics.w120
        Layout.fillWidth: true
        text: root.values[field.index] ?? ""
        validator: DoubleValidator {
          locale: "C"
          decimals: 8
        }
        onTextEdited: root.valueEdited(field.index, input.text)
      }

      Label {
        Layout.preferredWidth: Metrics.sp32
        text: root.units[field.index] ?? ""
        color: Colors.foreground.medium
        font: Fonts.caption
      }
    }
  }
}
