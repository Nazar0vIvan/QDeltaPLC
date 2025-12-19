import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import qdeltaplc_qml_module 1.0 // !!

Control {
  id: root

  property bool isAutExt: false

  Connections {
    target: plcRunner

    function onDataReady(data) {
      if (data.cmd && data.cmd === PlcMessage.SNAPSHOT ||
          data.chg && data.chg === PlcMessage.IOs) {
        autExt.color = data.x1[5] ? "green" : "red"; // data.x1[5] - AutExt
        root.isAutExt = !!data.x1[5];
      }
      if (data.chg && data.chg === PlcMessage.CELL_STATE) {
        switch (data.state) {
          case 0: {
            idle.color = "green";
            running.color = "red";
            done.color = "red";
            break;
          }
          case 1: {
            idle.color = "red";
            running.color = "green";
            done.color = "red";
            break;
          }
          case 2: {
            idle.color = "red";
            running.color = "red";
            done.color = "green";
            break;
          }
        }
      }
    }
    function onSocketStateChanged() {
      if (plcRunner.socketState === 0) {
        root.isAutExt = false;
        autExt.color = "red";
        idle.color = "red";
        running.color = "red";
        done.color = "red";
      }
    }
  }

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

    QxField {
      id: autExtField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "AUT_EXT : "

      Rectangle {
        id: autExt

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}
      }
    }
    QxField {
      id: idleField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "IDLE : "

      Rectangle {
        id: idle

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }
    QxField {
      id: runningField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "RUN : "

      Rectangle {
        id: running

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }
    QxField {
      id: doneField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "DONE : "

      Rectangle {
        id: done

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }

    RowLayout {
      id: rl


      QxButton {
        id: btnConnect

        enabled: plcRunner
        checked: plcRunner && plcRunner.socketState === 3
        text: checked ? "Disconnect" : "Connect"
        onClicked: {
          if (!plcRunner)
            return
          if (checked) {
            plcRunner.invoke("disconnectFromHost")
          } else {
            plcRunner.invoke("connectToHost")
          }
        }
      }

      QxButton {
        id: btnStartCell

        text: "Start Program"
        enabled: plcRunner && plcRunner.socketState === 3 && root.isAutExt

        onClicked: {
          const args = {
            "cmd": PlcMessage.SET_VAR,
            "var": PlcMessage.START_CELL,
            "attr": 1 // PGNO
          }
          plcRunner.invoke("writeMessage", args);
        }
      }

      QxButton {
        id: btnSftOk

        text: "Safety Ok"
        enabled: plcRunner && plcRunner.socketState === 3 && root.isAutExt
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
