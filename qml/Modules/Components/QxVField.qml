import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias color: label.color
  property alias spacing: cl.spacing
  default property alias content: slot.data

  implicitWidth: cl.implicitWidth // Math.max(root.labelWidth, label.implicitWidth) + rl.spacing + slot.data.width
  implicitHeight: cl.implicitHeight

  ColumnLayout {
    id: cl

    spacing: 8

    Label {
      id: label

      Layout.preferredWidth: Math.max(root.labelWidth, implicitWidth)
      Layout.preferredHeight: implicitHeight

      color: Styles.foreground.medium
      font: Styles.fonts.body
    }

    Item {
      id: slot

      Layout.preferredWidth: childrenRect.width
      Layout.preferredHeight: childrenRect.height
    }
  }
}
