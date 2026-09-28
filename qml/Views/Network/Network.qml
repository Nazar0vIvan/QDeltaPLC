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

  readonly property Backend.DeviceRunner currentRunner:
    Backend.Hub.device(root.selectedDevice.driver ?? "")

  readonly property var selectedDevice:
    devProfModel.device(cbDevice.currentIndex)

  readonly property bool usesPeerPort: root.selectedDevice.driver !== "rsi"

  // Fields are a draft. Switching profiles discards unsent edits and restores
  // the last accepted configuration, or the JSON defaults before first use.
  function loadDraft() {
    const applied = root.currentRunner ? root.currentRunner.data.connectionConfig : {}
    const source = Object.keys(applied).length > 0 ? applied : root.selectedDevice
    laInput.text = source.localAddress ?? ""
    lpInput.text = source.localPort >= 0 ? String(source.localPort) : ""
    paInput.text = source.peerAddress ?? ""
    ppInput.text = root.usesPeerPort && source.peerPort >= 0 ? String(source.peerPort) : ""
  }

  Component.onCompleted: root.loadDraft()

  function currentSocketConfig() {
    const config = {
      localAddress: laInput.text,
      localPort: Number(lpInput.text),
      peerAddress: paInput.text
    }

    if (root.usesPeerPort)
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
    title: qsTr("Configuration")
    contentHorizontalMargin: Metrics.sp12
    contentVerticalMargin: Metrics.sp16
    spacing: Metrics.sp8

    QxVField {
      labelText: qsTr("Device")

      QxComboBox {
        id: cbDevice

        Layout.preferredWidth: Metrics.w200
        model: devProfModel.names
        onActivated: index => root.loadDraft()
      }
    }

    QxVField {
      labelText: "Local Address"

      QxTextInput {
        id: laInput

        Layout.preferredWidth: Metrics.w120
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
        validator: IntValidator {
          bottom: 1
          top: 65535
        }
      }
    }

    QxVField {
      labelText: "Peer Address"

      QxTextInput {
        id: paInput

        Layout.preferredWidth: Metrics.w120
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
        enabled: root.usesPeerPort
        placeholder: root.usesPeerPort ? "" : qsTr("Learned")
        validator: IntValidator {
          bottom: 1
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
        if (runner && runner.isConnected)
          return true
        if (runner && !runner.isDisconnected)
          return false

        return laInput.acceptableInput && lpInput.acceptableInput
            && paInput.acceptableInput
            && (!root.usesPeerPort || ppInput.acceptableInput)
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
    title: qsTr("Connections")
    contentVerticalMargin: Metrics.sp12

    ConnectionsTable {
      id: conTable

      Layout.fillWidth: true
      model: devProfModel
      selectedRow: cbDevice.currentIndex
    }
  }
}
