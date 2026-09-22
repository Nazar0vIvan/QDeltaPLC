pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQml.Models

import Components 1.0
import Styles 1.0
import "../../Models"

QxPanel {
  id: root

  required property SceneModel sceneModel
  required property list<string> selectedObjectIds

  signal selectionRequested(string objectId, bool additive)
  signal visibilityRequested(SceneObject object, bool visible)
  signal renameRequested(SceneObject object, string name)

  // Groups and expansion are browser state, not properties of scene objects.
  ListModel {
    id: groups
    ListElement { name: qsTr("Rough Surfaces"); kind: "rough"; badge: "R"; expanded: true }
    ListElement { name: qsTr("Precise Surfaces"); kind: "precise"; badge: "P"; expanded: true }
    ListElement { name: qsTr("Edges"); kind: "edges"; badge: ""; expanded: true }
    ListElement { name: qsTr("Scan Paths"); kind: "scanPaths"; badge: ""; expanded: true }
    ListElement { name: qsTr("Machining Paths"); kind: "machiningPaths"; badge: ""; expanded: true }
  }

  function objectsForGroup(kind: string): var {
    return root.sceneModel.objects.filter(object => {
      switch (kind) {
      case "rough": return object.classification === SceneObject.Rough
      case "precise": return object.classification === SceneObject.Precise
      case "edges": return object.kind === SceneObject.Edge
      case "scanPaths": return object.kind === SceneObject.ScanPath
      case "machiningPaths": return object.kind === SceneObject.MachiningPath
      default: return false
      }
    })
  }

  function objectIcon(kind: int): url {
    switch (kind) {
    case SceneObject.Plane: return "qrc:/pics/plane.svg"
    case SceneObject.Cylinder: return "qrc:/pics/cylinder.svg"
    case SceneObject.Cone: return "qrc:/pics/cone.svg"
    case SceneObject.Edge: return "qrc:/pics/edge.svg"
    case SceneObject.ScanPath:
    case SceneObject.MachiningPath: return "qrc:/pics/path.svg"
    default: return ""
    }
  }

  title: qsTr("Scene")
  leftPadding: Metrics.w1
  rightPadding: Metrics.w1
  bottomPadding: Metrics.sp4

  ScrollView {
    id: scroll

    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    // Reserve the gutter even when the scrollbar fades out, keeping rows stable.
    rightPadding: scroll.ScrollBar.vertical.implicitWidth + Metrics.sp4
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    Column {
      width: scroll.availableWidth

      Repeater {
        model: groups

        delegate: Column {
          id: group

          required property int index
          required property string name
          required property string kind
          required property bool expanded
          required property string badge
          readonly property list<SceneObject> objects: root.objectsForGroup(group.kind)

          readonly property color objectColor: group.kind === "rough" ? Colors.secondary.base
                                              : group.kind === "precise" ? Colors.primary.base
                                              : group.kind === "edges" ? "#00C5C5"
                                              : Colors.minColor

          width: scroll.availableWidth

          ItemDelegate {
            id: groupButton

            width: group.width
            height: Metrics.h32
            padding: Metrics.sp4
            Accessible.name: group.name

            contentItem: RowLayout {
              spacing: Metrics.sp6

              Image {
                Layout.preferredWidth: Metrics.sz12
                Layout.preferredHeight: Metrics.sz12
                source: "qrc:/pics/arrow_dropdown.svg"
                fillMode: Image.PreserveAspectFit
                rotation: group.expanded ? 0 : -90
              }

              Image {
                Layout.preferredWidth: Metrics.sz16
                Layout.preferredHeight: Metrics.sz16
                source: "qrc:/pics/folder_full.svg"
                fillMode: Image.PreserveAspectFit
              }

              Label {
                Layout.fillWidth: true
                text: group.name
                color: Colors.foreground.high
                font: Fonts.caption
                elide: Text.ElideRight
              }
            }

            background: Rectangle {
              color: groupButton.hovered ? Colors.background.dp04 : "transparent"
              border.width: groupButton.visualFocus ? Metrics.w1 : 0
              border.color: Colors.primary.base
            }

            onClicked: groups.setProperty(group.index, "expanded", !group.expanded)
          }

          Column {
            width: group.width
            visible: group.expanded

            Repeater {
              model: group.objects

              delegate: Rectangle {
                id: objectRow

                required property SceneObject modelData
                objectName: "sceneObject-" + objectRow.modelData.objectId

                property bool editing: false
                readonly property bool selected: root.selectedObjectIds.indexOf(objectRow.modelData.objectId) >= 0

                width: group.width
                height: Metrics.h32
                color: objectRow.selected ? Colors.background.dp06
                       : rowHover.hovered ? Colors.background.dp04 : "transparent"

                HoverHandler { id: rowHover }

                function beginRename() {
                  nameEditor.text = objectRow.modelData.name
                  objectRow.editing = true
                  nameEditor.forceActiveFocus()
                  nameEditor.selectAll()
                }

                function finishRename() {
                  if (!objectRow.editing)
                    return
                  objectRow.editing = false
                  const name = nameEditor.text.trim()
                  if (name.length > 0 && name !== objectRow.modelData.name)
                    root.renameRequested(objectRow.modelData, name)
                }

                RowLayout {
                  anchors.fill: parent
                  spacing: Metrics.sp0

                  ItemDelegate {
                    id: objectButton

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 0
                    leftPadding: Metrics.sp32
                    rightPadding: Metrics.sp4
                    topPadding: Metrics.sp0
                    bottomPadding: Metrics.sp0
                    Accessible.name: objectRow.modelData.name
                    Accessible.selected: objectRow.selected
                    text: objectRow.editing ? "" : objectRow.modelData.name
                    font: Fonts.caption
                    spacing: Metrics.sp8
                    display: AbstractButton.TextBesideIcon
                    icon.source: root.objectIcon(objectRow.modelData.kind)
                    icon.width: Metrics.sz16
                    icon.height: Metrics.sz16
                    icon.color: group.objectColor
                    palette.text: objectRow.modelData.visible ? Colors.foreground.high : Colors.foreground.disabled

                    QxTextInput {
                      id: nameEditor
                      objectName: "sceneNameEditor"

                      anchors.left: parent.left
                      anchors.leftMargin: objectButton.leftPadding + Metrics.sz16 + objectButton.spacing
                      anchors.right: parent.right
                      anchors.rightMargin: objectButton.rightPadding
                      anchors.verticalCenter: parent.verticalCenter
                      visible: objectRow.editing
                      leftPadding: Metrics.sp4
                      rightPadding: Metrics.sp4
                      topPadding: Metrics.sp4
                      bottomPadding: Metrics.sp4
                      font: Fonts.caption
                      onAccepted: {
                        objectRow.finishRename()
                        objectButton.forceActiveFocus()
                      }
                      onEditingFinished: objectRow.finishRename()
                      Keys.onEscapePressed: {
                        objectRow.editing = false
                        objectButton.forceActiveFocus()
                      }
                    }

                    background: Rectangle {
                      color: "transparent"
                      border.width: objectButton.visualFocus ? Metrics.w1 : 0
                      border.color: Colors.primary.base
                    }

                    onClicked: root.selectionRequested(objectRow.modelData.objectId, false)
                    Keys.onPressed: event => {
                      if (event.key === Qt.Key_F2) {
                        objectRow.beginRename()
                        event.accepted = true
                      } else if (event.key === Qt.Key_Space && (event.modifiers & Qt.ControlModifier)) {
                        root.selectionRequested(objectRow.modelData.objectId, true)
                        event.accepted = true
                      }
                    }

                    MouseArea {
                      anchors.fill: parent
                      visible: !objectRow.editing
                      onClicked: mouse => {
                        objectButton.forceActiveFocus()
                        root.selectionRequested(objectRow.modelData.objectId, (mouse.modifiers & Qt.ControlModifier) !== 0)
                      }
                      onDoubleClicked: objectRow.beginRename()
                    }
                  }

                  Label {
                    Layout.rightMargin: 4

                    text: group.badge
                    color: group.objectColor
                    font: Fonts.caption
                  }

                  ToolButton {
                    id: visibilityButton
                    objectName: "sceneVisibilityButton"

                    Layout.preferredWidth: Metrics.sz24
                    Layout.fillHeight: true
                    padding: Metrics.sp4
                    icon.source: "qrc:/pics/eye.svg"
                    icon.width: Metrics.sz16
                    icon.height: Metrics.sz16
                    icon.color: visibilityButton.down || !objectRow.modelData.visible
                                ? Colors.foreground.disabled : Colors.foreground.high
                    opacity: visibilityButton.hovered && !visibilityButton.down ? 0.75 : 1.0
                    Accessible.name: (objectRow.modelData.visible ? qsTr("Hide %1") : qsTr("Show %1")).arg(objectRow.modelData.name)

                    background: Rectangle {
                      color: "transparent"
                      border.width: visibilityButton.visualFocus ? Metrics.w1 : 0
                      border.color: Colors.primary.base
                    }

                    onClicked: root.visibilityRequested(objectRow.modelData, !objectRow.modelData.visible)
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
