import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import robocrap_qml_module 1.0 as Backend

// Starts KUKA programs through the PLC (Automatic External). The enable rules repeat the checks of
// the PLC's FB_RunSetVar; X1.n are the KUKA Automatic External outputs wired to PLC module 1.
QxPanel {
  id: root

  required property Backend.DeviceRunner plc
  required property var x1

  readonly property int step: root.plc?.data.robotStep ?? 0
  readonly property int result: root.plc?.data.robotResult ?? 0
  readonly property int programState: root.plc?.data.robotProgramState ?? 0
  // Set after an operating-mode change while a program was held: restart from CELL, no Resume.
  readonly property bool resumeBlocked: !!root.plc?.data.robotResumeBlocked
  readonly property bool autExt: !!root.x1[0]
  readonly property bool stopMess: !!root.x1[3]
  readonly property bool proAct: !!root.x1[4]
  readonly property bool applRun: !!root.x1[6]
  readonly property bool inHome: !!root.x1[7]
  // A stopped program (P00 keeps APPL_RUN until the next number request); the PLC's $CONF_MESS
  // does not clear the stop message of the smartPAD STOP key, so it is acknowledged on the smartPAD.
  readonly property bool interrupted: root.applRun && !root.proAct

  readonly property var stepNames: ["IDLE", "PREP", "START", "SELECT"]
  readonly property var resultNames: ["-", "STARTED", "RESUMED", "TIMEOUT", "ABORTED", "AUT EXT LOST"]
  readonly property var stateNames: ["-", "running", "finished", "interrupted"]

  title: qsTr("Robot program (AUT EXT)")

  RowLayout {
    spacing: Metrics.sp12

    Text {
      text: qsTr("Program")
      opacity: root.enabled ? 1.0 : Colors.overlays.disabled
      color: Colors.foreground.high
      font: Fonts.body
    }

    QxComboBox {
      id: programBox

      model: ["1", "2", "3", "4"]
    }

    QxButton {
      text: qsTr("Run")
      enabled: root.step === 0 && root.autExt && root.inHome && !root.applRun

      onClicked: root.plc.invoke("runRobotProgram", { "program": programBox.currentIndex + 1 })
    }

    QxButton {
      text: qsTr("Resume")
      enabled: root.step === 0 && root.autExt && root.interrupted && !root.stopMess && !root.resumeBlocked

      onClicked: root.plc.invoke("resumeRobot")
    }

    QxButton {
      text: qsTr("Abort")
      enabled: root.step > 0

      onClicked: root.plc.invoke("abortRobotStart")
    }
  }

  // One elided line of fixed height; its width never drives the panel width.
  Text {
    Layout.fillWidth: true
    Layout.preferredWidth: 0

    text: !root.autExt ? qsTr("AUT EXT off")
          : root.step === 0 && root.interrupted
            ? (root.resumeBlocked ? qsTr("Program interrupted · mode changed · restart from CELL")
               : root.stopMess ? qsTr("Program interrupted · acknowledge message on smartPAD")
               : qsTr("Program interrupted · message acknowledged · Resume ready"))
          : qsTr("Step %1 · last start %2 · program %3")
            .arg(root.stepNames[root.step] ?? root.step)
            .arg(root.resultNames[root.result] ?? root.result)
            .arg(root.stateNames[root.programState] ?? root.programState)
    elide: Text.ElideRight
    opacity: root.enabled ? 1.0 : Colors.overlays.disabled
    color: Colors.foreground.medium
    font: Fonts.body
  }
}
