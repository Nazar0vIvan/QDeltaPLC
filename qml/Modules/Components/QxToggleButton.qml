import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic

import Styles 1.0

Button {
    id: root
    property string textOn:  "On"
    property string textOff: "Off"
    property bool checkableChecked: false

    signal turnedOn()
    signal turnedOff()

    checkable: true
    text: checked ? textOn : textOff

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 30
        radius: 6
        color: Styles.secondary.base
        border.color: Styles.background.dp04
    }
    contentItem: Text {
        text: root.text
        color: Styles.background.dp00
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.fill: parent
    }

    onCheckedChanged: {
        if (checked) turnedOn();
        else turnedOff();
    }
}
