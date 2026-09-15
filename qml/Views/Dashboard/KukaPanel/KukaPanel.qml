import QtQuick
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

import QDelta.Backend 1.0 as Backend

QxGroupBox {
  id: root

  property bool isValidBlade : false
  readonly property Backend.DeviceRunner rsi: Backend.Hub.device("rsi")

  ColumnLayout {
    id: cl

    spacing: UiMetrics.spacingMedium

    RowLayout {
      id: rl

      spacing: UiMetrics.spacingLarge

      RsiPosition {
        id: cartesianPosition

        enabled: false
        tags: ["X", "Y", "Z", "A", "B", "C"]
        dimension: "mm"
        title: "Cartesian Space"
      }
      RsiPosition {
        id: jointPosition

        enabled: false
        tags: ["A1", "A2", "A3", "A4", "A5", "A6"]
        dimension: "deg"
        title: "Joint Space"
      }
    }

    ColumnLayout {
      id: cl2

      spacing: UiMetrics.spacingSmall

      RowLayout {
        id: rl2

        spacing: UiMetrics.spacingSmall

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
          diameter: UiMetrics.indicatorSizeMedium
          ledColor: Styles.minColor
          isOn: root.rsi?.data.motionActive ?? false
        }
      }
    }
  }
}
