pragma Singleton

import QtQuick

QtObject {
  readonly property int controlHeightCompact: 24
  readonly property int controlHeight: 32
  readonly property int controlHeightLarge: 40

  readonly property int fieldWidthSmall: 80
  readonly property int fieldWidthMedium: 120
  readonly property int fieldWidthLarge: 200

  readonly property int sp0: 0
  readonly property int sp4: 4
  readonly property int sp6: 6
  readonly property int sp8: 8
  readonly property int sp10: 10
  readonly property int sp12: 12
  readonly property int sp16: 16
  readonly property int sp20: 20
  readonly property int sp24: 24
  readonly property int sp32: 32
  readonly property int sp36: 36
  readonly property int sp40: 40

  readonly property int spacingXSmall: sp4
  readonly property int spacingSmall: sp8
  readonly property int spacingMedium: sp12
  readonly property int spacingLarge: sp16
  readonly property int spacingXXLarge: sp32
  readonly property int controlHorizontalPadding: sp12
  readonly property int controlVerticalPadding: sp8
  readonly property int panelPadding: sp10

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
