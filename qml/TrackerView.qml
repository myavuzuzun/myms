import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgWindow

    property var engine
    property int selectedRow: 0
    property int selectedTrack: 0
    property int currentPlayingRow: engine ? engine.currentRow : 0

    function getTrackTitle(idx) {
        if (!engine) return "T" + (idx + 1)
        let name = engine.getInstrumentName(idx)
        return "T" + (idx + 1) + ": " + (name && name.length > 0 ? name : "Synth")
    }

    focus: true

    Connections {
        target: engine
        function onInstrumentChanged(inst) {
            root.selectedTrack = inst
        }
        function onProjectLoaded() {
            root.selectedTrack = 0
            root.gridModified()
        }
        function onNumTracksChanged() {
            root.gridModified()
        }
        function onNumRowsChanged() {
            root.gridModified()
        }
        function onGridCellChanged(track, row) {
            root.gridModified()
        }
        function onTrackTypeChanged(track, type) {
            root.gridModified()
        }
        function onCurrentPatternChanged() {
            root.gridModified()
        }
        function onNumPatternsChanged() {
            root.gridModified()
        }
    }

    function getNoteString(noteVal, trackIdx) {
        if (noteVal === 255) return "---"
        if (noteVal === 254) return "OFF"
        if (trackIdx !== undefined && engine && engine.getTrackType(trackIdx) === 2) {
            let pad = (noteVal >= 36 && noteVal <= 43) ? (noteVal - 36) : (noteVal % 8)
            const drumLabels = ["KICK", "SNAR", "CLAP", "CH-1", "OH-1", "LTOM", "RIM1", "CRSH"]
            return drumLabels[pad]
        }
        const names = ["C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"]
        let octave = Math.floor(noteVal / 12) - 1
        let noteIdx = noteVal % 12
        return names[noteIdx] + octave
    }

    function getHex2(val) {
        if (val === 255) return "--"
        let hex = val.toString(16).toUpperCase()
        return hex.length === 1 ? "0" + hex : hex
    }

    function getFxString(type, param) {
        if (type === 0 && param === 0) return "----"
        return getHex2(type) + getHex2(param)
    }

    signal gridModified()

    // 2-octave piano keyboard mapping with native Turkish Q & US support
    function getMidiFromKey(event) {
        let text = event.text
        if (!text || text.length === 0) return -1
        let isTr = (engine ? engine.keyboardLayout === "tr" : true)
        let baseOct = engine ? engine.baseOctave : 4
        let rootC = (baseOct + 1) * 12 // e.g. Oct 4 -> C-4 = 60

        if (isTr) {
            // === TURKISH Q KEYBOARD LAYOUT ===
            // Lower Octave White Keys (Bottom row)
            if (text === "z" || text === "Z") return rootC + 0
            if (text === "x" || text === "X") return rootC + 2
            if (text === "c" || text === "C") return rootC + 4
            if (text === "v" || text === "V") return rootC + 5
            if (text === "b" || text === "B") return rootC + 7
            if (text === "n" || text === "N") return rootC + 9
            if (text === "m" || text === "M") return rootC + 11
            if (text === "ö" || text === "Ö") return rootC + 12
            if (text === "ç" || text === "Ç") return rootC + 14
            if (text === ".")                 return rootC + 16

            // Lower Octave Black Keys (Home row above)
            if (text === "s" || text === "S") return rootC + 1
            if (text === "d" || text === "D") return rootC + 3
            if (text === "g" || text === "G") return rootC + 6
            if (text === "h" || text === "H") return rootC + 8
            if (text === "j" || text === "J") return rootC + 10
            if (text === "l" || text === "L") return rootC + 13
            if (text === "ş" || text === "Ş") return rootC + 15
            if (text === "i" || text === "İ") return rootC + 17
            if (text === ",")                 return rootC + 18

            // Upper Octave White Keys (Top row)
            if (text === "q" || text === "Q") return rootC + 12
            if (text === "w" || text === "W") return rootC + 14
            if (text === "e" || text === "E") return rootC + 16
            if (text === "r" || text === "R") return rootC + 17
            if (text === "t" || text === "T") return rootC + 19
            if (text === "y" || text === "Y") return rootC + 21
            if (text === "u" || text === "U") return rootC + 23
            // Key between U and O on Turkish Q generates 'ı' (dotless i) or 'I'
            if (text === "ı" || text === "I") return rootC + 24
            if (text === "o" || text === "O") return rootC + 26
            if (text === "p" || text === "P") return rootC + 28
            // Keys right of P on Turkish Q
            if (text === "ğ" || text === "Ğ") return rootC + 29
            if (text === "ü" || text === "Ü") return rootC + 31

            // Upper Octave Black Keys (Number row)
            if (text === "2") return rootC + 13
            if (text === "3") return rootC + 15
            if (text === "5") return rootC + 18
            if (text === "6") return rootC + 20
            if (text === "7") return rootC + 22
            if (text === "9") return rootC + 25
            if (text === "0") return rootC + 27
            if (text === "*" || text === "-") return rootC + 30
        } else {
            // === US QWERTY KEYBOARD LAYOUT ===
            // Lower Octave
            if (text === "z" || text === "Z") return rootC + 0
            if (text === "s" || text === "S") return rootC + 1
            if (text === "x" || text === "X") return rootC + 2
            if (text === "d" || text === "D") return rootC + 3
            if (text === "c" || text === "C") return rootC + 4
            if (text === "v" || text === "V") return rootC + 5
            if (text === "g" || text === "G") return rootC + 6
            if (text === "b" || text === "B") return rootC + 7
            if (text === "h" || text === "H") return rootC + 8
            if (text === "n" || text === "N") return rootC + 9
            if (text === "j" || text === "J") return rootC + 10
            if (text === "m" || text === "M") return rootC + 11
            if (text === ",")                 return rootC + 12
            if (text === "l" || text === "L") return rootC + 13
            if (text === ".")                 return rootC + 14
            if (text === ";")                 return rootC + 15
            if (text === "/")                 return rootC + 16
            if (text === "'")                 return rootC + 18

            // Upper Octave
            if (text === "q" || text === "Q") return rootC + 12
            if (text === "2") return rootC + 13
            if (text === "w" || text === "W") return rootC + 14
            if (text === "3") return rootC + 15
            if (text === "e" || text === "E") return rootC + 16
            if (text === "r" || text === "R") return rootC + 17
            if (text === "5") return rootC + 18
            if (text === "t" || text === "T") return rootC + 19
            if (text === "6") return rootC + 20
            if (text === "y" || text === "Y") return rootC + 21
            if (text === "7") return rootC + 22
            if (text === "u" || text === "U") return rootC + 23
            if (text === "i" || text === "I") return rootC + 24
            if (text === "9") return rootC + 25
            if (text === "o" || text === "O") return rootC + 26
            if (text === "0") return rootC + 27
            if (text === "p" || text === "P") return rootC + 28
            if (text === "[") return rootC + 29
            if (text === "=" || text === "+") return rootC + 30
            if (text === "]") return rootC + 31
        }
        return -1
    }

    Keys.onPressed: (event) => {
        let nTracks = engine ? engine.numTracks : 1
        let nRows = engine ? engine.numRows : 32

        if (event.key === Qt.Key_Space) {
            if (engine) {
                if (!engine.isPlaying) {
                    engine.setCurrentRow(root.selectedRow)
                    engine.play()
                } else {
                    engine.pause()
                }
            }
            event.accepted = true
            return
        }

        // Return / Enter: Play from start (Row 0)
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (engine) {
                engine.playFromStart()
            }
            event.accepted = true
            return
        }

        // Escape: Stop and rewind to Row 0
        if (event.key === Qt.Key_Escape) {
            if (engine) {
                engine.stop()
                root.selectedRow = 0
            }
            event.accepted = true
            return
        }

        // Octave shifting shortcuts (Supports Turkish ISO '<' / '>' as well as '[' / ']' and F1/F2)
        if (event.key === Qt.Key_BracketLeft || event.text === "<" || event.key === Qt.Key_F1) {
            if (engine) engine.setBaseOctave(engine.baseOctave - 1)
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_BracketRight || event.text === ">" || event.key === Qt.Key_F2) {
            if (engine) engine.setBaseOctave(engine.baseOctave + 1)
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Up) {
            root.selectedRow = (root.selectedRow - 1 + nRows) % nRows
            if (engine && !engine.isPlaying) {
                engine.setCurrentRow(root.selectedRow)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Down) {
            root.selectedRow = (root.selectedRow + 1) % nRows
            if (engine && !engine.isPlaying) {
                engine.setCurrentRow(root.selectedRow)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Left) {
            root.selectedTrack = (root.selectedTrack - 1 + nTracks) % nTracks
            if (engine) engine.setSelectedInstrument(root.selectedTrack)
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right || event.key === Qt.Key_Tab) {
            root.selectedTrack = (root.selectedTrack + 1) % nTracks
            if (engine) engine.setSelectedInstrument(root.selectedTrack)
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
            engine.setCell(root.selectedTrack, root.selectedRow, 255, 0, 255, 0, 0)
            root.gridModified()
            event.accepted = true
            return
        }

        // Note off: 1 or backslash
        if (event.key === Qt.Key_1 || event.key === Qt.Key_Backslash) {
            if (engine && engine.isEditMode) {
                engine.setCell(root.selectedTrack, root.selectedRow, 254, root.selectedTrack + 1, 255, 0, 0)
                root.gridModified()
                root.selectedRow = (root.selectedRow + 1) % nRows
            }
            event.accepted = true
            return
        }

        let note = getMidiFromKey(event)
        if (note >= 0) {
            let instId = root.selectedTrack + 1
            engine.noteOnInst(root.selectedTrack, note, 1.0)
            
            // If in Edit Mode, stamp into cell and advance row; if Jam mode, just preview!
            if (engine && engine.isEditMode) {
                engine.setCell(root.selectedTrack, root.selectedRow, note, instId, 255, 0, 0)
                root.gridModified()
                root.selectedRow = (root.selectedRow + 1) % nRows
            }
            event.accepted = true
            return
        }
    }

    Keys.onReleased: (event) => {
        let note = getMidiFromKey(event)
        if (note >= 0) {
            engine.noteOffInst(root.selectedTrack, note)
            event.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- TRACK HEADER ROW ---
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: theme.bgTrackHeader
            border.color: theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    width: 38
                    height: parent.height
                    Text {
                        anchors.centerIn: parent
                        text: "ROW"
                        font.pixelSize: 9
                        font.bold: true
                        color: theme.textMuted
                    }
                }

                Rectangle { width: 1; height: parent.height; color: theme.border }

                Repeater {
                    model: engine ? engine.numTracks : 1
                    Rectangle {
                        Layout.fillWidth: true
                        height: parent.height
                        color: (root.selectedTrack === index) ? theme.accent : theme.bgTrackHeader

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 4
                            spacing: 4

                            Text {
                                text: root.getTrackTitle(index)
                                font.pixelSize: 10
                                font.bold: true
                                color: (root.selectedTrack === index) ? "#ffffff" : theme.textMain
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            // Track Engine Badge (SYNTH / SF2 / DRUMS)
                            Rectangle {
                                width: 34
                                height: 14
                                radius: 2
                                property int trkType: engine ? engine.getTrackType(index) : 0
                                color: (root.selectedTrack === index) ? "rgba(0,0,0,0.3)" : theme.bgWell
                                border.color: (root.selectedTrack === index) ? "#ffffff" : theme.border

                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        let t = parent.trkType
                                        if (t === 1) return "SF2"
                                        if (t === 2) return "DRUM"
                                        return "SYN"
                                    }
                                    font.bold: true
                                    font.pixelSize: 8
                                    color: {
                                        let t = parent.trkType
                                        if (t === 1) return "#92ebb5"
                                        if (t === 2) return "#f7b731"
                                        return "#5cd6e8"
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (engine) {
                                            let nextType = (parent.trkType + 1) % 3
                                            engine.setTrackType(index, nextType)
                                            root.gridModified()
                                        }
                                    }
                                    ToolTip.visible: containsMouse
                                    ToolTip.text: "Track Engine: Click to cycle (SYNTH / SF2 / DRUMS)"
                                    ToolTip.delay: 400
                                }
                            }

                            // Delete track button
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 2
                                color: delMouse.containsMouse ? theme.stopAccent : "transparent"
                                visible: (engine && engine.numTracks > 1)
                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: (root.selectedTrack === index) ? "#ffffff" : theme.textMuted
                                }
                                MouseArea {
                                    id: delMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        let target = index
                                        if (root.selectedTrack >= engine.numTracks - 1) {
                                            root.selectedTrack = Math.max(0, engine.numTracks - 2)
                                        }
                                        engine.removeTrack(target)
                                        root.gridModified()
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: {
                                root.selectedTrack = index
                                if (engine) engine.setSelectedInstrument(index)
                                root.forceActiveFocus()
                            }
                        }

                        Rectangle {
                            anchors.right: parent.right
                            width: 1
                            height: parent.height
                            color: theme.border
                        }
                    }
                }

                // Add Track Button
                Rectangle {
                    width: 28
                    height: parent.height
                    color: addTrackMouse.containsMouse ? theme.btnHover : theme.btnBg
                    Text {
                        anchors.centerIn: parent
                        text: "+"
                        font.bold: true
                        font.pixelSize: 14
                        color: theme.accent
                    }
                    MouseArea {
                        id: addTrackMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (engine) {
                                engine.addTrack()
                                root.selectedTrack = engine.numTracks - 1
                                engine.setSelectedInstrument(root.selectedTrack)
                                root.gridModified()
                            }
                        }
                    }
                }
            }
        }

        // --- TRACKER PATTERN MATRIX (Compact 20px rows) ---
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: listView
                width: scrollView.width
                model: engine ? engine.numRows : 32
                boundsBehavior: Flickable.StopAtBounds

                Connections {
                    target: engine
                    function onCurrentRowChanged() {
                        if (engine.isPlaying || engine.isPaused) {
                            listView.positionViewAtIndex(engine.currentRow, ListView.Center)
                        }
                    }
                    function onPausedChanged() {
                        if (engine.isPaused) {
                            listView.positionViewAtIndex(engine.currentRow, ListView.Center)
                        }
                    }
                }

                delegate: Rectangle {
                    id: rowDelegate
                    width: listView.width
                    height: 20
                    property int rowIndex: index

                    color: {
                        if (root.engine && root.engine.isPlaying && root.engine.currentRow === index) {
                            return theme.playRow
                        }
                        if (root.engine && root.engine.isPaused && root.engine.currentRow === index) {
                            return theme.pauseRow
                        }
                        if (index === root.selectedRow) {
                            return theme.cursorRow
                        }
                        if (index % 16 === 0) {
                            return theme.bgRowBar
                        }
                        if (index % 4 === 0) {
                            return theme.bgRowBeat
                        }
                        return (index % 2 === 0) ? theme.bgWindow : theme.bgRowAlt
                    }

                    // Paused Row Border Highlight
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: theme.pauseAccent
                        border.width: 1
                        visible: (root.engine && root.engine.isPaused && root.engine.currentRow === index)
                        z: 10
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            width: 38
                            height: parent.height

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    if (root.engine && root.engine.isPaused && root.engine.currentRow === rowIndex) {
                                        return "❚❚" + (rowIndex < 10 ? "0" + rowIndex : "" + rowIndex)
                                    }
                                    return (rowIndex < 10 ? "0" + rowIndex : "" + rowIndex)
                                }
                                font.pixelSize: (root.engine && root.engine.isPaused && root.engine.currentRow === rowIndex) ? 9 : 10
                                font.bold: (rowIndex % 4 === 0 || (root.engine && root.engine.isPaused && root.engine.currentRow === rowIndex))
                                font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                color: {
                                    if (root.engine && root.engine.isPlaying && root.engine.currentRow === rowIndex) {
                                        return "#ffffff"
                                    }
                                    if (root.engine && root.engine.isPaused && root.engine.currentRow === rowIndex) {
                                        return theme.pauseAccent
                                    }
                                    return (rowIndex % 4 === 0) ? theme.textMain : theme.textDim
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.selectedRow = rowIndex
                                    if (engine) engine.setCurrentRow(rowIndex)
                                    root.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    root.selectedRow = rowIndex
                                    if (engine) {
                                        engine.setCurrentRow(rowIndex)
                                        engine.play()
                                    }
                                    root.forceActiveFocus()
                                }
                            }
                        }

                        Rectangle { width: 1; height: parent.height; color: theme.border }

                        Repeater {
                            model: engine ? engine.numTracks : 1
                            Item {
                                id: cellItem
                                Layout.fillWidth: true
                                height: parent.height
                                property int trackIdx: index
                                property var cellData: engine ? engine.getCell(trackIdx, rowIndex) : null

                                Connections {
                                    target: root
                                    function onGridModified() {
                                        if (engine) cellItem.cellData = engine.getCell(cellItem.trackIdx, rowIndex)
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    color: (root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex) ? theme.accent : "transparent"
                                    opacity: 0.25
                                }

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5

                                    Text {
                                        text: root.getNoteString(cellItem.cellData ? cellItem.cellData.note : 255, cellItem.trackIdx)
                                        font.pixelSize: 10
                                        font.bold: (cellItem.cellData && cellItem.cellData.note <= 127)
                                        font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                        color: {
                                            if (!cellItem.cellData || cellItem.cellData.note === 255) return theme.textDim
                                            if (cellItem.cellData.note === 254) return theme.stopAccent
                                            return theme.colNote
                                        }
                                    }

                                    Text {
                                        text: (cellItem.cellData && cellItem.cellData.note <= 127) ? root.getHex2(cellItem.cellData.instrument) : "--"
                                        font.pixelSize: 10
                                        font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                        color: (cellItem.cellData && cellItem.cellData.note <= 127) ? theme.colInst : theme.textDim
                                    }

                                    Text {
                                        text: (cellItem.cellData && cellItem.cellData.note <= 127 && cellItem.cellData.volume !== 255) ? root.getHex2(cellItem.cellData.volume) : "--"
                                        font.pixelSize: 10
                                        font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                        color: (cellItem.cellData && cellItem.cellData.note <= 127 && cellItem.cellData.volume !== 255) ? theme.colVol : theme.textDim
                                    }

                                    Text {
                                        text: cellItem.cellData ? root.getFxString(cellItem.cellData.effectType, cellItem.cellData.effectParam) : "----"
                                        font.pixelSize: 10
                                        font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                        color: (cellItem.cellData && cellItem.cellData.effectType !== 0) ? theme.colFx : theme.textDim
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.selectedTrack = cellItem.trackIdx
                                        root.selectedRow = rowIndex
                                        if (engine) {
                                            engine.setSelectedInstrument(cellItem.trackIdx)
                                            if (!engine.isPlaying) {
                                                engine.setCurrentRow(rowIndex)
                                            }
                                        }
                                        root.forceActiveFocus()
                                    }
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    width: 1
                                    height: parent.height
                                    color: theme.border
                                }
                            }
                        }

                        Item {
                            width: 28
                            height: parent.height
                        }
                    }
                }
            }
        }
    }
}
