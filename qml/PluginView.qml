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
    property int currentTrack: 0
    property var paramList: []
    property bool isLoaded: engine ? engine.isTrackPluginLoaded(currentTrack) : false

    function refreshParams() {
        if (!engine) return
        isLoaded = engine.isTrackPluginLoaded(currentTrack)
        if (isLoaded) {
            paramList = engine.getTrackPluginParams(currentTrack)
        } else {
            paramList = []
        }
    }

    onCurrentTrackChanged: refreshParams()

    Connections {
        target: engine
        function onTrackPluginChanged(t) {
            if (t === root.currentTrack) root.refreshParams()
        }
        function onTrackPluginParamsChanged(t) {
            if (t === root.currentTrack) root.refreshParams()
        }
        function onProjectLoaded() {
            root.refreshParams()
        }
    }

    Component.onCompleted: refreshParams()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // ==========================================
        // 1. TOP HEADER & PLUGIN CONTROLS
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: theme.bgHeader
            radius: 4
            border.color: theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                // Format Badge
                Rectangle {
                    width: 50
                    height: 24
                    radius: 3
                    color: {
                        if (!root.isLoaded) return theme.bgWell
                        let fmt = engine ? engine.getTrackPluginFormatName(root.currentTrack) : ""
                        if (fmt === "VST3") return "#00b894"
                        if (fmt === "CLAP") return "#6c5ce7"
                        return "#3daee9"
                    }
                    Text {
                        anchors.centerIn: parent
                        text: {
                            if (!root.isLoaded) return "PLUGIN"
                            return engine ? engine.getTrackPluginFormatName(root.currentTrack) : "PLUGIN"
                        }
                        font.bold: true
                        font.pixelSize: 10
                        color: "#ffffff"
                    }
                }

                // Plugin Name & Vendor
                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true

                    Text {
                        text: {
                            if (!root.isLoaded) return "No Plugin Loaded"
                            let name = engine ? engine.getTrackPluginName(root.currentTrack) : ""
                            return name ? name : "Unknown Plugin"
                        }
                        font.bold: true
                        font.pixelSize: 13
                        color: root.isLoaded ? theme.textHeading : theme.textMuted
                        elide: Text.ElideRight
                    }

                    Text {
                        text: {
                            if (!root.isLoaded) return "Track #" + (root.currentTrack + 1) + " (Slot Idle)"
                            let vendor = engine ? engine.getTrackPluginVendor(root.currentTrack) : ""
                            let path = engine ? engine.getTrackPluginPath(root.currentTrack) : ""
                            return (vendor ? vendor + " · " : "") + path
                        }
                        font.pixelSize: 9
                        color: theme.textMuted
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }

                // Action Buttons
                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    Button {
                        text: "🗔 OPEN GUI"
                        font.bold: true
                        font.pixelSize: 9
                        implicitHeight: 28
                        visible: root.isLoaded && (engine ? engine.hasTrackPluginNativeEditor(root.currentTrack) : false)
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
                            if (engine) engine.promptOpenPluginGui(root.currentTrack)
                        }
                    }

                    Button {
                        text: root.isLoaded ? "📁 CHANGE PLUGIN..." : "📁 LOAD PLUGIN..."
                        font.bold: true
                        font.pixelSize: 9
                        implicitHeight: 28
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
                            if (engine) engine.promptOpenPlugin(root.currentTrack)
                        }
                    }

                    Button {
                        text: "⟳ UNLOAD"
                        font.bold: true
                        font.pixelSize: 9
                        implicitHeight: 28
                        visible: root.isLoaded
                        background: Rectangle {
                            color: parent.hovered ? "#c0392b" : theme.btnBg
                            border.color: theme.border
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.parent.hovered ? "#ffffff" : theme.textMain
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (engine) engine.unloadTrackPlugin(root.currentTrack)
                        }
                    }
                }
            }
        }

        // ==========================================
        // 2. PARAMETERS / EMPTY STATE
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: theme.bgWell
            radius: 4
            border.color: theme.border
            clip: true

            // Empty State
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                visible: !root.isLoaded

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🔌"
                    font.pixelSize: 36
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "No Plugin Active on Track #" + (root.currentTrack + 1)
                    font.bold: true
                    font.pixelSize: 14
                    color: theme.textHeading
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Load any 64-bit Linux VST3 (.vst3), CLAP (.clap), or VST2 (.so) synthesizer plugin."
                    font.pixelSize: 11
                    color: theme.textMuted
                }

                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Button {
                        text: "Load Interactive GUI Synth"
                        font.bold: true
                        font.pixelSize: 10
                        implicitHeight: 28
                        background: Rectangle {
                            color: parent.hovered ? theme.btnHover : theme.btnBg
                            border.color: "#00cec9"
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "#00cec9"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (engine) engine.loadTrackPlugin(root.currentTrack, "plugins/gui_synth/demo_gui_synth.so")
                        }
                    }

                    Button {
                        text: "Load Demo CLAP Synth"
                        font.bold: true
                        font.pixelSize: 10
                        implicitHeight: 28
                        background: Rectangle {
                            color: parent.hovered ? theme.btnHover : theme.btnBg
                            border.color: "#6c5ce7"
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "#a29bfe"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (engine) engine.loadTrackPlugin(root.currentTrack, "plugins/clap/demo_synth_clap.clap")
                        }
                    }

                    Button {
                        text: "Load Demo VST2 Synth"
                        font.bold: true
                        font.pixelSize: 10
                        implicitHeight: 28
                        background: Rectangle {
                            color: parent.hovered ? theme.btnHover : theme.btnBg
                            border.color: "#d63031"
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "#ff7675"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (engine) engine.loadTrackPlugin(root.currentTrack, "plugins/vst2/demo_synth_vst2.so")
                        }
                    }
                }
            }

            // Loaded Parameters Grid
            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                visible: root.isLoaded
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Flow {
                    width: parent.width - 12
                    spacing: 10

                    Repeater {
                        model: root.paramList

                        Rectangle {
                            id: paramCard
                            width: 250
                            height: 64
                            color: theme.bgCard
                            radius: 4
                            border.color: theme.border

                            property int paramId: modelData.id
                            property real paramVal: modelData.value
                            property string paramName: modelData.name
                            property string paramDisp: modelData.display
                            property string paramLbl: modelData.label

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        Layout.fillWidth: true
                                        text: paramCard.paramName
                                        font.bold: true
                                        font.pixelSize: 10
                                        color: theme.textHeading
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: {
                                            if (paramCard.paramDisp && paramCard.paramDisp.length > 0) {
                                                return paramCard.paramDisp + (paramCard.paramLbl ? " " + paramCard.paramLbl : "")
                                            }
                                            return Math.round(paramCard.paramVal * 100) + "%"
                                        }
                                        font.pixelSize: 10
                                        color: theme.accent
                                        font.bold: true
                                    }
                                }

                                Item {
                                    id: pSliderArea
                                    Layout.fillWidth: true
                                    height: 24

                                    // Track Background Groove
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        height: 6
                                        radius: 3
                                        color: theme.bgHeader
                                        border.color: theme.border

                                        // Active fill
                                        Rectangle {
                                            width: Math.max(0, Math.min(parent.width, paramCard.paramVal * parent.width))
                                            height: parent.height
                                            radius: 3
                                            color: pMouse.pressed ? "#ffffff" : theme.accent
                                        }
                                    }

                                    // Handle Thumb
                                    Rectangle {
                                        width: 14
                                        height: 14
                                        radius: 7
                                        color: pMouse.pressed ? "#ffffff" : (pMouse.containsMouse ? "#55c0f5" : theme.accent)
                                        border.color: theme.border
                                        border.width: 1
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: Math.max(0, Math.min(parent.width - width, paramCard.paramVal * (parent.width - width)))
                                    }

                                    MouseArea {
                                        id: pMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        preventStealing: true
                                        cursorShape: Qt.PointingHandCursor

                                        function updateVal(mouseX) {
                                            let newVal = Math.max(0.0, Math.min(1.0, mouseX / parent.width))
                                            paramCard.paramVal = newVal
                                            if (root.engine) {
                                                root.engine.setTrackPluginParamValue(root.currentTrack, paramCard.paramId, newVal)
                                                let disp = root.engine.getTrackPluginParamDisplay(root.currentTrack, paramCard.paramId)
                                                if (disp && disp.length > 0) {
                                                    paramCard.paramDisp = disp
                                                } else {
                                                    paramCard.paramDisp = Math.round(newVal * 100) + "%"
                                                }
                                            }
                                        }

                                        onPressed: (mouse) => updateVal(mouse.x)
                                        onPositionChanged: (mouse) => {
                                            if (pressed) updateVal(mouse.x)
                                        }
                                        onWheel: (wheel) => {
                                            let delta = wheel.angleDelta.y > 0 ? 0.02 : -0.02
                                            let newVal = Math.max(0.0, Math.min(1.0, paramCard.paramVal + delta))
                                            paramCard.paramVal = newVal
                                            if (root.engine) {
                                                root.engine.setTrackPluginParamValue(root.currentTrack, paramCard.paramId, newVal)
                                                let disp = root.engine.getTrackPluginParamDisplay(root.currentTrack, paramCard.paramId)
                                                if (disp && disp.length > 0) {
                                                    paramCard.paramDisp = disp
                                                } else {
                                                    paramCard.paramDisp = Math.round(newVal * 100) + "%"
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
    }
}

