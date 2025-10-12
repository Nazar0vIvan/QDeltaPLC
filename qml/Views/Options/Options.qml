import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ListView {
  id: root

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 126

  topMargin: 14; bottomMargin: 14
  leftMargin: 14; rightMargin: 14

  spacing: 16
  interactive: false

  header: Text {
    height: 30
    text: qsTr("Options")
    font: Styles.fonts.title
    color: Styles.foreground.high
  }

  model: ObjectModel {

    Rectangle { // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    QxField { // pc address

      id: pcAddrField

      labelText: "PC Address:"
      height: root.fieldHeight
      labelWidth: root.labelWidth

      QxTextInput {
        id: pcAddr

        height: root.fieldHeight
        width: root.fieldWidth
        text: "192.168.1.100"
      }
    }

    Rectangle { // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection { // rsi config

      id: rsiOptions

      title: "Robot Sensor Interface"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField { // config file

        id: uploadFileField

        labelText: "Configuration File :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxUploadFile {
          id: uploadFile

          height: parent.height
          fieldWidth: 400
          imageSource: "qrc:/assets/pics/open.svg"
        }
      }

      QxField { // rsi local address

        id: rsiLpField

        labelText: "Local Port :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiLp

          height: root.fieldHeight
          placeholder: "defined by Configuration File"
          readOnly: true
        }
      }

      QxField { // rsi onlysend

        id: rsiOnlysendField

        labelText: "ONLYSEND :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiOnlysend

          height: root.fieldHeight
          placeholder: "defined by Configuration File"
          readOnly: true
        }
      }

      QxField { // rsi peer address

        id: rsiPaField

        labelText: "Peer Address :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiPa

          height: root.fieldHeight
          width: root.fieldWidth
          text: "192.168.1.4"
          validator: RegularExpressionValidator {
              regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
        }
      }

      QxField { // rsi peer port
        id: rsippField

        labelText: "Peer Port :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiPp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "1111"
          validator: IntValidator{ bottom: 0; top: 65535; }
        }
      }
    }

    Rectangle { // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection { // plc config
      id: plcOptions

      title: "PLC AS332T-A"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField { // plc local port
        id: plcLpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PC Port :"

        QxTextInput {
          id: plcLp

          width: root.fieldWidth
          height: root.fieldHeight
          text: "3333"
          validator: IntValidator{ bottom: 0; top: 65535; }
          onTextEdited: btnApply.enabled = true
        }

      }

      QxField { // plc peer address
        id:  plcPaField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PLC Address :"

        QxTextInput {
          id: plcPa

          width: root.fieldWidth
          height: root.fieldHeight
          text: "192.168.1.2"
          validator: RegularExpressionValidator {
              regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
          onTextEdited: btnApply.enabled = true
        }
      }

      QxField { // plc peer port
        id: plcPpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PLC Port :"

        QxTextInput {
          id: plcPp

          width: root.fieldWidth
          height: root.fieldHeight
          text: "5051"
          validator: IntValidator{ bottom: 0; top: 65535; }
          onTextEdited: btnApply.enabled = true
        }
      }
    }

    Rectangle { // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop { position: 0.0; color: Styles.secondary.dark }
        GradientStop { position: 1.0; color: Styles.background.dp00 }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection { // rdt config
      id: ftsOptions

      title: "Schunk FTS Delta-SI-660-60"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField { // local port
        id: ftsLpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PC Port :"

        QxTextInput {
          id: ftsLp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "59152"
          validator: IntValidator { bottom: 0; top: 65535 }
          onTextEdited: btnApply.enabled = true
        }
      }

      QxField { // peer address
        id: ftsPaField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "FTS Address :"

        QxTextInput {
          id: ftsPa

          height: root.fieldHeight
          width: root.fieldWidth
          text: "192.168.1.3"
          validator: RegularExpressionValidator {
              regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
          onTextEdited: btnApply.enabled = true
        }
      }

      QxField { // peer port
        id: ftsPpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "FTS Port :"

        QxTextInput {
          id: ftsPp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "49152"
          readOnly: true

          onTextEdited: btnApply.enabled = true
        }
      }

      Rectangle { // separator
        width: 400
        height: 1
        gradient: Gradient {
          GradientStop { position: 0.0; color: Styles.secondary.dark }
          GradientStop { position: 1.0; color: Styles.background.dp00 }
          orientation: Gradient.Horizontal
        }
      }
    }

    QxButton { // apply button
      id: btnApply

      text: "Apply"

      onClicked: {
        plcRunner.setSocketConfig({
          localAddress: pcAddr.text,
          localPort:    Number(plcLp.text),
          peerAddress:  plcPa.text,
          peerPort:     Number(plcPp.text)
        })
        ftsRunner.setSocketConfig({
          localAddress: pcAddr.text,
          localPort:    Number(ftsLp.text),
          peerAddress:  ftsPa.text,
          peerPort:     Number(ftsPp.text)
        })
        enabled = false
      }
    }
  }
  Item { Layout.fillHeight: true }
}
