import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic

import Styles 1.0

Button {
    id: control
    property string textOn:  "On"
    property string textOff: "Off"

    signal turnedOn()
    signal turnedOff()

    checkable: false
    text: checked ? textOn : textOff

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 30
        radius: 6
        color: control.checked ? Styles.background.dp04 : Styles.secondary.base
        border.color: control.checked ? Styles.foreground.high : Styles.secondary.dark
        opacity: control.pressed ? 0.8 : 1
    }
    contentItem: Text {
        anchors.fill: parent
        text: control.text
        color: control.checked ? Styles.foreground.high : Styles.background.dp00
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    onCheckedChanged: {
        if (checked) turnedOn();
        else turnedOff();
    }
}
