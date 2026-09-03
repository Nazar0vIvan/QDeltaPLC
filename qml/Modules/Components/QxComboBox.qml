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
      padding: 8

      font: Styles.fonts.body
      color: Styles.foreground.high

      verticalAlignment: Text.AlignVCenter
    }

    indicator: Image {
      anchors.right: parent.right
      anchors.rightMargin: 16
      anchors.verticalCenter: parent.verticalCenter

      width: 12
      source: "qrc:/pics/arrow_dropdown.svg"
      fillMode: Image.PreserveAspectFit

      rotation: comboBox.popup.visible ? -90 : 0

      smooth: true
      mipmap: true

      Behavior on rotation {
        NumberAnimation {
          duration: 220
          easing.type: Easing.OutCubic
        }
      }
    }

    background: Rectangle {
      color: Styles.background.dp00
      border.width: 1
      border.color: Styles.background.dp24
      radius: 4
    }

    delegate: ItemDelegate {
      id: delegate

      required property int index

      width: comboBox.width - 12

      padding: 6

      text: comboBox.textAt(index)

      highlighted: comboBox.highlightedIndex === index

      contentItem: Text {
        text: delegate.text

        font: Styles.fonts.body
        color: Styles.foreground.high

        verticalAlignment: Text.AlignVCenter
      }

      background: Rectangle {
        color: delegate.highlighted ? Styles.primary.highlight : "transparent"
        radius: 4
      }
    }

    popup: Popup {
      id: popup

      y: comboBox.height + 6
      width: comboBox.width

      implicitHeight: contentItem.implicitHeight + 16

      padding: 6

      contentItem: ListView {
        clip: true
        implicitHeight: contentHeight
        model: comboBox.popup.visible
               ? comboBox.delegateModel
               : null
        currentIndex: comboBox.highlightedIndex
        ScrollIndicator.vertical: ScrollIndicator { }
      }

      background: Rectangle {
        radius: 4
        color: Styles.background.dp01
        border{width: 1; color: Styles.background.dp24}
      }
    }
  }
}
