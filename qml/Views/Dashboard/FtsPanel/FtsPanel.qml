import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import RoboCrap.Backend 1.0 as Backend

QxGroupBox {
  id: root

  readonly property Backend.DeviceRunner fts: Backend.Hub.device("fts")

  ColumnLayout {
    id: cl

    spacing: Metrics.sp12

    FtsBars {
      id: bars

      fts: root.fts

      title: qsTr("Monitoring")
    }

    RowLayout {
      id: rl

      spacing: Metrics.sp8

      QxButton {
        id: btnStart

        enabled: root.fts?.isConnected ?? false
        checked: root.fts?.data.streaming ?? false
        text: checked ? "Stop" : "Start"
        onClicked: root.fts.invoke(checked ? "stopStreaming" : "startStreaming")
      }

      QxButton {
        id: btnBias

        enabled: root.fts?.data.streaming ?? false
        text: "Bias"
        onClicked: root.fts.invoke("bias")
      }

      QxButton {
        id: btnLog

        enabled: root.fts?.data.streaming ?? false
        text: "Record"
        onClicked: root.fts.invoke("startLogRecording")
      }

      QxButton {
        id: btnSaveToFile

        enabled: !(root.fts?.data.streaming ?? false)
        text: "Save"
        onClicked: root.fts.invoke("saveLogToDefaultFile")
      }
    }
  }
}
