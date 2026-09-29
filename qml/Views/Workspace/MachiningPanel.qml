pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RoboCrap.Backend 1.0 as Backend
import Components 1.0
import Styles 1.0

QxPanel {
  id: root
  required property Backend.Machining coordinator
  required property string inputName
  required property bool tcpConfigured
  property Backend.MachiningPath savedPath: null
  signal applyRequested()
  property bool advanced: false
  property var drafts: ({})
  readonly property var fields: [
    {key: "chamferSize", label: qsTr("Chamfer size, mm"), min: 0},
    {key: "stagingDistance", label: qsTr("P_s axial distance from end plane, mm"), min: 0.000001},
    {key: "leadInClearance", label: qsTr("Lead-in axial clearance, mm"), min: 0.000001},
    {key: "leadOutClearance", label: qsTr("Lead-out axial clearance, mm"), min: 0.000001},
    {key: "leadInSpan", label: qsTr("Lead-in span, deg"), min: 0.000001, max: 360},
    {key: "leadOutSpan", label: qsTr("Lead-out span, deg"), min: 0.000001, max: 360},
    {key: "leadInFeed", label: qsTr("Lead-in speed, mm/s"), min: 0.000001},
    {key: "machiningFeed", label: qsTr("Machining feed, mm/s"), min: 0.000001},
    {key: "leadOutFeed", label: qsTr("Lead-out speed, mm/s"), min: 0.000001},
    {key: "auxiliaryScale", label: qsTr("HOME PTP simulation, %"), min: 0.0001, max: 100},
    {key: "leadInIntervals", label: qsTr("Lead-in minimum intervals"), min: 1, max: 100000, integer: true, advanced: true},
    {key: "leadOutIntervals", label: qsTr("Lead-out minimum intervals"), min: 1, max: 100000, integer: true, advanced: true},
    {key: "acceleration", label: qsTr("TCP acceleration, mm/s²"), min: 0.000001, advanced: true}
  ]
  readonly property var allFields: {
    const result = root.fields.slice()
    for (let i = 0; i < 6; ++i) {
      result.push({key: "jointSpeed" + i, group: "jointSpeed", index: i, label: qsTr("A%1 speed, deg/s").arg(i + 1), min: 0.000001, advanced: true})
      result.push({key: "jointAcceleration" + i, group: "jointAcceleration", index: i, label: qsTr("A%1 acceleration, deg/s²").arg(i + 1), min: 0.000001, advanced: true})
    }
    return result
  }
  readonly property bool validDraft: root.allFields.every(field => {
    const text = root.drafts[field.key]
    const value = Number(text)
    return text !== undefined && String(text).trim().length > 0 && isFinite(value)
        && value >= field.min && (field.max === undefined || value <= field.max)
        && (!field.integer || Number.isInteger(value))
  })
  function resetDraft() {
    const values = {}
    for (const field of root.allFields) {
      const value = field.group ? root.coordinator.settings[field.group][field.index] : root.coordinator.settings[field.key]
      values[field.key] = String(field.key === "auxiliaryScale" ? value * 100 : value)
    }
    root.drafts = values
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
      settings[field.key] = field.key === "auxiliaryScale" ? value / 100 : value
    }
    const speeds = []
    const accelerations = []
    for (let i = 0; i < 6; ++i) {
      speeds.push(Number(root.drafts["jointSpeed" + i]))
      accelerations.push(Number(root.drafts["jointAcceleration" + i]))
    }
    settings.jointSpeed = speeds
    settings.jointAcceleration = accelerations
    root.coordinator.settings = settings
    return true
  }
  function applyDraft() {
    if (root.commitDraft()) root.applyRequested()
  }
  function previewDraft() {
    if (root.commitDraft()) root.coordinator.previewGeometry()
  }
  function focusEditor() {
    scroll.contentItem.contentY = 0
    fieldsRepeater.itemAt(0).focusEditor()
  }
  Component.onCompleted: root.resetDraft()
  Connections {
    target: root.coordinator
    function onInputChanged() { root.resetDraft() }
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
        text: qsTr("Dry Run resets the preview to HOME and plays the saved path. Stop freezes the preview; restarting returns to HOME.")
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      Label {
        Layout.fillWidth: true
        visible: root.coordinator.duration > 0
        text: qsTr("%1 · %2\n%3 / %4 s")
              .arg(root.coordinator.playing ? qsTr("Playing") : qsTr("Stopped"))
              .arg(root.coordinator.phase)
              .arg(root.coordinator.elapsed.toFixed(2)).arg(root.coordinator.duration.toFixed(2))
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
      Label {
        Layout.fillWidth: true
        text: qsTr("Equal surface setbacks; 45° for a perpendicular opening\nTCP: %1").arg(root.tcpConfigured ? qsTr("configured in Spindle End Effector") : qsTr("apply calibration in Spindle End Effector"))
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      Label {
        Layout.fillWidth: true
        text: qsTr("P_s lies on the hole axis, measured outward from its intersection with the end plane. TCP Y opposes hole Z. HOME movements pass through P_s. Straight transfers use the corresponding lead speed and stop at the lead junctions.")
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
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
          isOn: root.coordinator.settings.flipAxis
          Accessible.name: qsTr("Flip outward direction")
          onClicked: {
            let settings = root.coordinator.settings
            settings.flipAxis = !settings.flipAxis
            root.coordinator.settings = settings
          }
        }
      }
      Label {
        Layout.fillWidth: true
        text: qsTr("HOME PTP: 100% uses the configured simulation joint limits. Lower percentages scale both joint speed and acceleration for the approach and return. Actual KUKA PTP timing is not modeled.")
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      QxButton {
        text: qsTr("Preview frames (no IK)")
        enabled: root.validDraft && !root.coordinator.playing
        onClicked: root.previewDraft()
      }
      RowLayout {
        Layout.fillWidth: true
        QxCheckBox {
          checked: root.coordinator.showFrames
          Accessible.name: qsTr("Show geometry frames")
          onClicked: root.coordinator.showFrames = checked
        }
        Label {
          text: qsTr("Show geometry frames")
          color: Colors.foreground.medium
          font: Fonts.caption
        }
      }
      ColumnLayout {
        Layout.fillWidth: true
        visible: root.coordinator.showFrames
        QxComboBox {
          Layout.fillWidth: true
          model: [qsTr("Lead-in"), qsTr("Cutting"), qsTr("Lead-out"), qsTr("P_s approach"), qsTr("P_s return")]
          currentIndex: root.coordinator.framePhase
          onActivated: index => root.coordinator.framePhase = index
        }
        Label {
          Layout.fillWidth: true
          text: qsTr("Phase progress: %1%").arg((100 * root.coordinator.frameProgress).toFixed(1))
          color: Colors.foreground.medium
          font: Fonts.caption
        }
        Slider {
          Layout.fillWidth: true
          from: 0
          to: 1
          stepSize: 0.01
          value: root.coordinator.frameProgress
          enabled: root.coordinator.framePhase < 3
          Accessible.name: qsTr("Frame phase progress")
          onMoved: root.coordinator.frameProgress = value
        }
        QxComboBox {
          Layout.fillWidth: true
          model: [qsTr("Target TCP {T}"), qsTr("Local reference {i}, {i_L}, {i_O}")]
          currentIndex: root.coordinator.localFrame ? 1 : 0
          enabled: root.coordinator.framePhase < 3
          onActivated: index => root.coordinator.localFrame = (index === 1)
        }
        Label {
          Layout.fillWidth: true
          text: qsTr("X red, Y green, Z blue. {H}: hole frame; P_s: axial staging TCP. The selected frame is a geometric target, independent of the robot. Local lead frames retain the cutting orientation; TCP rotates during the lead. Edit geometry then press Preview frames to refresh.")
          wrapMode: Text.Wrap
          color: Colors.foreground.medium
          font: Fonts.caption
        }
      }
      QxButton {
        text: root.advanced ? qsTr("Hide simulation tuning") : qsTr("Simulation tuning")
        onClicked: root.advanced = !root.advanced
      }
      Label {
        Layout.fillWidth: true
        visible: root.advanced
        text: qsTr("Keep the defaults unless tuning preview timing or sampling. Minimum intervals seed adaptive sampling. TCP acceleration controls feed ramps; joint speed and acceleration limits can slow the motion. These are simulation settings, not calibrated KUKA limits.")
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      Repeater {
        id: fieldsRepeater
        model: root.allFields
        delegate: ColumnLayout {
          id: field
          required property var modelData
          function focusEditor() { editor.forceActiveFocus() }
          Layout.fillWidth: true
          visible: !field.modelData.advanced || root.advanced
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
        text: root.savedPath ? qsTr("Duration: %1 s\nLead/chamfer time multiplier: %2").arg(root.savedPath.duration.toFixed(2)).arg(root.savedPath.centralTimeScale.toFixed(2)) : ""
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
          text: qsTr("Apply")
          enabled: root.coordinator.canGenerate && root.validDraft
          onClicked: root.applyDraft()
        }
        QxButton {
          text: qsTr("Reset fields")
          onClicked: root.resetDraft()
        }
      }
    }
  }
}
