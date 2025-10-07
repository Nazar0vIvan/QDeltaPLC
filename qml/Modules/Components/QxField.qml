import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
    id: root

    property int labelWidth: 50
    property alias labelText: label.text
    property alias color: label.color
    property alias spacing: rl.spacing
    default property alias content: slot.data

    implicitWidth: Math.max(root.labelWidth, label.implicitWidth) + rl.spacing + slot.data.width

    RowLayout {
        id: rl

        anchors.fill: parent

        Label {
            id: label

            Layout.preferredWidth: Math.max(root.labelWidth, implicitWidth)
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter

            text: root.labelText
            color: Styles.foreground.high
            verticalAlignment: Text.AlignVCenter
        }

        Item {
            id: slot

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
