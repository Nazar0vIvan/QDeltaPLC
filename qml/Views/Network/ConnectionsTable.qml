import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

// Backend C++ types only. Must NOT be a URI that also contains QML files,
// otherwise the compiled-in copies shadow the ones on disk and Felgo Hot
// Reload has no effect on them.
import QDelta.Backend 1.0 as Backend

Item {
  id: root

  required property var model
  required property var runners
  property int selectedRow: -1

  implicitWidth: table.contentWidth
  implicitHeight: header.implicitHeight + table.contentHeight

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    HorizontalHeaderView {
      id: header

      Layout.fillWidth: true

      syncView: table

      delegate: Rectangle {
        required property var display

        implicitHeight: 34
        color: "transparent"

        Text {
          anchors {
            fill: parent
            leftMargin: column === Backend.DeviceProfileModel.DeviceColumn ? 16 : 0
          }

          text: display
          font: Styles.fonts.body
          color: Styles.foreground.medium

          verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
          anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
          }

          height: 1
          color: Styles.background.dp24
        }
      }
    }

    TableView {
      id: table

      // Edit these during Hot Reload. onColumnWidthsChanged -> forceLayout()
      // covers the case where only the array literal changes.
      property var columnWidths: [
        194, // Device
        130, // Local Address
        90,  // Local Port
        130, // Peer Address
        90,  // Peer Port
        100, // Protocol
        100, // Open Mode
        150  // Status
      ]

      property int rowHeight: 36

      onColumnWidthsChanged: forceLayout()
      onRowHeightChanged: forceLayout()

      columnWidthProvider: column => columnWidths[column] ?? -1
      rowHeightProvider: row => rowHeight

      Layout.fillWidth: true
      Layout.preferredHeight: contentHeight

      model: root.model

      interactive: false
      clip: true

      rowSpacing: 0
      columnSpacing: 0

      delegate: Rectangle {
        id: cell

        required property int row
        required property int column
        required property var display

        readonly property var runner:
          cell.row >= 0 && cell.row < root.runners.length
            ? root.runners[cell.row] : null

        readonly property bool connected:
          runner && runner.isConnected

        color: row === root.selectedRow ? Styles.background.dp12 : "transparent"

        TextInput {
          visible:
            cell.column !== Backend.DeviceProfileModel.StatusColumn

          anchors {
            fill: parent
            leftMargin: column === Backend.DeviceProfileModel.DeviceColumn ? 16 : 0
          }

          text: cell.display ?? ""

          readOnly: true
          selectByMouse: true
          clip: true

          font: Styles.fonts.body
          color: Styles.foreground.high

          selectionColor: Styles.primary.highlight
          selectedTextColor: Styles.foreground.high

          verticalAlignment: TextInput.AlignVCenter
        }

        Row {
          visible: cell.column === Backend.DeviceProfileModel.StatusColumn

          anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
          }

          spacing: 8

          Rectangle {
            anchors.verticalCenter: parent.verticalCenter

            width: 12
            height: 12
            radius: width / 2

            color: cell.connected ? Styles.minColor : Styles.maxColor
          }

          TextInput {
            width: contentWidth
            height: parent.height

            text: cell.connected ? qsTr("Connected") : qsTr("Disconnected")

            readOnly: true
            selectByMouse: true

            font: Styles.fonts.body
            color: Styles.foreground.high

            selectionColor: Styles.primary.highlight
            selectedTextColor: Styles.foreground.high

            verticalAlignment: TextInput.AlignVCenter
          }
        }

        Rectangle {
          anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
          }

          height: 1
          color: Styles.background.dp24
        }
      }
    }
  }
}
