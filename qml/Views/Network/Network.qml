import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Components 1.0
import Styles 1.0

import qdeltaplc_qml_module 1.0

Item  {
  id: root

  DeviceProfiles {
    id: deviceProfiles
  }

  readonly property var selectedDevice:
      deviceProfiles.device(cbDevice.currentIndex)

  ColumnLayout {
    id: cl

    anchors.fill: parent
    anchors.margins: 14

    spacing: 20

    Label {
      id: titleView

      Layout.preferredHeight: implicitHeight
      text: qsTr("Network")
      font: Styles.fonts.title
      color: Styles.foreground.high
    }

    NetworkPanel {
      id: configPanel

      title: "Configuration"

      QxComboBox {
        id: cbDevice

        label: qsTr("Device")
        model: deviceProfiles.names
      }

      QxVField {
        id: laField

        labelText: "Local Address"

        QxTextInput {
          id: laInput

          height: 32
          width: 160
          text: root.selectedDevice.localAddress ?? ""
          validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
        }
      }

      QxVField {
        id: lpField

        labelText: "Local Port"

        QxTextInput {
          id: lpInput

          height: 32
          width: 160

          text: root.selectedDevice.localPort >= 0
                ? String(root.selectedDevice.localPort)
                : "N/D"
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
        }
      }

      QxVField {
        id: paField

        labelText: "Peer Address"

        QxTextInput {
          id: paInput

          height: 32
          width: 160

          text: root.selectedDevice.peerAddress ?? ""
          validator: RegularExpressionValidator {
            regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
          }
        }
      }

      QxVField {
        id: ppField

        labelText: "Peer Port"

        QxTextInput {
          id: ppInput

          height: 32
          width: 160

          text: root.selectedDevice.peerPort >= 0
                ? String(root.selectedDevice.peerPort)
                : "N/D"
          validator: IntValidator {
            bottom: 0
            top: 65535
          }
        }
      }

      QxButton {
        id: btnCon

        text: "Connect"
        Layout.alignment: Qt.AlignBottom
        Layout.bottomMargin: 3

      }
    }

    NetworkPanel {
      id: conPanel

      title: "Connections"

      Layout.alignment: Qt.AlignVCenter
    }

    Item { Layout.fillHeight: true }
  }
}
