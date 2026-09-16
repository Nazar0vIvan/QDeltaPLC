import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property alias labelText: label.text
  property alias color: label.color
  property alias labelColor: label.color
  property alias spacing: cl.spacing
  default property alias content: slot.data

  implicitWidth: cl.implicitWidth
  implicitHeight: cl.implicitHeight

  ColumnLayout {
    id: cl

    spacing: Metrics.spacingSmall

    Label {
      id: label

      color: Colors.foreground.medium
      font: Fonts.body
    }

    RowLayout {
      id: slot

      Layout.alignment: Qt.AlignHCenter
      spacing: 0
    }
  }
}
