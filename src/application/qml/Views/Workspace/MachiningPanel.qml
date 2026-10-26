pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import robocrap_qml_module 1.0 as Backend
import Components 1.0
import Styles 1.0

QxPanel {
  id: root
  required property Backend.Machining coordinator
  required property Backend.Playback playback
  required property string inputName
  property Backend.MachiningPath savedPath: null
  signal generateRequested()
  signal cancelRequested()
  property bool draftFlipAxis: false
  property var originalSettings: ({})
  property var drafts: ({})
  readonly property var fields: [
    {key: "chamferSize", label: qsTr("Chamfer size, mm"), min: 0},
    {key: "chamferAngle", label: qsTr("Chamfer angle, deg"), min: 0.000001, max: 89.999999},
    {key: "stagingDistance", label: qsTr("Axial distance from the hole, mm"), min: 0.000001},
    {key: "leadInClearance", label: qsTr("Lead-in axial clearance, mm"), min: 0.000001},
    {key: "leadOutClearance", label: qsTr("Lead-out axial clearance, mm"), min: 0.000001},
    {key: "leadInSpan", label: qsTr("Lead-in span, deg"), min: 0.000001, max: 360},
    {key: "leadOutSpan", label: qsTr("Lead-out span, deg"), min: 0.000001, max: 360},
    {key: "machiningFeed", label: qsTr("Machining speed, mm/s"), min: 0.000001},
    {key: "accelerationDistanceFraction", label: qsTr("Acceleration/deceleration distance, %"), min: 0.0001, max: 100},
    {key: "transferFeed", label: qsTr("$VEL.CP, mm/s"), min: 0.000001},
    {key: "transferAcceleration", label: qsTr("$ACC.CP, mm/s²"), min: 0.000001},
    {key: "auxiliaryScale", label: qsTr("HOME PTP simulation, %"), min: 0.0001, max: 100}
  ]
  readonly property bool validDraft: root.fields.every(field => {
    const text = root.drafts[field.key]
    const value = Number(text)
    return text !== undefined && String(text).trim().length > 0 && isFinite(value)
        && value >= field.min && (field.max === undefined || value <= field.max)
  })
  function isPercentage(key) {
    return key === "auxiliaryScale" || key === "accelerationDistanceFraction"
  }
  function beginEditing() {
    const values = {}
    const original = {flipAxis: root.coordinator.settings.flipAxis}
    for (const field of root.fields) {
      const value = root.coordinator.settings[field.key]
      original[field.key] = value
      values[field.key] = String(root.isPercentage(field.key) ? value * 100 : value)
    }
    root.drafts = values
    root.originalSettings = original
    root.draftFlipAxis = original.flipAxis
  }
  function edit(key, text) {
    const values = Object.assign({}, root.drafts)
    values[key] = text
    root.drafts = values
  }
  function commitDraft() {
    if (!root.validDraft) return false
    let settings = root.coordinator.settings
    for (const field of root.fields) {
      const value = Number(root.drafts[field.key])
      settings[field.key] = root.isPercentage(field.key) ? value / 100 : value
    }
    settings.flipAxis = root.draftFlipAxis
    root.coordinator.settings = settings
    return true
  }
  function generateDraft() {
    if (root.commitDraft()) root.generateRequested()
  }
  function cancelEditing() {
    let settings = root.coordinator.settings
    for (const key of Object.keys(root.originalSettings)) settings[key] = root.originalSettings[key]
    root.coordinator.settings = settings
    root.beginEditing()
    root.cancelRequested()
  }
  function focusEditor() {
    scroll.contentItem.contentY = 0
    fieldsRepeater.itemAt(0).focusEditor()
  }
  Component.onCompleted: root.beginEditing()
  Connections {
    target: root.coordinator
    function onInputChanged() {
      if (!root.visible) root.beginEditing()
    }
  }
  title: qsTr("Internal chamfer")
  implicitHeight: 460
  ScrollView {
    id: scroll
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
      width: scroll.availableWidth
      spacing: Metrics.sp8
      Label {
        Layout.fillWidth: true
        visible: root.playback.duration > 0
        text: qsTr("%1 · %2\n%3 / %4 s")
              .arg(root.playback.playing ? qsTr("Playing") : root.playback.active ? qsTr("Paused") : qsTr("Stopped"))
              .arg(root.playback.phase)
              .arg(root.playback.elapsed.toFixed(2)).arg(root.playback.duration.toFixed(2))
        wrapMode: Text.Wrap
        color: Colors.foreground.high
        font: Fonts.caption
      }
      Label {
        Layout.fillWidth: true
        text: root.inputName.length ? root.inputName : qsTr("Select one hole edge or machining path.")
        wrapMode: Text.Wrap
        color: Colors.foreground.high
        font: Fonts.body
      }
      RowLayout {
        Layout.fillWidth: true
        Label {
          Layout.fillWidth: true
          text: qsTr("Flip outward direction")
          wrapMode: Text.Wrap
          color: Colors.foreground.high
          font: Fonts.body
        }
        QxSwitch {
          isOn: root.draftFlipAxis
          Accessible.name: qsTr("Flip outward direction")
          onClicked: root.draftFlipAxis = !root.draftFlipAxis
        }
      }
      Repeater {
        id: fieldsRepeater
        model: root.fields
        delegate: ColumnLayout {
          id: field
          required property var modelData
          function focusEditor() { editor.forceActiveFocus() }
          Layout.fillWidth: true
          spacing: Metrics.sp4
          Label {
            Layout.fillWidth: true
            text: field.modelData.label
            wrapMode: Text.Wrap
            color: Colors.foreground.medium
            font: Fonts.caption
          }
          QxTextInput {
            id: editor
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root.drafts[field.modelData.key] ?? ""
            Accessible.name: field.modelData.label
            validator: DoubleValidator { locale: "C"; decimals: 8 }
            onTextEdited: root.edit(field.modelData.key, editor.text)
          }
        }
      }
      Label {
        Layout.fillWidth: true
        visible: root.savedPath !== null
        text: root.savedPath ? qsTr("Dry Run: %1 s\nCentral path: %2 s")
              .arg(root.savedPath.duration.toFixed(3)).arg(root.savedPath.centralDuration.toFixed(3)) : ""
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      Label {
        Layout.fillWidth: true
        text: root.coordinator.error || root.coordinator.unavailableReason
              || (root.savedPath ? root.savedPath.incompatibility : "")
              || (!root.validDraft ? qsTr("Enter valid values in all fields.") : "")
        visible: text.length > 0
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      RowLayout {
        QxButton {
          text: qsTr("Generate")
          enabled: root.coordinator.canGenerate && root.validDraft
          onClicked: root.generateDraft()
        }
        QxButton {
          text: qsTr("Cancel")
          onClicked: root.cancelEditing()
        }
      }
    }
  }
}
