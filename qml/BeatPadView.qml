import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgCard
    radius: 4
    border.color: theme.border
    border.width: 1

    property var engine
    property int selectedPad: 0

    FileDialog {
        id: wavFileDialog
        title: "Load Sample into Pad " + (root.selectedPad + 1)
        nameFilters: ["Audio Files (*.wav *.flac *.mp3 *.ogg)", "All Files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            let path = selectedFile.toString()
            if (path.startsWith("file://")) {
                path = path.substring(7)
            }
            if (engine) engine.loadDrumSample(root.selectedPad, path)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // --- 8 MPC DRUM PADS (4x2 Grid) ---
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            RowLayout {
                spacing: 4
                Text {
                    text: "BEATPAD"
                    font.bold: true
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    color: theme.accent
                }
                Text {
                    text: "8-Slot Drum Sampler"
                    font.pixelSize: 9
                    color: theme.textMuted
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 4
                rows: 2
                rowSpacing: 6
                columnSpacing: 6

                Repeater {
                    model: 8

                    Rectangle {
                        id: padRect
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 3
                        property int padIdx: index
                        property bool isSelected: root.selectedPad === padIdx
                        property bool isFlashing: false

                        Timer {
                            id: flashTimer
                            interval: 85
                            onTriggered: padRect.isFlashing = false
                        }

                        color: {
                            if (padRect.isFlashing) return "#5cd6e8"
                            if (padRect.isSelected) return "#3a4750"
                            if (padMouse.containsMouse) return theme.btnHover
                            return theme.bgWell
                        }

                        border.color: {
                            if (padRect.isFlashing) return "#ffffff"
                            if (padRect.isSelected) return theme.accent
                            return theme.border
                        }
                        border.width: padRect.isSelected ? 1.5 : 1.0

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 1

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "P" + (padIdx + 1)
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: padRect.isSelected ? theme.accent : theme.textDim
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "[" + (padIdx + 1) + "]"
                                    font.pixelSize: 8
                                    font.family: "Monospace"
                                    color: theme.textDim
                                }
                            }

                            Item { Layout.fillHeight: true }

                            Text {
                                Layout.fillWidth: true
                                text: engine ? engine.getDrumPadName(padIdx) : "Pad " + (padIdx + 1)
                                font.pixelSize: 10
                                font.bold: true
                                color: (padRect.isFlashing || padRect.isSelected) ? "#ffffff" : theme.textMain
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            Item { Layout.fillHeight: true }
                        }

                        MouseArea {
                            id: padMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onPressed: {
                                root.selectedPad = padIdx
                                padRect.isFlashing = true
                                flashTimer.restart()
                                if (engine) {
                                    engine.triggerDrumPad(padIdx, 1.0)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: theme.border
        }

        // --- SELECTED PAD INSPECTOR ---
        ColumnLayout {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    text: "PAD " + (root.selectedPad + 1) + ":"
                    font.bold: true
                    font.pixelSize: 10
                    color: theme.accent
                }
                Text {
                    Layout.fillWidth: true
                    text: engine ? engine.getDrumPadName(root.selectedPad) : ""
                    font.pixelSize: 10
                    font.bold: true
                    color: theme.textMain
                    elide: Text.ElideRight
                }
            }

            // Load WAV Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Button {
                    id: loadWavBtn
                    Layout.fillWidth: true
                    implicitHeight: 24
                    onClicked: wavFileDialog.open()

                    background: Rectangle {
                        color: loadWavBtn.hovered ? theme.btnHover : theme.btnBg
                        radius: 2
                        border.color: theme.border
                    }

                    contentItem: Row {
                        spacing: 4
                        anchors.centerIn: parent
                        Text { text: "📂"; font.pixelSize: 9 }
                        Text { text: "LOAD SAMPLE (WAV)"; font.pixelSize: 8; font.bold: true; color: theme.textMain }
                    }
                }

                Button {
                    id: revBtn
                    implicitWidth: 42
                    implicitHeight: 24
                    property bool isRev: engine ? engine.getDrumPadReverse(root.selectedPad) : false
                    onClicked: if (engine) engine.setDrumPadReverse(root.selectedPad, !isRev)

                    background: Rectangle {
                        color: revBtn.isRev ? theme.accent : (revBtn.hovered ? theme.btnHover : theme.btnBg)
                        radius: 2
                        border.color: theme.border
                    }

                    contentItem: Text {
                        anchors.centerIn: parent
                        text: "REV"
                        font.pixelSize: 8
                        font.bold: true
                        color: revBtn.isRev ? "#ffffff" : theme.textMain
                    }
                }
            }

            // Knobs Row: PITCH, DECAY, VOL, PAN
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                BreezeKnob {
                    id: pitchKnob
                    label: "PITCH"
                    value: engine ? (engine.getDrumPadPitch(root.selectedPad) / 24.0 + 0.5) : 0.5
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: {
                        let semi = Math.round((internalValue - 0.5) * 48)
                        return (semi > 0 ? "+" : "") + semi + " st"
                    }
                    activeColor: theme.accent
                    onValueModified: (v) => {
                        let semi = (v - 0.5) * 48.0
                        if (engine) engine.setDrumPadPitch(root.selectedPad, semi)
                    }
                }

                BreezeKnob {
                    id: decayKnob
                    label: "DECAY"
                    value: engine ? (engine.getDrumPadDecay(root.selectedPad) / 2.0) : 0.25
                    minimumValue: 0.01
                    maximumValue: 1.0
                    displayValue: (internalValue * 2.0).toFixed(2) + "s"
                    activeColor: "#e67e22"
                    onValueModified: (v) => {
                        if (engine) engine.setDrumPadDecay(root.selectedPad, v * 2.0)
                    }
                }

                BreezeKnob {
                    id: volKnob
                    label: "VOL"
                    value: engine ? engine.getDrumPadVolume(root.selectedPad) : 0.9
                    minimumValue: 0.0
                    maximumValue: 1.5
                    displayValue: Math.round(internalValue * 100) + "%"
                    activeColor: theme.playAccent
                    onValueModified: (v) => {
                        if (engine) engine.setDrumPadVolume(root.selectedPad, v)
                    }
                }

                BreezeKnob {
                    id: panKnob
                    label: "PAN"
                    value: engine ? (engine.getDrumPadPan(root.selectedPad) * 0.5 + 0.5) : 0.5
                    minimumValue: 0.0
                    maximumValue: 1.0
                    displayValue: {
                        let p = Math.round((internalValue - 0.5) * 100)
                        if (p === 0) return "C"
                        return (p < 0 ? "L" + Math.abs(p) : "R" + p)
                    }
                    activeColor: theme.accent
                    onValueModified: (v) => {
                        let pan = (v - 0.5) * 2.0
                        if (engine) engine.setDrumPadPan(root.selectedPad, pan)
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    Connections {
        target: engine
        function onDrumPadsChanged() {
            if (engine) {
                pitchKnob.value = engine.getDrumPadPitch(root.selectedPad) / 24.0 + 0.5
                decayKnob.value = engine.getDrumPadDecay(root.selectedPad) / 2.0
                volKnob.value = engine.getDrumPadVolume(root.selectedPad)
                panKnob.value = engine.getDrumPadPan(root.selectedPad) * 0.5 + 0.5
            }
        }
        function onProjectLoaded() {
            root.selectedPad = 0
            if (engine) {
                pitchKnob.value = engine.getDrumPadPitch(0) / 24.0 + 0.5
                decayKnob.value = engine.getDrumPadDecay(0) / 2.0
                volKnob.value = engine.getDrumPadVolume(0)
                panKnob.value = engine.getDrumPadPan(0) * 0.5 + 0.5
            }
        }
    }
}

