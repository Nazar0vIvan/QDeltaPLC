import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

FocusScope {
    id: root

    property string label: ""

    property alias model: comboBox.model
    property alias textRole: comboBox.textRole
    property alias valueRole: comboBox.valueRole
    property alias currentIndex: comboBox.currentIndex

    readonly property string currentText: comboBox.currentText
    readonly property var currentValue: comboBox.currentValue

    signal activated(int index)

    implicitWidth: 200
    implicitHeight: labelItem.implicitHeight + 8 + comboBox.implicitHeight

    Label {
        id: labelItem

        width: root.width
        text: root.label

        font: Styles.fonts.body
        color: Styles.foreground.medium
    }

    ComboBox {
        id: comboBox

        y: labelItem.implicitHeight + 8
        width: root.width
        focus: true

        onActivated: index => root.activated(index)

        contentItem: Text {
            text: comboBox.displayText
            padding: 10

            font: Styles.fonts.body
            color: Styles.foreground.high

            verticalAlignment: Text.AlignVCenter
        }

        indicator: Image {
            x: comboBox.width - width - 16
            y: (comboBox.height - height) / 2
            width: 14
            source: "qrc:/pics/arrow_dropdown.svg"
            fillMode: Image.PreserveAspectFit
        }

        background: Rectangle {
            color: "transparent"
            border.width: 1
            border.color: Styles.background.dp24
            radius: 4
        }
    }
}
