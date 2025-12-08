import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import qdeltaplc_qml_module 1.0 // FOR NOW

Control {
  id: root

  property bool isAutExt: false

  Connections {
    target: plcRunner

    function onPlcDataReady(data) {
      if (!data.type) return;
        switch(data.chg) {
          case PlcMessage.IOs : {
            if (data.x1[5]) {
              autExt.color = "green";
              root.isAutExt = true;
            }
            else {
              autExt.color = "red";
              root.isAutExt = false;
            }
            break;
          }
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
      labelText: "AUT_EXT: "

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
      labelText: "IDLE: "

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
      labelText: "RUN: "

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
      labelText: "DONE: "

      Rectangle {
        id: done

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }

    QxButton {
      id: btnStartCell

      text: "Start"
      enabled: root.isAutExt

      onClicked: {
        const args = {
          "cmd": PlcMessage.SET_VAR,
          "var": PlcMessage.START_CELL,
          "attr": 1
        }
        plcRunner.invoke("writeMessage", args);
      }
    }

    QxButton {
      id: btnSftOk

      text: "SftOk"
      enabled: root.isAutExt
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
