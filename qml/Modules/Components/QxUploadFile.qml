import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Styles 1.0

Item {
  id: root

  property int fieldWidth: Metrics.fieldWidthLarge
  property string imageSource: ""
  property alias text: textField.text

  signal uploaded(string path)

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  FileDialog {
    id: openFile
    title: qsTr("Choose a file")
    nameFilters: ["All files (*)", "XML (*.xml)"]

    onAccepted: {
      root.uploaded(openFile.currentFile)
    }
  }

  RowLayout {
    id: rl

    spacing: Metrics.sp8

    TextField {
      id: textField

      Layout.preferredWidth: root.fieldWidth
      leftPadding: Metrics.sp12
      rightPadding: Metrics.sp12
      topPadding: Metrics.sp8
      bottomPadding: Metrics.sp8
      color: Colors.foreground.high
      selectionColor: Colors.primary.highlight
      selectByMouse: true
      readOnly: false
      font: Fonts.body

      background: Rectangle {
        color: textField.readOnly ? "transparent" : Colors.background.dp04
        radius: Metrics.radiusSmall
        border {
          width: textField.readOnly
                 ? 0
                 : textField.activeFocus
                   ? Metrics.separatorWidth
                   : textField.hovered
                     ? 0
                     : Metrics.borderWidth
          color: textField.activeFocus ? Colors.primary.base : Colors.background.dp12
        }
      }
    }

    Button {
      id: btnBrowse

      Layout.preferredWidth: implicitHeight
      padding: Metrics.sp4

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        source: root.imageSource
        mipmap: true
        smooth: true
        opacity: btnBrowse.pressed ? 0.8 : 1.0
      }
      background: Rectangle {
        color: "transparent"
        radius: Metrics.radiusSmall
        border {
          width: btnBrowse.hovered ? Metrics.borderWidth : 0
          color: Colors.background.dp04
        }
      }
      onClicked: openFile.open()
    }
  }
}
