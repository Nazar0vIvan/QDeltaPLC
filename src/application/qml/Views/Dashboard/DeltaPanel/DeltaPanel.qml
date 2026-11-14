import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import robocrap_qml_module 1.0 as Backend

QxGroupBox {
  id: root

  readonly property Backend.DeviceRunner plc: Backend.Hub.device("plc")

  readonly property var inactiveStates: [false, false, false, false, false, false, false, false]
  readonly property var x1: root.enabled ? (root.plc.data.x1 ?? root.inactiveStates) : root.inactiveStates
  readonly property var y1: root.enabled ? (root.plc.data.y1 ?? root.inactiveStates) : root.inactiveStates
  readonly property var x2: root.enabled ? (root.plc.data.x2 ?? root.inactiveStates) : root.inactiveStates
  readonly property var y2: root.enabled ? (root.plc.data.y2 ?? root.inactiveStates) : root.inactiveStates

  enabled: root.plc?.isConnected ?? false

  ColumnLayout {
    spacing: Metrics.sp12

    Text {
      Layout.fillWidth: true
      visible: !(root.plc?.isConnected ?? false)
      text: qsTr("Connect PLC in Settings.")
      color: Colors.foreground.medium
      font: Fonts.caption
      wrapMode: Text.WordWrap
    }

    RowLayout {
      id: rl

      spacing: Metrics.sp16

      LedsPanel {
        id: ledPanel

        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
        title: "Door Panel"

        ledStates: [
          root.enabled,
          moduleAP_T.yStates[3],
          moduleAP_T.yStates[4],
          moduleAP_T.yStates[7],
          moduleAP_T.yStates[6],
          moduleAP_T.yStates[5]
        ]
      }

      DeltaModuleAP {
        id: moduleAP_P

        plc: root.plc

        title: 'AS16AP11<font color="red">P</font>-A'
        xStates: root.x1
        yStates: root.y1

        // KUKA Automatic External interface: EL2809 outputs -> X1, Y1 -> EL1809 inputs.
        xTags: ["AUT EXT", "N/D", "PERI_RDY", "STOPMESS", "PRO_ACT", "PGNO_REQ", "APPL_RUN", "IN_HOME"]
        yTags: ["EXT_START", "DRV_ON", "CONF_MESS", "PGNO0", "PGNO1", "N/D", "N/D", "PROBE"]
        xLabel: 'IN / <font color="red">SOURCE</font>'
        yLabel: 'OUT / <font color="red">SOURCE</font>'
        xPlugged: [1, 0, 1, 1, 1, 1, 1, 1]
        yPlugged: [1, 1, 1, 1, 1, 0, 0, 1]
        yDisplayOnly: [0, 1, 2, 3, 4, 7] // driven by the PLC program (RobotIo, ProbeMap)
        moduleIndex: 1
      }

      DeltaModuleAP {
        id: moduleAP_T

        plc: root.plc

        title: 'AS16AP11<font color="#509dfd">T</font>-A'
        xStates: root.x2
        yStates: root.y2

        xTags: ["WP-500", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D", "N/D"]
        yTags: ["FRW MOT", "N/D", "N/D", "RUN", "GREEN1", "RED3", "RED2", "RED1"]
        xLabel: 'IN / <font color="#509dfd">SINK</font>'
        yLabel: 'OUT / <font color="#509dfd">SINK</font>'
        xPlugged: [1, 0, 0, 0, 0, 0, 0, 0]
        yPlugged: [1, 0, 0, 1, 1, 1, 1, 1]
        moduleIndex: 2
      }
    }

    RobotStartPanel {
      Layout.fillWidth: true

      plc: root.plc
      x1: root.x1
    }
  }
}
