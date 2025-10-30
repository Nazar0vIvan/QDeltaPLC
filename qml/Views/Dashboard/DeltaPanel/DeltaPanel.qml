import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import qdeltaplc_qml_module 1.0 // FOR NOW

QxGroupBox {
  id: root

  implicitWidth: leftPadding + rl.implicitWidth + rightPadding
  implicitHeight: topPadding + rl.implicitHeight + bottomPadding

  Connections {
    target: plcRunner

    function onPlcDataReady(data) {
      if (!data.cmd) return;
      switch(data.cmd) {
        case PlcMessage.SNAPSHOT: {
          moduleAP_P.refreshAll(data.x1, data.y1);
          moduleAP_T.refreshAll(data.x2, data.y2);
          break;
        }
        case PlcMessage.WRITE_IO: {
          console.log("data module: ", data.module)
          console.log("data state: ", data.state)
          if (data.module === 1)
            moduleAP_P.refreshY(data.state)
          else
            moduleAP_T.refreshY(data.state)
          break;
        }
        default: break;
      }
    }
  }

  RowLayout {
    id: rl

    spacing: 20

    DeltaModuleAP {
      id: moduleAP_P

      title: 'AS16AP11<font color="red">P</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      enabled: plcRunner.socketState === 3

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LEDG2", "RUN", "N/D"]
      xLabel: 'IN / <font color="red">SOURCE</font>'
      yLabel: 'OUT / <font color="red">SOURCE</font>'
      xPlugged: [0, 0, 0, 0, 0, 0, 0, 0]
      yPlugged: [0, 0, 0, 0, 0, 1, 1, 0]
      yDisplayOnly: [6] // !!! RUN
      moduleIndex: 1
    }

    DeltaModuleAP {
      id: moduleAP_T

      title: 'AS16AP11<font color="#509dfd">T</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      enabled: plcRunner.socketState === 3

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LEDR1", "LEDR2", "LEDR3"]
      xLabel: 'IN / <font color="#509dfd">SINK</font>'
      yLabel: 'OUT / <font color="#509dfd">SINK</font>'
      xPlugged: [0, 0, 0, 0, 0, 0, 0, 0]
      yPlugged: [0, 0, 0, 0, 0, 1, 1, 1]
      moduleIndex: 2
    }

    ColumnLayout {
      id: cl

      spacing: 20

      LedsPanel {
        id: ledPanel

        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
        title: "Door Panel"
      }

      DeltaNetwork {
        id: deltaNetwork

        Layout.alignment: Qt.AlignBottom
        title: "Network"
      }
    }
  }
}
