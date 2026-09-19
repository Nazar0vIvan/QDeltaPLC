import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import "DeltaPanel"
import "FtsPanel"
import "KukaPanel"

QxScrollView {
  id: root

  spacing: Metrics.sp16

  RowLayout {
    Layout.fillWidth: true

    DeltaPanel {
      id: deltaPanel

      title: "PLC AS332T-A"
    }
  }

  Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: Metrics.w1

    gradient: Gradient {
      orientation: Gradient.Horizontal

      GradientStop {
        position: 0.0
        color: Colors.secondary.dark
      }

      GradientStop {
        position: 1.0
        color: Colors.background.dp00
      }
    }
  }

  RowLayout {
    Layout.fillWidth: true
    spacing: Metrics.sp16

    KukaPanel {
      id: kukaPanel

      title: qsTr("Robot Sensor Interface")
      Layout.alignment: Qt.AlignTop
      Layout.topMargin: Metrics.sp12
    }

    FtsPanel {
      id: ftsPanel

      title: qsTr("FTS Delta-IP68-SI-660-60")
      Layout.alignment: Qt.AlignTop
      Layout.topMargin: Metrics.sp12
    }
  }
}
