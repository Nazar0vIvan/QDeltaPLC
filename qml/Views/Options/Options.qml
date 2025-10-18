import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ListView {
  id: root

  function onApply(runner, la, lp, pa, pp) {
    if (!runner)
      return
    runner.invoke("setSocketConfig", {
                    "localAddress": la.text,
                    "localPort": Number(lp.text),
                    "peerAddress": pa.text,
                    "peerPort": Number(pp.text)
                  })
  }

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 126

  topMargin: 14
  bottomMargin: 14
  leftMargin: 14
  rightMargin: 14

  spacing: 16
  interactive: false

  header: Text {
    height: 30
    text: qsTr("Options")
    font: Styles.fonts.title
    color: Styles.foreground.high
  }

  model: ObjectModel {

    Rectangle {
      // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop {
          position: 0.0
          color: Styles.secondary.dark
        }
        GradientStop {
          position: 1.0
          color: Styles.background.dp00
        }
        orientation: Gradient.Horizontal
      }
    }

    QxField {

      // pc address
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

    Rectangle {
      // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop {
          position: 0.0
          color: Styles.secondary.dark
        }
        GradientStop {
          position: 1.0
          color: Styles.background.dp00
        }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection {

      // rsi config
      id: rsiOptions

      title: "Robot Sensor Interface"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField {
        // config file
        id: uploadFileField

        labelText: "Configuration File :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxUploadFile {
          id: uploadFile

          height: parent.height
          fieldWidth: 400
          imageSource: "qrc:/assets/pics/open.svg"

          onUploaded: path => {
                        rsiRunner.invoke("parseConfigFile", {
                                           "path": path
                                         })
                        rsiBtnApply.enabled = true
                      }

          Connections {
            target: rsiRunner
            function onResultReady(method, out) {
              if (method === "parseConfigFile" && out) {
                uploadFile.text = out.path
                rsiLp.text = out.port
                rsiOnlysend.text = out.onlysend
              }
            }
          }
        }
      }

      QxField {
        // rsi local port
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

      QxField {

        // rsi onlysend
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

      QxField {

        // rsi peer address
        id: rsiPaField

        labelText: "RSI Address :"
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
          onTextEdited: rsiBtnApply.enabled = true
        }
      }

      QxField {
        // rsi peer port
        id: rsippField

        labelText: "RSI Port :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiPp

          height: root.fieldHeight
          width: root.fieldWidth
          text: "1111"
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
          onTextEdited: rsiBtnApply.enabled = true
        }
      }

      QxButton {
        // rsi apply button
        id: rsiBtnApply

        text: "Apply"
        enabled: pcAddr.text && rsiLp.text && rsiPa.text && rsiPp.text && !rsiRunner.isStreaming

        onClicked: {
          onApply(rsiRunner, pcAddr, rsiLp, rsiPa, rsiPp)
        }
      }
    }

    Rectangle {
      // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop {
          position: 0.0
          color: Styles.secondary.dark
        }
        GradientStop {
          position: 1.0
          color: Styles.background.dp00
        }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection {
      // plc config
      id: plcOptions

      title: "PLC AS332T-A"
      Layout.fillWidth: true
      gap: 20
      spacing: 12

      QxField {
        // plc local port
        id: plcLpField

        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: root.fieldHeight
        labelWidth: root.labelWidth
        labelText: "PC Port :"

        QxTextInput {
          id: plcLp

          width: root.fieldWidth
          height: root.fieldHeight
          text: "1234"
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
          onTextEdited: plcBtnApply.enabled = true
        }
      }

      QxField {
        // plc peer address
        id: plcPaField

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
          onTextEdited: plcBtnApply.enabled = true
        }
      }

      QxField {
        // plc peer port
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
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
          onTextEdited: plcBtnApply.enabled = true
        }
      }

      QxButton {
        // plc apply button
        id: plcBtnApply

        text: "Apply"
        enabled: pcAddr.text && plcLp.text && plcPa.text && plcPp.text
                 && plcRunner.socketState !== 3 // 3 - connected

        onClicked: {
          onApply(plcRunner, pcAddr, plcLp, plcPa, plcPp)
        }
      }
    }

    Rectangle {
      // separator
      width: 400
      height: 1
      gradient: Gradient {
        GradientStop {
          position: 0.0
          color: Styles.secondary.dark
        }
        GradientStop {
          position: 1.0
          color: Styles.background.dp00
        }
        orientation: Gradient.Horizontal
      }
    }

    OptionsSection {
      // fts config
      id: ftsOptions

      title: "Schunk FTS Delta-SI-660-60"
      Layout.fillWidth: true
      gap: 20
      spacing: 12
      enabled: !ftsRunner.isStreaming

      QxField {
        // fts local port
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
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
          onTextEdited: ftsBtnApply.enabled = true
        }
      }

      QxField {
        // fts peer address
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
          onTextEdited: ftsBtnApply.enabled = true
        }
      }

      QxField {
        // fts peer port
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
        }
      }

      QxButton {
        // fts apply button
        id: ftsBtnApply

        text: "Apply"
        enabled: pcAddr.text && ftsLp.text && ftsPa.text && ftsPp.text && !ftsRunner.isStreaming

        onClicked: {
          onApply(ftsRunner, pcAddr, ftsLp, ftsPa, ftsPp)
        }
      }
    }
  }
}
