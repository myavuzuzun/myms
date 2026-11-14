import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent
    visible: isOpen
    z: 9999

    StudioTheme { id: theme }

    property var engine
    property bool isOpen: false
    property int targetTrack: 0
    property int trackType: 0 // 0 = Synth, 1 = SoundFont, 2 = Drums

    function open(track, type) {
        targetTrack = track
        trackType = type
        if (engine) engine.setSelectedInstrument(track)
        if (modalTrackNameInput && engine) {
            modalTrackNameInput.text = engine.getInstrumentName(track)
        }
        isOpen = true
        root.forceActiveFocus()
    }

    Connections {
        target: engine
        function onInstrumentNameChanged(inst, name) {
            if (inst === root.targetTrack && modalTrackNameInput) {
                modalTrackNameInput.text = name
            }
        }
    }

    function close() {
        isOpen = false
    }

    // Modal Dark Backdrop (Scrim)
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.70

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Modal Card Window
    Rectangle {
        id: modalDialog
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 880)
        height: Math.min(parent.height - 40, 520)
        color: theme.bgCard
        radius: 6
        border.color: theme.borderLight
        border.width: 1
        clip: true

        // Intercept clicks so backdrop doesn't close on clicking inside the dialog
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header Title Bar
            Rectangle {
                Layout.fillWidth: true
                height: 38
                color: theme.bgHeader
                border.color: theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: {
                            if (root.trackType === 1) return "🎻"
                            if (root.trackType === 2) return "🥁"
                            if (root.trackType === 3) return "🔌"
                            return "🎹"
                        }
                        font.pixelSize: 14
                    }

                    Text {
                        text: "TRACK #" + (root.targetTrack + 1) + ":"
                        font.bold: true
                        font.pixelSize: 12
                        color: theme.textHeading
                    }

                    Rectangle {
                        width: 140
                        height: 24
                        radius: 3
                        color: theme.bgWell
                        border.color: modalTrackNameInput.activeFocus ? theme.accent : theme.border

                        TextInput {
                            id: modalTrackNameInput
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            verticalAlignment: TextInput.AlignVCenter
                            text: engine ? engine.getInstrumentName(root.targetTrack) : "Track"
                            font.bold: true
                            font.pixelSize: 11
                            color: theme.textHeading
                            selectByMouse: true
                            clip: true
                            onEditingFinished: {
                                if (engine && text.trim().length > 0) {
                                    engine.setInstrumentName(root.targetTrack, text.trim())
                                }
                            }
                        }
                    }

                    Text {
                        text: {
                            if (root.trackType === 1) return "— SoundFont Player (FontBox)"
                            if (root.trackType === 2) return "— 8-Slot Drum Sampler (BeatPad)"
                            if (root.trackType === 3) return "— Plugin Instrument (VST3 / CLAP / VST2)"
                            return "— Polyphonic Subtractive Synth (AuraSynth)"
                        }
                        font.pixelSize: 11
                        color: theme.textMuted
                    }

                    Item { Layout.fillWidth: true }

                    // Engine Quick Switch Tabs
                    Row {
                        spacing: 2
                        Rectangle {
                            width: 68
                            height: 24
                            radius: 3
                            color: root.trackType === 0 ? theme.accent : theme.btnBg
                            border.color: theme.border
                            Text {
                                anchors.centerIn: parent
                                text: "SYNTH"
                                font.bold: true
                                font.pixelSize: 9
                                color: root.trackType === 0 ? "#ffffff" : theme.textMuted
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.trackType = 0
                                    if (engine) engine.setTrackType(root.targetTrack, 0)
                                }
                            }
                        }

                        Rectangle {
                            width: 82
                            height: 24
                            radius: 3
                            color: root.trackType === 1 ? "#27ae60" : theme.btnBg
                            border.color: theme.border
                            Text {
                                anchors.centerIn: parent
                                text: "SOUNDFONT"
                                font.bold: true
                                font.pixelSize: 9
                                color: root.trackType === 1 ? "#ffffff" : theme.textMuted
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.trackType = 1
                                    if (engine) engine.setTrackType(root.targetTrack, 1)
                                }
                            }
                        }

                        Rectangle {
                            width: 72
                            height: 24
                            radius: 3
                            color: root.trackType === 2 ? "#f7b731" : theme.btnBg
                            border.color: theme.border
                            Text {
                                anchors.centerIn: parent
                                text: "BEATPAD"
                                font.bold: true
                                font.pixelSize: 9
                                color: root.trackType === 2 ? "#ffffff" : theme.textMuted
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.trackType = 2
                                    if (engine) engine.setTrackType(root.targetTrack, 2)
                                }
                            }
                        }

                        Rectangle {
                            width: 72
                            height: 24
                            radius: 3
                            color: root.trackType === 3 ? "#3daee9" : theme.btnBg
                            border.color: theme.border
                            Text {
                                anchors.centerIn: parent
                                text: "PLUGIN"
                                font.bold: true
                                font.pixelSize: 9
                                color: root.trackType === 3 ? "#ffffff" : theme.textMuted
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.trackType = 3
                                    if (engine) {
                                        engine.setTrackType(root.targetTrack, 3)
                                        if (!engine.isTrackPluginLoaded(root.targetTrack)) {
                                            engine.promptOpenPlugin(root.targetTrack)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle { width: 1; height: 16; color: theme.border }

                    // Close / Done Button
                    Button {
                        implicitHeight: 26
                        implicitWidth: 80
                        text: "✓ DONE"
                        font.bold: true
                        font.pixelSize: 10
                        background: Rectangle {
                            color: parent.hovered ? theme.playAccent : theme.btnBg
                            border.color: theme.border
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.parent.hovered ? "#ffffff" : theme.textMain
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.close()
                    }
                }
            }

            // Editor Content Area (StackLayout according to trackType)
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: Math.min(3, root.trackType)

                AuraSynthView {
                    engine: root.engine
                    currentInstrument: root.targetTrack
                }

                FontBoxView {
                    engine: root.engine
                    currentTrack: root.targetTrack
                }

                BeatPadView {
                    engine: root.engine
                }

                PluginView {
                    engine: root.engine
                    currentTrack: root.targetTrack
                }
            }
        }
    }

    // Keyboard Shortcuts inside Modal: Escape closes
    Keys.onEscapePressed: {
        root.close()
    }
}

