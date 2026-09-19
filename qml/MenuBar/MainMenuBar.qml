import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

MenuBar {
  id: root

  leftPadding: Metrics.sp36
  spacing: 6
  padding: Metrics.sp4
  background: Rectangle {
    color: Colors.background.dp00
  }

  delegate: MenuBarItem {
    id: menuBarItem

    padding: Metrics.sp4
    leftPadding: Metrics.sp8
    rightPadding: Metrics.sp8

    contentItem: Text {
      verticalAlignment: Text.AlignVCenter

      text: menuBarItem.text
      font: Fonts.caption
      color: menuBarItem.hovered ? Colors.foreground.high : Colors.foreground.medium
    }

    background: Rectangle {
      color: Colors.foreground.high
      radius: 4
      opacity: 0.2
      visible: menuBarItem.hovered
    }
  }

  Image {
    id: logoImage

    parent: root

    anchors {
      left: parent.left
      top: parent.top

      leftMargin: Metrics.sp8
      verticalCenter: parent.verticalCenter
    }

    width: 20
    height: 20

    source: "qrc:/pics/logo.svg"
    fillMode: Image.PreserveAspectFit
    smooth: true
  }

  Menu {
    title: qsTr("File")
    popupType: Popup.Window

    Action {
      text: qsTr("Quit")
      shortcut: StandardKey.Quit
      onTriggered: root.quit()
    }
  }

  Menu {
    title: qsTr("Edit")
    popupType: Popup.Window

    Action {
      text: qsTr("Cut")
      shortcut: StandardKey.Cut
      onTriggered: root.cut()
    }

    Action {
      text: qsTr("Copy")
      shortcut: StandardKey.Copy
      onTriggered: root.copy()
    }

    Action {
      text: qsTr("Paste")
      shortcut: StandardKey.Paste
      onTriggered: root.paste()
    }
  }

  Menu {
    title: qsTr("Help")
    popupType: Popup.Window

    Action {
      text: qsTr("About ...")
      onTriggered: root.about()
    }
  }
}
