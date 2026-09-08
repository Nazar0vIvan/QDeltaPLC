import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import QDelta.Backend 1.0 as Backend

QxGroupBox {
  id: root

  readonly property Backend.DeviceRunner plc: Backend.Hub.device("plc")

  readonly property var x1: root.plc?.data.x1 ?? [false, false, false, false, false, false, false, false]
  readonly property var y1: root.plc?.data.y1 ?? [false, false, false, false, false, false, false, false]
  readonly property var x2: root.plc?.data.x2 ?? [false, false, false, false, false, false, false, false]
  readonly property var y2: root.plc?.data.y2 ?? [false, false, false, false, false, false, false, false]

  implicitWidth: leftPadding + rl.implicitWidth + rightPadding
  implicitHeight: topPadding + rl.implicitHeight + bottomPadding

  RowLayout {
    id: rl

    spacing: 20

    LedsPanel {
      id: ledPanel

      Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
      title: "Door Panel"

      ledStates: [
        root.plc.isConnected,
        moduleAP_P.yStates[6],
        moduleAP_P.yStates[7],
        moduleAP_T.yStates[5],
        moduleAP_T.yStates[6],
        moduleAP_T.yStates[7]
      ]
    }

    DeltaModuleAP {
      id: moduleAP_P

      plc: root.plc

      title: 'AS16AP11<font color="red">P</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      enabled: root.plc.isConnected
      xStates: root.x1
      yStates: root.y1

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

      plc: root.plc

      title: 'AS16AP11<font color="#509dfd">T</font>-A'
      Layout.preferredWidth: implicitWidth
      Layout.preferredHeight: implicitHeight

      enabled: root.plc.isConnected
      xStates: root.x2
      yStates: root.y2

      xTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
      yTags: ["N/D", "N/D", "N/D", "N/D", "N/D", "LEDR1", "LEDR2", "LEDR3"]
      xLabel: 'IN / <font color="#509dfd">SINK</font>'
      yLabel: 'OUT / <font color="#509dfd">SINK</font>'
      xPlugged: [0, 0, 0, 0, 0, 0, 0, 0]
      yPlugged: [0, 0, 0, 0, 0, 1, 1, 1]
      moduleIndex: 2
    }

    KukaAutExt {
      id: kukaAutExt

      plc: root.plc

      Layout.alignment: Qt.AlignTop
      title: "KUKA AUT_EXT"
    }
  }
}
