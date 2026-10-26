import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0
import robocrap_qml_module 1.0 as Backend

QxPanel {
  id: root

  required property int selectionCount
  required property Backend.SceneObject selectedObject
  required property bool showPoints
  required property bool showNormals
  readonly property int labelColumnWidth: Math.ceil(Math.max(Metrics.w120,
      ...[qsTr("Name"), qsTr("Type"), qsTr("State"), qsTr("Source"), qsTr("Visible"),
          qsTr("Show Points"), qsTr("Show Normals"), qsTr("Normal"), qsTr("Origin"),
          qsTr("Width"), qsTr("Height"), qsTr("Points"), qsTr("Offset (d)"),
          qsTr("Radius"), qsTr("Axis"),
          qsTr("Diameter"), qsTr("Length"), qsTr("Nominal radius"), qsTr("Nominal diameter"),
          qsTr("Radius/Diameter"), qsTr("Fitted center"),
          qsTr("Fit RMS")].map(label => labelMetrics.advanceWidth(label))))

  FontMetrics {
    id: labelMetrics
    font: Fonts.body
  }
  readonly property bool diagnosticControlsVisible: root.selectedObject !== null
      && (root.selectedObject.kind === Backend.SceneObject.Plane
          || root.selectedObject.kind === Backend.SceneObject.Cylinder
          || root.selectedObject.kind === Backend.SceneObject.Cone
          || root.selectedObject.kind === Backend.SceneObject.Edge
          || root.selectedObject.kind === Backend.SceneObject.Sphere)
  readonly property bool sphereSelected: root.selectedObject !== null
      && root.selectedObject.kind === Backend.SceneObject.Sphere
  // Rows built by the selected object: {label, value}, {separator} or {label, toggle}, where
  // toggle names the object's showBallCenterSphere or showCompensatedSphere property.
  readonly property var geometryRows: root.selectedObject ? root.selectedObject.properties : []

  signal renameRequested(Backend.SceneObject object, string name)
  signal visibilityRequested(Backend.SceneObject object, bool visible)
  signal sphereDisplayRequested(Backend.SceneObject object, bool ballCenter, bool compensated)
  signal pointsToggled(bool checked)
  signal normalsToggled(bool checked)

  function typeLabel(kind: int): string {
    switch (kind) {
      case Backend.SceneObject.Plane: return qsTr("Plane")
      case Backend.SceneObject.Cylinder: return qsTr("Cylinder")
      case Backend.SceneObject.Cone: return qsTr("Cone")
      case Backend.SceneObject.Edge: return qsTr("Edge")
      case Backend.SceneObject.ScanPath: return qsTr("Scan path")
      case Backend.SceneObject.MachiningPath: return qsTr("Machining path")
      case Backend.SceneObject.Sphere: return qsTr("Calibration sphere")
      case Backend.SceneObject.CalibrationPath: return qsTr("Calibration path")
      default: return ""
    }
  }

  component PropertyValue: QxHField {
    property alias valueText: valueLabel.text

    Layout.fillWidth: true
    labelWidth: root.labelColumnWidth
    fixedLabelWidth: true

    TextEdit {
      id: valueLabel

      Layout.fillWidth: true
      Layout.minimumWidth: 0
      clip: true
      readOnly: true
      selectByMouse: true
      textFormat: TextEdit.PlainText
      wrapMode: TextEdit.NoWrap
      selectionColor: Colors.primary.highlight
      selectedTextColor: Colors.foreground.high
      color: Colors.foreground.high
      font: Fonts.body
    }
  }

  title: qsTr("Properties")

  // Commit any pending edit to its original object before displaying another.
  onSelectedObjectChanged: {
    if (nameInput)
      nameInput.focus = false
  }
  onSelectionCountChanged: {
    if (nameInput && root.selectionCount !== 1)
      nameInput.focus = false
  }

  Label {
    Layout.fillWidth: true
    visible: !root.selectedObject
    text: root.selectionCount <= 1 ? qsTr("Select an object.")
                                   : qsTr("%1 objects selected.").arg(root.selectionCount)
    color: Colors.foreground.medium
    font: Fonts.body
    wrapMode: Text.NoWrap
  }

  ScrollView {
    id: propertiesScroll
    objectName: "propertiesScroll"
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    contentWidth: availableWidth
    contentHeight: root.selectedObject ? propertiesContent.implicitHeight : 0
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: root.selectedObject ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

    ColumnLayout {
      id: propertiesContent
      width: propertiesScroll.contentWidth
      visible: root.selectedObject !== null
      spacing: Metrics.sp8

      // General object properties.
      QxHField {
        Layout.fillWidth: true
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Name")

        QxTextInput {
          id: nameInput
          objectName: "objectNameEditor"

          property Backend.SceneObject editObject: null
          property string draftName: ""
          property bool modified: false

          Layout.fillWidth: true
          Layout.minimumWidth: 0
          Layout.preferredWidth: Metrics.w120
          clip: true
          leftPadding: Metrics.sp6
          rightPadding: Metrics.sp6
          topPadding: Metrics.sp4
          bottomPadding: Metrics.sp4
          Accessible.name: qsTr("Object name")

          function commit() {
            if (!nameInput.modified)
              return
            nameInput.modified = false
            const name = nameInput.draftName.trim()
            if (name.length > 0)
              root.renameRequested(nameInput.editObject, name)
          }

          Binding {
            target: nameInput
            property: "text"
            value: root.selectedObject ? root.selectedObject.name : ""
            when: !nameInput.activeFocus
            restoreMode: Binding.RestoreNone
          }

          onTextEdited: {
            nameInput.editObject = root.selectedObject
            nameInput.draftName = nameInput.text
            nameInput.modified = true
          }
          onAccepted: {
            nameInput.commit()
            nameInput.focus = false
          }
          onEditingFinished: nameInput.commit()
          Keys.onEscapePressed: {
            nameInput.modified = false
            nameInput.focus = false
          }
        }
      }

      QxHField {
        Layout.fillWidth: true
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Type")

        Label {
          Layout.fillWidth: true
          Layout.minimumWidth: 0
          elide: Text.ElideRight
          text: root.selectedObject ? root.typeLabel(root.selectedObject.kind) : ""
          color: Colors.foreground.high
          font: Fonts.body
          wrapMode: Text.NoWrap
        }
      }

      QxHField {
        Layout.fillWidth: true
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("State")

        Label {
          Layout.fillWidth: true
          Layout.minimumWidth: 0
          elide: Text.ElideRight
          text: !root.selectedObject ? ""
                : root.selectedObject.classification === Backend.SceneObject.Rough ? qsTr("Rough")
                : root.selectedObject.classification === Backend.SceneObject.Precise ? qsTr("Precise")
                : qsTr("Not applicable")
          color: Colors.foreground.high
          font: Fonts.body
        }
      }

      QxHField {
        Layout.fillWidth: true
        visible: root.selectedObject !== null && root.selectedObject.sourceUrl.toString().length > 0
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Source")

        QxTextInput {
          id: sourceInput
          objectName: "objectSourceField"
          Layout.fillWidth: true
          Layout.minimumWidth: 0
          Layout.preferredWidth: Metrics.w120
          clip: true
          readOnly: true
          leftPadding: Metrics.sp6
          rightPadding: Metrics.sp6
          topPadding: Metrics.sp4
          bottomPadding: Metrics.sp4
          text: root.selectedObject ? root.selectedObject.sourceUrl.toString() : ""
          Accessible.name: qsTr("Source JSON")

          background: Rectangle {
            color: "transparent"
            radius: sourceInput.radius
            border.width: Metrics.w1
            border.color: sourceInput.activeFocus ? Colors.primary.base : Colors.background.dp12
          }
        }
      }

      QxHField {
        Layout.fillWidth: true
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Visible")

        QxCheckBox {
          objectName: "objectVisibilityCheckBox"
          checked: root.selectedObject ? root.selectedObject.visible : false
          padding: Metrics.sp0
          Accessible.name: qsTr("Object visible")

          onClicked: root.visibilityRequested(root.selectedObject, checked)
        }

        Item { Layout.fillWidth: true }
      }

      QxHField {
        Layout.fillWidth: true
        visible: root.diagnosticControlsVisible
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Show Points")

        QxCheckBox {
          objectName: "showPointsCheckBox"
          checked: root.showPoints
          Accessible.name: qsTr("Show Points")
          onClicked: root.pointsToggled(checked)
        }

        Item { Layout.fillWidth: true }
      }

      QxHField {
        Layout.fillWidth: true
        // Sphere readings have points but no normal lines.
        visible: root.diagnosticControlsVisible && !root.sphereSelected
        labelWidth: root.labelColumnWidth
        fixedLabelWidth: true
        labelText: qsTr("Show Normals")

        QxCheckBox {
          objectName: "showNormalsCheckBox"
          checked: root.showNormals
          Accessible.name: qsTr("Show Normals")
          onClicked: root.normalsToggled(checked)
        }

        Item { Layout.fillWidth: true }
      }

      Label {
        Layout.fillWidth: true
        Layout.topMargin: Metrics.sp8
        visible: root.geometryRows.length > 0
        text: qsTr("Geometry")
        color: Colors.foreground.medium
        font: Fonts.body
      }

      Repeater {
        model: root.geometryRows
        delegate: ColumnLayout {
          id: geometryRow
          required property var modelData
          readonly property bool separator: geometryRow.modelData.separator === true
          readonly property string toggle: geometryRow.modelData.toggle ?? ""

          Layout.fillWidth: true
          spacing: Metrics.sp0

          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.w1
            visible: geometryRow.separator
            color: Colors.background.dp12
          }

          // Group headings may be wider than the label column; their check box follows the text.
          QxHField {
            Layout.fillWidth: true
            visible: geometryRow.toggle.length > 0
            labelWidth: root.labelColumnWidth
            labelText: geometryRow.modelData.label ?? ""

            QxCheckBox {
              checked: root.selectedObject !== null
                       && (geometryRow.toggle === "showBallCenterSphere" ? root.selectedObject.showBallCenterSphere
                                                                         : root.selectedObject.showCompensatedSphere)
              padding: Metrics.sp0
              Accessible.name: qsTr("%1 visible").arg(geometryRow.modelData.label ?? "")
              onClicked: root.sphereDisplayRequested(root.selectedObject,
                  geometryRow.toggle === "showBallCenterSphere" ? checked : root.selectedObject.showBallCenterSphere,
                  geometryRow.toggle === "showCompensatedSphere" ? checked : root.selectedObject.showCompensatedSphere)
            }

            Item { Layout.fillWidth: true }
          }

          PropertyValue {
            visible: !geometryRow.separator && geometryRow.toggle.length === 0
            labelText: geometryRow.modelData.label ?? ""
            valueText: geometryRow.modelData.value ?? ""
          }
        }
      }
    }
  }
}
