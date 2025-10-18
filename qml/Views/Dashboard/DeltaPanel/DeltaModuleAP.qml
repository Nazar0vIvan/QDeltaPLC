import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Control {
  id: root

  required property var xTags
  required property var yTags
  required property var xPlugged
  required property var yPlugged
  property var yDisplayOnly: []
  property string xLabel: "Undefined"
  property string yLabel: "Undefined"
  property int moduleIndex: 1

  property bool updatingFromSegment: false

  readonly property int rowHeight: 22
  readonly property int labelWidth: 28
  readonly property int switchWidth: 36

  property string title: ""

  signal outputChanged(var outputState)

  topPadding: 40; bottomPadding: 10
  leftPadding: 10; rightPadding: 10

  background: Rectangle {
    color: "transparent"
    border{width: 1; color: Styles.background.dp12}
  }

  contentItem: RowLayout {

    spacing: 30

    ListView {
      id: x_lv

      implicitWidth: contentItem.childrenRect.width
      implicitHeight: contentItem.childrenRect.height
      spacing: 10

      interactive: false
      boundsBehavior: Flickable.StopAtBounds
      clip: true

      opacity: root.enabled ? 1.0 : 0.5

      header: Text {
        height: 26
        textFormat: Text.RichText
        text: root.xLabel
        font.weight: 600
        font.pixelSize: 11
        color: Styles.foreground.high
      }

      model: 8

      delegate: DeltaModuleInput {
        id: x_field

        required property int index

        enabled: root.xPlugged[index]
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

      opacity: root.enabled ? 1.0 : 0.5

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
        plugged: root.yPlugged[index]
        displayonly: yDisplayOnly.includes(index)
        labelText: "Y" + root.moduleIndex + "." + index
        tag: root.yTags[index]

        onCheckedChanged: {
          if (root.updatingFromSegment) return
          plcRunner.invoke("writeMessage",
                          {
                            id: "Y",
                            module: root.moduleIndex,
                            output: index,
                            state: checked
                          }
          )
        }
      }
    }
  }

  Label {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    leftPadding: 10; rightPadding: 10
    topPadding: 6; bottomPadding: 6

    background: Rectangle {
      color: Styles.background.dp01
      border{width: 1; color: Styles.background.dp12}
    }

    textFormat: Text.RichText
    text: root.title

    color: Styles.foreground.medium
    font{pixelSize: 12}
  }

  onOutputChanged: outputState => {
    plcRunner.invoke("writeMessage", outputState)
   }

  Connections {
    target: plcRunner
    function onSegmentChanged(segment) {
      if (!segment || !segment.outputs || segment.id !== "Y" || segment.moduleIndex !== root.moduleIndex ) return
      root.updatingFromSegment = true
      const n = Math.min(8, segment.outputs.length)
      for (let i = 0; i < n; ++i) {
        const it = y_lv.itemAtIndex(i)
        if (it) it.checked = segment.outputs[i] === 1
      }
      root.updatingFromSegment = false
    }
  }
}
