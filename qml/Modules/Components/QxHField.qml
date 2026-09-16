import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

RowLayout {
  id: root

  property int labelWidth: 0
  property alias labelText: label.text
  property alias color: label.color

  spacing: Metrics.spacingSmall

  Label {
    id: label

    Layout.preferredWidth: Math.max(root.labelWidth, label.implicitWidth)
    Layout.alignment: Qt.AlignVCenter

    verticalAlignment: Text.AlignVCenter
    color: Colors.foreground.high
    font: Fonts.body
  }
}
