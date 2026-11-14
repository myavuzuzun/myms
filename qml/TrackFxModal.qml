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
    property int targetTrack: 0 // -1 = Master Bus, >= 0 = Track

    signal requestEditInstrument(int track, int trackType)

    property var fxList: []       // Post-FX (or Master FX when targetTrack === -1)
    property var preFxList: []    // Pre-FX (plugins before instrument)
    property string selectedType: "post" // "pre", "inst", "post"
    property int selectedFxIndex: 0
    property int selectedPreFxIndex: 0

    property var currentFx: {
        if (selectedType === "pre") {
            return (preFxList && preFxList.length > 0 && selectedPreFxIndex >= 0 && selectedPreFxIndex < preFxList.length) ? preFxList[selectedPreFxIndex] : null
        } else if (selectedType === "post") {
            return (fxList && fxList.length > 0 && selectedFxIndex >= 0 && selectedFxIndex < fxList.length) ? fxList[selectedFxIndex] : null
        }
        return null
    }

    property int currentSlot: currentFx ? currentFx.slot : 0
    property var paramList: []

    function refreshParams() {
        if (!engine || selectedType === "inst" || !currentFx) {
            paramList = []
            return
        }
        if (selectedType === "pre") {
            paramList = engine.getTrackPreFxParams(targetTrack, currentSlot)
        } else {
            paramList = engine.getTrackFxParams(targetTrack, currentSlot)
        }
    }

    onTargetTrackChanged: refreshSlots()
    onSelectedFxIndexChanged: refreshParams()
    onSelectedPreFxIndexChanged: refreshParams()
    onSelectedTypeChanged: refreshParams()
    onFxListChanged: refreshParams()
    onPreFxListChanged: refreshParams()

    function open(track) {
        targetTrack = track
        if (engine && track >= 0) {
            engine.setSelectedInstrument(track)
        }
        if (fxTrackNameInput && engine && track >= 0) {
            fxTrackNameInput.text = engine.getInstrumentName(track)
        }
        refreshSlots()
        if (selectedType === "pre" && preFxList.length > 0) {
            selectedPreFxIndex = 0
        } else if (fxList.length > 0) {
            selectedType = "post"
            selectedFxIndex = 0
        } else if (targetTrack >= 0) {
            selectedType = "inst"
        } else {
            selectedType = "post"
        }
        refreshParams()
        isOpen = true
        root.forceActiveFocus()
    }

    function close() {
        isOpen = false
    }

    function refreshSlots() {
        if (!engine) return
        fxList = engine.getTrackFxList(targetTrack)
        if (targetTrack >= 0) {
            preFxList = engine.getTrackPreFxList(targetTrack)
        } else {
            preFxList = []
        }

        if (selectedType === "pre") {
            if (preFxList.length === 0) {
                if (fxList.length > 0) {
                    selectedType = "post"
                    selectedFxIndex = 0
                } else if (targetTrack >= 0) {
                    selectedType = "inst"
                }
            } else if (selectedPreFxIndex >= preFxList.length) {
                selectedPreFxIndex = Math.max(0, preFxList.length - 1)
            }
        } else if (selectedType === "post") {
            if (fxList.length === 0) {
                if (preFxList.length > 0) {
                    selectedType = "pre"
                    selectedPreFxIndex = 0
                } else if (targetTrack >= 0) {
                    selectedType = "inst"
                }
            } else if (selectedFxIndex >= fxList.length) {
                selectedFxIndex = Math.max(0, fxList.length - 1)
            }
        }
        refreshParams()
    }

    Connections {
        target: engine
        function onTrackFxChanged(t, s) {
            if (t === root.targetTrack) {
                root.refreshSlots()
            }
        }
        function onTrackFxParamsChanged(t, s) {
            if (t === root.targetTrack && root.selectedType === "post" && s === root.currentSlot) {
                root.refreshParams()
            }
        }
        function onTrackPreFxChanged(t, s) {
            if (t === root.targetTrack) {
                root.refreshSlots()
            }
        }
        function onTrackPreFxParamsChanged(t, s) {
            if (t === root.targetTrack && root.selectedType === "pre" && s === root.currentSlot) {
                root.refreshParams()
            }
        }
        function onInstrumentNameChanged(inst, name) {
            if (inst === root.targetTrack && fxTrackNameInput) {
                fxTrackNameInput.text = name
            }
        }
        function onProjectLoaded() {
            root.refreshSlots()
        }
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

    // Modal Window Card
    Rectangle {
        id: modalDialog
        anchors.centerIn: parent
        width: Math.min(parent.width - 30, 960)
        height: Math.min(parent.height - 30, 600)
        color: theme.bgCard
        radius: 6
        border.color: theme.borderLight
        border.width: 1
        clip: true

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ==========================================
            // 1. HEADER TITLE BAR (Clean, no internal Done button)
            // ==========================================
            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: theme.bgHeader
                border.color: theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: "🎛️"
                        font.pixelSize: 14
                    }

                    // For Master Bus
                    RowLayout {
                        visible: root.targetTrack === -1
                        spacing: 8
                        Text {
                            text: "MASTER BUS"
                            font.bold: true
                            font.pixelSize: 12
                            color: theme.textHeading
                        }
                        Text {
                            text: "— Master Insert Effects Chain"
                            font.pixelSize: 11
                            color: theme.textMuted
                        }
                    }

                    // For Track
                    RowLayout {
                        visible: root.targetTrack >= 0
                        spacing: 8
                        Text {
                            text: "TRACK #" + (root.targetTrack + 1) + ":"
                            font.bold: true
                            font.pixelSize: 12
                            color: theme.textHeading
                        }

                        Rectangle {
                            width: 140
                            height: 22
                            radius: 3
                            color: theme.bgWell
                            border.color: fxTrackNameInput.activeFocus ? theme.accent : theme.border

                            TextInput {
                                id: fxTrackNameInput
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                verticalAlignment: TextInput.AlignVCenter
                                text: (engine && root.targetTrack >= 0) ? engine.getInstrumentName(root.targetTrack) : "Track"
                                font.bold: true
                                font.pixelSize: 11
                                color: theme.textHeading
                                selectByMouse: true
                                clip: true
                                onEditingFinished: {
                                    if (engine && root.targetTrack >= 0 && text.trim().length > 0) {
                                        engine.setInstrumentName(root.targetTrack, text.trim())
                                    }
                                }
                            }
                        }

                        Text {
                            text: "— Signal Chain & Insert Effects"
                            font.pixelSize: 11
                            color: theme.textMuted
                        }
                    }

                    // Active Count Badge
                    Rectangle {
                        height: 20
                        implicitWidth: countText.implicitWidth + 12
                        radius: 10
                        property int totalActive: (root.targetTrack === -1) ? root.fxList.length : (root.preFxList.length + root.fxList.length)
                        color: totalActive > 0 ? theme.accent : theme.bgWell
                        border.color: theme.border
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: parent.totalActive + " ACTIVE"
                            font.bold: true
                            font.pixelSize: 9
                            color: parent.totalActive > 0 ? "#ffffff" : theme.textMuted
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Keyboard Hint
                    Text {
                        text: "Esc to close"
                        font.pixelSize: 10
                        color: theme.textDim
                    }
                }
            }

            // ==========================================
            // 2. MAIN TWO-PANE BODY
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // ------------------------------------------
                // LEFT PANE: SIGNAL CHAIN (Pre-FX -> Inst -> Post-FX)
                // ------------------------------------------
                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.fillHeight: true
                    color: theme.bgWell
                    border.color: theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        // Section Header
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: root.targetTrack === -1 ? "MASTER INSERT CHAIN" : "DEVICE SIGNAL CHAIN"
                                font.bold: true
                                font.pixelSize: 10
                                color: theme.textMuted
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "TOP ➔ BOTTOM"
                                font.pixelSize: 8
                                color: theme.textDim
                            }
                        }

                        // Scrollable Signal Chain Content
                        ScrollView {
                            id: chainScrollView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            ColumnLayout {
                                width: chainScrollView.availableWidth
                                spacing: 6

                                // ====================================================
                                // SECTION A: PRE-INSTRUMENT FX (Only for tracks >= 0)
                                // ====================================================
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    visible: root.targetTrack >= 0

                                    // Pre-FX Group Header
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 22
                                        color: "#2c253a"
                                        radius: 3
                                        border.color: "#5b437e"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 6
                                            Text {
                                                text: "1. PRE-INSTRUMENT FX"
                                                font.bold: true
                                                font.pixelSize: 9
                                                color: "#d8b4fe"
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text {
                                                text: root.preFxList.length + " loaded"
                                                font.pixelSize: 8
                                                color: "#a855f7"
                                            }
                                        }
                                    }

                                    // Pre-FX List
                                    Repeater {
                                        model: root.preFxList

                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 46
                                            radius: 4
                                            color: (root.selectedType === "pre" && index === root.selectedPreFxIndex) ? theme.bgCard : theme.bgHeader
                                            border.color: (root.selectedType === "pre" && index === root.selectedPreFxIndex) ? "#a855f7" : theme.border
                                            border.width: (root.selectedType === "pre" && index === root.selectedPreFxIndex) ? 2 : 1

                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    root.selectedType = "pre"
                                                    root.selectedPreFxIndex = index
                                                    root.refreshParams()
                                                }
                                            }

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 5
                                                spacing: 6

                                                // Badge
                                                Rectangle {
                                                    width: 32
                                                    height: 18
                                                    radius: 2
                                                    color: "#6b21a8"
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "PRE" + (index + 1)
                                                        font.bold: true
                                                        font.pixelSize: 8
                                                        color: "#ffffff"
                                                    }
                                                }

                                                // Format Badge
                                                Rectangle {
                                                    width: 34
                                                    height: 16
                                                    radius: 2
                                                    color: modelData.format === "VST3" ? "#00b894" : (modelData.format === "CLAP" ? "#6c5ce7" : "#e67e22")
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: modelData.format
                                                        font.bold: true
                                                        font.pixelSize: 7
                                                        color: "#ffffff"
                                                    }
                                                }

                                                // Name
                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 0
                                                    Text {
                                                        Layout.fillWidth: true
                                                        text: modelData.name
                                                        font.bold: true
                                                        font.pixelSize: 10
                                                        color: (root.selectedType === "pre" && index === root.selectedPreFxIndex) ? theme.textHeading : theme.textMain
                                                        elide: Text.ElideRight
                                                    }
                                                    Text {
                                                        text: modelData.numParams + " params"
                                                        font.pixelSize: 8
                                                        color: theme.textMuted
                                                    }
                                                }

                                                // Remove Button
                                                Rectangle {
                                                    width: 20
                                                    height: 20
                                                    radius: 3
                                                    color: preDelMouse.containsMouse ? "#c0392b" : "transparent"
                                                    border.color: preDelMouse.containsMouse ? "#e74c3c" : theme.border

                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "✕"
                                                        font.bold: true
                                                        font.pixelSize: 9
                                                        color: preDelMouse.containsMouse ? "#ffffff" : theme.textMuted
                                                    }

                                                    MouseArea {
                                                        id: preDelMouse
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            if (root.engine) {
                                                                root.engine.removeTrackPreFx(root.targetTrack, modelData.slot)
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Add Pre-Effect Button
                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 26
                                        text: "➕ ADD PRE-EFFECT..."
                                        font.bold: true
                                        font.pixelSize: 9
                                        background: Rectangle {
                                            color: parent.hovered ? "#3b2d52" : "#231c30"
                                            border.color: "#8b5cf6"
                                            radius: 3
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            font: parent.font
                                            color: "#d8b4fe"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (root.engine) root.engine.promptOpenTrackPreFxDialog(root.targetTrack, -1)
                                        }
                                    }
                                }

                                // Signal Arrow (Pre to Instrument)
                                Item {
                                    Layout.fillWidth: true
                                    height: 14
                                    visible: root.targetTrack >= 0
                                    Text {
                                        anchors.centerIn: parent
                                        text: "▼"
                                        font.pixelSize: 10
                                        color: theme.accent
                                    }
                                }

                                // ====================================================
                                // SECTION B: THE INSTRUMENT (Only for tracks >= 0)
                                // ====================================================
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 60
                                    radius: 4
                                    visible: root.targetTrack >= 0
                                    color: (root.selectedType === "inst") ? "#1a2c3d" : theme.bgHeader
                                    border.color: (root.selectedType === "inst") ? theme.accent : "#2980b9"
                                    border.width: (root.selectedType === "inst") ? 2 : 1

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedType = "inst"
                                            root.refreshParams()
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 6

                                        // Icon
                                        Text {
                                            text: {
                                                if (!root.engine || root.targetTrack < 0) return "🎹"
                                                let t = root.engine.getTrackType(root.targetTrack)
                                                if (t === 1) return "🎻"
                                                if (t === 2) return "🥁"
                                                if (t === 3) return "🔌"
                                                return "🎹"
                                            }
                                            font.pixelSize: 18
                                        }

                                        // Instrument Info
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            RowLayout {
                                                spacing: 4
                                                Text {
                                                    text: "2. INSTRUMENT"
                                                    font.bold: true
                                                    font.pixelSize: 8
                                                    color: theme.accent
                                                }
                                                Rectangle {
                                                    height: 14
                                                    implicitWidth: typeBadgeTxt.implicitWidth + 8
                                                    radius: 2
                                                    color: {
                                                        if (!root.engine || root.targetTrack < 0) return theme.accent
                                                        let t = root.engine.getTrackType(root.targetTrack)
                                                        if (t === 1) return "#27ae60"
                                                        if (t === 2) return "#f7b731"
                                                        if (t === 3) return "#3daee9"
                                                        return theme.accent
                                                    }
                                                    Text {
                                                        id: typeBadgeTxt
                                                        anchors.centerIn: parent
                                                        text: {
                                                            if (!root.engine || root.targetTrack < 0) return "SYNTH"
                                                            let t = root.engine.getTrackType(root.targetTrack)
                                                            if (t === 1) return "SOUNDFONT"
                                                            if (t === 2) return "BEATPAD"
                                                            if (t === 3) return "PLUGIN"
                                                            return "SYNTH"
                                                        }
                                                        font.bold: true
                                                        font.pixelSize: 7
                                                        color: "#ffffff"
                                                    }
                                                }
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: (root.engine && root.targetTrack >= 0) ? root.engine.getInstrumentName(root.targetTrack) : "Track"
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: theme.textHeading
                                                elide: Text.ElideRight
                                            }
                                        }

                                        // Quick Edit Sound Button
                                        Button {
                                            text: "⚙ EDIT"
                                            font.bold: true
                                            font.pixelSize: 9
                                            implicitHeight: 26
                                            background: Rectangle {
                                                color: parent.hovered ? theme.btnHover : theme.btnBg
                                                border.color: theme.accent
                                                radius: 3
                                            }
                                            contentItem: Text {
                                                text: parent.text
                                                font: parent.font
                                                color: theme.accent
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: {
                                                if (root.engine && root.targetTrack >= 0) {
                                                    root.requestEditInstrument(root.targetTrack, root.engine.getTrackType(root.targetTrack))
                                                }
                                            }
                                        }
                                    }
                                }

                                // Signal Arrow (Instrument to Post-FX)
                                Item {
                                    Layout.fillWidth: true
                                    height: 14
                                    visible: root.targetTrack >= 0
                                    Text {
                                        anchors.centerIn: parent
                                        text: "▼"
                                        font.pixelSize: 10
                                        color: theme.accent
                                    }
                                }

                                // ====================================================
                                // SECTION C: POST-INSTRUMENT FX (or Master Bus FX)
                                // ====================================================
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    // Post-FX Group Header
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 22
                                        color: root.targetTrack === -1 ? "#1e3a5f" : "#1a334d"
                                        radius: 3
                                        border.color: root.targetTrack === -1 ? theme.accent : "#2b5c8f"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 6
                                            Text {
                                                text: root.targetTrack === -1 ? "MASTER BUS INSERT FX" : "3. POST-INSTRUMENT FX"
                                                font.bold: true
                                                font.pixelSize: 9
                                                color: theme.accent
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text {
                                                text: root.fxList.length + " loaded"
                                                font.pixelSize: 8
                                                color: theme.textMuted
                                            }
                                        }
                                    }

                                    // Post-FX List
                                    Repeater {
                                        model: root.fxList

                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 46
                                            radius: 4
                                            color: (root.selectedType === "post" && index === root.selectedFxIndex) ? theme.bgCard : theme.bgHeader
                                            border.color: (root.selectedType === "post" && index === root.selectedFxIndex) ? theme.accent : theme.border
                                            border.width: (root.selectedType === "post" && index === root.selectedFxIndex) ? 2 : 1

                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    root.selectedType = "post"
                                                    root.selectedFxIndex = index
                                                    root.refreshParams()
                                                }
                                            }

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 5
                                                spacing: 6

                                                // Badge
                                                Rectangle {
                                                    width: 32
                                                    height: 18
                                                    radius: 2
                                                    color: (root.selectedType === "post" && index === root.selectedFxIndex) ? theme.accent : theme.bgWell
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: (root.targetTrack === -1 ? "M" : "FX") + (index + 1)
                                                        font.bold: true
                                                        font.pixelSize: 8
                                                        color: (root.selectedType === "post" && index === root.selectedFxIndex) ? "#ffffff" : theme.textMuted
                                                    }
                                                }

                                                // Format Badge
                                                Rectangle {
                                                    width: 34
                                                    height: 16
                                                    radius: 2
                                                    color: modelData.format === "VST3" ? "#00b894" : (modelData.format === "CLAP" ? "#6c5ce7" : "#e67e22")
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: modelData.format
                                                        font.bold: true
                                                        font.pixelSize: 7
                                                        color: "#ffffff"
                                                    }
                                                }

                                                // Name & Params Count
                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 0
                                                    Text {
                                                        Layout.fillWidth: true
                                                        text: modelData.name
                                                        font.bold: true
                                                        font.pixelSize: 10
                                                        color: (root.selectedType === "post" && index === root.selectedFxIndex) ? theme.textHeading : theme.textMain
                                                        elide: Text.ElideRight
                                                    }
                                                    Text {
                                                        text: modelData.numParams + " params"
                                                        font.pixelSize: 8
                                                        color: theme.textMuted
                                                    }
                                                }

                                                // Remove Button
                                                Rectangle {
                                                    width: 20
                                                    height: 20
                                                    radius: 3
                                                    color: delMouse.containsMouse ? "#c0392b" : "transparent"
                                                    border.color: delMouse.containsMouse ? "#e74c3c" : theme.border

                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "✕"
                                                        font.bold: true
                                                        font.pixelSize: 9
                                                        color: delMouse.containsMouse ? "#ffffff" : theme.textMuted
                                                    }

                                                    MouseArea {
                                                        id: delMouse
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            if (root.engine) {
                                                                root.engine.removeTrackFx(root.targetTrack, modelData.slot)
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Add Post-Effect Action
                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 26
                                        text: root.targetTrack === -1 ? "➕ ADD MASTER EFFECT..." : "➕ ADD POST-EFFECT..."
                                        font.bold: true
                                        font.pixelSize: 9
                                        background: Rectangle {
                                            color: parent.hovered ? theme.btnHover : theme.btnBg
                                            border.color: theme.accent
                                            radius: 3
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            font: parent.font
                                            color: theme.accent
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (root.engine) {
                                                root.engine.promptOpenTrackFxDialog(root.targetTrack, -1)
                                            }
                                        }
                                    }

                                    // Quick Demo FX Preset
                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 22
                                        text: "+ Add Demo Filter FX"
                                        font.pixelSize: 8
                                        background: Rectangle {
                                            color: parent.hovered ? theme.btnHover : theme.bgHeader
                                            border.color: theme.border
                                            radius: 3
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            font: parent.font
                                            color: theme.textMuted
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (root.engine) {
                                                root.engine.addTrackFx(root.targetTrack, "plugins/effects/demo_filter_fx.so")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ------------------------------------------
                // RIGHT PANE: SPACIOUS PARAMETER INSPECTOR
                // ------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: theme.bgCard

                    // CASE 1: Instrument Selected (targetTrack >= 0)
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        visible: root.selectedType === "inst" && root.targetTrack >= 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 70
                            color: theme.bgHeader
                            radius: 4
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12

                                Text {
                                    text: {
                                        if (!root.engine || root.targetTrack < 0) return "🎹"
                                        let t = root.engine.getTrackType(root.targetTrack)
                                        if (t === 1) return "🎻"
                                        if (t === 2) return "🥁"
                                        if (t === 3) return "🔌"
                                        return "🎹"
                                    }
                                    font.pixelSize: 28
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: (root.engine && root.targetTrack >= 0) ? root.engine.getInstrumentName(root.targetTrack) : "Track"
                                        font.bold: true
                                        font.pixelSize: 14
                                        color: theme.textHeading
                                    }
                                    Text {
                                        text: {
                                            if (!root.engine || root.targetTrack < 0) return "AuraSynth"
                                            let t = root.engine.getTrackType(root.targetTrack)
                                            if (t === 1) return "SoundFont2 GM Player (FontBox)"
                                            if (t === 2) return "8-Slot Drum Sampler (BeatPad)"
                                            if (t === 3) {
                                                let pName = root.engine.getTrackPluginName(root.targetTrack)
                                                let pFmt = root.engine.getTrackPluginFormatName(root.targetTrack)
                                                return "Plugin Instrument: " + pName + " [" + pFmt + "]"
                                            }
                                            return "Polyphonic Subtractive Synth (AuraSynth)"
                                        }
                                        font.pixelSize: 10
                                        color: theme.textMuted
                                    }
                                }

                                Row {
                                    spacing: 8
                                    Button {
                                        text: "🗔 OPEN GUI"
                                        font.bold: true
                                        font.pixelSize: 10
                                        implicitHeight: 30
                                        visible: {
                                            if (!root.engine || root.targetTrack < 0) return false
                                            return root.engine.getTrackType(root.targetTrack) === 3 && root.engine.hasTrackPluginNativeEditor(root.targetTrack)
                                        }
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
                                        onClicked: {
                                            if (root.engine && root.targetTrack >= 0) {
                                                root.engine.promptOpenPluginGui(root.targetTrack)
                                            }
                                        }
                                    }

                                    Button {
                                        text: "⚙ SOUND STUDIO EDITOR..."
                                        font.bold: true
                                        font.pixelSize: 10
                                        implicitHeight: 30
                                        background: Rectangle {
                                            color: parent.hovered ? theme.btnHover : theme.btnBg
                                            border.color: theme.accent
                                            radius: 3
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            font: parent.font
                                            color: theme.accent
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (root.engine && root.targetTrack >= 0) {
                                                root.requestEditInstrument(root.targetTrack, root.engine.getTrackType(root.targetTrack))
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: theme.bgWell
                            radius: 4
                            border.color: theme.border

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 10
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "Track Instrument Active"
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: theme.textHeading
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "This instrument sits in the middle of your signal chain.\nMusical notes pass through Pre-FX into this instrument, and generated audio feeds into Post-FX."
                                    font.pixelSize: 11
                                    color: theme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                Button {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "Open Full Sound Editor"
                                    font.bold: true
                                    font.pixelSize: 10
                                    implicitHeight: 32
                                    implicitWidth: 180
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
                                    onClicked: {
                                        if (root.engine && root.targetTrack >= 0) {
                                            root.requestEditInstrument(root.targetTrack, root.engine.getTrackType(root.targetTrack))
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // CASE 2: Plugin Effect Selected (Pre or Post) & Loaded
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10
                        visible: root.selectedType !== "inst" && root.currentFx !== null

                        // Inspector Header Card
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: theme.bgHeader
                            radius: 4
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                // Format Badge
                                Rectangle {
                                    width: 48
                                    height: 24
                                    radius: 3
                                    color: {
                                        if (!root.currentFx) return theme.bgWell
                                        let fmt = root.currentFx.format
                                        if (fmt === "VST3") return "#00b894"
                                        if (fmt === "CLAP") return "#6c5ce7"
                                        return "#e67e22"
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: root.currentFx ? root.currentFx.format : ""
                                        font.bold: true
                                        font.pixelSize: 10
                                        color: "#ffffff"
                                    }
                                }

                                // Name & Vendor
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text {
                                        text: root.currentFx ? root.currentFx.name : ""
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: theme.textHeading
                                    }
                                    Text {
                                        text: root.currentFx ? (root.currentFx.vendor + " · " + root.currentFx.path) : ""
                                        font.pixelSize: 9
                                        color: theme.textMuted
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }

                                // Actions
                                Row {
                                    spacing: 6
                                    Layout.alignment: Qt.AlignVCenter

                                    Button {
                                        text: "🗔 OPEN GUI"
                                        font.bold: true
                                        font.pixelSize: 9
                                        implicitHeight: 28
                                        visible: {
                                            if (!root.engine || !root.currentFx) return false
                                            if (root.selectedType === "pre") {
                                                return root.engine.hasTrackPreFxNativeEditor(root.targetTrack, root.currentSlot)
                                            } else {
                                                return root.engine.hasTrackFxNativeEditor(root.targetTrack, root.currentSlot)
                                            }
                                        }
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
                                        onClicked: {
                                            if (!root.engine) return
                                            if (root.selectedType === "pre") {
                                                root.engine.promptOpenTrackPreFxGui(root.targetTrack, root.currentSlot)
                                            } else {
                                                root.engine.promptOpenTrackFxGui(root.targetTrack, root.currentSlot)
                                            }
                                        }
                                    }

                                    Button {
                                        text: "📁 CHANGE..."
                                        font.bold: true
                                        font.pixelSize: 9
                                        implicitHeight: 28
                                        background: Rectangle {
                                            color: parent.hovered ? theme.btnHover : theme.btnBg
                                            border.color: theme.border
                                            radius: 3
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            font: parent.font
                                            color: theme.textMain
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (!root.engine) return
                                            if (root.selectedType === "pre") {
                                                root.engine.promptOpenTrackPreFxDialog(root.targetTrack, root.currentSlot)
                                            } else {
                                                root.engine.promptOpenTrackFxDialog(root.targetTrack, root.currentSlot)
                                            }
                                        }
                                    }

                                    Button {
                                        text: "✕ REMOVE"
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
                                            color: parent.parent.hovered ? "#ffffff" : theme.textMuted
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        onClicked: {
                                            if (!root.engine) return
                                            if (root.selectedType === "pre") {
                                                root.engine.removeTrackPreFx(root.targetTrack, root.currentSlot)
                                            } else {
                                                root.engine.removeTrackFx(root.targetTrack, root.currentSlot)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Parameters Title
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "PARAMETERS (" + root.paramList.length + ")"
                                font.bold: true
                                font.pixelSize: 10
                                color: theme.textMuted
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "Click or drag to adjust · Wheel to fine tune"
                                font.pixelSize: 9
                                color: theme.textDim
                            }
                        }

                        // Spacious Parameters Grid
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            Flow {
                                width: parent.width - 12
                                spacing: 12

                                Repeater {
                                    model: root.paramList

                                    Rectangle {
                                        id: fxParamCard
                                        width: Math.max(260, Math.floor((parent.width - 24) / 2))
                                        height: 64
                                        color: theme.bgWell
                                        radius: 4
                                        border.color: theme.border

                                        property int paramId: modelData.id
                                        property real paramVal: modelData.value
                                        property string paramName: modelData.name
                                        property string paramDisp: modelData.display

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 4

                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: fxParamCard.paramName
                                                    font.bold: true
                                                    font.pixelSize: 11
                                                    color: theme.textHeading
                                                    elide: Text.ElideRight
                                                }
                                                Text {
                                                    text: {
                                                        if (fxParamCard.paramDisp && fxParamCard.paramDisp.length > 0) {
                                                            return fxParamCard.paramDisp
                                                        }
                                                        return Math.round(fxParamCard.paramVal * 100) + "%"
                                                    }
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    color: theme.accent
                                                }
                                            }

                                            // Custom Rock-Solid DAW Slider
                                            Item {
                                                id: fxSliderArea
                                                Layout.fillWidth: true
                                                height: 22

                                                // Groove
                                                Rectangle {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: parent.width
                                                    height: 6
                                                    radius: 3
                                                    color: theme.bgHeader
                                                    border.color: theme.border

                                                    // Active Fill
                                                    Rectangle {
                                                        width: Math.max(0, Math.min(parent.width, fxParamCard.paramVal * parent.width))
                                                        height: parent.height
                                                        radius: 3
                                                        color: fxMouse.pressed ? "#ffffff" : theme.accent
                                                    }
                                                }

                                                // Thumb Handle
                                                Rectangle {
                                                    width: 14
                                                    height: 14
                                                    radius: 7
                                                    color: fxMouse.pressed ? "#ffffff" : (fxMouse.containsMouse ? "#55c0f5" : theme.accent)
                                                    border.color: theme.border
                                                    border.width: 1
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    x: Math.max(0, Math.min(parent.width - width, fxParamCard.paramVal * (parent.width - width)))
                                                }

                                                MouseArea {
                                                    id: fxMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    preventStealing: true
                                                    cursorShape: Qt.PointingHandCursor

                                                    function updateVal(mouseX) {
                                                        let newVal = Math.max(0.0, Math.min(1.0, mouseX / parent.width))
                                                        fxParamCard.paramVal = newVal
                                                        if (root.engine) {
                                                            if (root.selectedType === "pre") {
                                                                root.engine.setTrackPreFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                                let disp = root.engine.getTrackPreFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                                if (disp && disp.length > 0) fxParamCard.paramDisp = disp
                                                                else fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
                                                            } else {
                                                                root.engine.setTrackFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                                let disp = root.engine.getTrackFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                                if (disp && disp.length > 0) fxParamCard.paramDisp = disp
                                                                else fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
                                                            }
                                                        }
                                                    }

                                                    onPressed: (mouse) => updateVal(mouse.x)
                                                    onPositionChanged: (mouse) => {
                                                        if (pressed) updateVal(mouse.x)
                                                    }
                                                    onWheel: (wheel) => {
                                                        let delta = wheel.angleDelta.y > 0 ? 0.02 : -0.02
                                                        let newVal = Math.max(0.0, Math.min(1.0, fxParamCard.paramVal + delta))
                                                        fxParamCard.paramVal = newVal
                                                        if (root.engine) {
                                                            if (root.selectedType === "pre") {
                                                                root.engine.setTrackPreFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                                let disp = root.engine.getTrackPreFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                                if (disp && disp.length > 0) fxParamCard.paramDisp = disp
                                                                else fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
                                                            } else {
                                                                root.engine.setTrackFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                                let disp = root.engine.getTrackFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                                if (disp && disp.length > 0) fxParamCard.paramDisp = disp
                                                                else fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // CASE 3: Empty State (No Effects Loaded)
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: root.selectedType !== "inst" && root.currentFx === null

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "🎛️"
                            font.pixelSize: 48
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.targetTrack === -1 ? "No Master Insert Effects Loaded" : "No Effects Selected"
                            font.bold: true
                            font.pixelSize: 14
                            color: theme.textHeading
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.targetTrack === -1 ?
                                "Add mastering limiters, compressors, EQs or visualizers to the Master Bus." :
                                "Add pre-instrument processors or post-instrument audio effects from the left panel."
                            font.pixelSize: 11
                            color: theme.textMuted
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 8

                            Button {
                                implicitHeight: 32
                                implicitWidth: 160
                                text: root.targetTrack === -1 ? "➕ ADD MASTER EFFECT..." : "➕ ADD POST-EFFECT..."
                                font.bold: true
                                font.pixelSize: 10
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
                                onClicked: {
                                    if (root.engine) root.engine.promptOpenTrackFxDialog(root.targetTrack, -1)
                                }
                            }

                            Button {
                                implicitHeight: 32
                                implicitWidth: 160
                                text: "+ Add Demo Filter FX"
                                font.bold: true
                                font.pixelSize: 10
                                background: Rectangle {
                                    color: parent.hovered ? theme.btnHover : theme.btnBg
                                    border.color: theme.border
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: theme.textMain
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    if (root.engine) root.engine.addTrackFx(root.targetTrack, "plugins/effects/demo_filter_fx.so")
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Floating X Close Button (Above modal window, centered horizontally)
    Rectangle {
        id: floatingCloseButton
        width: 32
        height: 32
        radius: 16
        anchors.bottom: modalDialog.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: modalDialog.horizontalCenter
        color: closeMouseArea.containsMouse ? "#da4453" : "#31363b"
        border.color: closeMouseArea.containsMouse ? "#e74c3c" : theme.borderLight
        border.width: 1
        z: 10

        Text {
            anchors.centerIn: parent
            text: "✕"
            font.pixelSize: 13
            font.bold: true
            color: closeMouseArea.containsMouse ? "#ffffff" : theme.textHeading
        }

        MouseArea {
            id: closeMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.close()
            ToolTip.visible: containsMouse
            ToolTip.text: "Close Window (Esc)"
            ToolTip.delay: 300
        }
    }

    Keys.onEscapePressed: {
        root.close()
    }
}
