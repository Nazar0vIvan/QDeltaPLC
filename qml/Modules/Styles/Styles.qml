pragma Singleton
import QtQuick 2.12

QtObject {
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
    readonly property color disabled: "#6C6C6C"
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

  readonly property QtObject overlays: QtObject{
    readonly property real hovered:   0.08
    readonly property real pressed:   0.32
    readonly property real disabled:  0.38
  }

  readonly property QtObject fonts: QtObject{
    readonly property font title:   Qt.font({ family: "Roboto", pixelSize: 16 })
    readonly property font body:    Qt.font({ family: "Roboto", pixelSize: 14 })
    readonly property font caption: Qt.font({ family: "Roboto", pixelSize: 12 })
  }
}


