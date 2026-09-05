import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Components 1.0
import Styles 1.0

import QDelta.Backend 1.0 as Backend

Item  {
  id: root

  Backend.DeviceProfileModel {
    id: devProfModel
  }

  readonly property var runners: [
    plcRunner, // 0 - PLC
    rsiRunner, // 1 - RSI
    ftsRunner, // 2 - FTS
    null       // 3 - VFD (replace with vfdRunner when available)
  ]

  readonly property var currentRunner:
      cbDevice.currentIndex >= 0 && cbDevice.currentIndex < runners.length ?
      runners[cbDevice.currentIndex] : null

  readonly property var selectedDevice:
    devProfModel.device(cbDevice.currentIndex)

  function currentSocketConfig() {
    const config = {
      localAddress: laInput.text,
      localPort: Number(lpInput.text),
      peerAddress: paInput.text
    }

    if (root.selectedDevice.peerPort >= 0)
      config.peerPort = Number(ppInput.text)

    return config
  }

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

      contentHorizontalMargin: 14
      contentVerticalMargin: 16

      QxComboBox {
        id: cbDevice

        label: qsTr("Device")
        model: devProfModel.names
      }

      QxVField {
        id: laField

        labelText: "Local Address"

        QxTextInput {
          id: laInput

          height: 32
          width: 120
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
          width: 90

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
          width: 120

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
          width: 90

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

        readonly property var runner: root.currentRunner


        Layout.alignment: Qt.AlignBottom
        Layout.bottomMargin: 3

        checked: runner && runner.isConnected

        text: checked ? qsTr("Disconnect") : qsTr("Connect")

        enabled: {
          if (!runner) return false
          // Disconnect must always remain available.
          if (runner.isConnected) return true
          // Don't allow another request during a transitional socket state.
          if (!runner.isDisconnected) return false

          return laInput.acceptableInput
            && lpInput.acceptableInput
            && paInput.acceptableInput
            && (root.selectedDevice.peerPort < 0 || ppInput.acceptableInput)
        }
        onClicked: {
          if (!runner) return

          if (runner.isConnected) {
            runner.invoke("disconnect")
          } else {
            runner.invoke("connect", root.currentSocketConfig())
          }
        }
      }
    }

    NetworkPanel {
      id: conPanel

      title: "Connections"

      contentVerticalMargin: 14

      ConnectionsTable {
        id: conTable

        Layout.fillWidth: true
        model: devProfModel
        runners: root.runners
        selectedRow: cbDevice.currentIndex
      }

    }

    Item { Layout.fillHeight: true }
  }
}
