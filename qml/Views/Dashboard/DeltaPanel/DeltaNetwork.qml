import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import qdeltaplc_qml_module 1.0 // FOR NOW

Control {
  id: root

  property int fieldHeight: 23
  property int fieldWidth: 40
  property int labelWidth: 50
  property alias title: header.text

  Connections {
    target: plcRunner

    function onDataReady(data) {
      if (!data.cmd) return;
      switch(data.cmd) {
        case PlcMessage.READ_REG: {
          rRegValueInput.text = data.value;
          break;
        }
        case PlcMessage.WRITE_REG: {
          if (Number(wRegValueInput.text) === data.value) {
            wRegValueInput.confirmed = true;
          }
          break;
        }
        case PlcMessage.WRITE_RAW: {
          if (wRawValueInput.text === String(data.value)) {
            wRawValueInput.confirmed = true;
          }
          break;
        }
        default: break;
      }
    }
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

    spacing: 14

    // WRITE_RAW
    RowLayout {
      id: rl1

      spacing: 10

      Text {
        Layout.preferredWidth: root.labelWidth

        text: "W_RAW:"
        color: Styles.foreground.high
        font: Styles.fonts.body
      }

      RowLayout {
        id: rl11

        spacing: 0

        Label {
          Layout.preferredHeight: root.fieldHeight
          leftPadding: 4; rightPadding: 4;
          font: Styles.fonts.body
          text: "D"
          color: Styles.foreground.high
          verticalAlignment: Text.AlignVCenter
          background: Rectangle {
            color: Styles.background.dp04
            border {
              width: 1
              color: Styles.background.dp12
            }
          }
        }

        QxTextInput {
          id: wRawValueInput

          Layout.preferredWidth: 2*root.fieldWidth
          Layout.preferredHeight: root.fieldHeight
          radius: 0;
        }

        QxToolButton {
          id: btnWriteRaw

          Layout.preferredWidth: root.fieldHeight
          Layout.preferredHeight: root.fieldHeight

          radius: 0
          padding: 6
          enabled: plcRunner && plcRunner.socketState === 3
          imageSource: "qrc:/assets/pics/arrow_right.svg"
          onClicked: {
            if (!wRawValueInput.text) return;
            const args = {
              "cmd": PlcMessage.WRITE_RAW,
              "raw": wRawValueInput.text,
            }
            plcRunner.invoke("writeMessage", args);
          }
        }

      }
    }

    // WRITE_REG
    RowLayout {
      id: rl2

      spacing: 10

      Text {
        Layout.preferredWidth: root.labelWidth
        text: "W_REG:"
        color: Styles.foreground.high
        font: Styles.fonts.body
      }

      RowLayout {
        id: rl21

        spacing: 0

        Label {
          Layout.preferredHeight: root.fieldHeight
          leftPadding: 4; rightPadding: 4;
          font: Styles.fonts.body
          text: "D"
          color: Styles.foreground.high
          verticalAlignment: Text.AlignVCenter
          background: Rectangle {
            color: Styles.background.dp04
            border {
              width: 1
              color: Styles.background.dp12
            }
          }
        }

        QxTextInput {
          id: wRegAddrInput

          Layout.preferredWidth: root.fieldWidth
          Layout.preferredHeight: root.fieldHeight
          radius: 0;
        }

        QxTextInput {
          id: wRegValueInput

          Layout.preferredWidth: root.fieldWidth
          Layout.preferredHeight: root.fieldHeight
          radius: 0;
        }

        QxToolButton {
          id: btnWriteReg

          Layout.preferredWidth: root.fieldHeight
          Layout.preferredHeight: root.fieldHeight

          radius: 0
          padding: 6
          enabled: plcRunner && plcRunner.socketState === 3
          imageSource: "qrc:/assets/pics/arrow_right.svg"
          onClicked: {
            if (!wRegAddrInput.text || !wRegValueInput.text) return;
            const args = {
              "cmd": PlcMessage.WRITE_REG,
              "addr": Number(wRegAddrInput.text),
              "value": Number(wRegValueInput.text),

            }
            plcRunner.invoke("writeMessage", args);
          }
        }
      }
    }

    // READ_REG
    RowLayout {
      id: rl3

      spacing: 10

      Text {
        Layout.preferredWidth: root.labelWidth
        text: "R_REG:"
        color: Styles.foreground.high
        font: Styles.fonts.body
      }

      RowLayout {
        id: rl31

        spacing: 0

        Label {
          Layout.preferredHeight: root.fieldHeight
          leftPadding: 4; rightPadding: 4;
          font: Styles.fonts.body
          text: "D"
          color: Styles.foreground.high
          verticalAlignment: Text.AlignVCenter
          background: Rectangle {
            color: Styles.background.dp04
            border {
              width: 1
              color: Styles.background.dp12
            }
          }
        }

        QxTextInput {
          id: rRegInput

          Layout.preferredWidth: root.fieldWidth
          Layout.preferredHeight: root.fieldHeight
          radius: 0;
        }

        QxTextInput {
          id: rRegValueInput

          Layout.preferredWidth: root.fieldWidth
          Layout.preferredHeight: root.fieldHeight

          background: Rectangle {
            color: "transparent"
            radius: 0;
            border {
              width: 1
              color: Styles.background.dp12
            }
          }
        }

        QxToolButton {
          id: btnReadReg

          Layout.preferredWidth: root.fieldHeight
          Layout.preferredHeight: root.fieldHeight

          radius: 0
          padding: 6
          enabled: plcRunner && plcRunner.socketState === 3
          imageSource: "qrc:/assets/pics/arrow_right.svg"
          onClicked: {
            if (!rRegInput.text) return;
            const args = {
              "cmd": PlcMessage.READ_REG,
              "addr": Number(rRegInput.text),
            }
            plcRunner.invoke("writeMessage", args);
          }
        }

      }
    }

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
