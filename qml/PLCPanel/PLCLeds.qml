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

    clip: true
    interactive: false
    cellWidth: 100 + cellSpacing
    cellHeight: 200
    implicitWidth:  cols*cellWidth  + (cols-1)*cellSpacing
    implicitHeight: rows*cellHeight + (rows-1)*cellSpacing

    model: ListModel {
        ListElement { label: "Mains"; color: "yellow"; isOn: true;  w: 100; h: 200 }
        ListElement { label: "Y1.6";  color: "green";  isOn: false; w: 100; h: 200 }
        ListElement { label: "Y1.7";  color: "green";  isOn: false; w: 100; h: 200 }
        ListElement { label: "Y2.7";  color: "red";    isOn: false; w: 70;  h: 140 }
        ListElement { label: "Y2.6";  color: "red";    isOn: false; w: 70;  h: 140 }
        ListElement { label: "Y2.5";  color: "red";    isOn: false; w: 70;  h: 140 }
    }

    delegate: Item {
        width: root.cellWidth + root.cellSpacing
        height: root.cellHeight

        QxLed {
          anchors.centerIn: parent
          width: model.w; height: model.h
          labelText: model.label
          ledColor: model.color
          isOn: model.isOn
        }
    }
}
