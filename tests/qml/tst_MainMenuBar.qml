import QtQuick
import QtTest
import "../../qml/MenuBar"

TestCase {
  id: testCase

  name: "MainMenuBar"
  when: windowShown
  visible: true
  width: 500
  height: 200

  MainMenuBar {
    id: menuBar
    width: parent.width
  }

  SignalSpy {
    id: quitSpy
    target: menuBar
    signalName: "quitRequested"
  }

  function init() {
    failOnWarning(/TypeError|ReferenceError/)
    quitSpy.clear()
  }

  function test_quitRequestsOwnerAction() {
    verify(quitSpy.valid)
    const quitItem = menuBar.menuAt(0).itemAt(0)
    verify(quitItem.enabled)
    verify(quitItem.action.enabled)
    // Main.qml owns Qt.quit(); this isolated menu must only emit a request.
    quitItem.action.trigger()
    compare(quitSpy.count, 1)
    quitItem.action.trigger()
    compare(quitSpy.count, 2)
  }

  function test_unfinishedActionsAreDisabled() {
    const editMenu = menuBar.menuAt(1)
    compare(editMenu.count, 3)
    for (let index = 0; index < editMenu.count; ++index) {
      const item = editMenu.itemAt(index)
      verify(!item.action.enabled)
      verify(!item.enabled)
    }
    const aboutItem = menuBar.menuAt(2).itemAt(0)
    verify(!aboutItem.action.enabled)
    verify(!aboutItem.enabled)
    compare(quitSpy.count, 0)
  }
}
