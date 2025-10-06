import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

ListView {
    id: root
    spacing: 0
    delegate: NavigationMenuDelegate {
        width: root.width
        iconPath: model.iconPath
    }
    highlight: Rectangle {
        y: root.currentItem.y
        width: root.width; height: root.width
        radius: 4
        color: Styles.primary.base
        opacity: 0.2
        Behavior on y { NumberAnimation { duration: 300 }}
    }
    // highlightFollowsCurrentItem: false
    // section.criteria: ViewSection.FullString
    // section.delegate: Rectangle {
    //     width: parent.width; height: 40
    //     color: "transparent"
    //     Rectangle{
    //         width: parent.width - parent.height/2; height: 1
    //         anchors.centerIn: parent
    //         color: Styles.foreground.high
    //         opacity: 0.8
    //     }
    // }
}

