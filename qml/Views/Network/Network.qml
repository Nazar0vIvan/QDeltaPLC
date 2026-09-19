import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import RoboCrap.Backend 1.0 as Backend

QxScrollView {
  id: root

  Backend.DeviceProfileModel {
    id: devProfModel
  }

  readonly property list<Backend.DeviceRunner> runners: [
    Backend.Hub.device("plc"),
    Backend.Hub.device("rsi"),
    Backend.Hub.device("fts"),
    null
  ]

  readonly property Backend.DeviceRunner currentRunner:
    cbDevice.currentIndex >= 0 && cbDevice.currentIndex < runners.length
      ? runners[cbDevice.currentIndex]
      : null

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

  spacing: Metrics.sp16

  Label {
    text: qsTr("Network")
    font: Fonts.title
    color: Colors.foreground.high
  }

  NetworkPanel {
    id: configPanel

    Layout.fillWidth: true
    title: "Configuration"
    contentHorizontalMargin: Metrics.sp12
    contentVerticalMargin: Metrics.sp16
    spacing: Metrics.sp8

    QxVField {
      labelText: qsTr("Device")

      QxComboBox {
        id: cbDevice

        Layout.preferredWidth: Metrics.w200
        model: devProfModel.names
      }
    }

    QxVField {
      labelText: "Local Address"

      QxTextInput {
        id: laInput

        Layout.preferredWidth: Metrics.w120
        text: root.selectedDevice.localAddress ?? ""
        validator: RegularExpressionValidator {
          regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }
      }
    }

    QxVField {
      labelText: "Local Port"

      QxTextInput {
        id: lpInput

        Layout.preferredWidth: Metrics.w80
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
      labelText: "Peer Address"

      QxTextInput {
        id: paInput

        Layout.preferredWidth: Metrics.w120
        text: root.selectedDevice.peerAddress ?? ""
        validator: RegularExpressionValidator {
          regularExpression: /^(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)){3}$/
        }
      }
    }

    QxVField {
      labelText: "Peer Port"

      QxTextInput {
        id: ppInput

        Layout.preferredWidth: Metrics.w80
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

      readonly property Backend.DeviceRunner runner: root.currentRunner

      Layout.alignment: Qt.AlignBottom
      checked: runner && runner.isConnected
      text: checked ? qsTr("Disconnect") : qsTr("Connect")

      enabled: {
        if (!runner)
          return false
        if (runner.isConnected)
          return true
        if (!runner.isDisconnected)
          return false

        return laInput.acceptableInput
            && lpInput.acceptableInput
            && paInput.acceptableInput
            && (root.selectedDevice.peerPort < 0 || ppInput.acceptableInput)
      }

      onClicked: {
        if (!runner)
          return

        if (runner.isConnected)
          runner.invoke("disconnect")
        else
          runner.invoke("connect", root.currentSocketConfig())
      }
    }
  }

  NetworkPanel {
    id: conPanel

    Layout.fillWidth: true
    title: "Connections"
    contentVerticalMargin: Metrics.sp12

    ConnectionsTable {
      id: conTable

      Layout.fillWidth: true
      model: devProfModel
      runners: root.runners
      selectedRow: cbDevice.currentIndex
    }
  }
}
