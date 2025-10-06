import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property int fieldWidth: 50

  RowLayout {
    id: rl

    anchors.fill: parent

    TextField {
      id: textField

      Layout.fillWidth: true
      Layout.fillHeight: true

      leftPadding: 5
      color: Styles.foreground.high
      selectionColor: Styles.primary.highlight
      selectByMouse: true
      readOnly: false

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
      id: browseButton

      Layout.fillHeight: true
      Layout.preferredWidth: root.height
      padding: 3

      contentItem: Image {
        fillMode: Image.PreserveAspectFit
        source: "open.svg"
        mipmap: true
        smooth: true
      }
      background: Rectangle {
        color: "transparent"
        radius: 4
        border {
          width: browseButton.hovered ? 1 : 0
          color: Styles.background.dp04
        }
      }
    }
  }
}
