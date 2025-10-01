import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  ListView {
    id: lv

    spacing: 10
    anchors.fill: parent

    model: ListModel {
        ListElement { tag: "Fx"; from: -1980.0; to: 1980.0; idx: 3 }
        ListElement { tag: "Fy"; from: -660.0;  to: 660.0;  idx: 4 }
        ListElement { tag: "Fz"; from: -660.0;  to: 660.0;  idx: 5 }
        ListElement { tag: "Tx"; from: -60.0;   to: 60.0;   idx: 6 }
        ListElement { tag: "Ty"; from: -60.0;   to: 60.0;   idx: 7 }
        ListElement { tag: "Tz"; from: -60.0;   to: 60.0;   idx: 8 }
    }

    delegate: QxProgressBar {
      id: bar

      height: 20
      width: lv.width
      color: "orange"

      from: model.from
      to: model.to
      labelText: model.tag

      value: -1800.0 // ftsRunner.isStreaming && ftsRunner.lastReading.length !== 0 ? Number((ftsRunner.lastReading[model.idx])/1000000).toFixed(3) : 0.000
    }
  }
}
