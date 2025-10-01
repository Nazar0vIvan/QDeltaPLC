import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

GridView {
    id: root

    readonly property int cols: 3
    readonly property int rows: 2
    readonly property int cellSpacing: 10
    property int ledSize: 20

    interactive: false
    cellWidth: ledSize + cellSpacing
    cellHeight: ledSize
    implicitWidth:  cols*cellWidth  + (cols-1)*cellSpacing
    implicitHeight: rows*cellHeight + (rows-1)*cellSpacing

    model: ListModel {
        ListElement { label: "Mains"; color: "yellow"; isOn: false }
        ListElement { label: "Y1.6";  color: "green";  isOn: false }
        ListElement { label: "Y1.7";  color: "green";  isOn: false }
        ListElement { label: "Y2.7";  color: "red";    isOn: false }
        ListElement { label: "Y2.6";  color: "red";    isOn: false }
        ListElement { label: "Y2.5";  color: "red";    isOn: false }
    }

    delegate:
        QxLed {
          size: root.ledSize
          tag: model.label
          ledColor: model.color
          isOn: model.isOn
    }
}
