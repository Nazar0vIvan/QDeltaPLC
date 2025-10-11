import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import QtCharts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  ColumnLayout {
    id: cl

    spacing: 14

    FtsBars {
      id: bars

      title: qsTr("Monitoring")
    }

    QxButton {
      id: btnStart

      checked: ftsRunner && ftsRunner.isStreaming
      enabled: ftsRunner && ftsRunner.socketState === 4 // BoundState
      text: checked ? "Stop" : "Start"
      onClicked: {
        if (!ftsRunner) return
        if (checked) {
            ftsRunner.stopStreaming()
        } else {
            ftsRunner.startStreaming()
        }
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
