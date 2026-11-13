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

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        Column {
            Layout.alignment: Qt.AlignVCenter
            spacing: 1
            Text {
                text: "SMART MACROS"
                font.bold: true
                font.pixelSize: 10
                font.letterSpacing: 0.8
                color: theme.accent
            }
            Text {
                text: "Auto-Mastering"
                font.pixelSize: 9
                color: theme.textMuted
            }
        }

        Rectangle {
            width: 1
            height: 42
            color: theme.border
            Layout.alignment: Qt.AlignVCenter
        }

        BreezeKnob {
            id: punchKnob
            label: "PUNCH"
            value: engine ? engine.macroPunch : 0.20
            minimumValue: 0.0
            maximumValue: 1.0
            displayValue: Math.round(internalValue * 100) + "%"
            activeColor: "#e67e22"
            onValueModified: (v) => engine.setMacroPunch(v)
        }

        BreezeKnob {
            id: warmthKnob
            label: "WARMTH"
            value: engine ? engine.macroWarmth : 0.25
            minimumValue: 0.0
            maximumValue: 1.0
            displayValue: Math.round(internalValue * 100) + "%"
            activeColor: "#e74c3c"
            onValueModified: (v) => engine.setMacroWarmth(v)
        }

        BreezeKnob {
            id: toneKnob
            label: "TONE"
            value: engine ? engine.macroTone : 0.50
            minimumValue: 0.0
            maximumValue: 1.0
            displayValue: (internalValue < 0.48 ? "Warm" : (internalValue > 0.52 ? "Air" : "Flat"))
            activeColor: theme.accent
            onValueModified: (v) => engine.setMacroTone(v)
        }

        BreezeKnob {
            id: spaceKnob
            label: "SPACE"
            value: engine ? engine.macroSpace : 0.15
            minimumValue: 0.0
            maximumValue: 1.0
            displayValue: Math.round(internalValue * 100) + "%"
            activeColor: "#1abc9c"
            onValueModified: (v) => engine.setMacroSpace(v)
        }

        Rectangle {
            width: 1
            height: 42
            color: theme.border
            Layout.alignment: Qt.AlignVCenter
        }

        BreezeKnob {
            id: masterKnob
            label: "MASTER"
            value: engine ? engine.masterVolume : 0.85
            minimumValue: 0.0
            maximumValue: 1.5
            displayValue: (Math.round((internalValue - 1.0) * 12) + " dB")
            activeColor: theme.accent
            onValueModified: (v) => engine.setMasterVolume(v)
        }

        Item { Layout.fillWidth: true }
    }

    Connections {
        target: engine
        function onMacrosChanged() {
            if (engine) {
                punchKnob.value = engine.macroPunch
                warmthKnob.value = engine.macroWarmth
                toneKnob.value = engine.macroTone
                spaceKnob.value = engine.macroSpace
                masterKnob.value = engine.masterVolume
            }
        }
        function onProjectLoaded() {
            if (engine) {
                punchKnob.value = engine.macroPunch
                warmthKnob.value = engine.macroWarmth
                toneKnob.value = engine.macroTone
                spaceKnob.value = engine.macroSpace
                masterKnob.value = engine.masterVolume
            }
        }
    }
}
