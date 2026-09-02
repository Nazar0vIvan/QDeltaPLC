import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
    id: root

    required property string title
    default property alias content: contentLayout.children

    property int radius: 10

    implicitWidth: Math.max(
      header.implicitWidth,
      contentLayout.implicitWidth + 32)
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
            Layout.preferredHeight: 28

            leftPadding: 16
            rightPadding: 16
            topPadding: 6
            bottomPadding: 6

            text: root.title
            color: Styles.foreground.high
            font.pixelSize: 12

            background: Rectangle {
                color: Styles.background.dp03

                topLeftRadius: root.radius
                topRightRadius: root.radius
            }
        }

        ColumnLayout {
            id: contentLayout

            Layout.fillWidth: true
            Layout.fillHeight: true

            Layout.leftMargin: 16
            Layout.topMargin: 14
            Layout.rightMargin: 16
            Layout.bottomMargin: 14
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
