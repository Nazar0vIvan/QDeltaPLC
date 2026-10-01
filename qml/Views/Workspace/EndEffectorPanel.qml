import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Components 1.0
import Styles 1.0

QxPanel {
  id: root
  required property bool measuring
  required property url cadSource
  required property bool loading
  required property string loadError
  required property string measuringName
  required property real ballDiameter
  required property real stylusLength
  required property list<double> tcp
  required property bool tcpVisible
  signal browseRequested()
  signal retryRequested()
  signal tcpRequested(var values)
  signal tcpVisibilityRequested(bool visible)

  property list<string> drafts: []
  readonly property bool validDraft: root.drafts.length === 6 && root.drafts.every(
      value => value.trim().length > 0 && isFinite(Number(value)))

  function loadTcpDraft() {
    const values = []
    for (let i = 0; i < 6; ++i)
      values.push(root.tcp.length === 6 ? (root.tcp[i] === 0 ? "0.00" : String(root.tcp[i])) : "0.00")
    root.drafts = values
  }
  function editValue(index, value) {
    const values = root.drafts.slice()
    values[index] = value
    root.drafts = values
  }
  onTcpChanged: root.loadTcpDraft()
  Component.onCompleted: root.loadTcpDraft()
  title: root.measuring ? qsTr("Measuring End Effector") : qsTr("Spindle End Effector")
  implicitHeight: fields.implicitHeight + topPadding + bottomPadding

  ScrollView {
    id: scroll
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
      id: fields
      width: scroll.availableWidth
      spacing: Metrics.sp8
      QxHField {
        Layout.fillWidth: true
        labelText: qsTr("CAD")
        QxTextInput {
          id: sourceField
          Layout.fillWidth: true
          Layout.minimumWidth: 0
          readOnly: true
          text: root.cadSource.toString()
          Accessible.name: qsTr("End-effector CAD file")
          background: Rectangle {
            color: "transparent"
            radius: sourceField.radius
            border.width: Metrics.w1
            border.color: sourceField.activeFocus ? Colors.primary.base : Colors.background.dp12
          }
        }
        QxToolButton {
          id: browseButton
          imageSource: "qrc:/pics/open.svg"
          imageSize: Metrics.sz16
          Accessible.name: qsTr("Browse CAD file")
          ToolTip.visible: browseButton.hovered
          ToolTip.text: qsTr("Browse CAD file")
          onClicked: root.browseRequested()
        }
      }
      Label {
        Layout.fillWidth: true
        visible: root.loading || root.loadError.length > 0
        text: root.loading ? qsTr("Loading CAD…") : root.loadError
        wrapMode: Text.Wrap
        color: Colors.foreground.medium
        font: Fonts.caption
      }
      QxButton {
        visible: !root.loading && root.loadError.length > 0
        text: qsTr("Retry CAD load")
        onClicked: root.retryRequested()
      }
      Repeater {
        model: root.measuring ? [qsTr("Name"), qsTr("Ruby ball diameter, mm"), qsTr("Stylus length, mm")] : []
        delegate: ColumnLayout {
          id: measurement
          required property int index
          required property string modelData
          Layout.fillWidth: true
          spacing: Metrics.sp4
          Label {
            Layout.fillWidth: true
            text: measurement.modelData
            wrapMode: Text.Wrap
            font: Fonts.caption
            color: Colors.foreground.medium
          }
          Label {
            Layout.fillWidth: true
            text: measurement.index === 0 ? root.measuringName
                  : (measurement.index === 1 ? root.ballDiameter : root.stylusLength).toFixed(3)
            wrapMode: Text.Wrap
            font: Fonts.body
            color: Colors.foreground.high
          }
        }
      }
      Label {
        Layout.fillWidth: true
        visible: !root.measuring
        text: qsTr("TCP pose relative to flange")
        wrapMode: Text.Wrap
        font: Fonts.caption
        color: Colors.foreground.medium
      }
      Repeater {
        model: 6
        delegate: QxHField {
          id: coordinate
          required property int index
          Layout.fillWidth: true
          visible: !root.measuring
          labelWidth: Metrics.sp24
          labelText: ["X", "Y", "Z", "A", "B", "C"][coordinate.index]
          QxTextInput {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.h32
            Layout.minimumWidth: 0
            text: root.drafts[coordinate.index] ?? ""
            font: Fonts.caption
            Accessible.name: qsTr("TCP %1").arg(coordinate.labelText)
            validator: DoubleValidator { locale: "C"; decimals: 8 }
            onTextEdited: root.editValue(coordinate.index, input.text)
          }
          Label {
            text: coordinate.index < 3 ? qsTr("mm") : qsTr("deg")
            font: Fonts.caption
            color: Colors.foreground.medium
          }
        }
      }
      QxHField {
        Layout.fillWidth: true
        visible: !root.measuring
        labelText: qsTr("TCP trihedron")
        QxCheckBox {
          checked: root.tcpVisible
          enabled: root.tcp.length === 6
          Accessible.name: qsTr("TCP trihedron visible")
          onClicked: root.tcpVisibilityRequested(checked)
        }
        Label {
          text: qsTr("Visible")
          color: Colors.foreground.high
          font: Fonts.body
        }
        Item { Layout.fillWidth: true }
      }
      RowLayout {
        visible: !root.measuring
        QxButton {
          text: qsTr("Apply TCP")
          enabled: root.validDraft
          onClicked: root.tcpRequested(root.drafts.map(value => Number(value)))
        }
      }
    }
  }
}
