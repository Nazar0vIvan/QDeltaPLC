import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import QDelta.Backend 1.0 as Backend

Control {
  id: root

  required property Backend.DeviceRunner plc

  readonly property bool isAutExt: root.plc
                                   && root.plc.isConnected
                                   && root.plc.data.x1
                                   ? !!root.plc.data.x1[5]
                                   : false

  readonly property int cellState: root.plc
                                   && root.plc.isConnected
                                   && root.plc.data.cellState !== undefined
                                   ? root.plc.data.cellState
                                   : -1

  property string title: ""
  property int labelWidth: 70
  property int fieldHeight: 28
  property int fieldWidth: 50
  property int ledSize: 12

  topPadding: 40
  bottomPadding: 10
  leftPadding: 10
  rightPadding: 10

  contentItem: ColumnLayout {

    spacing: 6

    QxHField {
      id: autExtField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "AUT_EXT : "

      Rectangle {
        id: autExt

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: root.isAutExt ? "green" : "red"
        border{width: 1; color: Styles.background.dp12}
      }
    }
    QxHField {
      id: idleField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "IDLE : "

      Rectangle {
        id: idle

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: root.cellState === Backend.PlcMessage.IDLE ? "green" : "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }
    QxHField {
      id: runningField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "RUN : "

      Rectangle {
        id: running

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: root.cellState === Backend.PlcMessage.RUN ? "green" : "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }
    QxHField {
      id: doneField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "DONE : "

      Rectangle {
        id: done

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: root.cellState === Backend.PlcMessage.FIN ? "green" : "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }

    RowLayout {
      id: rl


      QxButton {
        id: btnConnect

        checked: root.plc.isConnected
        text: checked ? "Disconnect" : "Connect"
        onClicked: root.plc.invoke(checked ? "disconnect" : "connect")
      }

      QxButton {
        id: btnStartCell

        text: "Start Program"
        enabled: root.plc.isConnected && root.isAutExt
        onClicked: {
          const args = {
            "cmd": Backend.PlcMessage.SET_VAR,
            "var": Backend.PlcMessage.START_CELL,
            "attr": 1 // PGNO
          }
          root.plc.invoke("writeMessage", args);
        }
      }

      QxButton {
        id: btnSftOk

        text: "Safety Ok"
        enabled: root.plc.isConnected && root.isAutExt
      }
    }
  }

  background: Rectangle {
    color: "transparent"
    border {
      width: 1
      color: Styles.background.dp12
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top

    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    font: Styles.fonts.body
    textFormat: Text.RichText
    color: Styles.foreground.medium
    text: root.title

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: 1
        color: Styles.background.dp12
      }
    }
  }
}
