pragma Singleton

import QtQuick 2.12

QtObject {
  readonly property font title:
    Qt.font({
      "family": "Roboto",
      "pixelSize": 16,
      "bold": true
    })
  readonly property font subtitle:
    Qt.font({
      "family": "Roboto",
      "pixelSize": 14,
      "bold": true
    })
  readonly property font body:
    Qt.font({
       "family": "Roboto",
       "pixelSize": 13
     })
  readonly property font caption:
    Qt.font({
      "family": "Roboto",
      "pixelSize": 12
     })
}
