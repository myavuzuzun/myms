import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "trackerView"
    StudioTheme { id: theme }

    color: theme.bgWindow

    property var engine
    property int selectedRow: 0
    property int selectedTrack: 0
    property int selectedColumn: 0 // 0: Note, 1: Inst, 2: Vol, 3: FX
    property var cellEditorModal: null
    property int currentPlayingRow: engine ? engine.currentRow : 0

    property int trackColumnWidth: 160
    property int rowHeaderWidth: 38
    property int trackHeaderHeight: 28

    // ==========================================
    // Chunk Selection State & Helpers
    // ==========================================
    property bool hasSelection: false
    property int selStartTrack: -1
    property int selStartRow: -1
    property int selEndTrack: -1
    property int selEndRow: -1
    property bool isSelecting: false

    function minSelectedTrack() {
        return Math.min(selStartTrack, selEndTrack)
    }
    function maxSelectedTrack() {
        return Math.max(selStartTrack, selEndTrack)
    }
    function minSelectedRow() {
        return Math.min(selStartRow, selEndRow)
    }
    function maxSelectedRow() {
        return Math.max(selStartRow, selEndRow)
    }
    function isCellSelected(track, row) {
        if (!hasSelection) return false
        return (track >= minSelectedTrack() && track <= maxSelectedTrack() &&
                row >= minSelectedRow() && row <= maxSelectedRow())
    }
    function clearSelection() {
        hasSelection = false
        selStartTrack = -1
        selStartRow = -1
        selEndTrack = -1
        selEndRow = -1
        isSelecting = false
    }
    function selectAll() {
        let nTracks = engine ? engine.numTracks : 1
        let nRows = engine ? engine.numRows : 32
        selStartTrack = 0
        selStartRow = 0
        selEndTrack = nTracks - 1
        selEndRow = nRows - 1
        hasSelection = true
    }

    function copySelection() {
        if (!engine) return
        if (hasSelection) {
            engine.copyRange(minSelectedTrack(), minSelectedRow(), maxSelectedTrack(), maxSelectedRow())
        } else {
            engine.copyRange(selectedTrack, selectedRow, selectedTrack, selectedRow)
        }
    }

    function cutSelection() {
        if (!engine) return
        if (hasSelection) {
            engine.cutRange(minSelectedTrack(), minSelectedRow(), maxSelectedTrack(), maxSelectedRow())
            hasSelection = false
            gridModified()
        } else {
            engine.cutRange(selectedTrack, selectedRow, selectedTrack, selectedRow)
            gridModified()
        }
    }

    function pasteClipboard() {
        if (!engine) return
        engine.pasteClipboard(selectedTrack, selectedRow)
        gridModified()
    }

    function deleteSelection() {
        if (!engine) return
        if (hasSelection) {
            engine.deleteRange(minSelectedTrack(), minSelectedRow(), maxSelectedTrack(), maxSelectedRow())
            hasSelection = false
            gridModified()
        } else {
            let cell = engine.getCell(selectedTrack, selectedRow)
            if (cell) {
                if (selectedColumn === 0) {
                    engine.setCell(selectedTrack, selectedRow, 255, cell.instrument, cell.volume, cell.effectType, cell.effectParam)
                } else if (selectedColumn === 1) {
                    engine.setCell(selectedTrack, selectedRow, cell.note, selectedTrack + 1, cell.volume, cell.effectType, cell.effectParam)
                } else if (selectedColumn === 2) {
                    engine.setCell(selectedTrack, selectedRow, cell.note, cell.instrument, 255, cell.effectType, cell.effectParam)
                } else if (selectedColumn === 3) {
                    engine.setCell(selectedTrack, selectedRow, cell.note, cell.instrument, cell.volume, 0, 0)
                }
                gridModified()
            }
        }
    }

    function ensureTrackVisible(trackIdx) {
        if (typeof matrixListView === "undefined" || !matrixListView) return
        let trackX = trackIdx * root.trackColumnWidth
        let trackRight = trackX + root.trackColumnWidth
        let viewLeft = matrixListView.contentX
        let viewRight = viewLeft + matrixListView.width

        if (trackX < viewLeft) {
            matrixListView.contentX = Math.max(0, trackX)
        } else if (trackRight > viewRight) {
            matrixListView.contentX = Math.min(Math.max(0, matrixListView.contentWidth - matrixListView.width), trackRight - matrixListView.width)
        }
    }

    property int nameRefreshCounter: 0

    function getTrackTitle(idx) {
        nameRefreshCounter;
        if (!engine) return "T" + (idx + 1)
        let name = engine.getInstrumentName(idx)
        return "T" + (idx + 1) + ": " + (name && name.length > 0 ? name : "Synth")
    }

    focus: true

    Connections {
        target: engine
        function onInstrumentChanged(inst) {
            root.selectedTrack = inst
            root.ensureTrackVisible(inst)
        }
        function onInstrumentNameChanged(inst, name) {
            root.nameRefreshCounter++
        }
        function onProjectLoaded() {
            root.clearSelection()
            root.selectedTrack = 0
            root.gridModified()
            root.ensureTrackVisible(0)
        }
        function onNumTracksChanged() {
            root.clearSelection()
            root.gridModified()
            if (engine && root.selectedTrack >= engine.numTracks) {
                root.selectedTrack = Math.max(0, engine.numTracks - 1)
            }
            root.ensureTrackVisible(root.selectedTrack)
        }
        function onNumRowsChanged() {
            root.clearSelection()
            if (engine && root.selectedRow >= engine.numRows) {
                root.selectedRow = Math.max(0, engine.numRows - 1)
            }
            root.gridModified()
        }
        function onGridCellChanged(track, row) {
            root.gridModified()
        }
        function onTrackTypeChanged(track, type) {
            root.gridModified()
        }
        function onCurrentPatternChanged() {
            root.clearSelection()
            if (engine && root.selectedRow >= engine.numRows) {
                root.selectedRow = Math.max(0, engine.numRows - 1)
            }
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
    signal requestEditInstrument(int track, int trackType)
    signal requestEditCell(int track, int row)

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

        if (cellEditorModal && cellEditorModal.isOpen) {
            if (event.key === Qt.Key_Escape) {
                cellEditorModal.close()
                event.accepted = true
                return
            }
            return
        }

        // Escape: If chunk selection active, clear selection; else stop & rewind
        if (event.key === Qt.Key_Escape) {
            if (root.hasSelection) {
                root.clearSelection()
                event.accepted = true
                return
            }
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

        // Undo / Redo shortcuts (Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y)
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_Z)) {
            if (event.modifiers & Qt.ShiftModifier) {
                if (engine && engine.redo()) root.gridModified()
            } else {
                if (engine && engine.undo()) root.gridModified()
            }
            event.accepted = true
            return
        }
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_Y)) {
            if (engine && engine.redo()) root.gridModified()
            event.accepted = true
            return
        }

        // Chunk Selection & Clipboard shortcuts (Ctrl+C, Ctrl+X, Ctrl+V, Ctrl+A)
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_C)) {
            root.copySelection()
            event.accepted = true
            return
        }
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_X)) {
            root.cutSelection()
            event.accepted = true
            return
        }
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_V)) {
            root.pasteClipboard()
            event.accepted = true
            return
        }
        if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_A)) {
            root.selectAll()
            event.accepted = true
            return
        }

        // Shift + Arrow Keys: Expand rectangular chunk selection
        if (event.modifiers & Qt.ShiftModifier) {
            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down ||
                event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
                if (!root.hasSelection) {
                    root.selStartTrack = root.selectedTrack
                    root.selStartRow = root.selectedRow
                    root.selEndTrack = root.selectedTrack
                    root.selEndRow = root.selectedRow
                    root.hasSelection = true
                }
                if (event.key === Qt.Key_Up) {
                    root.selEndRow = Math.max(0, root.selEndRow - 1)
                    root.selectedRow = root.selEndRow
                    if (typeof matrixListView !== "undefined" && matrixListView) {
                        matrixListView.positionViewAtIndex(root.selectedRow, ListView.Contain)
                    }
                } else if (event.key === Qt.Key_Down) {
                    root.selEndRow = Math.min(nRows - 1, root.selEndRow + 1)
                    root.selectedRow = root.selEndRow
                    if (typeof matrixListView !== "undefined" && matrixListView) {
                        matrixListView.positionViewAtIndex(root.selectedRow, ListView.Contain)
                    }
                } else if (event.key === Qt.Key_Left) {
                    root.selEndTrack = Math.max(0, root.selEndTrack - 1)
                    root.selectedTrack = root.selEndTrack
                    root.ensureTrackVisible(root.selectedTrack)
                } else if (event.key === Qt.Key_Right) {
                    root.selEndTrack = Math.min(nTracks - 1, root.selEndTrack + 1)
                    root.selectedTrack = root.selEndTrack
                    root.ensureTrackVisible(root.selectedTrack)
                }
                event.accepted = true
                return
            }
        }

        if (event.key === Qt.Key_Up) {
            if (root.hasSelection) root.hasSelection = false
            root.selectedRow = (root.selectedRow - 1 + nRows) % nRows
            if (engine && !engine.isPlaying) {
                engine.setCurrentRow(root.selectedRow)
            }
            if (typeof matrixListView !== "undefined" && matrixListView) {
                matrixListView.positionViewAtIndex(root.selectedRow, ListView.Contain)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Down) {
            if (root.hasSelection) root.hasSelection = false
            root.selectedRow = (root.selectedRow + 1) % nRows
            if (engine && !engine.isPlaying) {
                engine.setCurrentRow(root.selectedRow)
            }
            if (typeof matrixListView !== "undefined" && matrixListView) {
                matrixListView.positionViewAtIndex(root.selectedRow, ListView.Contain)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Left) {
            if (root.hasSelection) root.hasSelection = false
            if (root.selectedColumn > 0) {
                root.selectedColumn--
            } else {
                root.selectedColumn = 3
                root.selectedTrack = (root.selectedTrack - 1 + nTracks) % nTracks
                if (engine) engine.setSelectedInstrument(root.selectedTrack)
                root.ensureTrackVisible(root.selectedTrack)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right || event.key === Qt.Key_Tab) {
            if (root.hasSelection) root.hasSelection = false
            if (root.selectedColumn < 3) {
                root.selectedColumn++
            } else {
                root.selectedColumn = 0
                root.selectedTrack = (root.selectedTrack + 1) % nTracks
                if (engine) engine.setSelectedInstrument(root.selectedTrack)
                root.ensureTrackVisible(root.selectedTrack)
            }
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
            if (root.hasSelection) {
                root.deleteSelection()
                event.accepted = true
                return
            }
            if (engine) {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                if (cell) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        engine.setCell(root.selectedTrack, root.selectedRow, 255, root.selectedTrack + 1, 255, 0, 0)
                    } else if (root.selectedColumn === 0) {
                        engine.setCell(root.selectedTrack, root.selectedRow, 255, cell.instrument, cell.volume, cell.effectType, cell.effectParam)
                    } else if (root.selectedColumn === 1) {
                        engine.setCell(root.selectedTrack, root.selectedRow, cell.note, root.selectedTrack + 1, cell.volume, cell.effectType, cell.effectParam)
                    } else if (root.selectedColumn === 2) {
                        engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, 255, cell.effectType, cell.effectParam)
                    } else if (root.selectedColumn === 3) {
                        engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, cell.volume, 0, 0)
                    }
                    root.gridModified()
                }
            }
            event.accepted = true
            return
        }

        // Instrument Column Typing (01..NN)
        if (root.selectedColumn === 1 && engine) {
            let digit = parseInt(event.text)
            if (!isNaN(digit)) {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let newInst = Math.max(1, Math.min(nTracks, digit === 0 ? 10 : digit))
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, newInst, cell.volume, cell.effectType, cell.effectParam)
                root.gridModified()
                if (engine.isEditMode) root.selectedRow = (root.selectedRow + 1) % nRows
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Plus || event.text === "+") {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let newInst = Math.min(nTracks, cell.instrument + 1)
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, newInst, cell.volume, cell.effectType, cell.effectParam)
                root.gridModified()
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Minus || event.text === "-") {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let newInst = Math.max(1, cell.instrument - 1)
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, newInst, cell.volume, cell.effectType, cell.effectParam)
                root.gridModified()
                event.accepted = true
                return
            }
        }

        // Volume Column Typing (00..FF hex)
        if (root.selectedColumn === 2 && engine) {
            let ch = event.text.toUpperCase()
            if ((ch >= "0" && ch <= "9") || (ch >= "A" && ch <= "F")) {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let v = parseInt(ch, 16)
                let newVol = v * 17
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, newVol, cell.effectType, cell.effectParam)
                root.gridModified()
                if (engine.isEditMode) root.selectedRow = (root.selectedRow + 1) % nRows
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Plus || event.text === "+") {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let cur = cell.volume === 255 ? 255 : cell.volume
                let newVol = Math.min(255, cur + 16)
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, newVol, cell.effectType, cell.effectParam)
                root.gridModified()
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Minus || event.text === "-") {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let cur = cell.volume === 255 ? 255 : cell.volume
                let newVol = Math.max(0, cur - 16)
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, newVol, cell.effectType, cell.effectParam)
                root.gridModified()
                event.accepted = true
                return
            }
        }

        // FX Column Typing (Effect Command / Param)
        if (root.selectedColumn === 3 && engine) {
            let ch = event.text.toUpperCase()
            if ((ch >= "0" && ch <= "9") || (ch >= "A" && ch <= "F")) {
                let cell = engine.getCell(root.selectedTrack, root.selectedRow)
                let v = parseInt(ch, 16)
                let newParam = (v * 16) + (cell.effectParam % 16)
                engine.setCell(root.selectedTrack, root.selectedRow, cell.note, cell.instrument, cell.volume, cell.effectType === 0 ? 16 : cell.effectType, newParam)
                root.gridModified()
                if (engine.isEditMode) root.selectedRow = (root.selectedRow + 1) % nRows
                event.accepted = true
                return
            }
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

        if (root.selectedColumn === 0) {
            let note = getMidiFromKey(event)
            if (note >= 0) {
                let instId = root.selectedTrack + 1
                engine.noteOnInst(root.selectedTrack, note, 1.0)
                
                // If in Edit Mode, stamp into cell and advance row; if Jam mode, just preview!
                if (engine && engine.isEditMode) {
                    let curCell = engine.getCell(root.selectedTrack, root.selectedRow)
                    let vol = curCell ? curCell.volume : 255
                    let fxT = curCell ? curCell.effectType : 0
                    let fxP = curCell ? curCell.effectParam : 0
                    engine.setCell(root.selectedTrack, root.selectedRow, note, instId, vol, fxT, fxP)
                    root.gridModified()
                    root.selectedRow = (root.selectedRow + 1) % nRows
                    if (typeof matrixListView !== "undefined" && matrixListView) {
                        matrixListView.positionViewAtIndex(root.selectedRow, ListView.Contain)
                    }
                }
                event.accepted = true
                return
            }
        }
    }

    Keys.onReleased: (event) => {
        let note = getMidiFromKey(event)
        if (note >= 0) {
            engine.noteOffInst(root.selectedTrack, note)
            event.accepted = true
        }
    }

    Item {
        id: mainTrackerArea
        anchors.fill: parent

        // 1. Pinned Top-Left Corner: Pattern Badge "P00"
        Rectangle {
            id: cornerBox
            x: 0
            y: 0
            width: root.rowHeaderWidth
            height: root.trackHeaderHeight
            color: theme.bgTrackHeader
            border.color: theme.border
            border.width: 1
            z: 20

            Text {
                anchors.centerIn: parent
                text: "P" + (engine ? (engine.currentPattern < 10 ? "0" + engine.currentPattern : "" + engine.currentPattern) : "00")
                font.pixelSize: 10
                font.bold: true
                font.family: "Monospace"
                color: theme.accent
            }
        }

        // 2. Pinned Top Bar: Track Headers Viewport (scrolls horizontally with matrixListView.contentX)
        Item {
            id: headerViewport
            x: root.rowHeaderWidth
            y: 0
            width: parent.width - root.rowHeaderWidth
            height: root.trackHeaderHeight
            clip: true
            z: 15

            WheelHandler {
                target: null
                onWheel: (event) => {
                    let delta = event.angleDelta.x !== 0 ? event.angleDelta.x : event.angleDelta.y
                    let newX = matrixListView.contentX - delta
                    matrixListView.contentX = Math.max(0, Math.min(matrixListView.contentWidth - matrixListView.width, newX))
                }
            }

            Rectangle {
                anchors.fill: parent
                color: theme.bgTrackHeader
                border.color: theme.border
                border.width: 1
            }

            Item {
                id: headerContent
                x: -matrixListView.contentX
                y: 0
                width: matrixListView.contentWidth
                height: parent.height

                Row {
                    spacing: 0
                    height: parent.height

                    Repeater {
                        model: engine ? engine.numTracks : 1
                        Rectangle {
                            width: root.trackColumnWidth
                            height: root.trackHeaderHeight
                            color: (root.selectedTrack === index) ? theme.accent : theme.bgTrackHeader

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 4
                                spacing: 3

                                // Move track left
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 2
                                    Layout.alignment: Qt.AlignVCenter
                                    color: moveLeftMouse.containsMouse ? "rgba(255,255,255,0.2)" : "transparent"
                                    visible: index > 0
                                    Text {
                                        anchors.centerIn: parent
                                        text: "◀"
                                        font.pixelSize: 8
                                        color: (root.selectedTrack === index) ? "#ffffff" : theme.textMuted
                                    }
                                    MouseArea {
                                        id: moveLeftMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            if (engine) {
                                                engine.moveTrack(index, index - 1)
                                                root.selectedTrack = index - 1
                                                root.gridModified()
                                            }
                                        }
                                        ToolTip.visible: containsMouse
                                        ToolTip.text: "Move Track Left"
                                        ToolTip.delay: 300
                                    }
                                }

                                // Move track right
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 2
                                    Layout.alignment: Qt.AlignVCenter
                                    color: moveRightMouse.containsMouse ? "rgba(255,255,255,0.2)" : "transparent"
                                    visible: engine && (index < engine.numTracks - 1)
                                    Text {
                                        anchors.centerIn: parent
                                        text: "▶"
                                        font.pixelSize: 8
                                        color: (root.selectedTrack === index) ? "#ffffff" : theme.textMuted
                                    }
                                    MouseArea {
                                        id: moveRightMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            if (engine) {
                                                engine.moveTrack(index, index + 1)
                                                root.selectedTrack = index + 1
                                                root.gridModified()
                                            }
                                        }
                                        ToolTip.visible: containsMouse
                                        ToolTip.text: "Move Track Right"
                                        ToolTip.delay: 300
                                    }
                                }

                                TextInput {
                                    id: trkHeaderInput
                                    text: root.getTrackTitle(index)
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: (root.selectedTrack === index) ? "#ffffff" : theme.textMain
                                    clip: true
                                    selectByMouse: true
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    onActiveFocusChanged: {
                                        if (activeFocus) root.selectedTrack = index
                                    }
                                    onEditingFinished: {
                                        if (engine && text.trim().length > 0) {
                                            let cleanName = text.trim()
                                            if (cleanName.startsWith("T" + (index + 1) + ": ")) {
                                                cleanName = cleanName.substring(("T" + (index + 1) + ": ").length)
                                            }
                                            engine.setInstrumentName(index, cleanName)
                                        }
                                    }
                                }

                                // Delete track button
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 2
                                    Layout.alignment: Qt.AlignVCenter
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
                                    root.clearSelection()
                                    root.selectedTrack = index
                                    if (engine) engine.setSelectedInstrument(index)
                                    root.ensureTrackVisible(index)
                                    root.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    root.clearSelection()
                                    root.selectedTrack = index
                                    if (engine) {
                                        engine.setSelectedInstrument(index)
                                        let trkType = engine.getTrackType(index)
                                        root.requestEditInstrument(index, trkType)
                                    }
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
                        height: root.trackHeaderHeight
                        color: addTrackMouse.containsMouse ? theme.btnHover : theme.btnBg
                        border.color: theme.border
                        border.width: 1
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
                                    root.ensureTrackVisible(root.selectedTrack)
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Pinned Left Column: Row Numbers Viewport (scrolls vertically with matrixListView.contentY)
        Item {
            id: rowNumViewport
            x: 0
            y: root.trackHeaderHeight
            width: root.rowHeaderWidth
            height: parent.height - root.trackHeaderHeight
            clip: true
            z: 10

            WheelHandler {
                target: null
                onWheel: (event) => {
                    let newY = matrixListView.contentY - event.angleDelta.y
                    matrixListView.contentY = Math.max(0, Math.min(matrixListView.contentHeight - matrixListView.height, newY))
                }
            }

            Item {
                id: rowNumContent
                x: 0
                y: -matrixListView.contentY
                width: parent.width
                height: matrixListView.contentHeight

                Column {
                    spacing: 0
                    width: parent.width

                    Repeater {
                        model: engine ? engine.numRows : 32
                        Rectangle {
                            width: root.rowHeaderWidth
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
                                visible: Boolean(root.engine && root.engine.isPaused && root.engine.currentRow === index)
                                z: 10
                            }

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

                            Rectangle {
                                anchors.right: parent.right
                                width: 1
                                height: parent.height
                                color: theme.border
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.clearSelection()
                                    root.selectedRow = rowIndex
                                    if (engine) engine.setCurrentRow(rowIndex)
                                    root.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    root.clearSelection()
                                    root.selectedRow = rowIndex
                                    if (engine) {
                                        engine.setCurrentRow(rowIndex)
                                        engine.play()
                                    }
                                    root.forceActiveFocus()
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4. Main Pattern Matrix Grid (scrolls both horizontally and vertically)
        Item {
            id: matrixArea
            x: root.rowHeaderWidth
            y: root.trackHeaderHeight
            width: parent.width - root.rowHeaderWidth
            height: parent.height - root.trackHeaderHeight
            clip: true

            ListView {
                id: matrixListView
                anchors.fill: parent
                contentWidth: (engine ? engine.numTracks : 1) * root.trackColumnWidth + 28
                flickableDirection: Flickable.HorizontalAndVerticalFlick
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                ScrollBar.horizontal: ScrollBar {
                    id: hScrollBar
                    policy: ScrollBar.AsNeeded
                }
                ScrollBar.vertical: ScrollBar {
                    id: vScrollBar
                    policy: ScrollBar.AsNeeded
                }

                Connections {
                    target: engine
                    function onCurrentRowChanged() {
                        if (engine.isPlaying || engine.isPaused) {
                            matrixListView.positionViewAtIndex(engine.currentRow, ListView.Center)
                        }
                    }
                    function onPausedChanged() {
                        if (engine.isPaused) {
                            matrixListView.positionViewAtIndex(engine.currentRow, ListView.Center)
                        }
                    }
                }

                model: engine ? engine.numRows : 32

                delegate: Rectangle {
                    id: rowDelegate
                    width: matrixListView.contentWidth
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
                        visible: Boolean(root.engine && root.engine.isPaused && root.engine.currentRow === index)
                        z: 10
                    }

                    Row {
                        spacing: 0
                        height: parent.height

                        Repeater {
                            model: engine ? engine.numTracks : 1
                            Item {
                                id: cellItem
                                width: root.trackColumnWidth
                                height: 20
                                property int trackIdx: index
                                property int activePattern: engine ? engine.currentPattern : 0
                                property var cellData: (engine && activePattern >= 0) ? engine.getCell(trackIdx, rowIndex) : null

                                onActivePatternChanged: {
                                    if (engine) cellItem.cellData = engine.getCell(cellItem.trackIdx, rowIndex)
                                }

                                Connections {
                                    target: root
                                    function onGridModified() {
                                        if (engine) cellItem.cellData = engine.getCell(cellItem.trackIdx, rowIndex)
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    visible: (!root.hasSelection && root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex)
                                    color: theme.accent
                                    opacity: 0.25
                                    z: 1
                                }

                                // Selection Chunk Highlight
                                Rectangle {
                                    anchors.fill: parent
                                    visible: root.isCellSelected(cellItem.trackIdx, rowIndex)
                                    color: theme.accent
                                    opacity: 0.35
                                    z: 3

                                    // Selection border outlines
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 1
                                        color: "#ffffff"
                                        opacity: 0.85
                                        visible: rowIndex === root.minSelectedRow()
                                    }
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 1
                                        color: "#ffffff"
                                        opacity: 0.85
                                        visible: rowIndex === root.maxSelectedRow()
                                    }
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        width: 1
                                        color: "#ffffff"
                                        opacity: 0.85
                                        visible: cellItem.trackIdx === root.minSelectedTrack()
                                    }
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        anchors.right: parent.right
                                        width: 1
                                        color: "#ffffff"
                                        opacity: 0.85
                                        visible: cellItem.trackIdx === root.maxSelectedTrack()
                                    }
                                }

                                Row {
                                    id: colsRow
                                    anchors.centerIn: parent
                                    spacing: 4
                                    z: 5

                                    // Col 0: Note
                                    Rectangle {
                                        id: col0Box
                                        width: noteText.implicitWidth + 4
                                        height: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        radius: 2
                                        property bool isColActive: (!root.hasSelection && root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex && root.selectedColumn === 0)
                                        color: isColActive ? "#3daee9" : "transparent"
                                        opacity: isColActive ? 0.35 : 1.0

                                        Text {
                                            id: noteText
                                            anchors.centerIn: parent
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
                                    }

                                    // Col 1: Inst
                                    Rectangle {
                                        id: col1Box
                                        width: instText.implicitWidth + 4
                                        height: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        radius: 2
                                        property bool isColActive: (!root.hasSelection && root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex && root.selectedColumn === 1)
                                        color: isColActive ? "#3daee9" : "transparent"
                                        opacity: isColActive ? 0.35 : 1.0

                                        Text {
                                            id: instText
                                            anchors.centerIn: parent
                                            text: (cellItem.cellData && cellItem.cellData.note <= 127) ? root.getHex2(cellItem.cellData.instrument) : "--"
                                            font.pixelSize: 10
                                            font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                            color: (cellItem.cellData && cellItem.cellData.note <= 127) ? theme.colInst : theme.textDim
                                        }
                                    }

                                    // Col 2: Vol
                                    Rectangle {
                                        id: col2Box
                                        width: volText.implicitWidth + 4
                                        height: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        radius: 2
                                        property bool isColActive: (!root.hasSelection && root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex && root.selectedColumn === 2)
                                        color: isColActive ? "#3daee9" : "transparent"
                                        opacity: isColActive ? 0.35 : 1.0

                                        Text {
                                            id: volText
                                            anchors.centerIn: parent
                                            text: (cellItem.cellData && cellItem.cellData.note <= 127 && cellItem.cellData.volume !== 255) ? root.getHex2(cellItem.cellData.volume) : "--"
                                            font.pixelSize: 10
                                            font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                            color: (cellItem.cellData && cellItem.cellData.note <= 127 && cellItem.cellData.volume !== 255) ? theme.colVol : theme.textDim
                                        }
                                    }

                                    // Col 3: FX
                                    Rectangle {
                                        id: col3Box
                                        width: fxText.implicitWidth + 4
                                        height: 16
                                        anchors.verticalCenter: parent.verticalCenter
                                        radius: 2
                                        property bool isColActive: (!root.hasSelection && root.selectedTrack === cellItem.trackIdx && root.selectedRow === rowIndex && root.selectedColumn === 3)
                                        color: isColActive ? "#3daee9" : "transparent"
                                        opacity: isColActive ? 0.35 : 1.0

                                        Text {
                                            id: fxText
                                            anchors.centerIn: parent
                                            text: cellItem.cellData ? root.getFxString(cellItem.cellData.effectType, cellItem.cellData.effectParam) : "----"
                                            font.pixelSize: 10
                                            font.family: "Monospace, JetBrains Mono, Hack, Noto Sans Mono"
                                            color: (cellItem.cellData && cellItem.cellData.effectType !== 0) ? theme.colFx : theme.textDim
                                        }
                                    }
                                }

                                MouseArea {
                                    id: cellMouseArea
                                    anchors.fill: parent
                                    z: 10
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    preventStealing: false

                                    property real pressX: 0
                                    property real pressY: 0
                                    property bool isCtrlSelect: false
                                    property bool dragStarted: false

                                    onPressed: (mouse) => {
                                        if (mouse.button === Qt.RightButton) {
                                            if (!root.isCellSelected(cellItem.trackIdx, rowIndex)) {
                                                root.hasSelection = false
                                                root.selectedTrack = cellItem.trackIdx
                                                root.selectedRow = rowIndex
                                                if (engine) {
                                                    engine.setSelectedInstrument(cellItem.trackIdx)
                                                    if (!engine.isPlaying) engine.setCurrentRow(rowIndex)
                                                }
                                            }
                                            root.ensureTrackVisible(cellItem.trackIdx)
                                            root.forceActiveFocus()
                                            trackerContextMenu.popup()
                                            return
                                        }

                                        pressX = mouse.x
                                        pressY = mouse.y
                                        dragStarted = false

                                        let hasCtrl = Boolean(mouse.modifiers & Qt.ControlModifier)
                                        let hasShift = Boolean(mouse.modifiers & Qt.ShiftModifier)
                                        isCtrlSelect = hasCtrl || hasShift

                                        if (isCtrlSelect) {
                                            cellMouseArea.preventStealing = true
                                            matrixListView.interactive = false
                                            root.isSelecting = true
                                            root.hasSelection = true

                                            if (hasShift) {
                                                if (!root.hasSelection) {
                                                    root.selStartTrack = root.selectedTrack
                                                    root.selStartRow = root.selectedRow
                                                }
                                                root.selEndTrack = cellItem.trackIdx
                                                root.selEndRow = rowIndex
                                                root.selectedTrack = cellItem.trackIdx
                                                root.selectedRow = rowIndex
                                            } else {
                                                root.selStartTrack = cellItem.trackIdx
                                                root.selStartRow = rowIndex
                                                root.selEndTrack = cellItem.trackIdx
                                                root.selEndRow = rowIndex
                                                root.selectedTrack = cellItem.trackIdx
                                                root.selectedRow = rowIndex
                                            }
                                        } else {
                                            cellMouseArea.preventStealing = false
                                            matrixListView.interactive = true
                                        }
                                    }

                                    onPositionChanged: (mouse) => {
                                        if (!(mouse.buttons & Qt.LeftButton)) return
                                        if (!isCtrlSelect) return // Without Ctrl: let Flickable scroll the rows smoothly!

                                        let dx = mouse.x - pressX
                                        let dy = mouse.y - pressY
                                        if (!dragStarted && (Math.abs(dx) > 3 || Math.abs(dy) > 3)) {
                                            dragStarted = true
                                            root.isSelecting = true
                                            root.hasSelection = true
                                        }

                                        if (dragStarted) {
                                            let ptToContent = cellItem.mapToItem(matrixListView.contentItem, mouse.x, mouse.y)
                                            let nTracks = engine ? engine.numTracks : 1
                                            let nRows = engine ? engine.numRows : 32
                                            let curTrack = Math.max(0, Math.min(nTracks - 1, Math.floor(ptToContent.x / root.trackColumnWidth)))
                                            let curRow = Math.max(0, Math.min(nRows - 1, Math.floor(ptToContent.y / 20)))

                                            root.selEndTrack = curTrack
                                            root.selEndRow = curRow
                                            root.selectedTrack = curTrack
                                            root.selectedRow = curRow
                                        }
                                    }

                                    onReleased: (mouse) => {
                                        if (mouse.button !== Qt.LeftButton) return
                                        if (isCtrlSelect) {
                                            root.isSelecting = false
                                            isCtrlSelect = false
                                            cellMouseArea.preventStealing = false
                                            matrixListView.interactive = true
                                        } else {
                                            root.hasSelection = false
                                            root.selectedTrack = cellItem.trackIdx
                                            root.selectedRow = rowIndex
                                            if (engine) {
                                                engine.setSelectedInstrument(cellItem.trackIdx)
                                                if (!engine.isPlaying) engine.setCurrentRow(rowIndex)
                                            }
                                            let p0 = col0Box.mapFromItem(cellItem, mouse.x, mouse.y)
                                            let p1 = col1Box.mapFromItem(cellItem, mouse.x, mouse.y)
                                            let p2 = col2Box.mapFromItem(cellItem, mouse.x, mouse.y)
                                            let p3 = col3Box.mapFromItem(cellItem, mouse.x, mouse.y)
                                            if (p0.x >= 0 && p0.x <= col0Box.width && p0.y >= 0 && p0.y <= col0Box.height) {
                                                root.selectedColumn = 0
                                            } else if (p1.x >= 0 && p1.x <= col1Box.width && p1.y >= 0 && p1.y <= col1Box.height) {
                                                root.selectedColumn = 1
                                            } else if (p2.x >= 0 && p2.x <= col2Box.width && p2.y >= 0 && p2.y <= col2Box.height) {
                                                root.selectedColumn = 2
                                            } else if (p3.x >= 0 && p3.x <= col3Box.width && p3.y >= 0 && p3.y <= col3Box.height) {
                                                root.selectedColumn = 3
                                            }
                                            root.ensureTrackVisible(cellItem.trackIdx)
                                            root.forceActiveFocus()
                                        }
                                    }

                                    onDoubleClicked: (mouse) => {
                                        if (mouse.button === Qt.LeftButton) {
                                            root.hasSelection = false
                                            if (root.cellEditorModal) root.cellEditorModal.open(cellItem.trackIdx, rowIndex)
                                            else root.requestEditCell(cellItem.trackIdx, rowIndex)
                                        }
                                    }

                                    onCanceled: () => {
                                        dragStarted = false
                                        root.isSelecting = false
                                        isCtrlSelect = false
                                        cellMouseArea.preventStealing = false
                                        matrixListView.interactive = true
                                    }
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    width: 1
                                    height: parent.height
                                    color: theme.border
                                    z: 2
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

    // ==========================================
    // Context Menu for Tracker Matrix Grid
    // ==========================================
    Menu {
        id: trackerContextMenu
        MenuItem {
            text: "Cu&t Chunk\tCtrl+X"
            onTriggered: root.cutSelection()
        }
        MenuItem {
            text: "&Copy Chunk\tCtrl+C"
            onTriggered: root.copySelection()
        }
        MenuItem {
            text: "&Paste Chunk\tCtrl+V"
            enabled: engine && engine.hasClipboardData
            onTriggered: root.pasteClipboard()
        }
        MenuItem {
            text: "&Delete Chunk\tDel"
            onTriggered: root.deleteSelection()
        }
        MenuSeparator {}
        MenuItem {
            text: "Select &All Pattern\tCtrl+A"
            onTriggered: root.selectAll()
        }
        MenuItem {
            text: "Deselect\tEsc"
            visible: root.hasSelection
            onTriggered: root.clearSelection()
        }
        MenuSeparator {}
        MenuItem {
            text: "Edit Cell Details..."
            onTriggered: {
                if (root.cellEditorModal) root.cellEditorModal.open(root.selectedTrack, root.selectedRow)
                else root.requestEditCell(root.selectedTrack, root.selectedRow)
            }
        }
        MenuItem {
            text: "Edit Instrument / Synth..."
            onTriggered: {
                if (engine) {
                    let trkType = engine.getTrackType(root.selectedTrack)
                    root.requestEditInstrument(root.selectedTrack, trkType)
                }
            }
        }
    }
}
