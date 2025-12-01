import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Components 1.0
import Styles 1.0


Control {
  id: root

  property string title: ""
  property int fieldHeight: 28
  property int fieldWidth: 50

  topPadding: 40
  bottomPadding: 10
  leftPadding: 10
  rightPadding: 10

  contentItem: ColumnLayout {

    spacing: 10

    QxField {
      id: autExtField

      labelText: "Status: "
      height: root.fieldHeight

      Rectangle {
        id: autExtStatus

        width: 14; height: 14
        radius: width / 2
        color: "red"
        anchors.verticalCenter: parent.verticalCenter
      }
    }

    QxField {
      id: pgnoField

      labelText: "PGNO: "
      height: root.fieldHeight

      QxTextInput {
        id: pgno

        height: root.fieldHeight
        width: root.fieldWidth
        text: "1"
      }
    }

    QxButton {
      id: btnStartProgram

      text: "Execute"
    }
  }

  background: Rectangle {
    color: "transparent"
    border {
      width: 1
      color: Styles.background.dp12
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top

    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    font: Styles.fonts.body
    textFormat: Text.RichText
    color: Styles.foreground.medium
    text: root.title

    background: Rectangle {
      color: Styles.background.dp01
      border {
        width: 1
        color: Styles.background.dp12
      }
    }
  }
}
