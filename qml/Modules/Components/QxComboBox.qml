pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

import Styles 1.0

ComboBox {
  id: root

  leftPadding: UiMetrics.controlHorizontalPadding
  rightPadding: UiMetrics.spacingXXLarge
  topPadding: UiMetrics.controlVerticalPadding
  bottomPadding: UiMetrics.controlVerticalPadding

  contentItem: Text {
    text: root.displayText
    font: Styles.fonts.body
    color: Styles.foreground.high
    verticalAlignment: Text.AlignVCenter
  }

  indicator: Image {
    anchors {
      right: parent.right
      rightMargin: UiMetrics.spacingMedium
      verticalCenter: parent.verticalCenter
    }

    width: UiMetrics.iconSizeSmall
    height: width
    source: "qrc:/pics/arrow_dropdown.svg"
    fillMode: Image.PreserveAspectFit
    rotation: root.popup.visible ? -90 : 0
    smooth: true
    mipmap: true

    Behavior on rotation {
      NumberAnimation {
        duration: UiMetrics.animationNormal
        easing.type: Easing.OutCubic
      }
    }
  }

  background: Rectangle {
    color: Styles.background.dp00
    border.width: UiMetrics.borderWidth
    border.color: Styles.background.dp24
    radius: UiMetrics.radiusSmall
  }

  delegate: ItemDelegate {
    id: delegate

    required property int index

    width: root.width - UiMetrics.spacingMedium
    padding: UiMetrics.spacingSmall
    text: root.textAt(delegate.index)
    highlighted: root.highlightedIndex === delegate.index

    contentItem: Text {
      text: delegate.text
      font: Styles.fonts.body
      color: Styles.foreground.high
      verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
      color: delegate.highlighted ? Styles.primary.highlight : "transparent"
      radius: UiMetrics.radiusSmall
    }
  }

  popup: Popup {
    id: popup

    y: root.height + UiMetrics.spacingXSmall
    width: root.width
    implicitHeight: contentItem.implicitHeight + 2 * padding
    padding: UiMetrics.spacingSmall

    contentItem: ListView {
      clip: true
      implicitHeight: contentHeight
      model: root.popup.visible ? root.delegateModel : null
      currentIndex: root.highlightedIndex
      ScrollIndicator.vertical: ScrollIndicator { }
    }

    background: Rectangle {
      radius: UiMetrics.radiusSmall
      color: Styles.background.dp01
      border {
        width: UiMetrics.borderWidth
        color: Styles.background.dp24
      }
    }
  }
}
