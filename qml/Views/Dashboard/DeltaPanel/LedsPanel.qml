import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Control {
  id: root

  property alias title: header.text
  property var ledStates: []

  topPadding: 40; bottomPadding: 10
  leftPadding: 10; rightPadding: 10

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp12}
  }

  contentItem: GridView {
    id: gv

    readonly property int cellSpacing: 20
    readonly property int ledSize: 36

    interactive: false
    cellWidth: ledSize + cellSpacing
    cellHeight: ledSize + 40
    implicitWidth: 3*cellWidth
    implicitHeight: 2*cellHeight

    model: ListModel {
      ListElement { label: "Mains";  ledColor: "yellow"; }
      ListElement { label: "RUN";    ledColor: "green";  }
      ListElement { label: "LEDG2";  ledColor: "green";  }
      ListElement { label: "LEDR1";  ledColor: "red";    }
      ListElement { label: "LEDR2";  ledColor: "red";    }
      ListElement { label: "LEDR3";  ledColor: "red";    }
    }

    delegate: Item {
      id: cell

      required property int index
      required property string label
      required property color ledColor

      width: gv.cellWidth
      height: gv.cellHeight

      QxLed {
        anchors.centerIn: parent
        ledColor: cell.ledColor
        tag: cell.label
        size: gv.ledSize
        isOn: !!root.ledStates[cell.index]
      }
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    leftPadding: 10; rightPadding: 10
    topPadding: 6; bottomPadding: 6

    background: Rectangle {
      color: Styles.background.dp01
      border{width: 1; color: Styles.background.dp12}
    }

    textFormat: Text.RichText

    color: Styles.foreground.medium
    font{pixelSize: 12}
  }
}
