import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import qdeltaplc_qml_module 1.0 // FOR NOW

Control {
  id: root

  Connections {
    target: plcRunner

    function onPlcDataReady(data) {
      if (!data.type) return;
        switch(data.cosType) {
          case PlcMessage.AUT_EXT : {
            autExt.color = data.value === true ? "green" : "red";
            break;
          }
          case PlcMessage.PRO_ACT : {
            proAct.color = data.value === true ? "green" : "red";
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

    spacing: 10

    QxField {
      id: autExtField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "$EXT: "

      Rectangle {
        id: autExt

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }

    QxField {
      id: proActField

      labelWidth: root.labelWidth
      height: root.fieldHeight
      labelText: "$PRO_ACT: "

      Rectangle {
        id: proAct

        anchors.verticalCenter: parent.verticalCenter
        width: root.ledSize; height: root.ledSize
        color: "red"
        border{width: 1; color: Styles.background.dp12}

      }
    }

    QxField {
      id: pgnoField

      labelText: "PGNO: "
      height: root.fieldHeight

      QxTextInput {
        id: pgno

        height: root.fieldHeight
        width: root.fieldWidth
        text: "1"
      }
    }

    QxButton {
      id: btnStartProgram

      text: "Execute"
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
