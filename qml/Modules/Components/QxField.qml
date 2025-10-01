import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

Item {
    id: root

    property string labelText: ""
    property int labelWidth: 0
    property alias color: label.color
    property alias spacing: rl.spacing
    default property alias content: slot.data

    implicitWidth: rl.implicitWidth
    implicitHeight: rl.implicitHeight

    RowLayout {
        id: rl

        Label {
            id: label
            text: root.labelText
            color: Styles.foreground.high
            Layout.preferredWidth: root.labelWidth
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            verticalAlignment: Text.AlignVCenter
        }

        Item {
            id: slot

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
