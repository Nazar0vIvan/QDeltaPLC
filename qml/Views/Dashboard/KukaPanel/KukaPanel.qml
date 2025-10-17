import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  implicitWidth: leftPadding + cl.implicitWidth + rightPadding
  implicitHeight: topPadding + cl.implicitHeight + bottomPadding

  ColumnLayout {
    id: cl

    spacing: 14

    RowLayout {
      id: rl

      spacing: 20

      RsiPosition {
        id: cartesianPosition

        tags: ["X", "Y", "Z", "A", "B", "C"]
        dimension: "mm"
        title: "Cartesian Space"
      }
      RsiPosition {
        id: jointPosition

        tags: ["A1", "A2", "A3", "A4", "A5", "A6"]
        dimension: "deg"
        title: "Joint Space"
      }
    }

    QxButton {
      id: btnStart

      checked: rsiRunner && rsiRunner.isStreaming
      enabled: rsiRunner && rsiRunner.socketState === 4 // BoundState
      text: checked ? "Stop" : "Start"
      onClicked: {
        if (!rsiRunner)
          return
        if (checked) {
          rsiRunner.stopStreaming()
        } else {
          rsiRunner.startStreaming()
        }
      }
    }
  }
}
