import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

import Styles 1.0

Item {
  id: root

  property int fieldWidth: 0
  property string imageSource: ""
  property alias text: textField.text

  signal uploaded(string path)

  implicitWidth: fieldWidth + rl.spacing + height

  FileDialog {
    id: openFile
    title: qsTr("Choose a file")
    nameFilters: ["All files (*)", "XML (*.xml)"]

    onAccepted: {
      uploaded(openFile.currentFile)
    }
  }

  RowLayout {
    id: rl

    anchors.fill: parent

    TextField {
      id: textField

      Layout.preferredWidth: root.fieldWidth
      Layout.preferredHeight: root.height

      leftPadding: 5
      color: Styles.foreground.high
      selectionColor: Styles.primary.highlight
      selectByMouse: true
      readOnly: false
      font: Styles.fonts.body

      background: Rectangle {
        color: textField.readOnly ? "transparent" : Styles.background.dp04
        radius: 4
        border {
          width: textField.readOnly ? 0 : textField.activeFocus ? 2 : textField.hovered ? 0 : 1
          color: textField.activeFocus ? Styles.primary.base : Styles.background.dp12
        }
      }
    }

    Button {
      id: btnBrowse

      Layout.preferredHeight: root.height
      Layout.preferredWidth: root.height
      padding: 3

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        source: root.imageSource
        mipmap: true
        smooth: true
        opacity: btnBrowse.pressed ? 0.8 : 1.0
      }
      background: Rectangle {
        color: "transparent"
        radius: 4
        border {
          width: btnBrowse.hovered ? 1 : 0
          color: Styles.background.dp04
        }
      }
      onClicked: openFile.open()
    }
  }
}
