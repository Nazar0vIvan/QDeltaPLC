import QtQuick
import QtQuick.Layouts

import Styles 1.0

Item {
  id: root

  property alias title: sectionName.text
  property alias gap: cl.spacing
  property alias spacing: fields.spacing
  default property alias content: fields.data

  implicitHeight: cl.implicitHeight

  ColumnLayout {
    id: cl

    Text {
      id: sectionName

      Layout.fillWidth: true
      Layout.alignment: Qt.AlignVCenter
      verticalAlignment: Text.AlignVCenter
      color: Styles.foreground.high
      font: Styles.fonts.title
    }

    ColumnLayout {
      id: fields


    }
  }
}
