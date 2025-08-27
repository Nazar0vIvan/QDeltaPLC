import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic

import Styles 1.0

Button {
    id: root
    property string textOn:  "On"
    property string textOff: "Off"

    signal turnedOn()
    signal turnedOff()

    checkable: true
    text: checked ? textOn : textOff

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 30
        radius: 6
        color: root.checked ? Styles.background.dp04 : Styles.secondary.base
        border.color: root.checked ? Styles.foreground.high : Styles.secondary.dark
    }
    contentItem: Text {
        text: root.text
        color: root.checked ? Styles.foreground.high : Styles.background.dp00
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.fill: parent
    }

    onCheckedChanged: {
        if (checked) turnedOn();
        else turnedOff();
    }
}
