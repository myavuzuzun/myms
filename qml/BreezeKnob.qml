import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 52
    height: 64

    StudioTheme { id: theme }

    property string label: "KNOB"
    property real value: 0.5
    property real internalValue: value
    property real minimumValue: 0.0
    property real maximumValue: 1.0
    property string displayValue: Math.round(internalValue * 100) + "%"
    property color activeColor: theme.accent

    onValueChanged: {
        internalValue = value
    }

    signal valueModified(real newValue)

    Column {
        anchors.fill: parent
        spacing: 2

        Text {
            width: parent.width
            text: root.label
            font.pixelSize: 9
            font.bold: true
            font.family: "Noto Sans, sans-serif"
            color: theme.textMuted
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Item {
            id: dialItem
            width: 32
            height: 32
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: mouseArea.containsMouse ? theme.bgWellHover : theme.bgWell
                border.color: mouseArea.containsMouse ? theme.accent : theme.border
                border.width: 1.5

                Rectangle {
                    width: 2.5
                    height: 9
                    color: root.activeColor
                    radius: 1.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 3
                    transformOrigin: Item.Bottom
                    rotation: -135 + (root.internalValue - root.minimumValue) / Math.max(0.0001, (root.maximumValue - root.minimumValue)) * 270
                }

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: theme.bgCard
                    border.color: theme.border
                    border.width: 1
                    anchors.centerIn: parent
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true

                property real startY: 0
                property real startVal: 0

                onPressed: (mouse) => {
                    startY = mouse.y
                    startVal = root.internalValue
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        let dy = startY - mouse.y
                        let range = root.maximumValue - root.minimumValue
                        let delta = (dy / 80.0) * range
                        let newVal = Math.max(root.minimumValue, Math.min(root.maximumValue, startVal + delta))
                        root.internalValue = newVal
                        root.valueModified(newVal)
                    }
                }

                onWheel: (wheel) => {
                    let step = (root.maximumValue - root.minimumValue) * 0.05
                    let newVal = wheel.angleDelta.y > 0 ? (root.internalValue + step) : (root.internalValue - step)
                    newVal = Math.max(root.minimumValue, Math.min(root.maximumValue, newVal))
                    root.internalValue = newVal
                    root.valueModified(newVal)
                }
            }
        }

        Text {
            width: parent.width
            text: root.displayValue
            font.pixelSize: 9
            font.bold: true
            font.family: "Noto Sans, sans-serif"
            color: mouseArea.containsMouse ? "#ffffff" : theme.textMain
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
}
