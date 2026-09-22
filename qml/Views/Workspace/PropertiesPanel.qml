import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0
import "../../Models"

QxPanel {
  id: root

  required property int selectionCount
  required property SceneObject selectedObject
  readonly property PlaneGeometry planeGeometry: root.selectedObject
                                                  ? root.selectedObject.geometry as PlaneGeometry : null

  signal renameRequested(SceneObject object, string name)
  signal visibilityRequested(SceneObject object, bool visible)

  function typeLabel(kind: int): string {
    switch (kind) {
    case SceneObject.Plane: return qsTr("Plane")
    case SceneObject.Cylinder: return qsTr("Cylinder")
    case SceneObject.Cone: return qsTr("Cone")
    case SceneObject.Edge: return qsTr("Edge")
    case SceneObject.ScanPath: return qsTr("Scan path")
    case SceneObject.MachiningPath: return qsTr("Machining path")
    default: return ""
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
    wrapMode: Text.WordWrap
  }

  ColumnLayout {
    Layout.fillWidth: true
    visible: root.selectedObject !== null
    spacing: Metrics.sp8

    QxHField {
      Layout.fillWidth: true
      labelWidth: Metrics.w80
      labelText: qsTr("Name")

      QxTextInput {
        id: nameInput
        objectName: "objectNameEditor"

        property SceneObject editObject: null
        property string draftName: ""
        property bool modified: false

        Layout.fillWidth: true
        Layout.minimumWidth: 0
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
      labelWidth: Metrics.w80
      labelText: qsTr("Type")

      Label {
        Layout.fillWidth: true
        text: root.selectedObject ? root.typeLabel(root.selectedObject.kind) : ""
        color: Colors.foreground.high
        font: Fonts.body
        wrapMode: Text.WordWrap
      }
    }

    QxHField {
      Layout.fillWidth: true
      labelWidth: Metrics.w80
      labelText: qsTr("State")

      Label {
        Layout.fillWidth: true
        text: !root.selectedObject ? ""
              : root.selectedObject.classification === SceneObject.Rough ? qsTr("Rough")
              : root.selectedObject.classification === SceneObject.Precise ? qsTr("Precise")
              : qsTr("Not applicable")
        color: Colors.foreground.high
        font: Fonts.body
      }
    }

    QxHField {
      Layout.fillWidth: true
      visible: root.selectedObject !== null && root.selectedObject.sourceUrl.toString().length > 0
      labelWidth: Metrics.w80
      labelText: qsTr("Source")

      QxTextInput {
        objectName: "objectSourceField"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        readOnly: true
        padding: Metrics.sp4
        text: root.selectedObject ? root.selectedObject.sourceUrl.toString() : ""
        Accessible.name: qsTr("Source JSON")
      }
    }

    QxHField {
      Layout.fillWidth: true
      labelWidth: Metrics.w80
      labelText: qsTr("Visible")

      CheckBox {
        objectName: "objectVisibilityCheckBox"
        checked: root.selectedObject ? root.selectedObject.visible : false
        padding: Metrics.sp0
        Accessible.name: qsTr("Object visible")
        onClicked: root.visibilityRequested(root.selectedObject, checked)
      }

      Item { Layout.fillWidth: true }
    }

    Label {
      Layout.fillWidth: true
      Layout.topMargin: Metrics.sp8
      visible: root.planeGeometry !== null
      text: qsTr("Geometry")
      color: Colors.foreground.medium
      font: Fonts.body
    }

    QxHField {
      Layout.fillWidth: true
      visible: root.planeGeometry !== null
      labelWidth: Metrics.w80
      labelText: qsTr("Normal")

      Label {
        objectName: "planeNormalValue"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: root.planeGeometry
              ? [root.planeGeometry.normalX, root.planeGeometry.normalY,
                 root.planeGeometry.normalZ].map(value => value.toPrecision(6)).join(", ") : ""
        wrapMode: Text.WrapAnywhere
        color: Colors.foreground.high
        font: Fonts.body
      }
    }

    QxHField {
      Layout.fillWidth: true
      visible: root.planeGeometry !== null
      labelWidth: Metrics.w80
      labelText: qsTr("Offset (d)")

      Label {
        objectName: "planeOffsetValue"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: root.planeGeometry ? root.planeGeometry.offset.toPrecision(6) : ""
        wrapMode: Text.WrapAnywhere
        color: Colors.foreground.high
        font: Fonts.body
      }
    }

    Label {
      Layout.fillWidth: true
      visible: root.planeGeometry !== null
      text: qsTr("nx·x + ny·y + nz·z + d = 0")
      wrapMode: Text.WordWrap
      color: Colors.foreground.medium
      font: Fonts.caption
    }
  }

  Item { Layout.fillHeight: true }
}
