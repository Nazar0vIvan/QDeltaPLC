pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

import RoboCrap.Backend 1.0 as Backend

QxPanel {
  id: root

  required property Backend.DeviceRunner plc
  required property var xTags
  required property var yTags
  required property var xPlugged
  required property var yPlugged

  property var yDisplayOnly: []
  property string xLabel: "Undefined"
  property string yLabel: "Undefined"
  property int moduleIndex: 1
  property var xStates: new Array(8).fill(false)
  property var yStates: new Array(8).fill(false)

  function refreshAll(xstates, ystates) {
    refreshX(xstates)
    refreshY(ystates)
  }

  function refreshX(xstates) {
    xStates = xstates.slice()
  }

  function refreshY(ystates) {
    yStates = ystates.slice()
  }

  function buildMasks(index, desiredOn, width = 8) {
    const full = (1 << width) - 1
    const bit = (1 << index) & full

    return {
      andMask: full ^ bit,
      orMask: desiredOn ? bit : 0x00
    }
  }

  function byteToBitString(n) {
    n = n & 0xFF
    return n.toString(2).padStart(8, "0")
  }

  RowLayout {
    spacing: UiMetrics.spacingXLarge

    ColumnLayout {
      spacing: UiMetrics.spacingSmall

      Text {
        text: root.xLabel
        textFormat: Text.RichText
        font: Styles.fonts.caption
        color: Styles.foreground.high
      }

      Repeater {
        model: 8

        delegate: DeltaModuleInput {
          required property int index

          enabled: root.xPlugged[index]
          ledSize: UiMetrics.indicatorSizeMedium
          labelText: "X" + root.moduleIndex + "." + index
          tag: root.xTags[index]
          isOn: root.xStates[index]
        }
      }
    }

    ColumnLayout {
      spacing: UiMetrics.spacingSmall

      Text {
        text: root.yLabel
        textFormat: Text.RichText
        font: Styles.fonts.caption
        color: Styles.foreground.high
      }

      Repeater {
        model: 8

        delegate: DeltaModuleOutput {
          id: output

          required property int index

          switchHeight: UiMetrics.indicatorSizeMedium
          switchWidth: UiMetrics.indicatorSizeLarge
          plugged: root.yPlugged[output.index]
          displayOnly: root.yDisplayOnly.includes(output.index)
          labelText: "Y" + root.moduleIndex + "." + output.index
          tag: root.yTags[output.index]
          isOn: root.yStates[output.index]

          onClicked: {
            const desired = !output.isOn
            const masks = root.buildMasks(output.index, desired)
            const args = {
              "cmd": Backend.PlcMessage.WRITE_IO,
              "module": root.moduleIndex,
              "andMask": masks.andMask,
              "orMask": masks.orMask
            }
            root.plc.invoke("writeMessage", args)
          }
        }
      }
    }
  }
}
