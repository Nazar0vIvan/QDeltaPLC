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
      if ((data.cmd && data.cmd === PlcMessage.SNAPSHOT) ||
          (data.chg && data.chg === PlcMessage.IOs)) {
        console.log("X1: ", data.x1);
        moduleAP_P.refreshAll(data.x1, data.y1);
        moduleAP_T.refreshAll(data.x2, data.y2);
      }
      if (data.cmd && data.cmd === PlcMessage.WRITE_IO) {
        if (data.module === 1)
          moduleAP_P.refreshY(data.state)
        else
          moduleAP_T.refreshY(data.state)
      }
    }

    function onSocketStateChanged() {
      if (plcRunner.socketState === 0) {
        moduleAP_P.refreshAll(Array(8).fill(false), Array(8).fill(false));
        moduleAP_T.refreshAll(Array(8).fill(false), Array(8).fill(false));
      }
    }
  }

  RowLayout {
    id: rl

    spacing: 20

    LedsPanel {
      id: ledPanel

      Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
      title: "Door Panel"

      states: [
        plcRunner.socketState === 3,
        moduleAP_P.yStates[6],
        moduleAP_P.yStates[7],
        moduleAP_T.yStates[5],
        moduleAP_T.yStates[6],
        moduleAP_T.yStates[7]
      ]
    }

    DeltaModuleAP {
      id: moduleAP_P

      title: 'AS16AP11<font color="red">P</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      enabled: plcRunner.socketState === 3

      xTags: ["RC_RDY1", "PERI_RDY", "STOPMESS", "PRO_ACT", "APPL_RUN", "EXT", "N/D", "N/D"]
      yTags: ["PGNO_0", "PGNO_1", "EXT_START", "CONF_MESS", "DRIVE_OFF", "DRIVES_ON", "RUN", "LEDG2"]
      xLabel: 'IN / <font color="red">SOURCE</font>'
      yLabel: 'OUT / <font color="red">SOURCE</font>'
      xPlugged: [1, 1, 1, 1, 1, 1, 0, 0]
      yPlugged: [1, 1, 1, 1, 1, 1, 1, 1]
      yDisplayOnly: [0, 1, 2, 3, 4, 5, 6] // locked by KRC
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

    DeltaNetwork {
      id: deltaNetwork

      Layout.alignment: Qt.AlignTop
      title: "Network"
    }

    KukaAutExt {
      id: kukaAutExt

      Layout.alignment: Qt.AlignTop
      title: "KUKA AUT_EXT"

    }
  }
}
