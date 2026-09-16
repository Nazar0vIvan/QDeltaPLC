pragma Singleton

import QtQuick

QtObject {
  readonly property int controlHeightCompact: 24
  readonly property int controlHeight: 32
  readonly property int controlHeightLarge: 40

  readonly property int fieldWidthSmall: 80
  readonly property int fieldWidthMedium: 120
  readonly property int fieldWidthLarge: 200

  readonly property int spacingXSmall: 4
  readonly property int spacingSmall: 8
  readonly property int spacingMedium: 12
  readonly property int spacingLarge: 16
  readonly property int spacingXLarge: 24
  readonly property int spacingXXLarge: 32

  readonly property int controlHorizontalPadding: 12
  readonly property int controlVerticalPadding: 8
  readonly property int panelPadding: 10
  readonly property int pageMargin: 20

  readonly property int indicatorSizeSmall: 12
  readonly property int indicatorSizeMedium: 20
  readonly property int indicatorSizeLarge: 36

  readonly property int iconSizeSmall: 12
  readonly property int iconSizeMedium: 16
  readonly property int iconSizeLarge: 24

  readonly property int radiusSmall: 4
  readonly property int radiusMedium: 6
  readonly property int radiusLarge: 10

  readonly property int borderWidth: 1
  readonly property int separatorWidth: 2

  readonly property int animationFast: 150
  readonly property int animationNormal: 220
}
