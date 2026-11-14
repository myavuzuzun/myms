import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgCard
    radius: 4
    border.color: theme.border
    border.width: 1

    property var engine
    signal requestEditInstrument(int track, int trackType)
    signal requestEditFx(int track)

    function gainToDb(gain) {
        if (gain <= 0.0001) return "-inf dB"
        let db = 20.0 * Math.log10(gain)
        if (db >= 0.0) return "+" + db.toFixed(1) + " dB"
        return db.toFixed(1) + " dB"
    }

    function panToString(pan) {
        if (Math.abs(pan - 0.5) < 0.04) return "CENTER"
        if (pan < 0.5) {
            let pct = Math.round((0.5 - pan) * 200)
            return "L " + pct + "%"
        } else {
            let pct = Math.round((pan - 0.5) * 200)
            return "R " + pct + "%"
        }
    }

    // High performance VU meter updater: DOES NOT rebuild delegates!
    Timer {
        id: meterTimer
        interval: 35
        running: root.visible && engine && engine.isPlaying
        repeat: true
        onTriggered: {
            if (!engine) return
            for (let i = 0; i < meterRepeater.count; ++i) {
                let item = meterRepeater.itemAt(i)
                if (item) {
                    let m = engine.getTrackMeters(i)
                    if (m && m.length >= 2) {
                        item.peakL = m[0]
                        item.peakR = m[1]
                    }
                }
            }
        }
    }

    // Horizontal Channel Strips (Maximizes vertical fader travel)
    ScrollView {
        anchors.fill: parent
        anchors.margins: 6
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff

        Row {
            id: channelRow
            spacing: 8
            height: parent.height

            Repeater {
                id: meterRepeater
                model: engine ? engine.numTracks : 1

                // --- INDIVIDUAL TRACK CHANNEL STRIP (Spacious 160px wide) ---
                Rectangle {
                    id: strip
                    width: 160
                    height: channelRow.height
                    color: (engine && engine.selectedInstrument === index) ? theme.bgWellHover : theme.bgWell
                    radius: 4
                    border.color: (engine && engine.selectedInstrument === index) ? theme.accent : theme.border
                    border.width: (engine && engine.selectedInstrument === index) ? 2 : 1

                    property int trackIdx: index
                    property string trackName: engine ? engine.getInstrumentName(trackIdx) : "Track"
                    property int trackType: engine ? engine.getTrackType(trackIdx) : 0
                    property real trackVol: engine ? engine.getTrackVolume(trackIdx) : 1.0
                    property real trackPan: engine ? engine.getTrackPan(trackIdx) : 0.5
                    property bool trackMute: engine ? engine.getTrackMute(trackIdx) : false
                    property bool trackSolo: engine ? engine.getTrackSolo(trackIdx) : false
                    property int trackSf2: engine ? engine.getTrackSf2Preset(trackIdx) : 0
                    property bool trackPluginLoaded: engine ? engine.isTrackPluginLoaded(trackIdx) : false
                    property string trackPluginName: engine ? engine.getTrackPluginName(trackIdx) : ""
                    property string trackPluginFormat: engine ? engine.getTrackPluginFormatName(trackIdx) : ""

                    property real peakL: 0.0
                    property real peakR: 0.0

                    Connections {
                        target: engine
                        function onTrackMixerChanged(t) {
                            if (t === strip.trackIdx || t === -1) {
                                strip.trackType = engine.getTrackType(strip.trackIdx)
                                strip.trackVol = engine.getTrackVolume(strip.trackIdx)
                                strip.trackPan = engine.getTrackPan(strip.trackIdx)
                                strip.trackMute = engine.getTrackMute(strip.trackIdx)
                                strip.trackSolo = engine.getTrackSolo(strip.trackIdx)
                                strip.trackSf2 = engine.getTrackSf2Preset(strip.trackIdx)
                                strip.trackName = engine.getInstrumentName(strip.trackIdx)
                                strip.trackPluginLoaded = engine.isTrackPluginLoaded(strip.trackIdx)
                                strip.trackPluginName = engine.getTrackPluginName(strip.trackIdx)
                                strip.trackPluginFormat = engine.getTrackPluginFormatName(strip.trackIdx)
                            }
                        }
                        function onTrackTypeChanged(t, type) {
                            if (t === strip.trackIdx) {
                                strip.trackType = type
                                strip.trackPluginLoaded = engine.isTrackPluginLoaded(strip.trackIdx)
                                strip.trackPluginName = engine.getTrackPluginName(strip.trackIdx)
                                strip.trackPluginFormat = engine.getTrackPluginFormatName(strip.trackIdx)
                            }
                        }
                        function onTrackPluginChanged(t) {
                            if (t === strip.trackIdx || t === -1) {
                                strip.trackType = engine.getTrackType(strip.trackIdx)
                                strip.trackPluginLoaded = engine.isTrackPluginLoaded(strip.trackIdx)
                                strip.trackPluginName = engine.getTrackPluginName(strip.trackIdx)
                                strip.trackPluginFormat = engine.getTrackPluginFormatName(strip.trackIdx)
                            }
                        }
                        function onInstrumentNameChanged(inst, name) {
                            if (inst === strip.trackIdx) {
                                strip.trackName = name
                            }
                        }
                        function onProjectLoaded() {
                            strip.trackType = engine ? engine.getTrackType(strip.trackIdx) : 0
                            strip.trackVol = engine ? engine.getTrackVolume(strip.trackIdx) : 1.0
                            strip.trackPan = engine ? engine.getTrackPan(strip.trackIdx) : 0.5
                            strip.trackMute = engine ? engine.getTrackMute(strip.trackIdx) : false
                            strip.trackSolo = engine ? engine.getTrackSolo(strip.trackIdx) : false
                            strip.trackSf2 = engine ? engine.getTrackSf2Preset(strip.trackIdx) : 0
                            strip.trackName = engine ? engine.getInstrumentName(strip.trackIdx) : "Track"
                            strip.trackPluginLoaded = engine ? engine.isTrackPluginLoaded(strip.trackIdx) : false
                            strip.trackPluginName = engine ? engine.getTrackPluginName(strip.trackIdx) : ""
                            strip.trackPluginFormat = engine ? engine.getTrackPluginFormatName(strip.trackIdx) : ""
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        // 1. Channel Header: Track Number & Name
                        Rectangle {
                            Layout.fillWidth: true
                            height: 24
                            color: (engine && engine.selectedInstrument === strip.trackIdx) ? theme.accent : theme.bgTrackHeader
                            radius: 3

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text {
                                    text: "#" + (strip.trackIdx + 1)
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: (engine && engine.selectedInstrument === strip.trackIdx) ? "#ffffff" : theme.accent
                                }

                                TextInput {
                                    id: stripNameInput
                                    Layout.fillWidth: true
                                    text: strip.trackName
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: (engine && engine.selectedInstrument === strip.trackIdx) ? "#ffffff" : theme.textMain
                                    selectByMouse: true
                                    clip: true
                                    onActiveFocusChanged: {
                                        if (activeFocus && engine) {
                                            engine.setSelectedInstrument(strip.trackIdx)
                                        }
                                    }
                                    onEditingFinished: {
                                        if (engine && text.trim().length > 0) {
                                            engine.setInstrumentName(strip.trackIdx, text.trim())
                                        }
                                    }
                                }
                            }
                        }

                        // 2. Engine Type Selector Button
                        Rectangle {
                            id: engineBtn
                            Layout.fillWidth: true
                            height: 22
                            color: engineMouse.containsMouse ? theme.btnHover : theme.btnBg
                            radius: 3
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4

                                Text {
                                    Layout.fillWidth: true
                                    text: {
                                        if (strip.trackType === 1) return "🎻 SOUNDFONT"
                                        if (strip.trackType === 2) return "🥁 BEATPAD"
                                        if (strip.trackType === 3) {
                                            return strip.trackPluginLoaded ? ("🔌 " + (strip.trackPluginName ? strip.trackPluginName : "PLUGIN")) : "🔌 PLUGIN INST"
                                        }
                                        return "🎹 AURASYNTH"
                                    }
                                    font.bold: true
                                    font.pixelSize: 9
                                    color: {
                                        if (strip.trackType === 1) return "#92ebb5"
                                        if (strip.trackType === 2) return "#f7b731"
                                        if (strip.trackType === 3) return "#3daee9"
                                        return theme.accent
                                    }
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: "▾"
                                    font.pixelSize: 8
                                    color: theme.textMuted
                                }
                            }

                            MouseArea {
                                id: engineMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: engineMenu.open()
                            }

                            Menu {
                                id: engineMenu
                                MenuItem {
                                    text: "🎹 AuraSynth (Poly Subtractive)"
                                    onTriggered: if (engine) engine.setTrackType(strip.trackIdx, 0)
                                }
                                MenuItem {
                                    text: "🎻 SoundFont (SF2 Multi-Voice)"
                                    onTriggered: {
                                        if (engine) {
                                            if (!engine.sf2Loaded) {
                                                engine.loadSoundFont("/usr/share/sounds/sf2/default-GM.sf2")
                                            }
                                            engine.setTrackType(strip.trackIdx, 1)
                                        }
                                    }
                                }
                                MenuItem {
                                    text: "🥁 BeatPad (8-Slot Sampler)"
                                    onTriggered: if (engine) engine.setTrackType(strip.trackIdx, 2)
                                }
                                MenuItem {
                                    text: "🔌 Plugin Instrument (VST3 / CLAP / VST2)"
                                    onTriggered: {
                                        if (engine) {
                                            engine.setTrackType(strip.trackIdx, 3)
                                            if (!engine.isTrackPluginLoaded(strip.trackIdx)) {
                                                engine.promptOpenPlugin(strip.trackIdx)
                                            }
                                        }
                                    }
                                }
                                MenuItem {
                                    text: "📁 Change Plugin..."
                                    visible: strip.trackType === 3
                                    onTriggered: {
                                        if (engine) engine.promptOpenPlugin(strip.trackIdx)
                                    }
                                }
                            }
                        }

                        // 3. SF2 Preset Selector (Visible only when SF2 is active)
                        Rectangle {
                            Layout.fillWidth: true
                            height: 20
                            color: sf2Mouse.containsMouse ? theme.btnHover : theme.bgCard
                            radius: 2
                            border.color: theme.border
                            visible: (strip.trackType === 1)

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 4
                                text: {
                                    if (!engine || !engine.sf2Loaded) return "No SF2 Loaded"
                                    let name = engine.getSoundFontPresetName(strip.trackSf2)
                                    return (strip.trackSf2 < 10 ? "00" + strip.trackSf2 : (strip.trackSf2 < 100 ? "0" + strip.trackSf2 : "" + strip.trackSf2)) + " " + (name ? name : "Preset")
                                }
                                font.pixelSize: 9
                                color: theme.textMain
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }

                            MouseArea {
                                id: sf2Mouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (engine) engine.setSelectedInstrument(strip.trackIdx)
                                    root.requestEditInstrument(strip.trackIdx, 1)
                                }
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Click to choose SoundFont Preset"
                                ToolTip.delay: 400
                            }
                        }

                        // 3b. Plugin Quick Display (Visible when Plugin Instrument is active)
                        Rectangle {
                            Layout.fillWidth: true
                            height: 20
                            color: plgMouse.containsMouse ? theme.btnHover : theme.bgCard
                            radius: 2
                            border.color: strip.trackPluginLoaded ? theme.accent : theme.border
                            visible: (strip.trackType === 3)

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 4
                                text: {
                                    if (!strip.trackPluginLoaded) return "+ LOAD PLUGIN..."
                                    return strip.trackPluginFormat + ": " + (strip.trackPluginName ? strip.trackPluginName : "Plugin")
                                }
                                font.pixelSize: 9
                                font.bold: true
                                color: strip.trackPluginLoaded ? theme.textHeading : theme.accent
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }

                            MouseArea {
                                id: plgMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (engine) {
                                        engine.setSelectedInstrument(strip.trackIdx)
                                        if (!engine.isTrackPluginLoaded(strip.trackIdx)) {
                                            engine.promptOpenPlugin(strip.trackIdx)
                                        } else {
                                            root.requestEditInstrument(strip.trackIdx, strip.trackType)
                                        }
                                    }
                                }
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Click to configure plugin"
                                ToolTip.delay: 400
                            }
                        }

                        // 4. Pan Slider (High Precision Horizontal Slider)
                        Rectangle {
                            Layout.fillWidth: true
                            height: 20
                            color: theme.bgCard
                            radius: 2
                            border.color: theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 4
                                spacing: 4

                                Text {
                                    text: "PAN"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: theme.textDim
                                }

                                // Pan Groove & Handle
                                Item {
                                    id: panTrackItem
                                    Layout.fillWidth: true
                                    height: parent.height

                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        height: 4
                                        radius: 2
                                        color: theme.border

                                        // Center detent mark
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 2
                                            height: 8
                                            color: theme.textMuted
                                        }
                                    }

                                    Rectangle {
                                        id: panCap
                                        width: 14
                                        height: 14
                                        radius: 2
                                        color: panMouseArea.pressed ? "#ffffff" : theme.accent
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: Math.max(0, Math.min(panTrackItem.width - width, strip.trackPan * (panTrackItem.width - width)))
                                    }

                                    MouseArea {
                                        id: panMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.SizeHorCursor
                                        hoverEnabled: true
                                        preventStealing: true

                                        property real startDragX: 0
                                        property real startPan: 0.5

                                        function updateFromPos(mouseX) {
                                            let avail = panTrackItem.width - panCap.width
                                            if (avail <= 0) return
                                            let clampedX = Math.max(0, Math.min(avail, mouseX - panCap.width / 2))
                                            let newPan = clampedX / avail
                                            if (Math.abs(newPan - 0.5) < 0.05) newPan = 0.5
                                            if (engine) engine.setTrackPan(strip.trackIdx, newPan)
                                        }

                                        onPressed: (mouse) => {
                                            let capX = panCap.x
                                            if (mouse.x >= capX && mouse.x <= capX + panCap.width) {
                                                startDragX = mouse.x
                                                startPan = strip.trackPan
                                            } else {
                                                updateFromPos(mouse.x)
                                                startDragX = mouse.x
                                                startPan = strip.trackPan
                                            }
                                        }

                                        onPositionChanged: (mouse) => {
                                            if (pressed) {
                                                let avail = panTrackItem.width - panCap.width
                                                if (avail <= 0) return
                                                let dx = mouse.x - startDragX
                                                let dPan = dx / avail
                                                let newPan = Math.max(0.0, Math.min(1.0, startPan + dPan))
                                                if (Math.abs(newPan - 0.5) < 0.05) newPan = 0.5
                                                if (engine) engine.setTrackPan(strip.trackIdx, newPan)
                                            }
                                        }

                                        onDoubleClicked: {
                                            if (engine) engine.setTrackPan(strip.trackIdx, 0.5)
                                        }

                                        onWheel: (wheel) => {
                                            if (engine) {
                                                let delta = (wheel.angleDelta.y > 0 ? 0.05 : -0.05)
                                                let nextPan = Math.max(0.0, Math.min(1.0, strip.trackPan + delta))
                                                engine.setTrackPan(strip.trackIdx, nextPan)
                                            }
                                        }
                                    }
                                }

                                Text {
                                    text: root.panToString(strip.trackPan)
                                    font.pixelSize: 8
                                    font.bold: true
                                    font.family: "Monospace"
                                    color: theme.textMain
                                    width: 36
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }

                        // 5. Mute, Solo & FX Buttons
                        RowLayout {
                            Layout.fillWidth: true
                            height: 22
                            spacing: 3

                            Button {
                                Layout.fillWidth: true
                                implicitHeight: 22
                                checkable: true
                                checked: strip.trackMute
                                onClicked: if (engine) engine.setTrackMute(strip.trackIdx, !strip.trackMute)
                                background: Rectangle {
                                    color: strip.trackMute ? theme.pauseAccent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                    border.color: strip.trackMute ? theme.pauseAccent : theme.border
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "MUTE"
                                    font.bold: true
                                    font.pixelSize: 8
                                    color: strip.trackMute ? "#ffffff" : theme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                implicitHeight: 22
                                checkable: true
                                checked: strip.trackSolo
                                onClicked: if (engine) engine.setTrackSolo(strip.trackIdx, !strip.trackSolo)
                                background: Rectangle {
                                    color: strip.trackSolo ? theme.accent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                    border.color: strip.trackSolo ? theme.accent : theme.border
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "SOLO"
                                    font.bold: true
                                    font.pixelSize: 8
                                    color: strip.trackSolo ? "#ffffff" : theme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Button {
                                id: fxBtn
                                Layout.fillWidth: true
                                implicitHeight: 22
                                property int fxCount: engine ? engine.getTrackFxCount(strip.trackIdx) : 0
                                Connections {
                                    target: engine
                                    function onTrackFxChanged(t, s) {
                                        if (t === strip.trackIdx || t === -1) {
                                            fxBtn.fxCount = engine.getTrackFxCount(strip.trackIdx)
                                        }
                                    }
                                    function onProjectLoaded() {
                                        fxBtn.fxCount = engine ? engine.getTrackFxCount(strip.trackIdx) : 0
                                    }
                                }
                                background: Rectangle {
                                    color: (fxBtn.fxCount > 0) ? theme.accent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                    border.color: (fxBtn.fxCount > 0) ? theme.accent : theme.border
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: (fxBtn.fxCount > 0) ? ("FX (" + fxBtn.fxCount + ")") : "FX"
                                    font.bold: true
                                    font.pixelSize: 8
                                    color: (fxBtn.fxCount > 0) ? "#ffffff" : (parent.hovered ? theme.textHeading : theme.textMuted)
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    if (engine) engine.setSelectedInstrument(strip.trackIdx)
                                    root.requestEditFx(strip.trackIdx)
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: (fxBtn.fxCount > 0) ? ("Open Insert FX Chain (" + fxBtn.fxCount + " active)") : "Open Insert FX Chain"
                                ToolTip.delay: 300
                            }
                        }

                        // 6. High-Precision Vertical Volume Fader & Dual VU Meter
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 50
                            spacing: 6

                            // Custom Rock-Solid Vertical Fader
                            Item {
                                id: faderArea
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                // Fader Track Groove
                                Rectangle {
                                    id: faderGroove
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 6
                                    height: parent.height
                                    radius: 3
                                    color: theme.bgWell

                                    // Unity Gain 0dB marker line (at volume = 1.0 out of 1.5 max)
                                    Rectangle {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        y: Math.round((1.0 - (1.0 / 1.5)) * (faderGroove.height - 20) + 10)
                                        width: 24
                                        height: 2
                                        color: theme.accent
                                        opacity: 0.7
                                    }
                                }

                                // Fader Cap (Tactile pro mixing console slider cap)
                                Rectangle {
                                    id: faderCap
                                    width: 38
                                    height: 20
                                    radius: 3
                                    anchors.horizontalCenter: faderGroove.horizontalCenter
                                    y: {
                                        let trackH = Math.max(1, faderArea.height - height)
                                        let norm = Math.max(0.0, Math.min(1.5, strip.trackVol)) / 1.5
                                        return Math.round((1.0 - norm) * trackH)
                                    }
                                    color: faderMouse.pressed ? "#ffffff" : (strip.trackMute ? theme.textDim : theme.btnHover)
                                    border.color: faderMouse.pressed ? theme.accent : theme.borderLight
                                    border.width: 1

                                    // Grip center notch
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: parent.width - 8
                                        height: 2
                                        color: faderMouse.pressed ? theme.accent : theme.borderLight
                                    }
                                }

                                MouseArea {
                                    id: faderMouse
                                    anchors.fill: parent
                                    cursorShape: Qt.SizeVerCursor
                                    hoverEnabled: true
                                    preventStealing: true

                                    property real startDragY: 0
                                    property real startVol: 1.0

                                    function updateFromPos(mouseY) {
                                        let trackH = Math.max(1, faderArea.height - faderCap.height)
                                        let clampedY = Math.max(0, Math.min(trackH, mouseY - faderCap.height / 2))
                                        let norm = 1.0 - (clampedY / trackH)
                                        let newVol = norm * 1.5
                                        if (Math.abs(newVol - 1.0) < 0.04) newVol = 1.0
                                        if (engine) engine.setTrackVolume(strip.trackIdx, newVol)
                                    }

                                    onPressed: (mouse) => {
                                        let capY = faderCap.y
                                        if (mouse.y >= capY && mouse.y <= capY + faderCap.height) {
                                            startDragY = mouse.y
                                            startVol = strip.trackVol
                                        } else {
                                            updateFromPos(mouse.y)
                                            startDragY = mouse.y
                                            startVol = strip.trackVol
                                        }
                                    }

                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            let trackH = Math.max(1, faderArea.height - faderCap.height)
                                            let dy = mouse.y - startDragY
                                            let dVol = -(dy / trackH) * 1.5
                                            let newVol = Math.max(0.0, Math.min(1.5, startVol + dVol))
                                            if (Math.abs(newVol - 1.0) < 0.04) newVol = 1.0
                                            if (engine) engine.setTrackVolume(strip.trackIdx, newVol)
                                        }
                                    }

                                    onDoubleClicked: {
                                        if (engine) engine.setTrackVolume(strip.trackIdx, 1.0)
                                    }

                                    onWheel: (wheel) => {
                                        if (engine) {
                                            let delta = (wheel.angleDelta.y > 0 ? 0.05 : -0.05)
                                            let nextVol = Math.max(0.0, Math.min(1.5, strip.trackVol + delta))
                                            engine.setTrackVolume(strip.trackIdx, nextVol)
                                        }
                                    }
                                }
                            }

                            // Dual Stereo VU Peak Level Meter (Wide 12px)
                            Rectangle {
                                width: 12
                                Layout.fillHeight: true
                                color: theme.bgWell
                                radius: 2
                                border.color: theme.border

                                Row {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    spacing: 2

                                    // Left Peak Meter Bar
                                    Rectangle {
                                        width: 4
                                        height: parent.height
                                        color: "transparent"

                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            width: parent.width
                                            height: Math.min(parent.height, parent.height * Math.min(1.0, strip.peakL * 1.3))
                                            color: strip.peakL > 0.95 ? theme.stopAccent : (strip.peakL > 0.75 ? theme.pauseAccent : theme.playAccent)
                                            radius: 1
                                        }
                                    }

                                    // Right Peak Meter Bar
                                    Rectangle {
                                        width: 4
                                        height: parent.height
                                        color: "transparent"

                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            width: parent.width
                                            height: Math.min(parent.height, parent.height * Math.min(1.0, strip.peakR * 1.3))
                                            color: strip.peakR > 0.95 ? theme.stopAccent : (strip.peakR > 0.75 ? theme.pauseAccent : theme.playAccent)
                                            radius: 1
                                        }
                                    }
                                }
                            }
                        }

                        // 7. Decibel Volume Label
                        Text {
                            Layout.fillWidth: true
                            text: root.gainToDb(strip.trackVol)
                            font.pixelSize: 10
                            font.bold: true
                            font.family: "Monospace"
                            color: strip.trackVol > 1.05 ? theme.pauseAccent : theme.textMain
                            horizontalAlignment: Text.AlignHCenter
                        }

                        // 8. Edit Instrument Button (Opens pop-up editor)
                        Button {
                            Layout.fillWidth: true
                            implicitHeight: 24
                            text: "⚙ EDIT SOUND"
                            font.bold: true
                            font.pixelSize: 10
                            background: Rectangle {
                                color: parent.hovered ? theme.accent : theme.btnBg
                                border.color: theme.border
                                radius: 3
                            }
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.parent.hovered ? "#ffffff" : theme.accent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                if (engine) engine.setSelectedInstrument(strip.trackIdx)
                                root.requestEditInstrument(strip.trackIdx, strip.trackType)
                            }
                        }
                    }
                }
            }

            // "+ ADD CHANNEL" Card at end of tracks
            Rectangle {
                width: 70
                height: channelRow.height
                color: addChannelMouse.containsMouse ? theme.btnHover : theme.bgWell
                radius: 4
                border.color: theme.border
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: 6
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "+"
                        font.bold: true
                        font.pixelSize: 22
                        color: addChannelMouse.containsMouse ? theme.accent : theme.textMuted
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "ADD\nTRACK"
                        horizontalAlignment: Text.AlignHCenter
                        font.bold: true
                        font.pixelSize: 9
                        color: addChannelMouse.containsMouse ? theme.accent : theme.textDim
                    }
                }

                MouseArea {
                    id: addChannelMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (engine) engine.addTrack()
                    }
                }
            }

            // Vertical Divider between Tracks and Master
            Rectangle {
                width: 2
                height: channelRow.height
                color: theme.border
            }

            // ==========================================
            // MASTER CHANNEL STRIP (Spacious 160px wide)
            // ==========================================
            Rectangle {
                id: masterStrip
                width: 160
                height: channelRow.height
                color: theme.bgWell
                radius: 4
                border.color: theme.borderLight
                border.width: 1

                property real masterVol: engine ? engine.masterVolume : 0.85
                property real masterPeakL: 0.0
                property real masterPeakR: 0.0

                Connections {
                    target: engine
                    function onMetersUpdated(pL, pR, rms) {
                        masterStrip.masterPeakL = pL
                        masterStrip.masterPeakR = pR
                    }
                    function onMacrosChanged() {
                        masterStrip.masterVol = engine ? engine.masterVolume : 0.85
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    // Master Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        color: theme.bgTrackHeader
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: "MASTER BUS"
                            font.bold: true
                            font.pixelSize: 11
                            color: theme.textHeading
                        }
                    }

                    // Soft Limiter / Output Peak Status
                    Rectangle {
                        Layout.fillWidth: true
                        height: 22
                        color: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? theme.stopAccent : theme.bgWellHover
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? "LIMITING ACTIVE" : "CLEAN OUTPUT"
                            font.bold: true
                            font.pixelSize: 9
                            color: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? "#ffffff" : theme.playAccent
                        }
                    }

                    // Master Volume Fader & Dual Stereo Level Meters
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 110
                        spacing: 6

                        Item {
                            id: masterFaderArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Rectangle {
                                id: masterFaderGroove
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 6
                                height: parent.height
                                radius: 3
                                color: theme.bgWell

                                // Unity Gain 0dB marker
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: Math.round((1.0 - (1.0 / 1.5)) * (masterFaderGroove.height - 20) + 10)
                                    width: 24
                                    height: 2
                                    color: theme.playAccent
                                    opacity: 0.7
                                }
                            }

                            Rectangle {
                                id: masterFaderCap
                                width: 38
                                height: 20
                                radius: 3
                                anchors.horizontalCenter: masterFaderGroove.horizontalCenter
                                y: {
                                    let trackH = Math.max(1, masterFaderArea.height - height)
                                    let norm = Math.max(0.0, Math.min(1.5, masterStrip.masterVol)) / 1.5
                                    return Math.round((1.0 - norm) * trackH)
                                }
                                color: masterFaderMouse.pressed ? "#ffffff" : theme.btnHover
                                border.color: masterFaderMouse.pressed ? theme.playAccent : theme.borderLight
                                border.width: 1

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width - 8
                                    height: 2
                                    color: masterFaderMouse.pressed ? theme.playAccent : theme.borderLight
                                }
                            }

                            MouseArea {
                                id: masterFaderMouse
                                anchors.fill: parent
                                cursorShape: Qt.SizeVerCursor
                                hoverEnabled: true
                                preventStealing: true

                                property real startDragY: 0
                                property real startVol: 0.85

                                function updateFromPos(mouseY) {
                                    let trackH = Math.max(1, masterFaderArea.height - masterFaderCap.height)
                                    let clampedY = Math.max(0, Math.min(trackH, mouseY - masterFaderCap.height / 2))
                                    let norm = 1.0 - (clampedY / trackH)
                                    let newVol = norm * 1.5
                                    if (Math.abs(newVol - 1.0) < 0.04) newVol = 1.0
                                    if (engine) engine.setMasterVolume(newVol)
                                }

                                onPressed: (mouse) => {
                                    let capY = masterFaderCap.y
                                    if (mouse.y >= capY && mouse.y <= capY + masterFaderCap.height) {
                                        startDragY = mouse.y
                                        startVol = masterStrip.masterVol
                                    } else {
                                        updateFromPos(mouse.y)
                                        startDragY = mouse.y
                                        startVol = masterStrip.masterVol
                                    }
                                }

                                onPositionChanged: (mouse) => {
                                    if (pressed) {
                                        let trackH = Math.max(1, masterFaderArea.height - masterFaderCap.height)
                                        let dy = mouse.y - startDragY
                                        let dVol = -(dy / trackH) * 1.5
                                        let newVol = Math.max(0.0, Math.min(1.5, startVol + dVol))
                                        if (Math.abs(newVol - 1.0) < 0.04) newVol = 1.0
                                        if (engine) engine.setMasterVolume(newVol)
                                    }
                                }

                                onDoubleClicked: { if (engine) engine.setMasterVolume(1.0) }
                                onWheel: (wheel) => {
                                    if (engine) {
                                        let delta = (wheel.angleDelta.y > 0 ? 0.05 : -0.05)
                                        let nextVol = Math.max(0.0, Math.min(1.5, masterStrip.masterVol + delta))
                                        engine.setMasterVolume(nextVol)
                                    }
                                }
                            }
                        }

                        // Master Dual Stereo Level Meter (14px wide)
                        Rectangle {
                            width: 14
                            Layout.fillHeight: true
                            color: theme.bgWell
                            radius: 2
                            border.color: theme.border

                            Row {
                                anchors.fill: parent
                                anchors.margins: 1
                                spacing: 2

                                Rectangle {
                                    width: 5
                                    height: parent.height
                                    color: "transparent"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: Math.min(parent.height, parent.height * Math.min(1.0, masterStrip.masterPeakL * 1.3))
                                        color: masterStrip.masterPeakL > 0.95 ? theme.stopAccent : (masterStrip.masterPeakL > 0.75 ? theme.pauseAccent : theme.playAccent)
                                        radius: 1
                                    }
                                }

                                Rectangle {
                                    width: 5
                                    height: parent.height
                                    color: "transparent"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: Math.min(parent.height, parent.height * Math.min(1.0, masterStrip.masterPeakR * 1.3))
                                        color: masterStrip.masterPeakR > 0.95 ? theme.stopAccent : (masterStrip.masterPeakR > 0.75 ? theme.pauseAccent : theme.playAccent)
                                        radius: 1
                                    }
                                }
                            }
                        }
                    }

                    // Master dB readout
                    Text {
                        Layout.fillWidth: true
                        text: root.gainToDb(masterStrip.masterVol)
                        font.pixelSize: 10
                        font.bold: true
                        font.family: "Monospace"
                        color: theme.textHeading
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // Master Unity Reset Button
                    Button {
                        Layout.fillWidth: true
                        implicitHeight: 24
                        text: "UNITY 0.0 dB"
                        font.bold: true
                        font.pixelSize: 9
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
                        onClicked: if (engine) engine.setMasterVolume(1.0)
                    }
                }
            }
        }
    }
}
