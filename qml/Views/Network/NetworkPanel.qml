import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
    id: root

    required property string title
    default property alias content: contentLayout.children
    property alias spacing: contentLayout.spacing

    property double contentHorizontalMargin: 0
    property double contentVerticalMargin: 0

    property int radius: 10

    implicitWidth: Math.max(
      header.implicitWidth,
      contentLayout.implicitWidth + 2 * contentHorizontalMargin)

    implicitHeight: layout.implicitHeight

    Rectangle {
      anchors.fill: parent

      color: "transparent"
      radius: root.radius
    }

    ColumnLayout {
      id: layout

      anchors.fill: parent
      spacing: 0

      Label {
        id: header

        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight

        leftPadding: 16
        rightPadding: 16
        topPadding: 10
        bottomPadding: 10

        text: root.title
        color: Styles.foreground.high
        font: Styles.fonts.subtitle

        background: Rectangle {
          color: Styles.background.dp03

          topLeftRadius: root.radius
          topRightRadius: root.radius
        }
      }

      RowLayout {
        id: contentLayout

        spacing: 10

        Layout.fillWidth: true
        Layout.fillHeight: true

        Layout.leftMargin: root.contentHorizontalMargin
        Layout.rightMargin: root.contentHorizontalMargin
        Layout.topMargin: root.contentVerticalMargin
        Layout.bottomMargin: root.contentVerticalMargin
      }
    }

    // border
    Rectangle {
      anchors.fill: parent

      color: "transparent"
      radius: root.radius

      border {
        width: 1
        color: Styles.background.dp24
      }
    }
}
