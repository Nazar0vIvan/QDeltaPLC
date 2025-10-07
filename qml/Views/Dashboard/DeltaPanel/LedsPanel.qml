import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

GridView {
    id: root

    readonly property int cellSpacing: 20
    readonly property int ledSize: 40

    interactive: false
    cellWidth: ledSize + cellSpacing
    cellHeight: ledSize + 40
    implicitWidth: 3*cellWidth
    implicitHeight: 2*cellHeight

    model: ListModel {
        ListElement { label: "Mains"; color: "yellow"; isOn: false }
        ListElement { label: "RUN";   color: "green";  isOn: false }
        ListElement { label: "LEDG2";  color: "green";  isOn: false }
        ListElement { label: "LEDR1";  color: "red";    isOn: false }
        ListElement { label: "LEDR2";  color: "red";    isOn: false }
        ListElement { label: "LEDR3";  color: "red";    isOn: false }
    }

    delegate: QxLed {
      ledColor: model.color
      tag: model.label
      size: root.ledSize
      isOn: (index < 2) ? (plcRunner.socketState === 3) : model.isOn
    }
}
