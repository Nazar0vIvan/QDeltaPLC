import QtQuick 2.12
import QtQuick.Layouts
import QtQuick.Controls 2.12
import QtQuick.Controls.Basic

import Styles 1.0
import Components 1.0

Window {
  width: 1000
  height: 1000
  visible: true
  title: qsTr("QDeltaPLC")
  color: Styles.background.dp00

  ColumnLayout {
    id: rl

    anchors.fill: parent
    anchors.topMargin: 10
    spacing: 10

    PLCConsole {
      id: plcPanel

      Layout.fillWidth: true;
      Layout.preferredHeight: 450
      Layout.leftMargin: 10; Layout.rightMargin: 10
    }

    Rectangle {
      id: separator

      Layout.fillWidth: true
      Layout.preferredHeight: 2
      color: Styles.background.dp04
    }

    FTSConsole {
      id: ftsPanel

      Layout.fillWidth: true;
      Layout.preferredHeight: 250
      Layout.leftMargin: 10; Layout.rightMargin: 10
    }

    Logger {
      id: loggerPanel

      Layout.fillHeight: true
      Layout.fillWidth: true
    }
  }
}
