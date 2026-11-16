import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgHeader
    height: 34
    border.color: theme.border
    border.width: 1

    property var engine

    // Drag-and-drop state tracking across the arranger timeline
    property int draggingFromIndex: -1
    property int dropTargetIndex: -1
    property bool isDragging: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        // ==========================================
        // 1. ARRANGER LABEL & PLAYBACK MODE TOGGLE
        // ==========================================
        RowLayout {
            spacing: 6
            Layout.alignment: Qt.AlignVCenter

            Text {
                text: "ARRANGER"
                font.bold: true
                font.pixelSize: 10
                font.letterSpacing: 0.5
                color: theme.textMuted
                Layout.alignment: Qt.AlignVCenter
            }

            // Segmented Playback Mode Control: [ PAT | SONG ]
            Rectangle {
                id: modeSegment
                height: 22
                width: 120
                radius: 3
                color: theme.bgWell
                border.color: theme.border
                border.width: 1
                Layout.alignment: Qt.AlignVCenter

                Row {
                    anchors.fill: parent

                    // Segment 1: PAT (Pattern Mode)
                    Rectangle {
                        id: patSegment
                        width: parent.width / 2
                        height: parent.height
                        radius: 2
                        color: (!engine || !engine.isSongMode) ? theme.accent : "transparent"

                        Row {
                            anchors.centerIn: parent
                            spacing: 3
                            Text {
                                text: "PAT"
                                font.bold: true
                                font.pixelSize: 10
                                color: (!engine || !engine.isSongMode) ? "#ffffff" : theme.textMuted
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (engine && engine.isSongMode) engine.toggleSongMode()
                            }
                            ToolTip.visible: containsMouse
                            ToolTip.text: "Pattern Mode (PAT)\nPlayback loops the active pattern being edited.\nIdeal for composition and sound design."
                            ToolTip.delay: 300
                        }
                    }

                    // Segment 2: SONG (Song Sequence Mode)
                    Rectangle {
                        id: songSegment
                        width: parent.width / 2
                        height: parent.height
                        radius: 2
                        color: (engine && engine.isSongMode) ? theme.playAccent : "transparent"

                        Row {
                            anchors.centerIn: parent
                            spacing: 4
                            Rectangle {
                                width: 5
                                height: 5
                                radius: 2.5
                                color: (engine && engine.isSongMode) ? "#ffffff" : theme.textDim
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "SONG"
                                font.bold: true
                                font.pixelSize: 10
                                color: (engine && engine.isSongMode) ? "#ffffff" : theme.textMuted
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (engine && !engine.isSongMode) engine.toggleSongMode()
                            }
                            ToolTip.visible: containsMouse
                            ToolTip.text: "Song Mode (SONG)\nPlayback plays the entire song sequence step-by-step.\nIdeal for arranging and full-track playback."
                            ToolTip.delay: 300
                        }
                    }
                }
            }
        }

        // Vertical Separator
        Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

        // ==========================================
        // 2. HORIZONTAL STEP SEQUENCE TIMELINE
        // ==========================================
        ListView {
            id: stepListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.horizontal: ScrollBar {
                id: hScrollBar
                policy: ScrollBar.AsNeeded
                height: 3
                anchors.bottom: parent.bottom
            }

            // Auto-scroll to playing step during song playback
            Connections {
                target: engine
                function onSongOrderIndexChanged(idx) {
                    if (engine && engine.isSongMode && stepListView) {
                        stepListView.positionViewAtIndex(idx, ListView.Contain)
                    }
                }
            }

            model: engine ? engine.songOrderLength : 1

            // Delegate: Single Step Card
            delegate: Item {
                id: delegateRoot
                width: 90
                height: 24
                anchors.verticalCenter: parent.verticalCenter

                property int stepIdx: index
                property int patNum: engine ? engine.getSongOrderStep(stepIdx) : 0
                property bool isPlayingStep: (engine && engine.isSongMode && engine.isPlaying && engine.songOrderIndex === stepIdx)
                property bool isSelectedStep: (engine && engine.songOrderIndex === stepIdx)
                property bool isBeingDragged: root.isDragging && root.draggingFromIndex === stepIdx
                property bool isDropTarget: root.isDragging && root.dropTargetIndex === stepIdx && root.draggingFromIndex !== stepIdx

                // Drop Indicator Marker (Left edge)
                Rectangle {
                    visible: delegateRoot.isDropTarget && root.dropTargetIndex <= root.draggingFromIndex
                    width: 3
                    height: parent.height
                    x: -4
                    color: theme.accent
                    radius: 1
                    z: 50
                }

                // Drop Indicator Marker (Right edge)
                Rectangle {
                    visible: delegateRoot.isDropTarget && root.dropTargetIndex > root.draggingFromIndex
                    width: 3
                    height: parent.height
                    x: parent.width + 1
                    color: theme.accent
                    radius: 1
                    z: 50
                }

                // Main Card Container (Rock-solid layout: no jumping/shifting on hover)
                Rectangle {
                    id: cardVisual
                    anchors.fill: parent
                    radius: 3
                    opacity: delegateRoot.isBeingDragged ? 0.35 : 1.0

                    color: delegateRoot.isPlayingStep ? "#1e3a29" : (delegateRoot.isSelectedStep ? theme.bgWellHover : (cardMouseArea.containsMouse ? theme.btnHover : theme.bgCard))
                    border.color: delegateRoot.isPlayingStep ? theme.playAccent : (delegateRoot.isSelectedStep ? theme.accent : (cardMouseArea.containsMouse ? theme.borderLight : theme.border))
                    border.width: (delegateRoot.isPlayingStep || delegateRoot.isSelectedStep) ? 1.5 : 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 3
                        anchors.rightMargin: 3
                        spacing: 2

                        // Step Sequence Index Badge (#01, #02...)
                        Rectangle {
                            width: delegateRoot.isPlayingStep ? 26 : 22
                            height: 18
                            radius: 2
                            color: delegateRoot.isPlayingStep ? theme.playAccent : theme.bgWell
                            Layout.alignment: Qt.AlignVCenter

                            Row {
                                anchors.centerIn: parent
                                spacing: 1
                                Text {
                                    visible: delegateRoot.isPlayingStep
                                    text: "▶"
                                    font.pixelSize: 7
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: (delegateRoot.stepIdx < 9 ? "0" : "") + (delegateRoot.stepIdx + 1)
                                    font.pixelSize: 9
                                    font.bold: true
                                    font.family: "Monospace"
                                    color: delegateRoot.isPlayingStep ? "#ffffff" : (delegateRoot.isSelectedStep ? theme.accent : theme.textDim)
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }

                        // Pattern Identifier Badge (PAT 00)
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            text: "PAT " + (delegateRoot.patNum < 10 ? "0" + delegateRoot.patNum : "" + delegateRoot.patNum)
                            font.bold: true
                            font.pixelSize: 10
                            font.family: "Monospace"
                            color: (delegateRoot.isPlayingStep || delegateRoot.isSelectedStep) ? "#ffffff" : theme.textMain
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideNone
                        }

                        // Pattern Selector Dropdown Arrow (▾)
                        Rectangle {
                            id: dropdownBtn
                            width: 14
                            height: 18
                            radius: 2
                            color: dropMouse.containsMouse ? theme.btnHover : "transparent"
                            Layout.alignment: Qt.AlignVCenter

                            Text {
                                anchors.centerIn: parent
                                text: "▾"
                                font.pixelSize: 8
                                color: dropMouse.containsMouse ? theme.accent : theme.textMuted
                            }

                            MouseArea {
                                id: dropMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (engine) {
                                        engine.setSongOrderIndex(delegateRoot.stepIdx)
                                        engine.setCurrentPattern(delegateRoot.patNum)
                                    }
                                    patMenu.popup()
                                }
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Change pattern for this step"
                                ToolTip.delay: 200
                            }
                        }
                    }

                    // Hover Delete Button (Overlay in top-right corner, zero layout shift)
                    Rectangle {
                        id: hoverDelBtn
                        width: 13
                        height: 13
                        radius: 6.5
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: -3
                        anchors.rightMargin: -3
                        color: delMouse.containsMouse ? theme.stopAccent : theme.bgWell
                        border.color: delMouse.containsMouse ? theme.stopAccent : theme.border
                        border.width: 1
                        z: 60
                        visible: (cardMouseArea.containsMouse || delMouse.containsMouse) && engine && (engine.songOrderLength > 1) && !root.isDragging

                        Text {
                            anchors.centerIn: parent
                            text: "×"
                            font.pixelSize: 9
                            font.bold: true
                            color: delMouse.containsMouse ? "#ffffff" : theme.textMuted
                        }

                        MouseArea {
                            id: delMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (engine) engine.removeSongOrderStep(delegateRoot.stepIdx)
                            }
                            ToolTip.visible: containsMouse
                            ToolTip.text: "Remove step from song"
                            ToolTip.delay: 200
                        }
                    }

                    // Pattern Selection Menu
                    Menu {
                        id: patMenu

                        Repeater {
                            model: engine ? engine.numPatterns : 1
                            MenuItem {
                                text: "Pattern " + (index < 10 ? "0" + index : "" + index) + (index === delegateRoot.patNum ? "  ✓" : "")
                                font.bold: index === delegateRoot.patNum
                                onTriggered: {
                                    if (engine) {
                                        engine.setSongOrderStep(delegateRoot.stepIdx, index)
                                        engine.setSongOrderIndex(delegateRoot.stepIdx)
                                        engine.setCurrentPattern(index)
                                    }
                                }
                            }
                        }

                        MenuSeparator {}

                        MenuItem {
                            text: "+ New Blank Pattern"
                            onTriggered: {
                                if (engine) {
                                    let newPat = engine.addPattern()
                                    engine.setSongOrderStep(delegateRoot.stepIdx, newPat)
                                    engine.setSongOrderIndex(delegateRoot.stepIdx)
                                    engine.setCurrentPattern(newPat)
                                }
                            }
                        }

                        MenuItem {
                            text: "⧉ Clone to New Pattern"
                            onTriggered: {
                                if (engine) {
                                    let newPat = engine.duplicatePattern(delegateRoot.patNum)
                                    engine.setSongOrderStep(delegateRoot.stepIdx, newPat)
                                    engine.setSongOrderIndex(delegateRoot.stepIdx)
                                    engine.setCurrentPattern(newPat)
                                }
                            }
                        }
                    }

                    // Right-Click Context Menu on Step Card
                    Menu {
                        id: cardContextMenu

                        MenuItem {
                            text: "▶ Play from Step " + (delegateRoot.stepIdx + 1)
                            onTriggered: {
                                if (engine) {
                                    engine.setSongOrderIndex(delegateRoot.stepIdx)
                                    engine.setCurrentPattern(delegateRoot.patNum)
                                    if (!engine.isSongMode) engine.toggleSongMode()
                                    if (!engine.isPlaying) engine.play()
                                }
                            }
                        }

                        MenuSeparator {}

                        MenuItem {
                            text: "+ Insert Step Before"
                            onTriggered: {
                                if (engine) engine.insertSongOrderStep(delegateRoot.stepIdx, delegateRoot.patNum)
                            }
                        }

                        MenuItem {
                            text: "+ Insert Step After"
                            onTriggered: {
                                if (engine) engine.insertSongOrderStep(delegateRoot.stepIdx + 1, delegateRoot.patNum)
                            }
                        }

                        MenuItem {
                            text: "⧉ Duplicate Step (Same Pattern)"
                            onTriggered: {
                                if (engine) engine.insertSongOrderStep(delegateRoot.stepIdx + 1, delegateRoot.patNum)
                            }
                        }

                        MenuItem {
                            text: "✨ Duplicate Pattern to New Step"
                            onTriggered: {
                                if (engine) engine.duplicatePatternStep(delegateRoot.stepIdx)
                            }
                        }

                        MenuItem {
                            text: "📄 Insert New Blank Pattern"
                            onTriggered: {
                                if (engine) engine.insertNewPatternStep(delegateRoot.stepIdx + 1)
                            }
                        }

                        MenuSeparator {}

                        MenuItem {
                            text: "◀ Move Step Left"
                            enabled: delegateRoot.stepIdx > 0
                            onTriggered: {
                                if (engine) engine.moveSongOrderStep(delegateRoot.stepIdx, delegateRoot.stepIdx - 1)
                            }
                        }

                        MenuItem {
                            text: "▶ Move Step Right"
                            enabled: engine && delegateRoot.stepIdx < engine.songOrderLength - 1
                            onTriggered: {
                                if (engine) engine.moveSongOrderStep(delegateRoot.stepIdx, delegateRoot.stepIdx + 1)
                            }
                        }

                        MenuSeparator {}

                        MenuItem {
                            text: "❌ Delete Step"
                            enabled: engine && engine.songOrderLength > 1
                            onTriggered: {
                                if (engine) engine.removeSongOrderStep(delegateRoot.stepIdx)
                            }
                        }
                    }

                    // Main Card Mouse Area (Selection, Drag-and-Drop, Wheel, Context Menu)
                    MouseArea {
                        id: cardMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: root.isDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                        property int pressStartX: 0
                        property bool dragStarted: false

                        onPressed: (mouse) => {
                            if (mouse.button === Qt.LeftButton) {
                                pressStartX = mouse.x
                                dragStarted = false
                            }
                        }

                        onPositionChanged: (mouse) => {
                            if (pressed && (mouse.buttons & Qt.LeftButton)) {
                                if (!dragStarted && Math.abs(mouse.x - pressStartX) > 8) {
                                    dragStarted = true
                                    root.isDragging = true
                                    root.draggingFromIndex = delegateRoot.stepIdx
                                }
                                if (dragStarted && engine) {
                                    let globalPos = mapToItem(stepListView.contentItem, mouse.x, mouse.y)
                                    let itemWidthWithSpacing = delegateRoot.width + stepListView.spacing
                                    let target = Math.floor(globalPos.x / itemWidthWithSpacing)
                                    target = Math.max(0, Math.min(engine.songOrderLength - 1, target))
                                    root.dropTargetIndex = target
                                }
                            }
                        }

                        onReleased: (mouse) => {
                            if (root.isDragging) {
                                if (root.dropTargetIndex >= 0 && root.dropTargetIndex !== root.draggingFromIndex && engine) {
                                    engine.moveSongOrderStep(root.draggingFromIndex, root.dropTargetIndex)
                                }
                                root.isDragging = false
                                root.draggingFromIndex = -1
                                root.dropTargetIndex = -1
                                dragStarted = false
                            } else if (mouse.button === Qt.LeftButton) {
                                // Clean, direct step selection (no invasive popup!)
                                if (engine) {
                                    engine.setSongOrderIndex(delegateRoot.stepIdx)
                                    engine.setCurrentPattern(delegateRoot.patNum)
                                }
                            }
                        }

                        onDoubleClicked: (mouse) => {
                            if (mouse.button === Qt.LeftButton) {
                                patMenu.popup()
                            }
                        }

                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                cardContextMenu.popup()
                            }
                        }

                        onWheel: (wheel) => {
                            if (engine && engine.numPatterns > 1) {
                                let delta = wheel.angleDelta.y > 0 ? 1 : -1
                                let newPat = (delegateRoot.patNum + delta + engine.numPatterns) % engine.numPatterns
                                engine.setSongOrderStep(delegateRoot.stepIdx, newPat)
                                engine.setCurrentPattern(newPat)
                            }
                        }

                        ToolTip.visible: containsMouse && !root.isDragging && !hoverDelBtn.visible
                        ToolTip.text: "Step " + (delegateRoot.stepIdx + 1) + ": Pattern " + (delegateRoot.patNum < 10 ? "0" : "") + delegateRoot.patNum +
                                      "\n• Click to select and edit pattern" +
                                      "\n• Drag horizontally to reorder" +
                                      "\n• Double-click or ▾ to change pattern" +
                                      "\n• Mouse wheel to cycle patterns" +
                                      "\n• Right-click for options"
                        ToolTip.delay: 400
                    }
                }
            }

            // Footer Component: Modern Add Step (+) Button and Dropdown
            footer: Item {
                width: 64
                height: 24
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    radius: 3
                    color: addBtnMouse.containsMouse ? theme.btnHover : theme.bgWell
                    border.color: addBtnMouse.containsMouse ? theme.accent : theme.border

                    Row {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            text: "+"
                            font.bold: true
                            font.pixelSize: 11
                            color: theme.accent
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "STEP"
                            font.bold: true
                            font.pixelSize: 10
                            color: theme.accent
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: addBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor

                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                addMenu.popup()
                            } else {
                                if (engine) {
                                    engine.addSongOrderStep(engine.currentPattern)
                                    // Set focus to the newly added step
                                    engine.setSongOrderIndex(engine.songOrderLength - 1)
                                }
                            }
                        }

                        ToolTip.visible: containsMouse
                        ToolTip.text: "Append step with current pattern (Right-click for options)"
                        ToolTip.delay: 300
                    }

                    Menu {
                        id: addMenu

                        MenuItem {
                            text: "+ Append Current Pattern (PAT " + (engine ? (engine.currentPattern < 10 ? "0" + engine.currentPattern : engine.currentPattern) : "00") + ")"
                            onTriggered: {
                                if (engine) {
                                    engine.addSongOrderStep(engine.currentPattern)
                                    engine.setSongOrderIndex(engine.songOrderLength - 1)
                                }
                            }
                        }

                        MenuItem {
                            text: "✨ Create & Append New Pattern"
                            onTriggered: {
                                if (engine) {
                                    let newPat = engine.insertNewPatternStep(engine.songOrderLength)
                                    engine.setSongOrderIndex(engine.songOrderLength - 1)
                                }
                            }
                        }

                        MenuItem {
                            text: "⧉ Clone Current Pattern to New Step"
                            onTriggered: {
                                if (engine) {
                                    engine.duplicatePatternStep(engine.songOrderLength - 1)
                                    engine.setSongOrderIndex(engine.songOrderLength - 1)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Vertical Separator
        Rectangle { width: 1; height: 18; color: theme.border; Layout.alignment: Qt.AlignVCenter }

        // ==========================================
        // 3. SONG TIMELINE READOUT & STATUS
        // ==========================================
        RowLayout {
            spacing: 6
            Layout.alignment: Qt.AlignVCenter

            Text {
                text: {
                    if (!engine) return "1 Step"
                    let total = engine.songOrderLength
                    let cur = engine.songOrderIndex + 1
                    if (engine.isSongMode) {
                        return "Step " + (cur < 10 ? "0" + cur : cur) + " / " + (total < 10 ? "0" + total : total) + "  ·  Song Mode"
                    } else {
                        return total + (total === 1 ? " Step in Sequence" : " Steps in Sequence")
                    }
                }
                font.pixelSize: 10
                font.bold: engine && engine.isSongMode
                color: (engine && engine.isSongMode) ? theme.playAccent : theme.textMuted
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }
}
