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

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LED_GREEN", "RUN", "N/D"]
      xLabel: 'IN / <font color="red">SOURCE</font>'
      yLabel: 'OUT / <font color="red">SOURCE</font>'
      xEnabled: [0,0,0,0,0,0,0,0]
      yEnabled: [0,0,0,0,0,1,1,0]
      moduleIndex: 1


    }

    DeltaModuleAP {
      id: moduleAP_T

      title: 'AS16AP11<font color="#509dfd">T</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: networkTcp.implicitHeight

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LED_RED1", "LED_RED2", "LED_RED3"]
      xLabel: 'IN / <font color="#509dfd">SINK</font>'
      yLabel: 'OUT / <font color="#509dfd">SINK</font>'
      xEnabled: [0,0,0,0,0,0,0,0]
      yEnabled: [0,0,0,0,0,1,1,1]
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

