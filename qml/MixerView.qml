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

    // Helper: decibels from linear gain (0..2.0)
    function gainToDb(gain) {
        if (gain <= 0.0001) return "-inf"
        let db = 20.0 * Math.log10(gain)
        if (db >= 0.0) return "+" + db.toFixed(1) + " dB"
        return db.toFixed(1) + " dB"
    }

    // Helper: pan label (0..1.0)
    function panToString(pan) {
        if (Math.abs(pan - 0.5) < 0.03) return "C"
        if (pan < 0.5) {
            let pct = Math.round((0.5 - pan) * 200)
            return "L " + pct + "%"
        } else {
            let pct = Math.round((pan - 0.5) * 200)
            return "R " + pct + "%"
        }
    }

    Timer {
        id: meterTimer
        interval: 30
        running: root.visible && engine && engine.isPlaying
        repeat: true
        onTriggered: {
            // Trigger repaints of track VU meters
            meterRepeater.model = 0
            meterRepeater.model = engine ? engine.numTracks : 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        // Top Toolbar of Mixer
        RowLayout {
            Layout.fillWidth: true
            height: 24
            spacing: 8

            Text {
                text: "TRACK MIXER"
                font.bold: true
                font.pixelSize: 11
                color: theme.textHeading
            }

            Rectangle { width: 1; height: 14; color: theme.border }

            Text {
                text: (engine ? engine.numTracks : 1) + " Tracks Active"
                font.pixelSize: 10
                color: theme.textMuted
            }

            Item { Layout.fillWidth: true }

            Button {
                implicitHeight: 22
                implicitWidth: 84
                text: "+ Add Track"
                font.pixelSize: 10
                background: Rectangle {
                    color: parent.hovered ? theme.btnHover : theme.btnBg
                    border.color: theme.border
                    radius: 2
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: theme.accent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (engine) engine.addTrack()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: theme.border
        }

        // Horizontal Channel Strips
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Row {
                id: channelRow
                spacing: 6
                height: parent.height

                Repeater {
                    id: meterRepeater
                    model: engine ? engine.numTracks : 1

                    // --- INDIVIDUAL TRACK CHANNEL STRIP ---
                    Rectangle {
                        id: strip
                        width: 122
                        height: channelRow.height
                        color: (engine && engine.selectedInstrument === index) ? theme.bgWellHover : theme.bgWell
                        radius: 3
                        border.color: (engine && engine.selectedInstrument === index) ? theme.accent : theme.border
                        border.width: (engine && engine.selectedInstrument === index) ? 2 : 1

                        property int trackIdx: index
                        property int trackType: engine ? engine.getTrackType(trackIdx) : 0
                        property real trackVol: engine ? engine.getTrackVolume(trackIdx) : 1.0
                        property real trackPan: engine ? engine.getTrackPan(trackIdx) : 0.5
                        property bool trackMute: engine ? engine.getTrackMute(trackIdx) : false
                        property bool trackSolo: engine ? engine.getTrackSolo(trackIdx) : false
                        property int trackSf2: engine ? engine.getTrackSf2Preset(trackIdx) : 0

                        property var meters: (engine && engine.isPlaying) ? engine.getTrackMeters(trackIdx) : [0.0, 0.0]
                        property real peakL: meters && meters.length > 0 ? meters[0] : 0.0
                        property real peakR: meters && meters.length > 1 ? meters[1] : 0.0

                        Connections {
                            target: engine
                            function onTrackMixerChanged(t) {
                                if (t === strip.trackIdx) {
                                    strip.trackType = engine.getTrackType(strip.trackIdx)
                                    strip.trackVol = engine.getTrackVolume(strip.trackIdx)
                                    strip.trackPan = engine.getTrackPan(strip.trackIdx)
                                    strip.trackMute = engine.getTrackMute(strip.trackIdx)
                                    strip.trackSolo = engine.getTrackSolo(strip.trackIdx)
                                    strip.trackSf2 = engine.getTrackSf2Preset(strip.trackIdx)
                                }
                            }
                            function onTrackTypeChanged(t, type) {
                                if (t === strip.trackIdx) strip.trackType = type
                            }
                            function onProjectLoaded() {
                                strip.trackType = engine ? engine.getTrackType(strip.trackIdx) : 0
                                strip.trackVol = engine ? engine.getTrackVolume(strip.trackIdx) : 1.0
                                strip.trackPan = engine ? engine.getTrackPan(strip.trackIdx) : 0.5
                                strip.trackMute = engine ? engine.getTrackMute(strip.trackIdx) : false
                                strip.trackSolo = engine ? engine.getTrackSolo(strip.trackIdx) : false
                                strip.trackSf2 = engine ? engine.getTrackSf2Preset(strip.trackIdx) : 0
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 4

                            // 1. Header (Track Name & Number)
                            Rectangle {
                                Layout.fillWidth: true
                                height: 22
                                color: (engine && engine.selectedInstrument === strip.trackIdx) ? theme.accent : theme.bgTrackHeader
                                radius: 2

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    spacing: 2

                                    Text {
                                        Layout.fillWidth: true
                                        text: (engine ? engine.getInstrumentName(strip.trackIdx) : "Track " + (strip.trackIdx + 1))
                                        font.bold: true
                                        font.pixelSize: 10
                                        color: (engine && engine.selectedInstrument === strip.trackIdx) ? "#ffffff" : theme.textMain
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: "#" + (strip.trackIdx + 1)
                                        font.pixelSize: 9
                                        color: (engine && engine.selectedInstrument === strip.trackIdx) ? "#ffffff" : theme.textDim
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (engine) engine.setSelectedInstrument(strip.trackIdx)
                                    }
                                }
                            }

                            // 2. Instrument Engine Selector (SYNTH / SF2 / DRUMS)
                            Rectangle {
                                id: engineSelector
                                Layout.fillWidth: true
                                height: 20
                                color: engineMouse.containsMouse ? theme.btnHover : theme.btnBg
                                radius: 2
                                border.color: theme.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    spacing: 2

                                    Text {
                                        id: typeLabel
                                        Layout.fillWidth: true
                                        text: {
                                            if (strip.trackType === 1) return "SF2"
                                            if (strip.trackType === 2) return "DRUMS"
                                            return "SYNTH"
                                        }
                                        font.bold: true
                                        font.pixelSize: 9
                                        color: {
                                            if (strip.trackType === 1) return "#92ebb5"
                                            if (strip.trackType === 2) return "#f7b731"
                                            return theme.accent
                                        }
                                        horizontalAlignment: Text.AlignHCenter
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
                                    onClicked: {
                                        engineMenu.open()
                                    }
                                }

                                Menu {
                                    id: engineMenu
                                    MenuItem {
                                        text: "🎹 AuraSynth (Poly Subtractive)"
                                        onTriggered: {
                                            if (engine) engine.setTrackType(strip.trackIdx, 0)
                                        }
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
                                        onTriggered: {
                                            if (engine) engine.setTrackType(strip.trackIdx, 2)
                                        }
                                    }
                                }
                            }

                            // 3. SF2 Preset Selector (Visible only when SF2 is selected)
                            Rectangle {
                                Layout.fillWidth: true
                                height: 20
                                color: sf2Mouse.containsMouse ? theme.btnHover : theme.btnBg
                                radius: 2
                                border.color: theme.border
                                visible: (strip.trackType === 1)

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 3
                                    anchors.rightMargin: 3
                                    text: {
                                        if (!engine || !engine.sf2Loaded) return "No SF2 Loaded"
                                        let name = engine.getSoundFontPresetName(strip.trackSf2)
                                        return (strip.trackSf2 < 10 ? "0" + strip.trackSf2 : "" + strip.trackSf2) + ": " + (name ? name : "Preset")
                                    }
                                    font.pixelSize: 8
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
                                        sf2PresetPopup.open()
                                    }
                                }

                                Popup {
                                    id: sf2PresetPopup
                                    width: 220
                                    height: 240
                                    y: 22
                                    modal: true
                                    focus: true
                                    background: Rectangle {
                                        color: theme.bgCard
                                        border.color: theme.border
                                        radius: 4
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 4

                                        Text {
                                            text: "Select SoundFont Preset"
                                            font.bold: true
                                            font.pixelSize: 10
                                            color: theme.textHeading
                                        }

                                        ScrollView {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            clip: true

                                            ListView {
                                                id: sf2List
                                                width: parent.width
                                                model: engine ? engine.sf2PresetCount : 0
                                                delegate: Rectangle {
                                                    width: sf2List.width
                                                    height: 20
                                                    color: (strip.trackSf2 === index) ? theme.accent : (pItemMouse.containsMouse ? theme.btnHover : "transparent")
                                                    radius: 2

                                                    Text {
                                                        anchors.fill: parent
                                                        anchors.leftMargin: 6
                                                        text: (index < 10 ? "00" + index : (index < 100 ? "0" + index : "" + index)) + ": " + (engine ? engine.getSoundFontPresetName(index) : "")
                                                        font.pixelSize: 9
                                                        color: (strip.trackSf2 === index) ? "#ffffff" : theme.textMain
                                                        verticalAlignment: Text.AlignVCenter
                                                        elide: Text.ElideRight
                                                    }

                                                    MouseArea {
                                                        id: pItemMouse
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        onClicked: {
                                                            if (engine) engine.setTrackSf2Preset(strip.trackIdx, index)
                                                            sf2PresetPopup.close()
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // 4. Pan Control (Compact Knob / Slider)
                            RowLayout {
                                Layout.fillWidth: true
                                height: 18
                                spacing: 2

                                Text {
                                    text: "PAN"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: theme.textDim
                                }

                                Slider {
                                    id: panSlider
                                    Layout.fillWidth: true
                                    from: 0.0
                                    to: 1.0
                                    value: strip.trackPan
                                    onMoved: {
                                        if (engine) engine.setTrackPan(strip.trackIdx, value)
                                    }

                                    background: Rectangle {
                                        x: panSlider.leftPadding
                                        y: panSlider.topPadding + panSlider.availableHeight / 2 - height / 2
                                        width: panSlider.availableWidth
                                        height: 3
                                        radius: 1
                                        color: theme.border

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 2
                                            height: 7
                                            color: theme.textMuted
                                        }
                                    }

                                    handle: Rectangle {
                                        x: panSlider.leftPadding + panSlider.visualPosition * (panSlider.availableWidth - width)
                                        y: panSlider.topPadding + panSlider.availableHeight / 2 - height / 2
                                        width: 8
                                        height: 12
                                        radius: 1
                                        color: panSlider.pressed ? "#ffffff" : theme.accent
                                        border.color: theme.borderLight
                                    }
                                }

                                Text {
                                    text: root.panToString(strip.trackPan)
                                    font.pixelSize: 8
                                    font.family: "Monospace"
                                    color: theme.textMuted
                                    width: 32
                                    horizontalAlignment: Text.AlignRight
                                }
                            }

                            // 5. Mute & Solo Buttons
                            RowLayout {
                                Layout.fillWidth: true
                                height: 20
                                spacing: 4

                                // Mute Button
                                Button {
                                    Layout.fillWidth: true
                                    implicitHeight: 20
                                    checkable: true
                                    checked: strip.trackMute
                                    onClicked: {
                                        if (engine) engine.setTrackMute(strip.trackIdx, !strip.trackMute)
                                    }
                                    background: Rectangle {
                                        color: strip.trackMute ? theme.pauseAccent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                        border.color: strip.trackMute ? theme.pauseAccent : theme.border
                                        radius: 2
                                    }
                                    contentItem: Text {
                                        text: "M"
                                        font.bold: true
                                        font.pixelSize: 9
                                        color: strip.trackMute ? "#ffffff" : theme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                // Solo Button
                                Button {
                                    Layout.fillWidth: true
                                    implicitHeight: 20
                                    checkable: true
                                    checked: strip.trackSolo
                                    onClicked: {
                                        if (engine) engine.setTrackSolo(strip.trackIdx, !strip.trackSolo)
                                    }
                                    background: Rectangle {
                                        color: strip.trackSolo ? theme.accent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                        border.color: strip.trackSolo ? theme.accent : theme.border
                                        radius: 2
                                    }
                                    contentItem: Text {
                                        text: "S"
                                        font.bold: true
                                        font.pixelSize: 9
                                        color: strip.trackSolo ? "#ffffff" : theme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            // 6. Volume Fader & Live Peak Meter
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 4

                                // Vertical Volume Fader
                                Slider {
                                    id: volSlider
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    orientation: Qt.Vertical
                                    from: 0.0
                                    to: 1.5
                                    value: strip.trackVol
                                    onMoved: {
                                        if (engine) engine.setTrackVolume(strip.trackIdx, value)
                                    }

                                    background: Rectangle {
                                        x: volSlider.leftPadding + volSlider.availableWidth / 2 - width / 2
                                        y: volSlider.topPadding
                                        width: 4
                                        height: volSlider.availableHeight
                                        radius: 2
                                        color: theme.border

                                        // 0dB unity gain reference marker
                                        Rectangle {
                                            y: volSlider.topPadding + (1.0 - (1.0 / 1.5)) * volSlider.availableHeight - 1
                                            x: -4
                                            width: 12
                                            height: 2
                                            color: theme.textMuted
                                        }
                                    }

                                    handle: Rectangle {
                                        x: volSlider.leftPadding + volSlider.availableWidth / 2 - width / 2
                                        y: volSlider.topPadding + (1.0 - volSlider.visualPosition) * (volSlider.availableHeight - height)
                                        width: 18
                                        height: 10
                                        radius: 2
                                        color: volSlider.pressed ? "#ffffff" : (strip.trackMute ? theme.textDim : theme.accent)
                                        border.color: theme.borderLight
                                    }
                                }

                                // Dual Stereo VU Peak Level Meter
                                Rectangle {
                                    width: 10
                                    Layout.fillHeight: true
                                    color: theme.bgWell
                                    radius: 1
                                    border.color: theme.border

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 1
                                        spacing: 1

                                        // Left Peak Meter Bar
                                        Rectangle {
                                            width: 3
                                            height: parent.height
                                            color: "transparent"

                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                width: parent.width
                                                height: Math.min(parent.height, parent.height * Math.min(1.0, strip.peakL * 1.3))
                                                color: strip.peakL > 0.95 ? theme.stopAccent : (strip.peakL > 0.75 ? theme.pauseAccent : theme.playAccent)
                                            }
                                        }

                                        // Right Peak Meter Bar
                                        Rectangle {
                                            width: 3
                                            height: parent.height
                                            color: "transparent"

                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                width: parent.width
                                                height: Math.min(parent.height, parent.height * Math.min(1.0, strip.peakR * 1.3))
                                                color: strip.peakR > 0.95 ? theme.stopAccent : (strip.peakR > 0.75 ? theme.pauseAccent : theme.playAccent)
                                            }
                                        }
                                    }
                                }
                            }

                            // 7. Gain dB readout
                            Text {
                                Layout.fillWidth: true
                                text: root.gainToDb(strip.trackVol)
                                font.pixelSize: 8
                                font.family: "Monospace"
                                color: strip.trackVol > 1.05 ? theme.pauseAccent : theme.textMuted
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // 8. Edit Instrument Button
                            Button {
                                Layout.fillWidth: true
                                implicitHeight: 22
                                text: "⚙ EDIT"
                                font.bold: true
                                font.pixelSize: 9
                                background: Rectangle {
                                    color: parent.hovered ? theme.btnHover : theme.btnBg
                                    border.color: theme.border
                                    radius: 2
                                }
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: theme.accent
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

                // Vertical Divider between Tracks and Master
                Rectangle {
                    width: 1
                    height: channelRow.height
                    color: theme.border
                }

                // ==========================================
                // MASTER CHANNEL STRIP
                // ==========================================
                Rectangle {
                    id: masterStrip
                    width: 130
                    height: channelRow.height
                    color: theme.bgWell
                    radius: 3
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
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        // Master Header
                        Rectangle {
                            Layout.fillWidth: true
                            height: 22
                            color: theme.bgTrackHeader
                            radius: 2

                            Text {
                                anchors.centerIn: parent
                                text: "MASTER"
                                font.bold: true
                                font.pixelSize: 10
                                color: theme.textHeading
                            }
                        }

                        // Soft Limiter / Peak Status
                        Rectangle {
                            Layout.fillWidth: true
                            height: 16
                            color: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? theme.stopAccent : theme.bgWellHover
                            radius: 2

                            Text {
                                anchors.centerIn: parent
                                text: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? "LIMITING" : "CLEAN"
                                font.bold: true
                                font.pixelSize: 8
                                color: (masterStrip.masterPeakL > 0.98 || masterStrip.masterPeakR > 0.98) ? "#ffffff" : theme.playAccent
                            }
                        }

                        // Master Volume Fader & Level Meters
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            Slider {
                                id: masterSlider
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                orientation: Qt.Vertical
                                from: 0.0
                                to: 1.5
                                value: masterStrip.masterVol
                                onMoved: {
                                    if (engine) engine.setMasterVolume(value)
                                }

                                background: Rectangle {
                                    x: masterSlider.leftPadding + masterSlider.availableWidth / 2 - width / 2
                                    y: masterSlider.topPadding
                                    width: 4
                                    height: masterSlider.availableHeight
                                    radius: 2
                                    color: theme.border

                                    Rectangle {
                                        y: masterSlider.topPadding + (1.0 - (1.0 / 1.5)) * masterSlider.availableHeight - 1
                                        x: -4
                                        width: 12
                                        height: 2
                                        color: theme.textMuted
                                    }
                                }

                                handle: Rectangle {
                                    x: masterSlider.leftPadding + masterSlider.availableWidth / 2 - width / 2
                                    y: masterSlider.topPadding + (1.0 - masterSlider.visualPosition) * (masterSlider.availableHeight - height)
                                    width: 18
                                    height: 10
                                    radius: 2
                                    color: masterSlider.pressed ? "#ffffff" : theme.playAccent
                                    border.color: theme.borderLight
                                }
                            }

                            // Master Stereo VU Level Meter
                            Rectangle {
                                width: 14
                                Layout.fillHeight: true
                                color: theme.bgWell
                                radius: 1
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
                                        }
                                    }
                                }
                            }
                        }

                        // Master dB readout
                        Text {
                            Layout.fillWidth: true
                            text: root.gainToDb(masterStrip.masterVol)
                            font.pixelSize: 8
                            font.family: "Monospace"
                            color: theme.textMain
                            horizontalAlignment: Text.AlignHCenter
                        }

                        // Master Reset unity 0dB
                        Button {
                            Layout.fillWidth: true
                            implicitHeight: 22
                            text: "UNITY (0dB)"
                            font.bold: true
                            font.pixelSize: 8
                            background: Rectangle {
                                color: parent.hovered ? theme.btnHover : theme.btnBg
                                border.color: theme.border
                                radius: 2
                            }
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: theme.textMuted
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                if (engine) engine.setMasterVolume(1.0)
                            }
                        }
                    }
                }
            }
        }
    }
}
