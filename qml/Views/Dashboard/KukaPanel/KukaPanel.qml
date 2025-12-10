import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  property int switchWidth: 20
  property int switchHeight: 10

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  Connections {
    target: rsiRunner

    function onSocketReady() {
      btnStart.enabled = true;
    }
  }

  ColumnLayout {
    id: cl

    spacing: 14

    RowLayout {
      id: rl

      spacing: 20

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
      // QxSwitch {
      //   id: swMode // jog / program

      //   Layout.preferredWidth: root.switchWidth
      //   Layout.preferredHeight: root.switchHeight
      // }
    }

    RowLayout {
      id: rl2

      spacing: 10

      QxButton {
        id: genTraj

        text: "Generate Trajectory"
        enabled: rsiRunner
        onClicked: {
          if (!rsiRunner) return;
          rsiRunner.invoke("generateTrajectory");
        }
      }

      QxButton {
        id: btnStart

        enabled: false
        text: checked ? "Stop RSI" : "Run RSI"
        onClicked: {
          if (!rsiRunner) return;
          if (checked) {
            rsiRunner.invoke("stopStreaming");
          } else {
            rsiRunner.invoke("startStreaming");
          }
        }
      }
      Rectangle {
        id: ledRsiOn

        Layout.preferredWidth: 20
        Layout.preferredHeight: 20

        radius: 10
        color: rsiRunner.isStreaming ? "green" : "red"
      }
    }
  }
}
