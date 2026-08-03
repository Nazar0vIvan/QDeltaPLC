import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property int labelWidth: 0
  property alias labelText: label.text
  property alias color: label.color
  property alias spacing: rl.spacing
  default property alias content: slot.data

  implicitWidth: rl.implicitWidth // Math.max(root.labelWidth, label.implicitWidth) + rl.spacing + slot.data.width
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    Label {
      id: label

      Layout.preferredWidth: Math.max(root.labelWidth, implicitWidth)
      Layout.preferredHeight: root.height
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      color: Styles.foreground.high
      font: Styles.fonts.body
    }

    Item {
      id: slot

      Layout.preferredWidth: childrenRect.width
      Layout.preferredHeight: root.height
      Layout.alignment: Qt.AlignVCenter
    }
  }
}
