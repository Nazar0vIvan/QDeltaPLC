import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

QxPanel {
  id: root

  enum Mode { Measuring, Machining }
  enum Submode { Rough, Precise }

  required property int mode
  required property int submode

  signal modeRequested(int mode)
  signal submodeRequested(int submode)

  title: qsTr("Workflow")
  spacing: Metrics.sp8

  component ModeButton: Button {
    id: control

    property color selectedColor: Colors.secondary.base
    property color selectedBorderColor: Colors.secondary.dark
    readonly property color foregroundColor: !control.enabled ? Colors.foreground.disabled
                                             : control.checked ? "black" : "white"

    checkable: true
    autoExclusive: true
    padding: Metrics.sp8
    leftPadding: Metrics.sp6
    rightPadding: Metrics.sp6
    font: Fonts.caption
    spacing: Metrics.sp6
    display: AbstractButton.TextBesideIcon
    icon.width: Metrics.sz20
    icon.height: Metrics.sz20
    icon.color: control.foregroundColor
    palette.buttonText: control.foregroundColor
    // Basic.Button uses brightText for checked buttons.
    palette.brightText: control.foregroundColor
    palette.disabled.buttonText: Colors.foreground.disabled
    palette.disabled.brightText: Colors.foreground.disabled

    Layout.fillWidth: true
    Layout.preferredWidth: 1
    Layout.minimumWidth: implicitWidth
    Layout.minimumHeight: Metrics.h40

    background: Rectangle {
      radius: Metrics.r4
      color: !control.enabled ? Colors.background.dp04
             : control.checked ? control.selectedColor
             : control.down ? Colors.background.dp12
             : control.hovered ? Colors.background.dp08
             : Colors.background.dp04
      border.width: control.enabled && (control.checked || control.visualFocus)
                    ? Metrics.w2 : Metrics.w1
      border.color: control.visualFocus ? Colors.primary.light
                    : control.checked && control.enabled ? control.selectedBorderColor
                    : Colors.background.dp12
    }
  }

  Label {
    text: qsTr("Mode")
    color: Colors.foreground.medium
    font: Fonts.caption
  }

  RowLayout {
    Layout.fillWidth: true
    spacing: Metrics.sp8

    ModeButton {
      text: qsTr("Measuring")
      icon.source: "qrc:/pics/measuring.svg"
      checked: root.mode === WorkflowPanel.Measuring
      onClicked: root.modeRequested(WorkflowPanel.Measuring)
    }

    ModeButton {
      text: qsTr("Machining")
      icon.source: "qrc:/pics/machining.svg"
      checked: root.mode === WorkflowPanel.Machining
      onClicked: root.modeRequested(WorkflowPanel.Machining)
    }
  }

  ColumnLayout {
    Layout.fillWidth: true
    spacing: Metrics.sp8

    Label {
      text: qsTr("Submode")
      color: Colors.foreground.medium
      font: Fonts.caption
    }

    RowLayout {
      Layout.fillWidth: true
      enabled: root.mode === WorkflowPanel.Measuring
      spacing: Metrics.sp8

      ModeButton {
        text: qsTr("Rough")
        icon.source: "qrc:/pics/rough.svg"
        checked: root.submode === WorkflowPanel.Rough
        onClicked: root.submodeRequested(WorkflowPanel.Rough)
      }

      ModeButton {
        text: qsTr("Precise")
        icon.source: "qrc:/pics/precise.svg"
        selectedColor: Colors.primary.base
        selectedBorderColor: Colors.primary.dark
        checked: root.submode === WorkflowPanel.Precise
        onClicked: root.submodeRequested(WorkflowPanel.Precise)
      }
    }

    Label {
      Layout.fillWidth: true
      text: qsTr("Not available in machining")
      color: Colors.foreground.medium
      font: Fonts.caption
      wrapMode: Text.WordWrap
      // Keep this row in the layout in both modes to prevent vertical movement.
      opacity: root.mode === WorkflowPanel.Machining ? 1 : 0
    }
  }
}
