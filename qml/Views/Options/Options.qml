import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ListView {
  id: root

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 126

  topMargin: 14
  bottomMargin: 14
  leftMargin: 14
  rightMargin:14

  header: Text {
    height: 30
    text: qsTr("Options")
    font: Styles.fonts.title
    color: Styles.foreground.high
  }

  spacing: 16

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

      id: plcLaField

      labelText: "PC Address:"
      height: root.fieldHeight
      labelWidth: root.labelWidth

      QxTextInput {
        id: plcLa

        height: root.fieldHeight
        width: root.fieldWidth
        text: "192.168.1.1"
        readOnly: true
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
          fieldWidth: 200
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

      QxField { // rsi peer address

        id: rsiPaField

        labelText: "Peer Address :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiPa

          height: root.fieldHeight
          placeholder: "defined by Configuration File"
          readOnly: true
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
        }
      }

      QxField { // peer port
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

    OptionsSection {
      id: rdtOptions

      title: "Schunk FTS Delta-SI-660-60"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField { // local port
        id: rdtLpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PC Port :"

        QxTextInput {
          id: rdLp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "59152"
          validator: IntValidator { bottom: 0; top: 65535 }
        }
      }

      QxField { // peer address
        id: rdtPaField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "FTS IP :"

        QxTextInput {
          id: rdtPa

          height: root.fieldHeight
          width: root.fieldWidth
          text: "192.168.1.3"
          validator: RegularExpressionValidator {
              regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
        }
      }

      QxField { // peer port
        id: rdtPpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "FTS Port :"

        QxTextInput {
          id: rdtPp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "49152"
          readOnly: true
        }
      }

      Rectangle { // separator
        width: 300
        height: 1
        gradient: Gradient {
          GradientStop { position: 0.0; color: Styles.secondary.dark }
          GradientStop { position: 1.0; color: Styles.background.dp00 }
          orientation: Gradient.Horizontal
        }
      }
    }

    QxButton {
      id: btnApply

      text: "Apply"
    }
  }
  Item {
    Layout.fillHeight: true
  }
}
