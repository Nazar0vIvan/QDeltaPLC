import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import Styles 1.0
import Components 1.0

Item {
  id: root

  ColumnLayout {
    id: cl

    anchors.fill: parent
    spacing: 30

    Text {
      id: title

      text: qsTr("Schunk FTS Delta-IP68-SI-660-60")
      color: Styles.foreground.high
      font{pixelSize: 20; bold: true}
    }

    RowLayout {
      id: rl

      Layout.fillHeight: true
      Layout.fillWidth: true;

      NetworkUdp {
        id: networkUdpPanel

        title: qsTr("Network")
        Layout.preferredWidth: 300
        Layout.preferredHeight: 300
      }

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
    }
  }
}
