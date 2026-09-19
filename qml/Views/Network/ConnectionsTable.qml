pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import RoboCrap.Backend 1.0 as Backend

Item {
  id: root

  required property Backend.DeviceProfileModel model
  required property list<Backend.DeviceRunner> runners
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
        id: headerCell

        required property var display
        required property int column

        implicitHeight: Metrics.h32
        color: "transparent"

        Text {
          anchors {
            fill: parent
            leftMargin: headerCell.column === Backend.DeviceProfileModel.DeviceColumn
                        ? Metrics.sp16
                        : 0
          }

          text: headerCell.display
          font: Fonts.body
          color: Colors.foreground.medium

          verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
          anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
          }

          height: Metrics.w1
          color: Colors.background.dp24
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

      property int rowHeight: Metrics.h32

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

        readonly property Backend.DeviceRunner runner:
          cell.row >= 0 && cell.row < root.runners.length
            ? root.runners[cell.row]
            : null

        readonly property bool connected:
          runner && runner.isConnected

        color: cell.row === root.selectedRow
               ? Colors.background.dp12
               : "transparent"

        TextInput {
          visible:
            cell.column !== Backend.DeviceProfileModel.StatusColumn

          anchors {
            fill: parent
            leftMargin: cell.column === Backend.DeviceProfileModel.DeviceColumn
                        ? Metrics.sp16
                        : 0
          }

          text: cell.display ?? ""

          readOnly: true
          selectByMouse: true
          clip: true

          font: Fonts.body
          color: Colors.foreground.high

          selectionColor: Colors.primary.highlight
          selectedTextColor: Colors.foreground.high

          verticalAlignment: TextInput.AlignVCenter
        }

        Row {
          visible: cell.column === Backend.DeviceProfileModel.StatusColumn

          anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
          }

          spacing: Metrics.sp8

          Rectangle {
            anchors.verticalCenter: parent.verticalCenter

            width: Metrics.sz12
            height: width
            radius: width / 2

            color: cell.connected ? Colors.minColor : Colors.maxColor
          }

          TextInput {
            width: contentWidth
            height: parent.height

            text: cell.connected ? qsTr("Connected") : qsTr("Disconnected")

            readOnly: true
            selectByMouse: true

            font: Fonts.body
            color: Colors.foreground.high

            selectionColor: Colors.primary.highlight
            selectedTextColor: Colors.foreground.high

            verticalAlignment: TextInput.AlignVCenter
          }
        }

        Rectangle {
          anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
          }

          height: Metrics.w1
          color: Colors.background.dp24
        }
      }
    }
  }
}
