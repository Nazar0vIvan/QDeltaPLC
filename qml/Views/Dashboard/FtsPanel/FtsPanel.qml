import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import RoboCrap.Backend 1.0 as Backend

QxGroupBox {
  id: root

  readonly property Backend.DeviceRunner fts: Backend.Hub.device("fts")
  readonly property bool connected: root.fts?.isConnected ?? false
  readonly property bool streaming: root.fts?.data.streaming ?? false
  readonly property bool startPending: root.fts?.data.startPending ?? false
  readonly property bool stopRequested: root.fts?.data.stopRequested ?? false
  readonly property bool recording: root.fts?.data.recording ?? false
  readonly property bool saving: root.fts?.data.saving ?? false

  ColumnLayout {
    id: cl

    spacing: Metrics.sp12

    Text {
      Layout.fillWidth: true
      visible: !root.connected
      text: qsTr("Connect FTS in Settings.")
      color: Colors.foreground.medium
      font: Fonts.caption
      wrapMode: Text.WordWrap
    }

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

        enabled: root.connected && !root.startPending && !root.saving
        checked: root.streaming
        text: root.streaming ? qsTr("Stop") : qsTr("Start")
        onClicked: root.fts.invoke(root.streaming ? "stopStreaming" : "startStreaming")
      }

      QxButton {
        id: btnBias

        enabled: root.connected && root.streaming && !root.startPending && !root.stopRequested
        text: qsTr("Bias")
        onClicked: root.fts.invoke("bias")
      }

      QxButton {
        id: btnLog

        enabled: root.connected && (root.fts?.data.canRecord ?? false)
        checked: root.recording
        text: root.recording ? qsTr("Recording…") : qsTr("Record")
        onClicked: root.fts.invoke("startLogRecording")
      }

      QxButton {
        id: btnSaveToFile

        enabled: root.connected && (root.fts?.data.canSave ?? false)
        text: root.saving ? qsTr("Saving…") : qsTr("Save")
        onClicked: root.fts.invoke("saveLogToDefaultFile")
      }
    }
  }
}
