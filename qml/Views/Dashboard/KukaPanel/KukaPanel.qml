import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  Connections {
    target: rsiRunner

    function onRsiReady() {
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

        tags: ["X", "Y", "Z", "A", "B", "C"]
        dimension: "mm"
        title: "Cartesian Space"
      }
      RsiPosition {
        id: jointPosition

        tags: ["A1", "A2", "A3", "A4", "A5", "A6"]
        dimension: "deg"
        title: "Joint Space"
      }
    }

    QxButton {
      id: genTraj

      text: "GenTraj"
      enabled: rsiRunner
      onClicked: {
        if (!rsiRunner) return;
        rsiRunner.invoke("generateTrajectory");
      }
    }

    QxButton {
      id: btnStart

      enabled: false
      text: checked ? "Stop" : "Start"
      onClicked: {
        if (!rsiRunner) return;
        if (checked) {
          rsiRunner.invoke("stopStreaming");
        } else {
          rsiRunner.invoke("startStreaming");
        }
      }
    }
  }
}
