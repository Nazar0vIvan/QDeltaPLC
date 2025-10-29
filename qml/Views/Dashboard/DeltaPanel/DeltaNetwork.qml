import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import qdeltaplc_qml_module 1.0 // FOR NOW

Control {
  id: root

  property int fieldHeight: 24
  property int fieldWidth: 112

  property alias title: header.text


  function buildMasks(index, newState) {
    const bit = (1 << index) & 0xFF;
    let andMask; let orMask;

    if (newState) {
      andMask = 0xFF;
      orMask  = bit;
    } else {
        andMask = (~bit) & 0xFF;
        orMask  = 0x00;
    }
    return { andMask, orMask };
  }

  function byteToBitString(value) {
    const v = value & 0xFF;
    const bits = v.toString(2).padStart(8, "0");
    return bits.slice(0, 4) + " " + bits.slice(4);
  }

  topPadding: 40
  bottomPadding: 10
  leftPadding: 10
  rightPadding: 10

  background: Rectangle {
    color: "transparent"
    border {
      width: 1
      color: Styles.background.dp12
    }
  }

  contentItem: ColumnLayout {

    spacing: 20

    RowLayout {
      id: rl1

      QxField {
        // message
        id: msgField

        Layout.preferredHeight: root.fieldHeight
        labelText: "Message:"

        QxTextInput {
          id: msg

          width: root.fieldWidth
          height: root.fieldHeight
        }
      }

      QxToolButton {
        id: btnSend

        enabled: plcRunner && plcRunner.socketState === 3
        imageSource: "qrc:/assets/pics/send.svg"
        Layout.preferredWidth: 22
        Layout.preferredHeight: 22

        onClicked: {
        if (!msg.text) return;
          plcRunner.invoke("writeMessage",
                           {
                            "cmd": PlcMessage.WRITE_RAW,
                            "raw": msg.text,
                           });
        }
      }
    }

    RowLayout {
      id: rl2

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
        id: test

        // enabled: plcRunner && plcRunner.socketState === 3
        text: "read Y2"
        onClicked: {
          if (!plcRunner) return;
          // plcRunner.invoke("writeMessage",
          //                  {
          //                    "cmd": 0x0F,
          //                    "dev": 0x0059,
          //                    "module": 2,
          //                  })
        }
      }
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

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: 1
        color: Styles.background.dp12
      }
    }

    textFormat: Text.RichText

    color: Styles.foreground.medium
    font {
      pixelSize: 12
    }
  }
}
