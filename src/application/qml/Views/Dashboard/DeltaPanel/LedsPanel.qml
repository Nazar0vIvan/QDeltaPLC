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
        ListElement { label: "GREEN1"; ledColor: "green" }
        ListElement { label: "RED1"; ledColor: "red" }
        ListElement { label: "RED2"; ledColor: "red" }
        ListElement { label: "RED3"; ledColor: "red" }
      }

      delegate: QxVField {
        id: field

        required property int index
        required property string label
        required property color ledColor

        Layout.alignment: Qt.AlignHCenter
        labelAlignment: Qt.AlignHCenter
        labelText: field.label
        labelColor: root.enabled ? Colors.foreground.high : Colors.foreground.disabled

        QxLed {
          diameter: Metrics.sz36
          ledColor: root.enabled ? field.ledColor : Colors.foreground.disabled
          isOn: root.enabled && !!root.ledStates[field.index]
        }
      }
    }
  }
}
