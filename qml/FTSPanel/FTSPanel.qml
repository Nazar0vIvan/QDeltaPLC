import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import Styles 1.0
import Components 1.0

ColumnLayout {
  id: root

  spacing: 30

  Text {
    id: title

    text: qsTr("Schunk FTS Delta-IP68-SI-660-60")
    color: Styles.foreground.high
    font{pixelSize: 20; bold: true}
  }

  RowLayout {
    id: rl

    Layout.preferredHeight: 300
    Layout.fillWidth: true

    NetworkUdp {
      id: networkUdpPanel

      title: qsTr("Network")
      Layout.preferredHeight: parent.height
      Layout.preferredWidth: 300
    }

    ColumnLayout {
      id: barsCL

      Layout.preferredHeight: parent.height
      Layout.preferredWidth: 300

      QxProgressBar {
        id: fz

        Layout.preferredHeight: 20
        Layout.preferredWidth: parent.width
        Layout.alignment: Qt.AlignTop

        from: -2000
        to:   2000
        color: "orange"
        labelText: qsTr("Fz")

        value: ftsRunner.lastReading ? Number(ftsRunner.lastReading) : 0.0
      }
    }

    Item { Layout.fillWidth: true }
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
