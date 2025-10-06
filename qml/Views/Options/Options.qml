import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ListView {
  id: root

  property int fieldHeight: 28
  property int fieldWidth: 120
  property int labelWidth: 100

  header: Text {
    text: qsTr("Options")
    font: Styles.fonts.title
    color: Styles.foreground.high
  }

  model: ObjectModel {
    OptionsSection { // rsi config

      id: rsiOptions

      title: "Robot Sensor Interface"
      Layout.fillWidth: true

      QxField { // config file

        id: uploadFileField

        labelText: "Configuration File :"
        height: root.fieldHeight

        QxUploadFile {
          id: uploadFile

          height: parent.height
          width: 200
        }
      }
      QxField { // rsi local address

        id: rsiLaField

        labelText: "Local Address :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: rsiLa

          height: root.fieldHeight
          placeholder: "defined by Configuration File"
          readOnly: true
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
    OptionsSection {
      id: plcOptions

      title: "PLC AS332T-A"
      Layout.fillWidth: true

      QxField {
        // plc local address
        id: plcLaField

        labelText: "PC IP :"
        height: root.fieldHeight
        labelWidth: root.labelWidth

        QxTextInput {
          id: plcLla

          height: root.fieldHeight
          width: root.fieldWidth
          text: "192.168.1.1"
          readOnly: true
        }
      }
    }
  }
  Item {
    Layout.fillHeight: true
  }
}
