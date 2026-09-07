import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import QDelta.Backend 1.0 as Backend

QxGroupBox {
  id: root

  property bool isValidBlade : false
  readonly property var rsi: Backend.Hub.device("rsi")

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  Connections {
    target: root.rsi

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
    }

    ColumnLayout {
      id: cl2

      spacing: 10

      RowLayout {
        id: rl2

        spacing: 10

        QxButton {
          id: genTraj

          text: "Generate Trajectory"
          onClicked: root.rsi.invoke("generateTrajectory")
        }

        QxButton {
          id: btnStart

          enabled: false
          text: root.rsi.motionActive ? "Stop RSI" : "Start RSI"
          onClicked: root.rsi.invoke(root.rsi.motionActive ? "stopStreaming" : "startStreaming")
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
}
