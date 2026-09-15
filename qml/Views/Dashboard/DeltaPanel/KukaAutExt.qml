pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import RoboCrap.Backend 1.0 as Backend

QxPanel {
  id: root

  required property Backend.DeviceRunner plc

  readonly property bool isAutExt: root.plc
                                   && root.plc.isConnected
                                   && root.plc.data.x1
                                   ? !!root.plc.data.x1[5]
                                   : false

  readonly property int cellState: root.plc
                                   && root.plc.isConnected
                                   && root.plc.data.cellState !== undefined
                                   ? root.plc.data.cellState
                                   : -1

  readonly property var indicators: [
    { label: "AUT_EXT", isOn: root.isAutExt },
    { label: "IDLE", isOn: root.cellState === Backend.PlcMessage.IDLE },
    { label: "RUN", isOn: root.cellState === Backend.PlcMessage.RUN },
    { label: "DONE", isOn: root.cellState === Backend.PlcMessage.FIN }
  ]

  spacing: UiMetrics.spacingSmall

  Repeater {
    model: root.indicators

    delegate: QxHField {
      id: field

      required property var modelData

      labelWidth: UiMetrics.fieldWidthSmall
      labelText: field.modelData.label

      QxLed {
        Layout.alignment: Qt.AlignVCenter
        diameter: UiMetrics.indicatorSizeSmall
        ledColor: Styles.minColor
        isOn: field.modelData.isOn
      }
    }
  }

  RowLayout {
    spacing: UiMetrics.spacingSmall

    QxButton {
      id: btnConnect

      checked: root.plc.isConnected
      text: checked ? "Disconnect" : "Connect"
      onClicked: root.plc.invoke(checked ? "disconnect" : "connect")
    }

    QxButton {
      id: btnStartCell

      text: "Start Program"
      enabled: root.plc.isConnected && root.isAutExt
      onClicked: {
        const args = {
          "cmd": Backend.PlcMessage.SET_VAR,
          "var": Backend.PlcMessage.START_CELL,
          "attr": 1
        }
        root.plc.invoke("writeMessage", args)
      }
    }

    QxButton {
      id: btnSftOk

      text: "Safety Ok"
      enabled: root.plc.isConnected && root.isAutExt
    }
  }
}
