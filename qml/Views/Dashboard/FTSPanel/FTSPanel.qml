import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  ColumnLayout {
    id: cl

    FtsBars { id: bars }
  }
}







// ChartView {
//     id: chart
//     width: 600; height: 300
//     legend.visible: false
//     antialiasing: true

//     ValueAxis { id: axX; min: 0; max: 10 }   // 5 sec window (for example)
//     ValueAxis { id: axY; min: -400; max: 400 } // adjust for your sensor

//     LineSeries {
//         id: series
//         axisX: axX
//         axisY: axY
//         color: "red"
//     }

//     Component.onCompleted: chartBridge.setSeries(series)
// }
