import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

QxGroupBox {
  id: root

  required property var xTags
  required property var yTags
  property string xLabel: "Undefined"
  property string yLabel: "Undefined"
  property int moduleIndex: 1

  readonly property int rowHeight: 22
  readonly property int labelWidth: 28
  readonly property int switchWidth: 36
  readonly property int viewWidth: 110
  readonly property int viewHeight: 8*root.rowHeight + 7*spacing

  implicitWidth: leftPadding + rl.implicitWidth + rightPadding
  implicitHeight: topPadding + rl.implicitHeight + bottomPadding

  RowLayout {
    id: rl

    spacing: 30

    ListView {
      id: x_lv

      implicitWidth: contentItem.childrenRect.width
      implicitHeight: contentItem.childrenRect.height
      spacing: 10

      interactive: false
      boundsBehavior: Flickable.StopAtBounds
      clip: true

      header: Text {
        height: 26
        textFormat: Text.RichText
        text: root.xLabel
        font.weight: 600
        font.pixelSize: 12
        color: Styles.foreground.high
      }

      model: 8

      delegate: DeltaModuleInput {
        id: x_field

        required property int index

        ledSize: root.rowHeight
        labelText: "X" + root.moduleIndex + "." + index
        tag: root.xTags[index]
      }
    }

    ListView {
      id: y_lv

      implicitWidth: contentItem.childrenRect.width
      implicitHeight: contentItem.childrenRect.height
      spacing: 10

      interactive: false
      boundsBehavior: Flickable.StopAtBounds
      clip: true

      header: Text {
        height: 26
        textFormat: Text.RichText
        text: root.yLabel
        font.weight: 600
        font.pixelSize: 12
        color: Styles.foreground.high
      }

      model: 8

      delegate: DeltaModuleOutput {
        id: y_field

        required property int index

        switchHeight: root.rowHeight
        switchWidth: root.switchWidth
        labelText: "Y" + root.moduleIndex + "." + index
        tag: root.yTags[index]
      }
    }
  }
}
