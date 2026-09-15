import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import "DeltaPanel"
import "FtsPanel"
import "KukaPanel"

QxScrollPage {
  id: root

  pageTopPadding: UiMetrics.controlHeightLarge
  spacing: UiMetrics.spacingLarge

  RowLayout {
    Layout.fillWidth: true

    DeltaPanel {
      id: deltaPanel

      title: "PLC AS332T-A"
    }
  }

  Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: UiMetrics.borderWidth

    gradient: Gradient {
      orientation: Gradient.Horizontal

      GradientStop {
        position: 0.0
        color: Styles.secondary.dark
      }

      GradientStop {
        position: 1.0
        color: Styles.background.dp00
      }
    }
  }

  RowLayout {
    Layout.fillWidth: true
    spacing: UiMetrics.spacingLarge

    KukaPanel {
      id: kukaPanel

      title: qsTr("Robot Sensor Interface")
      Layout.alignment: Qt.AlignTop
      Layout.topMargin: UiMetrics.spacingMedium
    }

    FtsPanel {
      id: ftsPanel

      title: qsTr("FTS Delta-IP68-SI-660-60")
      Layout.alignment: Qt.AlignTop
      Layout.topMargin: UiMetrics.spacingMedium
    }
  }
}
