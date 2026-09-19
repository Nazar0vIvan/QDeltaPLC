import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components 1.0
import Styles 1.0
import RoboCrap.Viewport3D 1.0 as Viewport3D

ColumnLayout {
  id: root

  property list<string> jointDrafts: []
  property list<string> flangeDrafts: []

  function formatted(values) {
    const result = []
    for (let i = 0; i < values.length; ++i)
      result.push(Number(values[i]).toFixed(4))
    return result
  }

  function refreshPose() {
    root.jointDrafts = root.formatted(Viewport3D.OccController.jointAngles)
    root.flangeDrafts = root.formatted(Viewport3D.OccController.flangePose)
  }

  function editJoint(index, value) {
    const draft = root.jointDrafts.slice()
    draft[index] = value
    root.jointDrafts = draft
  }

  function editFlange(index, value) {
    const draft = root.flangeDrafts.slice()
    draft[index] = value
    root.flangeDrafts = draft
  }

  spacing: Metrics.sp12

  Label {
    text: qsTr("Robot preview")
    font: Fonts.title
    color: Colors.foreground.high
  }

  Label {
    Layout.fillWidth: true
    Layout.preferredWidth: Metrics.w200
    wrapMode: Text.WordWrap
    text: Viewport3D.OccController.loading
          ? qsTr("Loading robot CAD…")
          : qsTr("Local FK / IK preview. Angles are in degrees; position is the flange, not a tool tip.")
    color: Colors.foreground.medium
    font: Fonts.caption
  }

  Label {
    Layout.fillWidth: true
    Layout.preferredWidth: Metrics.w200
    visible: text.length > 0
    wrapMode: Text.WordWrap
    textFormat: Text.PlainText
    text: Viewport3D.OccController.errorString
    color: Colors.maxColor
    font: Fonts.body
  }

  Label {
    Layout.fillWidth: true
    Layout.preferredWidth: Metrics.w200
    visible: text.length > 0
    wrapMode: Text.WordWrap
    textFormat: Text.PlainText
    text: Viewport3D.OccController.warningString
    color: Colors.secondary.base
    font: Fonts.caption
  }

  QxButton {
    visible: !Viewport3D.OccController.ready && !Viewport3D.OccController.loading
    text: qsTr("Retry loading")
    onClicked: Viewport3D.OccController.loadRobot()
  }

  SixValueEditor {
    id: jointEditor

    Layout.fillWidth: true
    enabled: Viewport3D.OccController.ready
    title: qsTr("Joint angles")
    labels: ["q1", "q2", "q3", "q4", "q5", "q6"]
    units: ["deg", "deg", "deg", "deg", "deg", "deg"]
    values: root.jointDrafts
    onValueEdited: (index, value) => root.editJoint(index, value)
  }

  QxButton {
    enabled: Viewport3D.OccController.ready && jointEditor.valid
    text: qsTr("Apply FK")
    onClicked: Viewport3D.OccController.solveFK(jointEditor.numbers())
  }

  SixValueEditor {
    id: flangeEditor

    Layout.fillWidth: true
    enabled: Viewport3D.OccController.ready
    title: qsTr("Flange pose")
    labels: ["X", "Y", "Z", "A", "B", "C"]
    units: ["mm", "mm", "mm", "deg", "deg", "deg"]
    values: root.flangeDrafts
    onValueEdited: (index, value) => root.editFlange(index, value)
  }

  QxButton {
    enabled: Viewport3D.OccController.ready && flangeEditor.valid
    text: qsTr("Apply IK")
    onClicked: Viewport3D.OccController.solveIK(flangeEditor.numbers())
  }

  Label {
    Layout.fillWidth: true
    Layout.preferredWidth: Metrics.w200
    wrapMode: Text.WordWrap
    text: qsTr("Right drag: rotate · Middle drag: pan · Wheel: zoom")
    color: Colors.foreground.medium
    font: Fonts.caption
  }

  Connections {
    target: Viewport3D.OccController
    function onPoseChanged() { root.refreshPose() }
  }

  Component.onCompleted: root.refreshPose()
}
