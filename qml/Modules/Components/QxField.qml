import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Styles 1.0

Item {
    id: root

    property string labelText: ""
    property real labelWidth: label.implicitWidth
    property alias color: label.color
    property alias spacing: rl.spacing
    default property alias content: slot.data

    RowLayout {
        id: rl

        anchors.fill: parent

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
