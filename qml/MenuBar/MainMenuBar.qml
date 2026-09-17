import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0

MenuBar {
  id: root

  leftPadding: 40
  implicitHeight: 34

  Image {
    id: logoImage

    parent: root

    anchors {
      left: parent.left
      leftMargin: 6
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

    Action {
      text: qsTr("Quit")
      shortcut: StandardKey.Quit
      onTriggered: root.quit()
    }
  }

  Menu {
    title: qsTr("Edit")

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

    Action {
      text: qsTr("About ...")
      onTriggered: root.about()
    }
  }

  delegate: MenuBarItem {
    id: menuBarItem

    contentItem: Text {
      verticalAlignment: Text.AlignVCenter

      text: menuBarItem.text
      font: Fonts.body
      color: Colors.foreground.high
    }

    background: Rectangle {
      color: menuBarItem.hovered
             ? Colors.primary.transparent
             : "transparent"
    }
  }

  background: Rectangle {
    color: Colors.background.dp04
  }
}
