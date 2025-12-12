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

    function onTrajectoryReady() {
      btnStart.enabled = true;
    }

    function onMotionFinished() {
      ledRsi.color = "red";
    }

    function onMotionStarted() {
      ledRsi.color = "green";
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
        text: rsiRunner && rsiRunner.motionActive ? "Stop RSI" : "Start RSI"
        onClicked: {
          if (!rsiRunner) return;
          if (rsiRunner.motionActive) {
            rsiRunner.invoke("stopStreaming");
          } else {
            rsiRunner.invoke("startStreaming");
          }
        }
      }
      Rectangle {
        id: ledRsi

        Layout.preferredWidth: 20
        Layout.preferredHeight: 20
        radius: 10
        color: "red"
      }
    }
  }
}
