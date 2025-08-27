import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.15

import Styles 1.0

Item {
    id: root

    property int labelWidth: 100
    property int textFieldWidth: 150
    property alias labelText: label.text
    property alias text: txtField.text
    property alias isReadonly: txtField.readOnly
    property alias spacing: rl.spacing

    RowLayout {
        id: rl

        anchors.fill: parent
        spacing: 10

        Text {
            id: label

            Layout.preferredHeight: root.height
            Layout.preferredWidth:  root.labelWidth
            Layout.alignment: Qt.AlignCenter | Qt.AlignLeft
            color: Styles.foreground.high
            verticalAlignment: Text.AlignVCenter

        }

        TextField {
            id: txtField

            height: root.height
            readOnly: false
            Layout.preferredHeight: root.height
            Layout.preferredWidth:  root.textFieldWidth
            color: Styles.foreground.high
            background: Rectangle {
                color: Styles.background.dp04
            }
        }
    }
}
