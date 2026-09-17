import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2

// import QtQuick.Controls.Basic

import AppStyles 1.0

MenuBar{
  id: control

  signal newFile
  signal open
  signal save
  signal saveAs
  signal quit

  signal undo
  signal redo
  signal сut
  signal copy
  signal paste

  signal startTestTimer
  signal stopTestTimer

  AppMenu {
    title: qsTr("File")

    Action{ id: newFile; text: qsTr("New ...");     shortcut: StandardKey.New;  onTriggered: { control.newFile(); console.log("new") }}
    Action{ id: open;    text: qsTr("Open ...");    shortcut: StandardKey.Open; onTriggered: { control.open();    console.log("open") }}
    Action{ id: save;    text: qsTr("Save");        shortcut: StandardKey.Save; onTriggered: { control.save();    console.log("save")}}
    Action{ id: saveAs;  text: qsTr("Save As ..."); shortcut: "Ctrl+Shift+S";   onTriggered: { control.saveAs();  console.log("save As") }}
    MenuSeparator{
      leftPadding: 0; rightPadding: 0
      contentItem: Rectangle {
        implicitWidth: 180; implicitHeight: 1
        color: "gray"
      }
    }
    Action{ id: quit; text: qsTr("Quit"); shortcut: StandardKey.Quit; onTriggered: { control.quit(); console.log("quit") }}
  }

  AppMenu {
    title: qsTr("Edit")

    Action{ id: undo; text: qsTr("Undo"); shortcut: StandardKey.Undo; onTriggered: { control.open(); console.log("undo") }}
    Action{ id: redo; text: qsTr("Redo"); shortcut: StandardKey.Redo; onTriggered: { control.save(); console.log("redo") }}
    MenuSeparator{
      leftPadding: 0; rightPadding: 0
      contentItem: Rectangle {
        implicitWidth: 180
        implicitHeight: 1
        color: "gray"
      }
    }
    Action{ id: cut;   text: qsTr("Cut");   shortcut: StandardKey.Cut;   onTriggered: { control.saveAs(); console.log("cut")}}
    Action{ id: copy;  text: qsTr("Copy");  shortcut: StandardKey.Copy;  onTriggered: { control.undo();   console.log("copy") }}
    Action{ id: paste; text: qsTr("Paste"); shortcut: StandardKey.Paste; onTriggered: { control.redo();   console.log("save") }}
  }

  AppMenu {
    title: qsTr("Tools")

    Action {
      id: startTestTimerAction

      text: qsTr("Start Test Timer")
      icon.source: "/menu/play.svg"
      onTriggered: { control.startTestTimer(); console.log("start test timer"); enabled = false }
    }
    Action {
      id: stopTestTimerAction

      text: qsTr("Stop Test Timer")
      icon.source: "/menu/stop.svg"
      enabled: !startTestTimerAction.enabled
      onTriggered: { control.stopTestTimer(); console.log("stop test timer"); startTestTimerAction.enabled = true }
    }
  }

  AppMenu {
    title: qsTr("View")
  }

  AppMenu {
    title: qsTr("Help")
  }

  delegate: MenuBarItem{
    id: menuBarItem

    contentItem: Text{
      verticalAlignment: Text.AlignVCenter
      leftPadding: 8; rightPadding: 8
      text: menuBarItem.text
      font: Styles.fonts.body
      color: Styles.foreground.high
    }
    background: Rectangle {
      color: menuBarItem.hovered ? Styles.primary.transparent : "transparent"
    }
  }
  background: Rectangle{ color: Styles.background.dp00 }

  Component.onCompleted: {
    control.startTestTimer.connect(testTimer.start);
    control.stopTestTimer.connect(testTimer.stop);
  }
}



