pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

ComboBox {
  id: root

  leftPadding: Metrics.sp12
  rightPadding: Metrics.sp32
  topPadding: Metrics.sp8
  bottomPadding: Metrics.sp8

  contentItem: Text {
    text: root.displayText
    font: Fonts.body
    color: Colors.foreground.high
    verticalAlignment: Text.AlignVCenter
  }

  indicator: Image {
    anchors {
      right: parent.right
      rightMargin: Metrics.sp12
      verticalCenter: parent.verticalCenter
    }

    width: Metrics.iconSizeSmall
    height: width
    source: "qrc:/pics/arrow_dropdown.svg"
    fillMode: Image.PreserveAspectFit
    rotation: root.popup.visible ? -90 : 0
    smooth: true
    mipmap: true

    Behavior on rotation {
      NumberAnimation {
        duration: Metrics.animationNormal
        easing.type: Easing.OutCubic
      }
    }
  }

  background: Rectangle {
    color: Colors.background.dp00
    border.width: Metrics.borderWidth
    border.color: Colors.background.dp24
    radius: Metrics.radiusSmall
  }

  delegate: ItemDelegate {
    id: delegate

    required property int index

    width: ListView.view.width
    padding: Metrics.sp8

    text: root.textAt(delegate.index)
    highlighted: root.highlightedIndex === delegate.index

    contentItem: Text {
      text: delegate.text
      font: Fonts.body
      color: Colors.foreground.high
      verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
      color: delegate.highlighted ? Colors.primary.highlight : "transparent"
      radius: Metrics.radiusSmall
    }
  }

  popup: Popup {
    id: popup

    y: root.height + Metrics.sp4
    width: root.width
    implicitHeight: contentItem.implicitHeight + 2 * padding
    padding: Metrics.sp8

    contentItem: ListView {
      clip: true
      implicitHeight: contentHeight
      model: root.popup.visible ? root.delegateModel : null
      currentIndex: root.highlightedIndex
      ScrollIndicator.vertical: ScrollIndicator { }
    }

    background: Rectangle {
      radius: Metrics.radiusSmall
      color: Colors.background.dp01
      border {
        width: Metrics.borderWidth
        color: Colors.background.dp24
      }
    }
  }
}
