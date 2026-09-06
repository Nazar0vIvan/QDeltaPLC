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

  implicitWidth: cl.implicitWidth
  implicitHeight: cl.implicitHeight

  ColumnLayout {
    id: cl

    spacing: 8

    Label {
      id: label

      Layout.preferredWidth: implicitWidth
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
