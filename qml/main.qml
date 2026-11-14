import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    StudioTheme { id: theme }

    property real meterPeakL: 0.0
    property real meterPeakR: 0.0
    property bool bottomDeckCollapsed: false
    property real bottomDeckHeight: 330

    Connections {
        target: mymsEngine
        function onMetersUpdated(pL, pR, rms) {
            rootItem.meterPeakL = pL
            rootItem.meterPeakR = pR
        }
    }

    Rectangle {
        anchors.fill: parent
        color: theme.bgWindow

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ==========================================
            // 1. TOOLBAR / ADVANCED PLAYBACK TRANSPORT
            // ==========================================
            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: theme.bgHeader
                border.color: theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    // Play / Pause Button
                    Button {
                        id: playBtn
                        implicitWidth: 72
                        implicitHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        checkable: true
                        checked: mymsEngine ? mymsEngine.isPlaying : false
                        onClicked: if (mymsEngine) mymsEngine.togglePlay()

                        background: Rectangle {
                            color: {
                                if (mymsEngine && mymsEngine.isPlaying) return theme.playAccent
                                if (mymsEngine && mymsEngine.isPaused) return theme.pauseRow
                                return playBtn.hovered ? theme.btnHover : theme.btnBg
                            }
                            radius: 2
                            border.color: {
                                if (mymsEngine && mymsEngine.isPlaying) return theme.playAccent
                                if (mymsEngine && mymsEngine.isPaused) return theme.pauseAccent
                                return theme.border
                            }
                            border.width: 1
                        }

                        contentItem: Row {
                            spacing: 4
                            anchors.centerIn: parent
                            Text {
                                text: (mymsEngine && mymsEngine.isPlaying) ? "❚❚" : "▶"
                                font.pixelSize: 10
                                color: (mymsEngine && mymsEngine.isPaused) ? theme.pauseAccent : "#ffffff"
                            }
                            Text {
                                text: {
                                    if (mymsEngine && mymsEngine.isPlaying) return "PAUSE"
                                    if (mymsEngine && mymsEngine.isPaused) return "RESUME"
                                    return "PLAY"
                                }
                                font.bold: true
                                font.pixelSize: 9
                                color: (mymsEngine && mymsEngine.isPaused) ? theme.pauseAccent : "#ffffff"
                            }
                        }
                    }

                    // Stop Button
                    Button {
                        id: stopBtn
                        implicitWidth: 58
                        implicitHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: if (mymsEngine) mymsEngine.stop()

                        background: Rectangle {
                            color: stopBtn.hovered ? theme.btnHover : theme.btnBg
                            radius: 2
                            border.color: theme.border
                            border.width: 1
                        }

                        contentItem: Row {
                            spacing: 4
                            anchors.centerIn: parent
                            Text { text: "■"; font.pixelSize: 10; color: theme.stopAccent }
                            Text { text: "STOP"; font.bold: true; font.pixelSize: 9; color: theme.textMain }
                        }
                    }

                    // Loop Toggle Button
                    Button {
                        id: loopBtn
                        implicitWidth: 58
                        implicitHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        checkable: true
                        checked: mymsEngine ? mymsEngine.isLooping : false
                        onClicked: if (mymsEngine) mymsEngine.setLooping(!mymsEngine.isLooping)

                        background: Rectangle {
                            color: loopBtn.checked ? theme.btnHover : theme.btnBg
                            radius: 2
                            border.color: loopBtn.checked ? theme.accent : theme.border
                            border.width: 1
                        }

                        contentItem: Row {
                            spacing: 3
                            anchors.centerIn: parent
                            Text { text: "🔁"; font.pixelSize: 9 }
                            Text {
                                text: "LOOP"
                                font.bold: true
                                font.pixelSize: 9
                                color: loopBtn.checked ? theme.accent : theme.textMuted
                            }
                        }
                    }

                    // Edit / Jam Mode Toggle
                    Button {
                        id: modeBtn
                        implicitWidth: 64
                        implicitHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: if (mymsEngine) mymsEngine.setEditMode(!mymsEngine.isEditMode)

                        background: Rectangle {
                            color: (mymsEngine && mymsEngine.isEditMode) ? "#4a2d32" : theme.btnBg
                            radius: 2
                            border.color: (mymsEngine && mymsEngine.isEditMode) ? theme.stopAccent : theme.border
                            border.width: 1
                        }

                        contentItem: Row {
                            spacing: 4
                            anchors.centerIn: parent
                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: (mymsEngine && mymsEngine.isEditMode) ? theme.stopAccent : theme.accent
                            }
                            Text {
                                text: (mymsEngine && mymsEngine.isEditMode) ? "EDIT" : "JAM"
                                font.bold: true
                                font.pixelSize: 9
                                color: (mymsEngine && mymsEngine.isEditMode) ? "#ffffff" : theme.textMain
                            }
                        }
                    }

                    Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

                    // Current Project Name (Blank if untitled)
                    Text {
                        text: mymsEngine ? mymsEngine.projectName : ""
                        font.pixelSize: 11
                        font.bold: true
                        color: theme.textHeading
                        Layout.alignment: Qt.AlignVCenter
                        visible: text.length > 0
                    }

                    Rectangle {
                        width: 1
                        height: 18
                        color: theme.border
                        Layout.alignment: Qt.AlignVCenter
                        visible: mymsEngine && mymsEngine.projectName.length > 0
                    }

                    // Base Octave Control (LMMS style)
                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "OCT"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                        Rectangle {
                            width: 60
                            height: 24
                            color: theme.bgCard
                            radius: 2
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0
                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: octMinusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "-"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: octMinusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: mymsEngine.setBaseOctave(mymsEngine.baseOctave - 1)
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: "" + (mymsEngine ? mymsEngine.baseOctave : 4)
                                    font.bold: true
                                    font.pixelSize: 10
                                    color: theme.accent
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: octPlusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "+"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: octPlusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: if (mymsEngine) mymsEngine.setBaseOctave(mymsEngine.baseOctave + 1)
                                    }
                                }
                            }
                        }
                    }

                    // BPM Control (Direct Editable Input)
                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "BPM"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                        Rectangle {
                            width: 80
                            height: 24
                            color: bpmInput.activeFocus ? theme.bgWell : theme.bgCard
                            radius: 2
                            border.color: bpmInput.activeFocus ? theme.accent : theme.border

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0
                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: bpmMinusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "-"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: bpmMinusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: if (mymsEngine) mymsEngine.setBpm(Math.max(20, mymsEngine.bpm - 1))
                                    }
                                }

                                TextInput {
                                    id: bpmInput
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    text: mymsEngine ? mymsEngine.bpm.toFixed(1) : "120.0"
                                    font.bold: true
                                    font.pixelSize: 10
                                    font.family: "Monospace"
                                    color: theme.accent
                                    horizontalAlignment: TextInput.AlignHCenter
                                    verticalAlignment: TextInput.AlignVCenter
                                    selectByMouse: true

                                    property bool isEditing: false

                                    onActiveFocusChanged: {
                                        if (activeFocus) {
                                            isEditing = true
                                            selectAll()
                                        } else {
                                            isEditing = false
                                            commitValue()
                                        }
                                    }

                                    onAccepted: {
                                        commitValue()
                                        focus = false
                                    }

                                    function commitValue() {
                                        let v = parseFloat(text)
                                        if (!isNaN(v) && v >= 20 && v <= 999) {
                                            if (mymsEngine) mymsEngine.setBpm(v)
                                        } else {
                                            text = mymsEngine ? mymsEngine.bpm.toFixed(1) : "120.0"
                                        }
                                    }

                                    Connections {
                                        target: mymsEngine
                                        function onBpmChanged(newBpm) {
                                            if (!bpmInput.isEditing) {
                                                bpmInput.text = newBpm.toFixed(1)
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: bpmPlusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "+"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: bpmPlusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: if (mymsEngine) mymsEngine.setBpm(Math.min(999, mymsEngine.bpm + 1))
                                    }
                                }
                            }
                        }
                    }

                    // Pattern Length (Rows)
                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "ROWS"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                        Row {
                            spacing: 1
                            Repeater {
                                model: [16, 32, 64]
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 2
                                    color: (mymsEngine && mymsEngine.numRows === modelData) ? theme.accent : theme.bgCard
                                    border.color: theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: "" + modelData
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (mymsEngine && mymsEngine.numRows === modelData) ? "#ffffff" : theme.textMuted
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: if (mymsEngine) mymsEngine.setNumRows(modelData)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

                    // Pattern Selector & Multi-Pattern Controls
                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "PAT"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                        Rectangle {
                            width: 76
                            height: 24
                            color: theme.bgCard
                            radius: 2
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                spacing: 0
                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: patMinusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "-"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: patMinusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: if (mymsEngine && mymsEngine.currentPattern > 0) mymsEngine.setCurrentPattern(mymsEngine.currentPattern - 1)
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: mymsEngine ? ((mymsEngine.currentPattern < 10 ? "0" + mymsEngine.currentPattern : "" + mymsEngine.currentPattern) + "/" + (mymsEngine.numPatterns < 10 ? "0" + mymsEngine.numPatterns : "" + mymsEngine.numPatterns)) : "00/01"
                                    font.bold: true
                                    font.pixelSize: 10
                                    font.family: "Monospace"
                                    color: theme.accent
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                Rectangle {
                                    width: 16
                                    height: parent.height
                                    color: patPlusMouse.containsMouse ? theme.btnHover : "transparent"
                                    Text { anchors.centerIn: parent; text: "+"; font.bold: true; color: theme.textMain }
                                    MouseArea {
                                        id: patPlusMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            if (mymsEngine) {
                                                if (mymsEngine.currentPattern < mymsEngine.numPatterns - 1) {
                                                    mymsEngine.setCurrentPattern(mymsEngine.currentPattern + 1)
                                                } else {
                                                    mymsEngine.addPattern()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Song Mode Toggle
                    Button {
                        id: songModeBtn
                        implicitWidth: 46
                        implicitHeight: 24
                        Layout.alignment: Qt.AlignVCenter
                        checkable: true
                        checked: mymsEngine ? mymsEngine.isSongMode : false
                        onClicked: if (mymsEngine) mymsEngine.toggleSongMode()

                        background: Rectangle {
                            color: songModeBtn.checked ? theme.accent : (songModeBtn.hovered ? theme.btnHover : theme.btnBg)
                            radius: 2
                            border.color: songModeBtn.checked ? theme.accent : theme.border
                        }

                        contentItem: Text {
                            anchors.centerIn: parent
                            text: (mymsEngine && mymsEngine.isSongMode) ? "SONG" : "PAT"
                            font.bold: true
                            font.pixelSize: 9
                            color: songModeBtn.checked ? "#ffffff" : theme.textMuted
                        }
                    }

                    Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

                    // Row & Track Badges
                    Rectangle {
                        height: 24
                        width: 84
                        Layout.alignment: Qt.AlignVCenter
                        color: rowBadgeMouse.containsMouse ? theme.btnHover : theme.bgCard
                        radius: 2
                        border.color: (mymsEngine && mymsEngine.isPaused) ? theme.pauseAccent : theme.border
                        Row {
                            anchors.centerIn: parent
                            spacing: 3
                            Text {
                                text: (mymsEngine && mymsEngine.isPaused) ? "PAUSED:" : "ROW:"
                                font.pixelSize: 9
                                font.bold: true
                                color: (mymsEngine && mymsEngine.isPaused) ? theme.pauseAccent : theme.textMuted
                            }
                            Text {
                                text: mymsEngine ? ((mymsEngine.currentRow < 10 ? "0" + mymsEngine.currentRow : "" + mymsEngine.currentRow) + "/" + mymsEngine.numRows) : "00/32"
                                font.pixelSize: 10
                                font.bold: true
                                font.family: "Monospace"
                                color: (mymsEngine && mymsEngine.isPaused) ? theme.pauseAccent : theme.playAccent
                            }
                        }

                        MouseArea {
                            id: rowBadgeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onWheel: (wheel) => {
                                if (mymsEngine) {
                                    let delta = wheel.angleDelta.y > 0 ? -1 : 1
                                    let nRows = mymsEngine.numRows
                                    let newRow = (mymsEngine.currentRow + delta + nRows) % nRows
                                    mymsEngine.setCurrentRow(newRow)
                                }
                            }
                            onClicked: {
                                if (mymsEngine) {
                                    mymsEngine.setCurrentRow(0)
                                }
                            }
                        }
                    }

                    // Keyboard Layout Switcher
                    Rectangle {
                        width: 54
                        height: 24
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: layoutMouse.containsMouse ? theme.btnHover : theme.bgCard
                        border.color: theme.border

                        Row {
                            anchors.centerIn: parent
                            spacing: 3
                            Text {
                                text: mymsEngine ? mymsEngine.keyboardLayout.toUpperCase() : "TR"
                                font.pixelSize: 9
                                font.bold: true
                                color: theme.accent
                            }
                            Text {
                                text: "⌨"
                                font.pixelSize: 9
                                color: theme.textMuted
                            }
                        }

                        MouseArea {
                            id: layoutMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: if (mymsEngine) mymsEngine.toggleKeyboardLayout()
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Master VU Meter
                    Row {
                        spacing: 4
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "OUT"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                        LevelMeter {
                            peakL: rootItem.meterPeakL
                            peakR: rootItem.meterPeakR
                        }
                    }

                    Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

                    Row {
                        spacing: 4
                        Layout.alignment: Qt.AlignVCenter
                        Rectangle { width: 6; height: 6; radius: 3; color: theme.playAccent }
                        Text {
                            text: "48kHz | <1%"
                            font.pixelSize: 9
                            font.family: "Monospace"
                            color: theme.textMuted
                        }
                    }
                }
            }

            // ==========================================
            // SONG SEQUENCE ARRANGER BAR
            // ==========================================
            SongArrangerBar {
                id: songArrangerBar
                Layout.fillWidth: true
                engine: mymsEngine
            }

            // ==========================================
            // 2. MAIN TRACKER MATRIX (Center)
            // ==========================================
            TrackerView {
                id: trackerView
                Layout.fillWidth: true
                Layout.fillHeight: true
                engine: mymsEngine
                onRequestEditInstrument: (track, trackType) => {
                    editorModal.open(track, trackType)
                }
            }

            // ==========================================
            // RESIZABLE SPLITTER HANDLE
            // ==========================================
            Rectangle {
                id: splitBar
                Layout.fillWidth: true
                height: 5
                color: splitMouse.containsMouse || splitMouse.pressed ? theme.accent : theme.border

                Rectangle {
                    anchors.centerIn: parent
                    width: 36
                    height: 2
                    radius: 1
                    color: splitMouse.containsMouse || splitMouse.pressed ? "#ffffff" : theme.borderLight
                }

                MouseArea {
                    id: splitMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitVCursor

                    property real startGlobalY: 0
                    property real startDeckH: 0

                    onPressed: (mouse) => {
                        let pt = splitBar.mapToItem(rootItem, mouse.x, mouse.y)
                        startGlobalY = pt.y
                        startDeckH = rootItem.bottomDeckHeight
                    }

                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            let pt = splitBar.mapToItem(rootItem, mouse.x, mouse.y)
                            let dy = pt.y - startGlobalY
                            let newH = startDeckH - dy
                            rootItem.bottomDeckHeight = Math.max(90, Math.min(rootItem.height - 180, newH))
                            if (rootItem.bottomDeckCollapsed && newH > 90) {
                                rootItem.bottomDeckCollapsed = false
                            }
                        }
                    }

                    onDoubleClicked: {
                        rootItem.bottomDeckCollapsed = !rootItem.bottomDeckCollapsed
                    }
                }
            }

            // ==========================================
            // 3. BOTTOM CONTROL DOCK (Resizable & Collapsible)
            // ==========================================
            Rectangle {
                id: bottomControlDock
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: rootItem.bottomDeckCollapsed ? 26 : rootItem.bottomDeckHeight
                Layout.minimumHeight: rootItem.bottomDeckCollapsed ? 26 : 90
                Layout.maximumHeight: rootItem.bottomDeckCollapsed ? 26 : (rootItem.height - 180)
                color: theme.bgCard
                border.color: theme.border
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 26
                        color: theme.bgHeader
                        border.color: theme.border
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 4

                            Rectangle {
                                width: 110
                                height: parent.height - 2
                                Layout.alignment: Qt.AlignBottom
                                color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 0) ? theme.bgCard : "transparent"
                                border.color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 0) ? theme.border : "transparent"
                                border.width: 1
                                radius: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: "🎚 TRACK MIXER"
                                    font.bold: true
                                    font.pixelSize: 10
                                    color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 0) ? theme.accent : theme.textMuted
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        rootItem.bottomDeckCollapsed = false
                                        bottomStack.currentIndex = 0
                                    }
                                }
                            }

                            Rectangle {
                                width: 120
                                height: parent.height - 2
                                Layout.alignment: Qt.AlignBottom
                                color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 1) ? theme.bgCard : "transparent"
                                border.color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 1) ? theme.border : "transparent"
                                border.width: 1
                                radius: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: "🎛 SMART MACROS"
                                    font.bold: true
                                    font.pixelSize: 10
                                    color: (!rootItem.bottomDeckCollapsed && bottomStack.currentIndex === 1) ? theme.accent : theme.textMuted
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        rootItem.bottomDeckCollapsed = false
                                        bottomStack.currentIndex = 1
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                width: 22
                                height: 20
                                radius: 2
                                color: collapseMouse.containsMouse ? theme.btnHover : "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: rootItem.bottomDeckCollapsed ? "▲" : "▼"
                                    font.pixelSize: 9
                                    color: theme.textMuted
                                }
                                MouseArea {
                                    id: collapseMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: rootItem.bottomDeckCollapsed = !rootItem.bottomDeckCollapsed
                                }
                            }
                        }
                    }

                    StackLayout {
                        id: bottomStack
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !rootItem.bottomDeckCollapsed
                        currentIndex: 0

                        MixerView {
                            id: mixerView
                            engine: mymsEngine
                            onRequestEditInstrument: (track, trackType) => {
                                editorModal.open(track, trackType)
                            }
                            onRequestEditFx: (track) => {
                                fxModal.open(track)
                            }
                        }

                        MacroView {
                            engine: mymsEngine
                        }
                    }
                }
            }

            // ==========================================
            // 4. FOOTER (Keyboard Reference & Status)
            // ==========================================
            Rectangle {
                Layout.fillWidth: true
                height: 20
                color: theme.bgHeader
                border.color: theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    Text {
                        text: (mymsEngine && mymsEngine.keyboardLayout === "tr")
                            ? ("Turkish Q: [Z..Ö] Oct " + (mymsEngine ? mymsEngine.baseOctave : 4) + "  |  [Q..P, Ğ, Ü] Oct " + ((mymsEngine ? mymsEngine.baseOctave : 4) + 1) + "  |  [ < / > ] Oct +/-  |  [Space] Play  |  [1] Note Off")
                            : ("US QWERTY: [Z..M] Oct " + (mymsEngine ? mymsEngine.baseOctave : 4) + "  |  [Q..P] Oct " + ((mymsEngine ? mymsEngine.baseOctave : 4) + 1) + "  |  [ [ / ] ] Oct +/-  |  [Space] Play  |  [1] Note Off")
                        font.pixelSize: 9
                        font.family: "Noto Sans, sans-serif"
                        color: theme.textMuted
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: ((mymsEngine && mymsEngine.keyboardLayout === "tr") ? "Turkish Q (Active)" : "US QWERTY (Active)") + " · Drag bar to resize dock"
                        font.pixelSize: 9
                        color: theme.accent
                    }
                }
            }
        }
    }

    // Pop-up Instrument Studio Editor Modal (AuraSynth / FontBox / BeatPad / Plugin)
    InstrumentEditorModal {
        id: editorModal
        objectName: "editorModal"
        engine: mymsEngine
    }

    // Pop-up Track Insert FX Chain Modal
    TrackFxModal {
        id: fxModal
        objectName: "fxModal"
        engine: mymsEngine
        onRequestEditInstrument: (track, trackType) => {
            editorModal.open(track, trackType)
        }
    }
}
