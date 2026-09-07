import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import QtCharts

import Styles 1.0
import Components 1.0

import QDelta.Backend 1.0 as Backend

QxGroupBox {
  id: root

  readonly property var fts: Backend.Hub.device("fts")

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  ColumnLayout {
    id: cl

    spacing: 14

    FtsBars {
      id: bars

      title: qsTr("Monitoring")
    }

    RowLayout {
      id: rl

      QxButton {
        id: btnStart

        checked: root.fts.isStreaming
        text: checked ? "Stop" : "Start"
        onClicked: root.fts.invoke(checked ? "stopStreaming" : "startStreaming")
      }

      QxButton {
        id: btnBias

        enabled: root.fts && root.fts.isStreaming
        text: "Bias"
        onClicked: root.fts.invoke("bias")
      }

      QxButton {
        id: btnLog

        enabled: root.fts.isStreaming
        text: "Record"
        onClicked: root.fts.invoke("startLogRecording")
      }

      QxButton {
        id: btnSaveToFile

        enabled: !root.fts.isStreaming
        text: "Save"
        onClicked: root.fts.invoke("saveLogToDefaultFile")
      }
    }
  }
}

/*
ChartView {
    id: chart
    width: 600; height: 300
    legend.visible: false
    antialiasing: true

    ValueAxis { id: axX; min: 0; max: 10 }   // 5 sec window (for example)
    ValueAxis { id: axY; min: -400; max: 400 } // adjust for your sensor

    LineSeries {
        id: series
        axisX: axX
        axisY: axY
        color: "red"
    }

    Component.onCompleted: chartBridge.setSeries(series)
}
*/
