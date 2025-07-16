pragma Singleton
import QtQuick 2.12

QtObject {

  readonly property color foo: "red"

  readonly property QtObject background: QtObject {
    readonly property color dp00: "#121212"
    readonly property color dp01: "#6C6C6C"
    readonly property color dp02: "#232323"
    readonly property color dp03: "#252525"
    readonly property color dp04: "#272727"
    readonly property color dp06: "#2C2C2C"
    readonly property color dp08: "#2F2F2F"
    readonly property color dp12: "#333333"
    readonly property color dp16: "#353535"
    readonly property color dp24: "#383838"
  }

  readonly property QtObject foreground: QtObject {
    readonly property color high: "#e0e0e0"
    readonly property color medium: "#A0A0A0"
    readonly property color disable: "#6C6C6C"
  }

  readonly property color minColor: "#4ed964"
  readonly property color midColor: "#ffff00"
  readonly property color maxColor: "#ff3a31"

  readonly property QtObject primary: QtObject{
    readonly property color base:        "#509dfd"
    readonly property color light:       "#b5d5fe"
    readonly property color dark:        "#0367e4"
    readonly property color transparent: "#50509dfd"
    readonly property color highlight:   "#80509dfd"
  }

  readonly property QtObject secondary: QtObject{
    readonly property color base:  "#fdb050"
    readonly property color light: "#ffc174"
    readonly property color dark:  "#ff920b"
  }

  enum ButtonType{ Text = 0, Outlined = 1, Contained = 2 }

  readonly property QtObject overlays: QtObject{
    readonly property QtObject text: QtObject{
      readonly property real hovered:   0.04
      readonly property real pressed:   0.16
    }
    readonly property QtObject outlined: QtObject{
      readonly property real hovered:   0.04
      readonly property real pressed:   0.16
    }
    readonly property QtObject contained: QtObject{
      readonly property real hovered:   0.08
      readonly property real pressed:   0.32
    }
    readonly property real disabled:  0.38
  }

  property var buttonType2Overlay: new Map([[0, overlays.text], [1, overlays.outlined], [2, overlays.contained]])

  readonly property QtObject fonts: QtObject{
    readonly property font logo:      Qt.font({ family: "Open-Sans", pixelSize: 22 })
    readonly property font headline1: Qt.font({ family: "Roboto", pixelSize: 20, bold: true })
    readonly property font headline2: Qt.font({ family: "Roboto", pixelSize: 16, bold: true })
    readonly property font title:     Qt.font({ family: "Roboto", pixelSize: 16 })
    readonly property font subtitle:  Qt.font({ family: "Roboto", pixelSize: 14, bold: true})
    readonly property font body:      Qt.font({ family: "Roboto", pixelSize: 14 })
    readonly property font caption:   Qt.font({ family: "Roboto", pixelSize: 12 })
    readonly property font log:       Qt.font({ family: "Consolas", pixelSize: 14 })

    readonly property font gaugeTitle:     Qt.font({ family: "Arial", pixelSize: 18 })
    readonly property font gaugeValue:     Qt.font({ family: "Arial", pixelSize: 28 })
    readonly property font numericTitle:   Qt.font({ family: "Arial", pixelSize: 22 })
    readonly property font numericValue:   Qt.font({ family: "Arial", pixelSize: 28 })
    readonly property font chartTitle:     Qt.font({ family: "Arial", pixelSize: 12 })
    readonly property font chartAxisTitle: Qt.font({ family: "Arial", pixelSize: 12 })
    readonly property font chartAxisLabel: Qt.font({ family: "Arial", pixelSize: 10 })
    readonly property font chartLegend:    Qt.font({ family: "Arial", pixelSize: 12 })
  }

  readonly property int drawerWidth: 220
}


