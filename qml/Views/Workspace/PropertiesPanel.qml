import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0
import RoboCrap.Backend 1.0 as Backend

QxPanel {
  id: root

  required property int selectionCount
  required property Backend.SceneObject selectedObject
  readonly property Backend.PlaneGeometry planeGeometry: root.selectedObject
                                                         ? root.selectedObject.geometry as Backend.PlaneGeometry : null
  readonly property Backend.CylinderGeometry cylinderGeometry: root.selectedObject
                                                               ? root.selectedObject.geometry as Backend.CylinderGeometry : null

  readonly property Backend.CircleGeometry circleGeometry: root.selectedObject
                                                           ? root.selectedObject.geometry as Backend.CircleGeometry : null

  signal renameRequested(Backend.SceneObject object, string name)
  signal visibilityRequested(Backend.SceneObject object, bool visible)

  function typeLabel(kind: int): string {
    switch (kind) {
      case Backend.SceneObject.Plane: return qsTr("Plane")
      case Backend.SceneObject.Circle: return qsTr("Circle")
      case Backend.SceneObject.Cylinder: return qsTr("Cylinder")
      case Backend.SceneObject.Cone: return qsTr("Cone")
      case Backend.SceneObject.Edge: return qsTr("Edge")
      case Backend.SceneObject.ScanPath: return qsTr("Scan path")
      case Backend.SceneObject.MachiningPath: return qsTr("Machining path")
      default: return ""
    }
  }

  component PropertyValue: QxHField {
    property alias valueText: valueLabel.text
    property alias valueObjectName: valueLabel.objectName
    property alias wrapMode: valueLabel.wrapMode

    Layout.fillWidth: true
    labelWidth: Metrics.w80

    Label {
      id: valueLabel

      Layout.fillWidth: true
      Layout.minimumWidth: 0
      wrapMode: Text.WrapAnywhere
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
    wrapMode: Text.WordWrap
  }

  ScrollView {
    id: propertiesScroll
    objectName: "propertiesScroll"
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
      width: propertiesScroll.availableWidth
      visible: root.selectedObject !== null
      spacing: Metrics.sp8

      // General object properties.
      QxHField {
        Layout.fillWidth: true
        labelWidth: Metrics.w80
        labelText: qsTr("Name")

        QxTextInput {
          id: nameInput
          objectName: "objectNameEditor"

          property Backend.SceneObject editObject: null
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

        QxCheckBox {
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
        visible: root.planeGeometry !== null || root.cylinderGeometry !== null || root.circleGeometry !== null
        text: qsTr("Geometry")
        color: Colors.foreground.medium
        font: Fonts.body
      }

      // Plane geometry.
      PropertyValue {
        visible: root.planeGeometry !== null
        labelText: qsTr("Normal")
        valueObjectName: "planeNormalValue"
        valueText: root.planeGeometry
              ? [root.planeGeometry.normalX, root.planeGeometry.normalY,
                 root.planeGeometry.normalZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.planeGeometry !== null && root.planeGeometry.hasBounds
        labelText: qsTr("Origin")
        valueObjectName: "planeOriginValue"
        valueText: root.planeGeometry && root.planeGeometry.hasBounds
              ? [root.planeGeometry.originX, root.planeGeometry.originY,
                 root.planeGeometry.originZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.planeGeometry !== null && root.planeGeometry.hasBounds
        labelText: qsTr("Width")
        valueObjectName: "planeWidthValue"
        valueText: root.planeGeometry && root.planeGeometry.hasBounds ? root.planeGeometry.width.toPrecision(6) : ""
      }

      PropertyValue {
        visible: root.planeGeometry !== null && root.planeGeometry.hasBounds
        labelText: qsTr("Height")
        valueObjectName: "planeHeightValue"
        valueText: root.planeGeometry && root.planeGeometry.hasBounds ? root.planeGeometry.height.toPrecision(6) : ""
      }

      PropertyValue {
        visible: root.planeGeometry !== null && root.planeGeometry.hasBounds
        labelText: qsTr("Points")
        valueObjectName: "planePointCountValue"
        valueText: root.planeGeometry && root.planeGeometry.hasBounds ? String(root.planeGeometry.pointCount) : ""
      }

      PropertyValue {
        visible: root.planeGeometry !== null
        labelText: qsTr("Offset (d)")
        valueObjectName: "planeOffsetValue"
        valueText: root.planeGeometry ? root.planeGeometry.offset.toPrecision(6) : ""
      }

      Label {
        Layout.fillWidth: true
        visible: root.planeGeometry !== null
        text: qsTr("nx·x + ny·y + nz·z + d = 0")
        wrapMode: Text.WordWrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }

      // Circle geometry.
      PropertyValue {
        visible: root.circleGeometry !== null
        labelText: qsTr("Center")
        valueObjectName: "circleCenterValue"
        valueText: root.circleGeometry
                   ? [root.circleGeometry.centerX, root.circleGeometry.centerY,
                      root.circleGeometry.centerZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.circleGeometry !== null
        labelText: qsTr("Radius")
        valueObjectName: "circleRadiusValue"
        valueText: root.circleGeometry ? root.circleGeometry.radius.toPrecision(6) : ""
      }

      PropertyValue {
        visible: root.circleGeometry !== null
        labelText: qsTr("Normal")
        valueObjectName: "circleNormalValue"
        valueText: root.circleGeometry
                   ? [root.circleGeometry.normalX, root.circleGeometry.normalY,
                      root.circleGeometry.normalZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.circleGeometry !== null
        labelText: qsTr("Points")
        valueObjectName: "circlePointCountValue"
        valueText: root.circleGeometry ? String(root.circleGeometry.pointCount) : ""
      }

      PropertyValue {
        visible: root.circleGeometry !== null
        labelText: qsTr("Fit RMS")
        valueObjectName: "circleResidualValue"
        valueText: root.circleGeometry ? root.circleGeometry.rmsResidual.toPrecision(6) : ""
      }

      // Cylinder geometry.
      PropertyValue {
        visible: root.cylinderGeometry !== null
        labelText: qsTr("Origin")
        valueObjectName: "cylinderOriginValue"
        valueText: root.cylinderGeometry
              ? [root.cylinderGeometry.originX, root.cylinderGeometry.originY,
                 root.cylinderGeometry.originZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.cylinderGeometry !== null
        labelText: qsTr("Axis")
        valueObjectName: "cylinderAxisValue"
        valueText: root.cylinderGeometry
              ? [root.cylinderGeometry.axisX, root.cylinderGeometry.axisY,
                 root.cylinderGeometry.axisZ].map(value => value.toPrecision(6)).join(", ") : ""
      }

      PropertyValue {
        visible: root.cylinderGeometry !== null
        labelText: qsTr("Radius")
        valueObjectName: "cylinderRadiusValue"
        valueText: root.cylinderGeometry ? root.cylinderGeometry.radius.toPrecision(6) : ""
        wrapMode: Text.NoWrap
      }

      PropertyValue {
        visible: root.cylinderGeometry !== null
        labelText: qsTr("Length")
        valueObjectName: "cylinderLengthValue"
        valueText: root.cylinderGeometry ? root.cylinderGeometry.length.toPrecision(6) : ""
        wrapMode: Text.NoWrap
      }

      PropertyValue {
        visible: root.cylinderGeometry !== null
        labelText: qsTr("Points")
        valueObjectName: "cylinderPointCountValue"
        valueText: root.cylinderGeometry ? String(root.cylinderGeometry.pointCount) : ""
        wrapMode: Text.NoWrap
      }
    }
  }
}
