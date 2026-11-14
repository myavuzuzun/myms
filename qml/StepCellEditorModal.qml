import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    StudioTheme { id: theme }

    property var engine
    property int targetTrack: 0
    property int targetRow: 0
    property bool isOpen: false

    property int cellNote: 255
    property int cellInst: 1
    property int cellVol: 255
    property int cellFxType: 0
    property int cellFxParam: 0

    property int editOctave: 4

    visible: isOpen

    function open(track, row) {
        targetTrack = track
        targetRow = row
        if (engine) {
            let c = engine.getCell(track, row)
            if (c) {
                cellNote = c.note
                cellInst = (c.instrument > 0) ? c.instrument : (track + 1)
                cellVol = c.volume
                cellFxType = c.effectType
                cellFxParam = c.effectParam

                if (cellNote <= 127) {
                    editOctave = Math.floor(cellNote / 12) - 1
                    if (editOctave < 1) editOctave = 1
                    if (editOctave > 8) editOctave = 8
                }
            }
        }
        isOpen = true
        root.forceActiveFocus()
    }

    function close() {
        isOpen = false
    }

    function apply() {
        if (engine) {
            engine.setCell(targetTrack, targetRow, cellNote, cellInst, cellVol, cellFxType, cellFxParam)
        }
        close()
    }

    function clearCell() {
        if (engine) {
            engine.setCell(targetTrack, targetRow, 255, 0, 255, 0, 0)
        }
        close()
    }

    // Modal Backdrop Scrim
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.65
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Modal Dialog Window
    Rectangle {
        id: modalDialog
        anchors.centerIn: parent
        width: 540
        height: 420
        color: theme.bgCard
        radius: 6
        border.color: theme.borderLight
        border.width: 1
        clip: false

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        // Centered Floating Close Button
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
            spacing: 10

            // Header Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "📝 STEP CELL INSPECTOR & EDITOR"
                    font.bold: true
                    font.pixelSize: 12
                    color: theme.textHeading
                }

                Rectangle {
                    width: 120
                    height: 20
                    radius: 3
                    color: theme.bgWell
                    border.color: theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "T" + (root.targetTrack + 1) + " · Row " + (root.targetRow < 10 ? "0" + root.targetRow : "" + root.targetRow)
                        font.bold: true
                        font.pixelSize: 9
                        font.family: "Monospace"
                        color: theme.accent
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "[NOTE] [INST] [VOL] [FX]"
                    font.pixelSize: 9
                    font.bold: true
                    font.family: "Monospace"
                    color: theme.textMuted
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

            // 1. NOTE SELECTOR
            Rectangle {
                Layout.fillWidth: true
                height: 72
                color: theme.bgWell
                radius: 4
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text { text: "NOTE & PITCH:"; font.bold: true; font.pixelSize: 9; color: theme.colNote }
                        Text {
                            text: {
                                if (root.cellNote === 255) return "--- (No Note)"
                                if (root.cellNote === 254) return "OFF (Note Cut / Release)"
                                const names = ["C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"]
                                let oct = Math.floor(root.cellNote / 12) - 1
                                return names[root.cellNote % 12] + oct + " (MIDI " + root.cellNote + ")"
                            }
                            font.bold: true
                            font.pixelSize: 10
                            font.family: "Monospace"
                            color: theme.textHeading
                        }

                        Item { Layout.fillWidth: true }

                        // Octave Adjuster
                        RowLayout {
                            spacing: 2
                            Text { text: "OCT:"; font.pixelSize: 9; color: theme.textMuted }
                            Rectangle {
                                width: 16; height: 18; radius: 2; color: octMinus.containsMouse ? theme.btnHover : theme.btnBg; border.color: theme.border
                                Text { anchors.centerIn: parent; text: "−"; font.bold: true; color: theme.textMain }
                                MouseArea { id: octMinus; anchors.fill: parent; hoverEnabled: true; onClicked: root.editOctave = Math.max(1, root.editOctave - 1) }
                            }
                            Text { text: "" + root.editOctave; font.bold: true; font.pixelSize: 10; color: theme.accent; Layout.alignment: Qt.AlignVCenter }
                            Rectangle {
                                width: 16; height: 18; radius: 2; color: octPlus.containsMouse ? theme.btnHover : theme.btnBg; border.color: theme.border
                                Text { anchors.centerIn: parent; text: "+"; font.bold: true; color: theme.textMain }
                                MouseArea { id: octPlus; anchors.fill: parent; hoverEnabled: true; onClicked: root.editOctave = Math.min(8, root.editOctave + 1) }
                            }
                        }
                    }

                    // 12 Semitones Buttons + OFF + CLEAR
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Repeater {
                            model: ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
                            Rectangle {
                                Layout.fillWidth: true
                                height: 26
                                radius: 2
                                property int midiVal: (root.editOctave + 1) * 12 + index
                                color: (root.cellNote === midiVal) ? theme.accent : (modelData.includes("#") ? "#252525" : theme.btnBg)
                                border.color: (root.cellNote === midiVal) ? "#ffffff" : theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.bold: true
                                    font.pixelSize: 9
                                    color: (root.cellNote === parent.midiVal) ? "#ffffff" : theme.textMain
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.cellNote = parent.midiVal
                                }
                            }
                        }

                        Rectangle {
                            width: 38
                            height: 26
                            radius: 2
                            color: root.cellNote === 254 ? theme.stopAccent : (offMouse.containsMouse ? theme.btnHover : theme.bgWellHover)
                            border.color: theme.border
                            Text { anchors.centerIn: parent; text: "OFF"; font.bold: true; font.pixelSize: 8; color: "#ffffff" }
                            MouseArea {
                                id: offMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.cellNote = 254
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Note Off (Silences voice)"
                                ToolTip.delay: 300
                            }
                        }

                        Rectangle {
                            width: 32
                            height: 26
                            radius: 2
                            color: root.cellNote === 255 ? "#444444" : (emptyMouse.containsMouse ? theme.btnHover : theme.btnBg)
                            border.color: theme.border
                            Text { anchors.centerIn: parent; text: "---"; font.bold: true; font.pixelSize: 8; color: theme.textMuted }
                            MouseArea {
                                id: emptyMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.cellNote = 255
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Empty step (No note triggered)"
                                ToolTip.delay: 300
                            }
                        }
                    }
                }
            }

            // 2. INSTRUMENT & VOLUME ROW
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Instrument ID Picker (01, 02...)
                Rectangle {
                    Layout.fillWidth: true
                    height: 52
                    color: theme.bgWell
                    radius: 4
                    border.color: theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        Column {
                            spacing: 1
                            Text { text: "INST ID (HEX):"; font.bold: true; font.pixelSize: 8; color: theme.colInst }
                            Text {
                                text: {
                                    let hex = root.cellInst.toString(16).toUpperCase()
                                    return (hex.length === 1 ? "0" + hex : hex) + " · " + (engine ? engine.getInstrumentName(root.cellInst - 1) : "")
                                }
                                font.bold: true
                                font.pixelSize: 9
                                font.family: "Monospace"
                                color: theme.textHeading
                            }
                        }

                        Item { Layout.fillWidth: true }

                        RowLayout {
                            spacing: 3
                            Button {
                                implicitWidth: 24; implicitHeight: 24
                                text: "−"
                                onClicked: root.cellInst = Math.max(1, root.cellInst - 1)
                            }
                            Rectangle {
                                width: 32; height: 24; radius: 2; color: theme.bgCard; border.color: theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: root.cellInst < 10 ? "0" + root.cellInst : "" + root.cellInst
                                    font.bold: true; font.pixelSize: 10; font.family: "Monospace"; color: theme.accent
                                }
                            }
                            Button {
                                implicitWidth: 24; implicitHeight: 24
                                text: "+"
                                onClicked: root.cellInst = Math.min(engine ? engine.numTracks : 32, root.cellInst + 1)
                            }
                        }
                    }
                }

                // Volume / Velocity (00..FF)
                Rectangle {
                    Layout.fillWidth: true
                    height: 52
                    color: theme.bgWell
                    radius: 4
                    border.color: theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "VOLUME / VELOCITY:"; font.bold: true; font.pixelSize: 8; color: theme.colVol }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: root.cellVol === 255 ? "DEFAULT (--)" : (root.cellVol.toString(16).toUpperCase() + " (" + Math.round((root.cellVol / 255) * 100) + "%)")
                                font.bold: true; font.pixelSize: 9; font.family: "Monospace"; color: theme.textHeading
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 255
                                stepSize: 1
                                value: root.cellVol === 255 ? 255 : root.cellVol
                                onMoved: root.cellVol = Math.round(value)
                            }

                            Button {
                                implicitHeight: 22
                                text: "DEFAULT"
                                font.pixelSize: 8
                                font.bold: true
                                onClicked: root.cellVol = 255
                            }
                        }
                    }
                }
            }

            // 3. EFFECT COMMAND (FX) & PARAMETER
            Rectangle {
                Layout.fillWidth: true
                height: 78
                color: theme.bgWell
                radius: 4
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "EFFECT COMMAND (FX):"; font.bold: true; font.pixelSize: 8; color: theme.colFx }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: {
                                let tHex = root.cellFxType.toString(16).toUpperCase()
                                if (tHex.length === 1) tHex = "0" + tHex
                                let pHex = root.cellFxParam.toString(16).toUpperCase()
                                if (pHex.length === 1) pHex = "0" + pHex
                                return "HEX: " + tHex + pHex + " · Value: " + root.cellFxParam
                            }
                            font.bold: true; font.pixelSize: 9; font.family: "Monospace"; color: theme.accent
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        // Effect Type Dropdown
                        ComboBox {
                            id: fxCombo
                            Layout.preferredWidth: 200
                            model: [
                                { text: "00: None (----)", type: 0 },
                                { text: "01: Pitch Slide Up", type: 1 },
                                { text: "02: Pitch Slide Down", type: 2 },
                                { text: "0C: Retrigger / Note Cut", type: 12 },
                                { text: "10: Filter Cutoff", type: 16 },
                                { text: "0E: Delay / Echo", type: 14 }
                            ]
                            textRole: "text"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].type === root.cellFxType) return i
                                }
                                return 0
                            }
                            onActivated: (index) => {
                                root.cellFxType = model[index].type
                            }
                        }

                        // Effect Param Slider
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 255
                            stepSize: 1
                            value: root.cellFxParam
                            onMoved: root.cellFxParam = Math.round(value)
                        }

                        Button {
                            implicitHeight: 24
                            text: "CLEAR FX"
                            font.pixelSize: 8
                            font.bold: true
                            onClicked: {
                                root.cellFxType = 0
                                root.cellFxParam = 0
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Action Buttons Footer
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    text: "✕ CLEAR CELL"
                    font.bold: true
                    font.pixelSize: 9
                    implicitHeight: 28
                    background: Rectangle {
                        color: parent.hovered ? "#c0392b" : theme.btnBg
                        border.color: theme.border
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.parent.hovered ? "#ffffff" : theme.stopAccent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.clearCell()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "CANCEL"
                    font.pixelSize: 9
                    implicitHeight: 28
                    onClicked: root.close()
                }

                Button {
                    text: "✓ APPLY TO STEP"
                    font.bold: true
                    font.pixelSize: 10
                    implicitHeight: 28
                    implicitWidth: 140
                    background: Rectangle {
                        color: parent.hovered ? theme.btnHover : theme.accent
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.apply()
                }
            }
        }
    }

    Keys.onEscapePressed: (event) => {
        event.accepted = true
        root.close()
    }
}
