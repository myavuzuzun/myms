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
        isOpen = true
        root.forceActiveFocus()
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
                        text: {
                            let trackName = engine ? engine.getInstrumentName(root.targetTrack) : "Track"
                            let engineName = "Polyphonic Subtractive Synth (AuraSynth)"
                            if (root.trackType === 1) engineName = "SoundFont Player (FontBox)"
                            else if (root.trackType === 2) engineName = "8-Slot Drum Sampler (BeatPad)"
                            else if (root.trackType === 3) engineName = "Plugin Instrument (VST3 / CLAP / VST2)"
                            return "EDITING TRACK #" + (root.targetTrack + 1) + ": " + trackName + " — " + engineName
                        }
                        font.bold: true
                        font.pixelSize: 12
                        color: theme.textHeading
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
                                    if (engine) engine.setTrackType(root.targetTrack, 3)
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

