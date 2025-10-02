import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

GridView {
    id: root

    readonly property int cellSpacing: 10
    readonly property int ledSize: 40

    interactive: false
    cellWidth: ledSize + cellSpacing
    cellHeight: ledSize + 40
    implicitWidth: 3*cellWidth
    implicitHeight: 2*cellHeight

    model: ListModel {
        ListElement { label: "Mains"; color: "yellow"; isOn: false }
        ListElement { label: "RUN";   color: "green";  isOn: false }
        ListElement { label: "Y1.7";  color: "green";  isOn: false }
        ListElement { label: "Y2.7";  color: "red";    isOn: false }
        ListElement { label: "Y2.6";  color: "red";    isOn: false }
        ListElement { label: "Y2.5";  color: "red";    isOn: false }
    }

    delegate: QxLed {
      size: root.ledSize
      tag: model.label
      ledColor: model.color
      isOn: (index < 2) ? (plcRunner.socketState === 3) : model.isOn
    }
}
