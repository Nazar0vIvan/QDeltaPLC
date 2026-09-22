import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0

QxPanel {
  id: root

  required property int selectionCount
  required property string objectId
  required property string selectedName
  required property string objectType
  required property string classification
  required property bool objectVisible

  signal renameRequested(string objectId, string name)
  signal visibilityRequested(string objectId, bool objectVisible)

  title: qsTr("Properties")

  // Commit any pending edit to its original object before displaying another.
  onObjectIdChanged: {
    if (nameInput)
      nameInput.focus = false
  }
  onSelectionCountChanged: {
    if (nameInput && root.selectionCount !== 1)
      nameInput.focus = false
  }

  Label {
    Layout.fillWidth: true
    visible: root.selectionCount !== 1
    text: root.selectionCount === 0 ? qsTr("Select an object.")
                                   : qsTr("%1 objects selected.").arg(root.selectionCount)
    color: Colors.foreground.medium
    font: Fonts.body
    wrapMode: Text.WordWrap
  }

  ColumnLayout {
    Layout.fillWidth: true
    visible: root.selectionCount === 1
    spacing: Metrics.sp8

    QxHField {
      Layout.fillWidth: true
      labelWidth: Metrics.w80
      labelText: qsTr("Name")

      QxTextInput {
        id: nameInput

        property string editObjectId: ""
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
            root.renameRequested(nameInput.editObjectId, name)
        }

        Binding {
          target: nameInput
          property: "text"
          value: root.selectedName
          when: !nameInput.activeFocus
          restoreMode: Binding.RestoreNone
        }

        onTextEdited: {
          nameInput.editObjectId = root.objectId
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
        text: root.objectType
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
        text: root.classification
        color: Colors.foreground.high
        font: Fonts.body
      }
    }

    QxHField {
      Layout.fillWidth: true
      labelWidth: Metrics.w80
      labelText: qsTr("Visible")

      CheckBox {
        checked: root.objectVisible
        padding: Metrics.sp0
        Accessible.name: qsTr("Object visible")
        onClicked: root.visibilityRequested(root.objectId, checked)
      }

      Item { Layout.fillWidth: true }
    }
  }

  Item { Layout.fillHeight: true }
}
