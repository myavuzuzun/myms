import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    StudioTheme { id: theme }

    property var engine
    property bool isOpen: false

    visible: isOpen

    function open() {
        isOpen = true
        root.forceActiveFocus()
    }

    function close() {
        isOpen = false
    }

    // Modal Scrim Backdrop
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.65

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Modal Window Card
    Rectangle {
        id: modalDialog
        anchors.centerIn: parent
        width: 580
        height: 220
        color: theme.bgCard
        radius: 6
        border.color: theme.borderLight
        border.width: 1
        clip: false

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        // Floating Centered Circular Close Button Above Window
        Rectangle {
            id: floatingCloseBtn
            width: 30
            height: 30
            radius: 15
            anchors.bottom: modalDialog.top
            anchors.bottomMargin: 10
            anchors.horizontalCenter: modalDialog.horizontalCenter
            color: closeMouse.containsMouse ? theme.stopAccent : theme.bgCard
            border.color: closeMouse.containsMouse ? "#ffffff" : theme.borderLight
            border.width: 1
            z: 999

            Text {
                anchors.centerIn: parent
                text: "✕"
                font.pixelSize: 13
                font.bold: true
                color: closeMouse.containsMouse ? "#ffffff" : theme.textHeading
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.close()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            // Header Title Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "🎛 AUTO-MASTERING SMART MACROS"
                    font.bold: true
                    font.pixelSize: 12
                    font.letterSpacing: 0.5
                    color: theme.textHeading
                }

                Rectangle {
                    width: 65
                    height: 16
                    radius: 2
                    color: "#1e3a5f"
                    border.color: theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "MASTER BUS"
                        font.bold: true
                        font.pixelSize: 8
                        color: theme.accent
                    }
                }

                Item { Layout.fillWidth: true }

                // Quick Reset Button
                Button {
                    implicitHeight: 22
                    text: "↺ RESET"
                    font.pixelSize: 9
                    font.bold: true
                    background: Rectangle {
                        color: parent.hovered ? theme.btnHover : theme.btnBg
                        border.color: theme.border
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.parent.hovered ? "#ffffff" : theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (engine) {
                            engine.setMacroPunch(0.20)
                            engine.setMacroWarmth(0.25)
                            engine.setMacroTone(0.50)
                            engine.setMacroSpace(0.15)
                            engine.setMasterVolume(0.85)
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.border
            }

            // Knobs Row
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 16

                Item { Layout.fillWidth: true }

                BreezeKnob {
                    id: punchKnob
                    label: "PUNCH"
                    value: engine ? engine.macroPunch : 0.20
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: Math.round(internalValue * 100) + "%"
                    activeColor: "#e67e22"
                    onValueModified: (v) => { if (engine) engine.setMacroPunch(v); }
                }

                BreezeKnob {
                    id: warmthKnob
                    label: "WARMTH"
                    value: engine ? engine.macroWarmth : 0.25
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: Math.round(internalValue * 100) + "%"
                    activeColor: "#e74c3c"
                    onValueModified: (v) => { if (engine) engine.setMacroWarmth(v); }
                }

                BreezeKnob {
                    id: toneKnob
                    label: "TONE"
                    value: engine ? engine.macroTone : 0.50
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: (internalValue < 0.48 ? "Warm" : (internalValue > 0.52 ? "Air" : "Flat"))
                    activeColor: theme.accent
                    onValueModified: (v) => { if (engine) engine.setMacroTone(v); }
                }

                BreezeKnob {
                    id: spaceKnob
                    label: "SPACE"
                    value: engine ? engine.macroSpace : 0.15
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: Math.round(internalValue * 100) + "%"
                    activeColor: "#1abc9c"
                    onValueModified: (v) => { if (engine) engine.setMacroSpace(v); }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    Layout.topMargin: 10
                    Layout.bottomMargin: 10
                    color: theme.border
                }

                BreezeKnob {
                    id: masterKnob
                    label: "MASTER"
                    value: engine ? engine.masterVolume : 0.85
                    minimumValue: 0.0
                    maximumValue: 1.5
                    displayValue: (Math.round((internalValue - 1.0) * 12) + " dB")
                    activeColor: theme.accent
                    onValueModified: (v) => { if (engine) engine.setMasterVolume(v); }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }

    Keys.onEscapePressed: (event) => {
        event.accepted = true
        root.close()
    }

    Connections {
        target: engine
        function onMacrosChanged() {
            if (engine) {
                punchKnob.value = engine.macroPunch
                warmthKnob.value = engine.macroWarmth
                toneKnob.value = engine.macroTone
                spaceKnob.value = engine.macroSpace
                masterKnob.value = engine.masterVolume
            }
        }
        function onProjectLoaded() {
            if (engine) {
                punchKnob.value = engine.macroPunch
                warmthKnob.value = engine.macroWarmth
                toneKnob.value = engine.macroTone
                spaceKnob.value = engine.macroSpace
                masterKnob.value = engine.masterVolume
            }
        }
    }
}
