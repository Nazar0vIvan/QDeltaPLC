import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Styles 1.0

Item {
  id: root

  property int fieldWidth: UiMetrics.fieldWidthLarge
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

    spacing: UiMetrics.spacingSmall

    TextField {
      id: textField

      Layout.preferredWidth: root.fieldWidth
      leftPadding: UiMetrics.controlHorizontalPadding
      rightPadding: UiMetrics.controlHorizontalPadding
      topPadding: UiMetrics.controlVerticalPadding
      bottomPadding: UiMetrics.controlVerticalPadding
      color: Styles.foreground.high
      selectionColor: Styles.primary.highlight
      selectByMouse: true
      readOnly: false
      font: Styles.fonts.body

      background: Rectangle {
        color: textField.readOnly ? "transparent" : Styles.background.dp04
        radius: UiMetrics.radiusSmall
        border {
          width: textField.readOnly
                 ? 0
                 : textField.activeFocus
                   ? UiMetrics.separatorWidth
                   : textField.hovered
                     ? 0
                     : UiMetrics.borderWidth
          color: textField.activeFocus ? Styles.primary.base : Styles.background.dp12
        }
      }
    }

    Button {
      id: btnBrowse

      Layout.preferredWidth: implicitHeight
      padding: UiMetrics.spacingXSmall

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        source: root.imageSource
        mipmap: true
        smooth: true
        opacity: btnBrowse.pressed ? 0.8 : 1.0
      }
      background: Rectangle {
        color: "transparent"
        radius: UiMetrics.radiusSmall
        border {
          width: btnBrowse.hovered ? UiMetrics.borderWidth : 0
          color: Styles.background.dp04
        }
      }
      onClicked: openFile.open()
    }
  }
}
