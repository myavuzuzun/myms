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
    property string searchQuery: ""
    property int currentTrack: engine ? engine.selectedInstrument : 0
    property int currentTrackPreset: engine ? engine.getTrackSf2Preset(root.currentTrack) : 0

    Connections {
        target: engine
        function onTrackMixerChanged(t) {
            if (t === root.currentTrack || t === -1) {
                root.currentTrackPreset = engine ? engine.getTrackSf2Preset(root.currentTrack) : 0
            }
        }
        function onInstrumentChanged(t) {
            root.currentTrack = t
            root.currentTrackPreset = engine ? engine.getTrackSf2Preset(root.currentTrack) : 0
        }
        function onProjectLoaded() {
            root.currentTrackPreset = engine ? engine.getTrackSf2Preset(root.currentTrack) : 0
        }
    }

    onCurrentTrackChanged: {
        root.currentTrackPreset = engine ? engine.getTrackSf2Preset(root.currentTrack) : 0
    }

    FileDialog {
        id: sf2FileDialog
        title: "Load SoundFont2 Bank (.sf2)"
        nameFilters: ["SoundFont Files (*.sf2)", "All Files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            let path = selectedFile.toString()
            if (path.startsWith("file://")) {
                path = path.substring(7)
            }
            if (engine) engine.loadSoundFont(path)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // Left Panel: SoundFont Bank Info & Master Controls
        ColumnLayout {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            spacing: 6

            RowLayout {
                spacing: 4
                Text {
                    text: "FONTBOX"
                    font.bold: true
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    color: theme.accent
                }
                Text {
                    text: "SF2 Engine"
                    font.pixelSize: 9
                    color: theme.textMuted
                }
            }

            // Loaded Path Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                color: theme.bgWell
                radius: 2
                border.color: theme.border

                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    Text {
                        text: "ACTIVE SOUNDFONT BANK"
                        font.pixelSize: 8
                        font.bold: true
                        color: theme.textDim
                    }
                    Text {
                        width: parent.width
                        text: {
                            if (!engine || !engine.sf2Loaded) return "No SoundFont Loaded"
                            let p = engine.sf2Path
                            let slash = p.lastIndexOf("/")
                            return (slash >= 0) ? p.substring(slash + 1) : p
                        }
                        font.pixelSize: 10
                        font.bold: true
                        color: (engine && engine.sf2Loaded) ? theme.textMain : theme.stopAccent
                        elide: Text.ElideMiddle
                    }
                }
            }

            // Target Track & Active Preset Indicator
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: theme.bgWell
                radius: 2
                border.color: theme.accent
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    Text {
                        text: "TARGET TRACK #" + (root.currentTrack + 1) + " (" + (engine ? engine.getInstrumentName(root.currentTrack) : "Track") + ")"
                        font.pixelSize: 8
                        font.bold: true
                        color: theme.accent
                    }
                    Text {
                        width: parent.width
                        text: {
                            if (!engine || !engine.sf2Loaded) return "No SF2 Loaded"
                            let name = engine.getSoundFontPresetName(root.currentTrackPreset)
                            return (root.currentTrackPreset < 10 ? "00" + root.currentTrackPreset : (root.currentTrackPreset < 100 ? "0" + root.currentTrackPreset : "" + root.currentTrackPreset)) + ": " + (name ? name : "Preset")
                        }
                        font.pixelSize: 10
                        font.bold: true
                        color: theme.textMain
                        elide: Text.ElideRight
                    }
                }
            }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Button {
                    id: loadSf2Btn
                    Layout.fillWidth: true
                    implicitHeight: 26
                    onClicked: sf2FileDialog.open()

                    background: Rectangle {
                        color: loadSf2Btn.hovered ? theme.btnHover : theme.btnBg
                        radius: 2
                        border.color: theme.border
                    }

                    contentItem: Row {
                        spacing: 4
                        anchors.centerIn: parent
                        Text { text: "📂"; font.pixelSize: 9 }
                        Text { text: "LOAD .SF2"; font.pixelSize: 9; font.bold: true; color: theme.textMain }
                    }
                }
            }

            // Volume Knob
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                BreezeKnob {
                    label: "SF2 VOL"
                    value: engine ? engine.sf2Volume : 0.85
                    minimumValue: 0.0
                    maximumValue: 1.5
                    displayValue: Math.round(internalValue * 100) + "%"
                    activeColor: theme.accent
                    onValueModified: (v) => { if (engine) engine.setSoundFontVolume(v); }
                }
            }

            Item { Layout.fillHeight: true }
        }

        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: theme.border
        }

        // Center / Right Panel: Searchable Preset Browser
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            // Preset Search Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "PRESETS (" + (engine ? engine.sf2PresetCount : 0) + "):"
                    font.pixelSize: 9
                    font.bold: true
                    color: theme.textMuted
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 24
                    color: theme.bgWell
                    radius: 2
                    border.color: searchInput.activeFocus ? theme.accent : theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 3
                        spacing: 4
                        Text { text: "🔍"; font.pixelSize: 9; color: theme.textDim }
                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            font.pixelSize: 10
                            color: theme.textMain
                            selectByMouse: true
                            onTextChanged: root.searchQuery = text.toLowerCase()
                        }
                    }
                }
            }

            // Presets Grid / List View
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: theme.bgWell
                radius: 2
                border.color: theme.border
                clip: true

                GridView {
                    id: presetGrid
                    anchors.fill: parent
                    anchors.margins: 4
                    cellWidth: 155
                    cellHeight: 26
                    boundsBehavior: Flickable.StopAtBounds

                    model: engine ? engine.sf2PresetCount : 0

                    delegate: Rectangle {
                        width: presetGrid.cellWidth - 4
                        height: 22
                        radius: 2
                        property string presetName: engine ? engine.getSoundFontPresetName(index) : ""
                        property bool matchesSearch: root.searchQuery === "" || presetName.toLowerCase().indexOf(root.searchQuery) !== -1
                        visible: matchesSearch

                        property bool isTrackPreset: (root.currentTrackPreset === index)

                        color: {
                            if (isTrackPreset) return theme.accent
                            if (mouseArea.containsMouse) return theme.btnHover
                            return theme.bgCard
                        }
                        border.color: isTrackPreset ? theme.accent : theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 4

                            Text {
                                text: "" + index
                                font.pixelSize: 8
                                font.family: "Monospace"
                                color: isTrackPreset ? "#ffffff" : theme.textDim
                            }

                            Text {
                                Layout.fillWidth: true
                                text: presetName
                                font.pixelSize: 9
                                font.bold: isTrackPreset
                                color: isTrackPreset ? "#ffffff" : theme.textMain
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (engine) {
                                    engine.setTrackSf2Preset(root.currentTrack, index)
                                    root.currentTrackPreset = index
                                    // Audition note C-4 (60) on target track
                                    engine.noteOnInst(root.currentTrack, 60, 0.9)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
