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
    property int currentInstrument: engine ? engine.selectedInstrument : 0
    property bool isSyncing: false

    property int osc1Wave: 0
    property int osc2Wave: 1
    property real oscMix: 0.45
    property real detuneSemi: 0.0
    property real detuneCents: 8.0

    property real filterCutoff: 3500.0
    property real filterRes: 0.35
    property real filterEnv: 0.45

    property real ampA: 0.005
    property real ampD: 0.25
    property real ampS: 0.65
    property real ampR: 0.25

    function loadFromEngine() {
        if (!engine) return
        isSyncing = true
        let p = engine.getInstParams(currentInstrument)
        root.osc1Wave = p.osc1
        root.osc2Wave = p.osc2
        root.oscMix = p.mix
        root.detuneSemi = p.detuneSemi
        root.detuneCents = p.detuneCents
        root.filterCutoff = p.cutoff
        root.filterRes = p.resonance
        root.filterEnv = p.envAmount
        root.ampA = p.ampA
        root.ampD = p.ampD
        root.ampS = p.ampS
        root.ampR = p.ampR
        nameInput.text = engine.getInstrumentName(currentInstrument)
        isSyncing = false
    }

    Component.onCompleted: loadFromEngine()

    Connections {
        target: engine
        function onInstrumentChanged(inst) {
            root.currentInstrument = inst
            root.loadFromEngine()
        }
        function onProjectLoaded() {
            root.currentInstrument = engine ? engine.selectedInstrument : 0
            root.loadFromEngine()
        }
    }

    function applySynth() {
        if (!engine || isSyncing) return
        engine.setInstOsc1(currentInstrument, osc1Wave)
        engine.setInstOsc2(currentInstrument, osc2Wave)
        engine.setInstOscMix(currentInstrument, oscMix)
        engine.setInstDetune(currentInstrument, detuneSemi, detuneCents)
        engine.setInstFilter(currentInstrument, filterCutoff, filterRes, filterEnv)
        engine.setInstAmpEnv(currentInstrument, ampA, ampD, ampS, ampR)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // --- ACTIVE TRACK INSTRUMENT CARD ---
        ColumnLayout {
            spacing: 6
            Layout.preferredWidth: 125
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "TRACK " + (root.currentInstrument + 1)
                    font.bold: true
                    font.pixelSize: 10
                    font.letterSpacing: 0.8
                    color: theme.accent
                }

                Item { Layout.fillWidth: true }

                // Quick Add Track Button
                Rectangle {
                    width: 18
                    height: 18
                    radius: 2
                    color: addTrackQuickMouse.containsMouse ? theme.btnHover : theme.btnBg
                    border.color: theme.border
                    Text { anchors.centerIn: parent; text: "+"; font.bold: true; color: theme.accent; font.pixelSize: 11 }
                    MouseArea {
                        id: addTrackQuickMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (engine) {
                                engine.addTrack()
                                engine.setSelectedInstrument(engine.numTracks - 1)
                            }
                        }
                    }
                }
            }

            // Custom User Name Input Field
            Rectangle {
                Layout.fillWidth: true
                height: 24
                color: theme.bgWell
                radius: 3
                border.color: nameInput.activeFocus ? theme.accent : theme.border

                TextInput {
                    id: nameInput
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    verticalAlignment: TextInput.AlignVCenter
                    font.pixelSize: 11
                    font.bold: true
                    color: theme.textMain
                    selectByMouse: true
                    onTextChanged: {
                        if (!root.isSyncing && engine) {
                            engine.setInstrumentName(root.currentInstrument, text)
                        }
                    }
                }
            }

            // Track Switcher Tabs (Only existing tracks, no predefined list!)
            Row {
                spacing: 2
                Layout.fillWidth: true
                Repeater {
                    model: engine ? engine.numTracks : 1
                    Rectangle {
                        width: Math.max(22, (125 - ((engine ? engine.numTracks : 1) - 1) * 2) / (engine ? engine.numTracks : 1))
                        height: 20
                        radius: 2
                        color: (root.currentInstrument === index) ? theme.accent : (tabMouse.containsMouse ? theme.btnHover : theme.btnBg)
                        border.color: (root.currentInstrument === index) ? theme.accent : theme.border
                        Text {
                            anchors.centerIn: parent
                            text: "T" + (index + 1)
                            font.pixelSize: 9
                            font.bold: (root.currentInstrument === index)
                            color: (root.currentInstrument === index) ? "#ffffff" : theme.textMuted
                        }
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: engine.setSelectedInstrument(index)
                        }
                    }
                }
            }

            // Init Patch Button
            Rectangle {
                Layout.fillWidth: true
                height: 20
                radius: 2
                color: initMouse.containsMouse ? theme.btnHover : theme.btnBg
                border.color: theme.border

                Text {
                    anchors.centerIn: parent
                    text: "Init Patch"
                    font.pixelSize: 9
                    color: theme.textMuted
                }

                MouseArea {
                    id: initMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.osc1Wave = 0
                        root.osc2Wave = 1
                        root.oscMix = 0.45
                        root.detuneSemi = 0.0
                        root.detuneCents = 8.0
                        root.filterCutoff = 3500.0
                        root.filterRes = 0.35
                        root.filterEnv = 0.40
                        root.ampA = 0.005
                        root.ampD = 0.25
                        root.ampS = 0.65
                        root.ampR = 0.25
                        root.applySynth()
                    }
                }
            }
        }

        Rectangle { width: 1; height: parent.height - 8; color: theme.border; Layout.alignment: Qt.AlignVCenter }

        // --- OSCILLATORS SECTION (Compact) ---
        ColumnLayout {
            spacing: 4
            Text {
                text: "DUAL OSCILLATORS"
                font.bold: true
                font.pixelSize: 10
                font.letterSpacing: 0.6
                color: theme.accent
            }

            RowLayout {
                spacing: 8
                Column {
                    spacing: 2
                    Text { text: "OSC 1"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                    Row {
                        spacing: 2
                        Repeater {
                            model: ["SAW", "SQR", "TRI", "SIN", "NOI"]
                            Rectangle {
                                width: 25
                                height: 18
                                radius: 2
                                color: (root.osc1Wave === index) ? theme.accent : theme.btnBg
                                border.color: (root.osc1Wave === index) ? theme.accent : theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: (root.osc1Wave === index) ? "#ffffff" : theme.textMuted
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: { root.osc1Wave = index; root.applySynth() }
                                }
                            }
                        }
                    }
                }

                Column {
                    spacing: 2
                    Text { text: "OSC 2"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                    Row {
                        spacing: 2
                        Repeater {
                            model: ["SAW", "SQR", "TRI", "SIN", "NOI"]
                            Rectangle {
                                width: 25
                                height: 18
                                radius: 2
                                color: (root.osc2Wave === index) ? theme.accent : theme.btnBg
                                border.color: (root.osc2Wave === index) ? theme.accent : theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: (root.osc2Wave === index) ? "#ffffff" : theme.textMuted
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: { root.osc2Wave = index; root.applySynth() }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                spacing: 6
                BreezeKnob {
                    label: "MIX"
                    value: root.oscMix
                    displayValue: Math.round(internalValue * 100) + "%"
                    onValueModified: (v) => { root.oscMix = v; root.applySynth() }
                }
                BreezeKnob {
                    label: "OCT"
                    value: (root.detuneSemi + 24.0) / 48.0
                    displayValue: (root.detuneSemi > 0 ? "+" : "") + Math.round(root.detuneSemi) + "st"
                    onValueModified: (v) => {
                        root.detuneSemi = Math.round((v * 48.0) - 24.0)
                        displayValue = (root.detuneSemi > 0 ? "+" : "") + Math.round(root.detuneSemi) + "st"
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "DETUNE"
                    value: (root.detuneCents + 25.0) / 50.0
                    displayValue: Math.round(root.detuneCents) + "c"
                    onValueModified: (v) => {
                        root.detuneCents = (v - 0.5) * 50.0
                        displayValue = Math.round(root.detuneCents) + "c"
                        root.applySynth()
                    }
                }
            }
        }

        Rectangle { width: 1; height: parent.height - 8; color: theme.border; Layout.alignment: Qt.AlignVCenter }

        // --- FILTER SECTION (Compact) ---
        ColumnLayout {
            spacing: 4
            Text {
                text: "ZDF FILTER"
                font.bold: true
                font.pixelSize: 10
                font.letterSpacing: 0.6
                color: theme.colInst
            }

            RowLayout {
                spacing: 6
                BreezeKnob {
                    label: "CUTOFF"
                    activeColor: theme.colInst
                    value: Math.pow(Math.max(0, root.filterCutoff - 80.0) / 16000.0, 1.0 / 2.5)
                    displayValue: Math.round(root.filterCutoff) + " Hz"
                    onValueModified: (v) => {
                        root.filterCutoff = 80.0 + Math.pow(v, 2.5) * 16000.0
                        displayValue = Math.round(root.filterCutoff) + " Hz"
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "RESONANCE"
                    activeColor: theme.colInst
                    value: root.filterRes
                    displayValue: Math.round(internalValue * 100) + "%"
                    onValueModified: (v) => {
                        root.filterRes = v
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "ENV DEPTH"
                    activeColor: theme.colInst
                    value: root.filterEnv
                    displayValue: Math.round(internalValue * 100) + "%"
                    onValueModified: (v) => {
                        root.filterEnv = v
                        root.applySynth()
                    }
                }
            }
        }

        Rectangle { width: 1; height: parent.height - 8; color: theme.border; Layout.alignment: Qt.AlignVCenter }

        // --- ENVELOPE SECTION (Compact) ---
        ColumnLayout {
            spacing: 4
            Text {
                text: "AMP ENVELOPE"
                font.bold: true
                font.pixelSize: 10
                font.letterSpacing: 0.6
                color: theme.colVol
            }

            RowLayout {
                spacing: 4
                BreezeKnob {
                    label: "ATTACK"
                    activeColor: theme.colVol
                    value: Math.sqrt(Math.max(0, root.ampA - 0.001) / 1.5)
                    displayValue: (root.ampA < 0.1 ? Math.round(root.ampA * 1000) + "ms" : (root.ampA.toFixed(2) + "s"))
                    onValueModified: (v) => {
                        root.ampA = 0.001 + Math.pow(v, 2) * 1.5
                        displayValue = (root.ampA < 0.1 ? Math.round(root.ampA * 1000) + "ms" : (root.ampA.toFixed(2) + "s"))
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "DECAY"
                    activeColor: theme.colVol
                    value: Math.sqrt(Math.max(0, root.ampD - 0.01) / 2.0)
                    displayValue: (root.ampD < 0.1 ? Math.round(root.ampD * 1000) + "ms" : (root.ampD.toFixed(2) + "s"))
                    onValueModified: (v) => {
                        root.ampD = 0.01 + Math.pow(v, 2) * 2.0
                        displayValue = (root.ampD < 0.1 ? Math.round(root.ampD * 1000) + "ms" : (root.ampD.toFixed(2) + "s"))
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "SUSTAIN"
                    activeColor: theme.colVol
                    value: root.ampS
                    displayValue: Math.round(internalValue * 100) + "%"
                    onValueModified: (v) => {
                        root.ampS = v
                        root.applySynth()
                    }
                }
                BreezeKnob {
                    label: "RELEASE"
                    activeColor: theme.colVol
                    value: Math.sqrt(Math.max(0, root.ampR - 0.01) / 3.0)
                    displayValue: (root.ampR < 0.1 ? Math.round(root.ampR * 1000) + "ms" : (root.ampR.toFixed(2) + "s"))
                    onValueModified: (v) => {
                        root.ampR = 0.01 + Math.pow(v, 2) * 3.0
                        displayValue = (root.ampR < 0.1 ? Math.round(root.ampR * 1000) + "ms" : (root.ampR.toFixed(2) + "s"))
                        root.applySynth()
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
