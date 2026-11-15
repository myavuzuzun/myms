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

    // Export Configuration State
    property string exportRange: "song" // "song" or "pattern"
    property string exportFormat: "wav" // "wav", "mp3", "ogg", "flac"
    property real tailSeconds: 2.0

    // Format specific options
    property int wavBitDepth: 24       // 16, 24, 32
    property int wavSampleRate: 48000   // 44100, 48000, 96000

    property int mp3Bitrate: 320       // 128, 192, 256, 320
    property int mp3SampleRate: 48000  // 44100, 48000

    property int oggQuality: 8         // 4, 6, 8, 10
    property int oggSampleRate: 48000  // 44100, 48000

    property int flacBitDepth: 24      // 16, 24
    property int flacCompression: 8    // 5, 8
    property int flacSampleRate: 48000 // 44100, 48000, 96000

    property string outputPath: ""
    property real exportProgress: 0.0
    property string statusText: "Ready to export."
    property bool isComplete: false
    property bool hasError: false
    property string lastExportedPath: ""

    function open() {
        if (!outputPath || outputPath === "") {
            if (engine) {
                outputPath = engine.getDefaultExportPath(exportFormat)
            } else {
                outputPath = "/tmp/untitled_export." + exportFormat
            }
        }
        exportProgress = 0.0
        statusText = "Ready to export."
        isComplete = false
        hasError = false
        isOpen = true
        root.forceActiveFocus()
    }

    function close() {
        if (engine && engine.isExporting) {
            return // Prevent closing while actively rendering
        }
        isOpen = false
    }

    function updateExtension(newFmt) {
        if (!outputPath || outputPath === "") {
            if (engine) outputPath = engine.getDefaultExportPath(newFmt)
            return
        }
        let dotIdx = outputPath.lastIndexOf(".")
        if (dotIdx > 0) {
            outputPath = outputPath.substring(0, dotIdx + 1) + newFmt
        } else {
            outputPath = outputPath + "." + newFmt
        }
    }

    function startExport() {
        if (!engine || engine.isExporting) return

        isComplete = false
        hasError = false
        exportProgress = 0.0
        statusText = "Preparing audio render..."

        let sampleRate = 48000
        if (exportFormat === "wav") sampleRate = wavSampleRate
        else if (exportFormat === "mp3") sampleRate = mp3SampleRate
        else if (exportFormat === "ogg") sampleRate = oggSampleRate
        else if (exportFormat === "flac") sampleRate = flacSampleRate

        let params = {
            "filepath": outputPath,
            "format": exportFormat,
            "song_mode": (exportRange === "song"),
            "pattern_idx": (engine ? engine.currentPattern : 0),
            "tail_seconds": tailSeconds,
            "sample_rate": sampleRate,
            "wav_bit_depth": wavBitDepth,
            "mp3_bitrate": mp3Bitrate,
            "ogg_quality": oggQuality,
            "flac_bit_depth": flacBitDepth,
            "flac_compression": flacCompression
        }

        engine.startExport(params)
    }

    Connections {
        target: engine
        function onExportProgressChanged(pct, msg) {
            root.exportProgress = Math.max(0.0, Math.min(1.0, pct))
            root.statusText = msg
        }
        function onExportFinished(success, msg) {
            if (success) {
                root.isComplete = true
                root.hasError = false
                root.exportProgress = 1.0
                root.lastExportedPath = msg
                root.statusText = "Export completed successfully!"
            } else {
                root.isComplete = false
                root.hasError = true
                root.statusText = "Export failed: " + msg
            }
        }
    }

    // Modal Scrim Backdrop
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.70

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Modal Dialog Window Card
    Rectangle {
        id: modalDialog
        anchors.centerIn: parent
        width: Math.min(parent.width - 24, 600)
        height: Math.min(parent.height - 24, 530)
        color: theme.bgCard
        radius: 6
        border.color: theme.borderLight
        border.width: 1
        clip: false

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            // ==========================================
            // 1. Header Title & Subtitle
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    width: 28
                    height: 28
                    radius: 4
                    color: theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "⤓"
                        font.bold: true
                        font.pixelSize: 14
                        color: "#ffffff"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: "AUDIO EXPORT (OFFLINE MIXDOWN)"
                        font.bold: true
                        font.pixelSize: 13
                        font.letterSpacing: 0.5
                        color: theme.textHeading
                    }
                    Text {
                        text: "Studio quality master mixdown for WAV, MP3, OGG Vorbis, and FLAC."
                        font.pixelSize: 10
                        color: theme.textMuted
                    }
                }

                // Active Rendering Indicator
                Rectangle {
                    visible: engine ? engine.isExporting : false
                    width: 90
                    height: 22
                    radius: 3
                    color: "#2c3e50"
                    border.color: theme.accent
                    Row {
                        anchors.centerIn: parent
                        spacing: 4
                        Text { text: "●"; color: theme.accent; font.pixelSize: 8 }
                        Text { text: "RENDERING"; color: "#ffffff"; font.pixelSize: 8; font.bold: true }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

            // ==========================================
            // 2. Export Mode / Range (Song vs Pattern)
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "RANGE:"
                    font.bold: true
                    font.pixelSize: 10
                    color: theme.textMuted
                    Layout.preferredWidth: 90
                }

                RowLayout {
                    spacing: 8

                    Rectangle {
                        width: 160
                        height: 26
                        radius: 3
                        color: (root.exportRange === "song") ? theme.accent : theme.bgWell
                        border.color: (root.exportRange === "song") ? theme.accentHover : theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            spacing: 6
                            Text {
                                text: (root.exportRange === "song") ? "◉" : "○"
                                font.pixelSize: 11
                                color: (root.exportRange === "song") ? "#ffffff" : theme.textMuted
                            }
                            Text {
                                text: "Song Mode (Arrangement)"
                                font.bold: true
                                font.pixelSize: 9
                                color: (root.exportRange === "song") ? "#ffffff" : theme.textMain
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !(engine && engine.isExporting)
                            onClicked: root.exportRange = "song"
                        }
                    }

                    Rectangle {
                        width: 150
                        height: 26
                        radius: 3
                        color: (root.exportRange === "pattern") ? theme.accent : theme.bgWell
                        border.color: (root.exportRange === "pattern") ? theme.accentHover : theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            spacing: 6
                            Text {
                                text: (root.exportRange === "pattern") ? "◉" : "○"
                                font.pixelSize: 11
                                color: (root.exportRange === "pattern") ? "#ffffff" : theme.textMuted
                            }
                            Text {
                                text: "Current Pattern (" + (engine ? (engine.currentPattern < 10 ? "0" + engine.currentPattern : "" + engine.currentPattern) : "00") + ")"
                                font.bold: true
                                font.pixelSize: 9
                                color: (root.exportRange === "pattern") ? "#ffffff" : theme.textMain
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !(engine && engine.isExporting)
                            onClicked: root.exportRange = "pattern"
                        }
                    }
                }
            }

            // ==========================================
            // 3. Audio Format Selector (WAV / MP3 / OGG / FLAC)
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "FORMAT:"
                    font.bold: true
                    font.pixelSize: 10
                    color: theme.textMuted
                    Layout.preferredWidth: 90
                }

                RowLayout {
                    spacing: 6

                    Repeater {
                        model: [
                            { id: "wav", label: "WAV", desc: "Uncompressed PCM" },
                            { id: "mp3", label: "MP3", desc: "LAME 320k" },
                            { id: "ogg", label: "OGG", desc: "Vorbis Q8" },
                            { id: "flac", label: "FLAC", desc: "Lossless Audio" }
                        ]

                        Rectangle {
                            width: 74
                            height: 26
                            radius: 3
                            color: (root.exportFormat === modelData.id) ? theme.accent : theme.bgWell
                            border.color: (root.exportFormat === modelData.id) ? theme.accentHover : theme.border

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                font.bold: true
                                font.pixelSize: 10
                                color: (root.exportFormat === modelData.id) ? "#ffffff" : theme.textMain
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                enabled: !(engine && engine.isExporting)
                                onClicked: {
                                    root.exportFormat = modelData.id
                                    root.updateExtension(modelData.id)
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // 4. Format-Specific Settings Container
            // ==========================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                radius: 4
                color: theme.bgWell
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // --- WAV Settings ---
                    RowLayout {
                        visible: root.exportFormat === "wav"
                        Layout.fillWidth: true
                        spacing: 16

                        RowLayout {
                            spacing: 6
                            Text { text: "Bit Depth:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 16, label: "16-bit PCM" },
                                    { val: 24, label: "24-bit PCM" },
                                    { val: 32, label: "32-bit Float" }
                                ]
                                Rectangle {
                                    width: 76
                                    height: 22
                                    radius: 2
                                    color: (root.wavBitDepth === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.wavBitDepth === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.wavBitDepth === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.wavBitDepth = modelData.val
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Sample Rate:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 44100, label: "44.1 kHz" },
                                    { val: 48000, label: "48.0 kHz" },
                                    { val: 96000, label: "96.0 kHz" }
                                ]
                                Rectangle {
                                    width: 62
                                    height: 22
                                    radius: 2
                                    color: (root.wavSampleRate === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.wavSampleRate === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.wavSampleRate === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.wavSampleRate = modelData.val
                                    }
                                }
                            }
                        }
                    }

                    // --- MP3 Settings ---
                    RowLayout {
                        visible: root.exportFormat === "mp3"
                        Layout.fillWidth: true
                        spacing: 16

                        RowLayout {
                            spacing: 6
                            Text { text: "Bitrate:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 128, label: "128 kbps" },
                                    { val: 192, label: "192 kbps" },
                                    { val: 256, label: "256 kbps" },
                                    { val: 320, label: "320 kbps" }
                                ]
                                Rectangle {
                                    width: 64
                                    height: 22
                                    radius: 2
                                    color: (root.mp3Bitrate === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.mp3Bitrate === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.mp3Bitrate === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.mp3Bitrate = modelData.val
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Sample Rate:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 44100, label: "44.1 kHz" },
                                    { val: 48000, label: "48.0 kHz" }
                                ]
                                Rectangle {
                                    width: 62
                                    height: 22
                                    radius: 2
                                    color: (root.mp3SampleRate === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.mp3SampleRate === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.mp3SampleRate === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.mp3SampleRate = modelData.val
                                    }
                                }
                            }
                        }
                    }

                    // --- OGG Settings ---
                    RowLayout {
                        visible: root.exportFormat === "ogg"
                        Layout.fillWidth: true
                        spacing: 16

                        RowLayout {
                            spacing: 6
                            Text { text: "Quality:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 4, label: "Q4 (~128k)" },
                                    { val: 6, label: "Q6 (~192k)" },
                                    { val: 8, label: "Q8 (~256k)" },
                                    { val: 10, label: "Q10 (~500k)" }
                                ]
                                Rectangle {
                                    width: 72
                                    height: 22
                                    radius: 2
                                    color: (root.oggQuality === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.oggQuality === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.oggQuality === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.oggQuality = modelData.val
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Sample Rate:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 44100, label: "44.1 kHz" },
                                    { val: 48000, label: "48.0 kHz" }
                                ]
                                Rectangle {
                                    width: 62
                                    height: 22
                                    radius: 2
                                    color: (root.oggSampleRate === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.oggSampleRate === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.oggSampleRate === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.oggSampleRate = modelData.val
                                    }
                                }
                            }
                        }
                    }

                    // --- FLAC Settings ---
                    RowLayout {
                        visible: root.exportFormat === "flac"
                        Layout.fillWidth: true
                        spacing: 14

                        RowLayout {
                            spacing: 6
                            Text { text: "Bit Depth:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 16, label: "16-bit" },
                                    { val: 24, label: "24-bit" }
                                ]
                                Rectangle {
                                    width: 54
                                    height: 22
                                    radius: 2
                                    color: (root.flacBitDepth === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.flacBitDepth === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.flacBitDepth === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.flacBitDepth = modelData.val
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Compression:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 5, label: "Level 5" },
                                    { val: 8, label: "Level 8" }
                                ]
                                Rectangle {
                                    width: 58
                                    height: 22
                                    radius: 2
                                    color: (root.flacCompression === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.flacCompression === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.flacCompression === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.flacCompression = modelData.val
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Rate:"; font.pixelSize: 9; font.bold: true; color: theme.textMuted }
                            Repeater {
                                model: [
                                    { val: 44100, label: "44.1k" },
                                    { val: 48000, label: "48.0k" },
                                    { val: 96000, label: "96.0k" }
                                ]
                                Rectangle {
                                    width: 48
                                    height: 22
                                    radius: 2
                                    color: (root.flacSampleRate === modelData.val) ? theme.accent : theme.btnBg
                                    border.color: (root.flacSampleRate === modelData.val) ? theme.accentHover : theme.border
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: (root.flacSampleRate === modelData.val) ? "#ffffff" : theme.textMain
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !(engine && engine.isExporting)
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.flacSampleRate = modelData.val
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: (root.exportFormat === "wav")
                            ? "Industry standard broadcast master WAV, zero loss."
                            : (root.exportFormat === "mp3")
                            ? "Universal high-bitrate MP3 encoded via FFmpeg libmp3lame."
                            : (root.exportFormat === "ogg")
                            ? "High quality Vorbis audio, efficient compression for modern platforms."
                            : "Free Lossless Audio Codec (FLAC), bit-perfect lossless compression."
                        font.pixelSize: 9
                        color: theme.textDim
                    }
                }
            }

            // ==========================================
            // 5. Reverb / Delay Tail Duration
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "TAIL TIME:"
                    font.bold: true
                    font.pixelSize: 10
                    color: theme.textMuted
                    Layout.preferredWidth: 90
                }

                RowLayout {
                    spacing: 6

                    Repeater {
                        model: [
                            { val: 0.0, label: "0.0s" },
                            { val: 1.0, label: "1.0s" },
                            { val: 2.0, label: "2.0s" },
                            { val: 4.0, label: "4.0s" }
                        ]

                        Rectangle {
                            width: 50
                            height: 24
                            radius: 3
                            color: (root.tailSeconds === modelData.val) ? theme.accent : theme.bgWell
                            border.color: (root.tailSeconds === modelData.val) ? theme.accentHover : theme.border

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                font.bold: true
                                font.pixelSize: 9
                                color: (root.tailSeconds === modelData.val) ? "#ffffff" : theme.textMain
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !(engine && engine.isExporting)
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.tailSeconds = modelData.val
                            }
                        }
                    }

                    Text {
                        text: "Captures natural reverb & delay tail decay."
                        font.pixelSize: 9
                        color: theme.textDim
                        Layout.leftMargin: 6
                    }
                }
            }

            // ==========================================
            // 6. Destination File Path & Browse
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "DESTINATION:"
                    font.bold: true
                    font.pixelSize: 10
                    color: theme.textMuted
                    Layout.preferredWidth: 90
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 28
                    radius: 3
                    color: theme.bgWell
                    border.color: destInput.activeFocus ? theme.accent : theme.border

                    TextInput {
                        id: destInput
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        text: root.outputPath
                        font.pixelSize: 10
                        font.family: "Monospace"
                        color: theme.textMain
                        selectByMouse: true
                        enabled: !(engine && engine.isExporting)
                        onTextChanged: root.outputPath = text
                    }
                }

                Button {
                    text: "📁 BROWSE..."
                    font.bold: true
                    font.pixelSize: 9
                    implicitHeight: 28
                    implicitWidth: 88
                    enabled: !(engine && engine.isExporting)

                    background: Rectangle {
                        color: parent.hovered ? theme.btnHover : theme.btnBg
                        border.color: parent.hovered ? theme.accent : theme.border
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.enabled ? theme.btnText : theme.textDim
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (engine) {
                            let chosen = engine.browseExportFilePath(root.outputPath, root.exportFormat)
                            if (chosen && chosen !== "") {
                                root.outputPath = chosen
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

            // ==========================================
            // 7. Live Progress & Status Display
            // ==========================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: root.statusText
                        font.pixelSize: 10
                        font.bold: true
                        color: root.hasError ? theme.stopAccent : (root.isComplete ? theme.playAccent : theme.textMain)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: Math.round(root.exportProgress * 100) + "%"
                        font.pixelSize: 10
                        font.bold: true
                        font.family: "Monospace"
                        color: root.hasError ? theme.stopAccent : (root.isComplete ? theme.playAccent : theme.accent)
                    }
                }

                // Progress Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 8
                    radius: 3
                    color: theme.bgWell
                    border.color: theme.border
                    clip: true

                    Rectangle {
                        width: parent.width * root.exportProgress
                        height: parent.height
                        radius: 3
                        color: root.hasError ? theme.stopAccent : (root.isComplete ? theme.playAccent : theme.accent)

                        Behavior on width {
                            NumberAnimation { duration: 80 }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // ==========================================
            // 8. Action Buttons Footer
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Completed Actions
                Button {
                    visible: root.isComplete && !engine.isExporting
                    text: "📁 SHOW IN DOLPHIN / FOLDER"
                    font.bold: true
                    font.pixelSize: 10
                    implicitHeight: 32
                    implicitWidth: 190

                    background: Rectangle {
                        color: parent.hovered ? theme.btnHover : theme.btnBg
                        border.color: parent.hovered ? theme.accent : theme.border
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (engine) engine.openContainingFolder(root.lastExportedPath ? root.lastExportedPath : root.outputPath)
                    }
                }

                Item { Layout.fillWidth: true }

                // Abort Button during active export
                Button {
                    visible: engine ? engine.isExporting : false
                    text: "⏹ ABORT EXPORT"
                    font.bold: true
                    font.pixelSize: 10
                    implicitHeight: 32
                    implicitWidth: 120

                    background: Rectangle {
                        color: parent.hovered ? theme.stopAccent : "#802020"
                        border.color: theme.stopAccent
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
                        if (engine) engine.cancelExport()
                    }
                }

                // Cancel / Close Button
                Button {
                    visible: !(engine && engine.isExporting)
                    text: root.isComplete ? "DONE" : "CANCEL"
                    font.bold: true
                    font.pixelSize: 10
                    implicitHeight: 32
                    implicitWidth: 90

                    background: Rectangle {
                        color: parent.hovered ? theme.btnHover : theme.btnBg
                        border.color: theme.border
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.close()
                }

                // Start Export Button
                Button {
                    visible: !(engine && engine.isExporting) && !root.isComplete
                    text: "⤓ START EXPORT"
                    font.bold: true
                    font.pixelSize: 10
                    implicitHeight: 32
                    implicitWidth: 140

                    background: Rectangle {
                        color: parent.hovered ? theme.accentHover : theme.accent
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.startExport()
                }
            }
        }

        // Floating Centered Circular Close Button Above Window
        Rectangle {
            id: floatingCloseBtn
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
            visible: !(engine && engine.isExporting)

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
    }

    Keys.onEscapePressed: (event) => {
        if (!(engine && engine.isExporting)) {
            root.close()
            event.accepted = true
        }
    }
}

