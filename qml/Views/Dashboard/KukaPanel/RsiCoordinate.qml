import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Styles 1.0
import Components 1.0

Item {
  id: root

  property alias tag: tag.text

  implicitWidth: rl.implicitWidth
  implicitHeight: rl.implicitHeight

  RowLayout {
    id: rl

    spacing: 10

    Text {
      id: tag

      color: Styles.foreground.high
    }

    QxStepButton {
      id: btnStep
    }
  }
}
