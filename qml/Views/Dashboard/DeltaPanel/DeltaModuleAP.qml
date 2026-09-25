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
    spacing: Metrics.sp24

    ColumnLayout {
      spacing: Metrics.sp8

      Text {
        text: root.xLabel
        opacity: root.enabled ? 1.0 : Colors.overlays.disabled
        textFormat: Text.RichText
        font: Fonts.caption
        color: Colors.foreground.high
      }

      Repeater {
        model: 8

        delegate: DeltaModuleInput {
          id: input

          required property int index

          plugged: root.xPlugged[input.index]
          ledSize: Metrics.sz20
          labelText: "X" + root.moduleIndex + "." + input.index
          tag: root.xTags[input.index]
          isOn: root.xStates[input.index]
        }
      }
    }

    ColumnLayout {
      spacing: Metrics.sp8

      Text {
        text: root.yLabel
        opacity: root.enabled ? 1.0 : Colors.overlays.disabled
        textFormat: Text.RichText
        font: Fonts.caption
        color: Colors.foreground.high
      }

      Repeater {
        model: 8

        delegate: DeltaModuleOutput {
          id: output

          required property int index

          switchHeight: Metrics.sz20
          switchWidth: Metrics.sz36
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
