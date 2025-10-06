import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

ColumnLayout {
  id: rool

  spacing: 30

  Text {
    text: qsTr("PLC AS332T-A")
    color: Styles.foreground.high
    font{pixelSize: 20; bold: true}
  }

  RowLayout {
    id: rl

    Layout.fillWidth: true
    Layout.preferredHeight: childrenRect.height
    spacing: 20

    NetworkTcp {
      id: networkTcp

      title: qsTr("Network")
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight
      Layout.alignment: Qt.AlignTop
    }

    DeltaModuleAP {
      id: moduleAP_P

      title: 'AS16AP11<font color="red">P</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: networkTcp.implicitHeight

      // enabled: plcRunner.socketState === 3

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LEDG2", "RUN", "N/D"]
      xLabel: 'IN / <font color="red">SOURCE</font>'
      yLabel: 'OUT / <font color="red">SOURCE</font>'
      xPlugged: [0,0,0,0,0,0,0,0]
      yPlugged: [0,0,0,0,0,1,1,0]
      yDisplayOnly: [6] // !!! RUN
      moduleIndex: 1
    }

    DeltaModuleAP {
      id: moduleAP_T

      title: 'AS16AP11<font color="#509dfd">T</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: networkTcp.implicitHeight

      // enabled: plcRunner.socketState === 3

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LEDR1", "LEDR2", "LEDR3"]
      xLabel: 'IN / <font color="#509dfd">SINK</font>'
      yLabel: 'OUT / <font color="#509dfd">SINK</font>'
      xPlugged: [0,0,0,0,0,0,0,0]
      yPlugged: [0,0,0,0,0,1,1,1]
      moduleIndex: 2
    }

    LedsPanel {
      id: ledPanel

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight
      Layout.alignment: Qt.AlignTop


    }

    Item { Layout.fillWidth: true }
  }
}

