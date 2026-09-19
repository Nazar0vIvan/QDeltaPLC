pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxPanel {
  id: root

  property var ledStates: []

  GridLayout {
    columns: 3
    rowSpacing: Metrics.sp16
    columnSpacing: Metrics.sp16

    Repeater {
      model: ListModel {
        ListElement { label: "Mains"; ledColor: "yellow" }
        ListElement { label: "RUN"; ledColor: "green" }
        ListElement { label: "LEDG2"; ledColor: "green" }
        ListElement { label: "LEDR1"; ledColor: "red" }
        ListElement { label: "LEDR2"; ledColor: "red" }
        ListElement { label: "LEDR3"; ledColor: "red" }
      }

      delegate: QxVField {
        id: field

        required property int index
        required property string label
        required property color ledColor

        labelText: field.label

        QxLed {
          diameter: Metrics.sz36
          ledColor: field.ledColor
          isOn: !!root.ledStates[field.index]
        }
      }
    }
  }
}
