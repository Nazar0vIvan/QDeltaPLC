import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

MenuBar {
  id: root

  signal quitRequested()
  signal plcPanelRequested()
  signal ftsPanelRequested()
  signal settingsRequested()

  component ContentMenu: Menu {
    popupType: Popup.Window
    height: opened ? implicitHeight : undefined
  }

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

  ContentMenu {
    title: qsTr("File")

    Action {
      text: qsTr("Quit")
      shortcut: StandardKey.Quit
      onTriggered: root.quitRequested()
    }
  }

  ContentMenu {
    title: qsTr("Edit")

    Action {
      text: qsTr("Cut")
      shortcut: StandardKey.Cut
      enabled: false
    }

    Action {
      text: qsTr("Copy")
      shortcut: StandardKey.Copy
      enabled: false
    }

    Action {
      text: qsTr("Paste")
      shortcut: StandardKey.Paste
      enabled: false
    }

    MenuSeparator { }

    Action {
      text: qsTr("Settings")
      onTriggered: root.settingsRequested()
    }
  }

  ContentMenu {
    title: qsTr("Views")

    Action {
      text: qsTr("PLC Panel")
      onTriggered: root.plcPanelRequested()
    }

    Action {
      text: qsTr("FTS Panel")
      onTriggered: root.ftsPanelRequested()
    }
  }

  ContentMenu {
    title: qsTr("Help")

    Action {
      text: qsTr("About ...")
      enabled: false
    }
  }
}
