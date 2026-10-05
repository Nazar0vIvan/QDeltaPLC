import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

RowLayout {
  id: root

  property int labelWidth: 0
  property bool fixedLabelWidth: false
  property alias labelText: label.text
  property alias color: label.color

  spacing: Metrics.sp8

  Label {
    id: label

    Layout.minimumWidth: root.fixedLabelWidth ? root.labelWidth : 0
    Layout.preferredWidth: root.fixedLabelWidth ? root.labelWidth
                                              : Math.max(root.labelWidth, label.implicitWidth)
    Layout.maximumWidth: root.fixedLabelWidth ? root.labelWidth : Number.POSITIVE_INFINITY
    Layout.alignment: Qt.AlignVCenter

    verticalAlignment: Text.AlignVCenter
    color: Colors.foreground.high
    font: Fonts.body
  }
}
