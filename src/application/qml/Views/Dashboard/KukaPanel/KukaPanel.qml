import QtQuick
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import robocrap_qml_module 1.0 as Backend

QxGroupBox {
  id: root

  property bool isValidBlade : false
  readonly property Backend.DeviceRunner rsi: Backend.Hub.device("rsi")

  ColumnLayout {
    id: cl

    spacing: Metrics.sp12

    ColumnLayout {
      id: cl2

      spacing: Metrics.sp8

      RowLayout {
        id: rl2

        spacing: Metrics.sp8

        QxButton {
          id: genTraj

          text: "Generate Trajectory"
          onClicked: root.rsi.invoke("generateTrajectory")
        }

        QxButton {
          id: btnStart

          enabled: root.rsi
                   && root.rsi.isConnected
                   && (root.rsi.data.trajectoryReady ?? false)
          text: root.rsi?.data.motionActive ? "Stop RSI" : "Start RSI"
          onClicked: root.rsi.invoke(root.rsi.data.motionActive ? "stopStreaming" : "startStreaming")
        }

        QxLed {
          id: ledRsi

          Layout.alignment: Qt.AlignVCenter
          diameter: Metrics.sz20
          ledColor: Colors.minColor
          isOn: root.rsi?.data.motionActive ?? false
        }
      }
    }
  }
}
