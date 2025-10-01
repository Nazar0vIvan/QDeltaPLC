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

      title: qsTr("AS16AP11P-A")
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      xLabel: 'IN / <font color="red">SOURCE</font'
      yLabel: 'OUT / <font color="red">SOURCE</font'
      moduleIndex: 1
    }
/*
    DeltaModuleAP {
      id: moduleAP_T

      title: qsTr("AS16AP11T-A")
      Layout.preferredWidth: implicitWidth;
      //Layout.preferredHeight: ledPanel.implicitHeight

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      xLabel: 'IN / <font color="blue">SINK</font'
      yLabel: 'OUT / <font color="blue">SINK</font'
      moduleIndex: 2
    }

    LedsPanel {
      id: ledPanel

      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight
    }
*/
    Item { Layout.fillWidth: true }
  }
}

