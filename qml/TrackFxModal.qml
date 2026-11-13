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
    property int targetTrack: 0
    property var fxSlots: []

    function open(track) {
        targetTrack = track
        if (engine) engine.setSelectedInstrument(track)
        if (fxTrackNameInput && engine) {
            fxTrackNameInput.text = engine.getInstrumentName(track)
        }
        refreshSlots()
        isOpen = true
        root.forceActiveFocus()
    }

    function close() {
        isOpen = false
    }

    function refreshSlots() {
        if (!engine) return
        fxSlots = engine.getTrackFxSlots(targetTrack)
    }

    Connections {
        target: engine
        function onTrackFxChanged(t, s) {
            if (t === root.targetTrack || t === -1) {
                root.refreshSlots()
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
        width: Math.min(parent.width - 40, 820)
        height: Math.min(parent.height - 40, 560)
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
            // 1. HEADER TITLE BAR
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
                            text: engine ? engine.getInstrumentName(root.targetTrack) : "Track"
                            font.bold: true
                            font.pixelSize: 11
                            color: theme.textHeading
                            selectByMouse: true
                            clip: true
                            onEditingFinished: {
                                if (engine && text.trim().length > 0) {
                                    engine.setInstrumentName(root.targetTrack, text.trim())
                                }
                            }
                        }
                    }

                    Text {
                        text: "— INSERT FX CHAIN"
                        font.bold: true
                        font.pixelSize: 11
                        color: theme.textMuted
                    }

                    Rectangle {
                        height: 18
                        width: 70
                        radius: 3
                        color: theme.bgWell
                        border.color: theme.border
                        Text {
                            anchors.centerIn: parent
                            text: {
                                let c = engine ? engine.getTrackFxCount(root.targetTrack) : 0
                                return c + " / 4 ACTIVE"
                            }
                            font.bold: true
                            font.pixelSize: 9
                            color: theme.accent
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        implicitHeight: 26
                        implicitWidth: 80
                        text: "✓ DONE"
                        font.bold: true
                        font.pixelSize: 10
                        background: Rectangle {
                            color: parent.hovered ? theme.playAccent : theme.btnBg
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
                        onClicked: root.close()
                    }
                }
            }

            // ==========================================
            // 2. SIGNAL FLOW DIAGRAM
            // ==========================================
            Rectangle {
                Layout.fillWidth: true
                height: 38
                color: theme.bgWell
                border.color: theme.border
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        height: 22
                        width: 90
                        radius: 3
                        color: theme.bgCard
                        border.color: theme.border
                        Text {
                            anchors.centerIn: parent
                            text: "🎵 GENERATOR"
                            font.bold: true
                            font.pixelSize: 9
                            color: theme.textMain
                        }
                    }

                    Repeater {
                        model: 4
                        RowLayout {
                            spacing: 8
                            Text {
                                text: "➔"
                                font.pixelSize: 10
                                color: theme.textMuted
                            }
                            Rectangle {
                                height: 22
                                width: 85
                                radius: 3
                                property bool isLoaded: (root.fxSlots && index < root.fxSlots.length) ? root.fxSlots[index].loaded : false
                                color: isLoaded ? theme.accent : theme.bgCard
                                border.color: isLoaded ? theme.accent : theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        if (parent.isLoaded) {
                                            let name = root.fxSlots[index].name
                                            return name ? name : ("FX " + (index + 1))
                                        }
                                        return "FX " + (index + 1) + " (EMPTY)"
                                    }
                                    font.bold: true
                                    font.pixelSize: 8
                                    color: parent.isLoaded ? "#ffffff" : theme.textDim
                                    elide: Text.ElideRight
                                    width: parent.width - 8
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }

                    Text {
                        text: "➔"
                        font.pixelSize: 10
                        color: theme.textMuted
                    }

                    Rectangle {
                        height: 22
                        width: 80
                        radius: 3
                        color: theme.bgCard
                        border.color: theme.border
                        Text {
                            anchors.centerIn: parent
                            text: "🎚️ MIXER"
                            font.bold: true
                            font.pixelSize: 9
                            color: theme.textMain
                        }
                    }
                }
            }

            // ==========================================
            // 3. FX SLOTS LIST
            // ==========================================
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Repeater {
                        model: (root.fxSlots ? root.fxSlots.length : 4)

                        Rectangle {
                            id: slotCard
                            Layout.fillWidth: true
                            property var slotData: (root.fxSlots && index < root.fxSlots.length) ? root.fxSlots[index] : null
                            property bool isLoaded: slotData ? slotData.loaded : false
                            property bool showParams: false
                            property var paramList: []
                            height: (isLoaded && showParams && paramList.length > 0) ? (84 + 10 + Math.min(220, Math.ceil(paramList.length / 2) * 54)) : 84
                            radius: 4
                            color: isLoaded ? theme.bgCard : theme.bgWell
                            border.color: isLoaded ? theme.accent : theme.border
                            border.width: isLoaded ? 1 : 1
                            clip: true

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    // Slot Index Number Badge
                                    Rectangle {
                                        width: 32
                                        height: 32
                                        radius: 3
                                        color: slotCard.isLoaded ? theme.accent : theme.bgHeader
                                        border.color: theme.border
                                        Text {
                                            anchors.centerIn: parent
                                            text: "0" + (index + 1)
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: slotCard.isLoaded ? "#ffffff" : theme.textMuted
                                        }
                                    }

                                    // Loaded View
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        visible: slotCard.isLoaded

                                        RowLayout {
                                            spacing: 8
                                            // Format Badge
                                            Rectangle {
                                                width: 44
                                                height: 18
                                                radius: 2
                                                color: {
                                                    let fmt = slotCard.slotData ? slotCard.slotData.format : ""
                                                    if (fmt === "VST3") return "#00b894"
                                                    if (fmt === "CLAP") return "#6c5ce7"
                                                    return "#3daee9"
                                                }
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: slotCard.slotData ? slotCard.slotData.format : "FX"
                                                    font.bold: true
                                                    font.pixelSize: 9
                                                    color: "#ffffff"
                                                }
                                            }

                                            Text {
                                                text: slotCard.slotData ? slotCard.slotData.name : "Effect Plugin"
                                                font.bold: true
                                                font.pixelSize: 13
                                                color: theme.textHeading
                                            }
                                        }

                                        Text {
                                            text: {
                                                if (!slotCard.slotData) return ""
                                                let v = slotCard.slotData.vendor
                                                let p = slotCard.slotData.path
                                                return (v ? v + " · " : "") + p
                                            }
                                            font.pixelSize: 9
                                            color: theme.textMuted
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                    }

                                    // Loaded Action Buttons
                                    RowLayout {
                                        spacing: 6
                                        visible: slotCard.isLoaded
                                        Layout.alignment: Qt.AlignVCenter

                                        Button {
                                            implicitHeight: 28
                                            text: slotCard.showParams ? "▲ PARAMS" : "🎛 PARAMS"
                                            font.bold: true
                                            font.pixelSize: 9
                                            background: Rectangle {
                                                color: slotCard.showParams ? theme.accent : (parent.hovered ? theme.btnHover : theme.btnBg)
                                                border.color: theme.border
                                                radius: 3
                                            }
                                            contentItem: Text {
                                                text: parent.text
                                                font: parent.font
                                                color: slotCard.showParams ? "#ffffff" : theme.textMain
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: {
                                                slotCard.showParams = !slotCard.showParams
                                                if (slotCard.showParams && engine) {
                                                    slotCard.paramList = engine.getTrackFxParams(root.targetTrack, index)
                                                }
                                            }
                                        }

                                        Button {
                                            implicitHeight: 28
                                            text: "🗔 OPEN GUI"
                                            font.bold: true
                                            font.pixelSize: 9
                                            visible: slotCard.slotData ? slotCard.slotData.has_gui : false
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
                                                if (engine) engine.promptOpenTrackFxGui(root.targetTrack, index)
                                            }
                                        }

                                        Button {
                                            implicitHeight: 28
                                            text: "📁 CHANGE..."
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
                                            onClicked: {
                                                if (engine) engine.promptOpenTrackFxDialog(root.targetTrack, index)
                                            }
                                        }

                                        Button {
                                            implicitHeight: 28
                                            text: "✕ REMOVE"
                                            font.bold: true
                                            font.pixelSize: 9
                                            background: Rectangle {
                                                color: parent.hovered ? "#c0392b" : theme.btnBg
                                                border.color: theme.border
                                                radius: 3
                                            }
                                            contentItem: Text {
                                                text: parent.text
                                                font: parent.font
                                                color: parent.parent.hovered ? "#ffffff" : theme.stopAccent
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: {
                                                if (engine) engine.unloadTrackFx(root.targetTrack, index)
                                            }
                                        }
                                    }

                                    // Empty View
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 12
                                        visible: !slotCard.isLoaded

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Text {
                                                text: "Empty Insert Effect Slot #" + (index + 1)
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: theme.textMuted
                                            }
                                            Text {
                                                text: "Insert any 64-bit Linux VST3 (.vst3), CLAP (.clap), or VST2 (.so) audio effect plugin."
                                                font.pixelSize: 9
                                                color: theme.textDim
                                            }
                                        }

                                        Button {
                                            implicitHeight: 28
                                            text: "Load Demo Filter FX"
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
                                                if (engine) engine.loadTrackFx(root.targetTrack, index, "plugins/effects/demo_filter_fx.so")
                                            }
                                        }

                                        Button {
                                            implicitHeight: 28
                                            text: "📁 BROWSE PLUGIN..."
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
                                            onClicked: {
                                                if (engine) engine.promptOpenTrackFxDialog(root.targetTrack, index)
                                            }
                                        }
                                    }
                                }

                                // Expandable DAW Parameter Controls Section
                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    visible: slotCard.isLoaded && slotCard.showParams && slotCard.paramList.length > 0
                                    clip: true
                                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                    Flow {
                                        width: parent.width - 8
                                        spacing: 8

                                        Repeater {
                                            model: slotCard.paramList

                                            Rectangle {
                                                id: fxParamCard
                                                width: Math.floor((parent.width - 8) / 2)
                                                height: 48
                                                color: theme.bgWell
                                                radius: 3
                                                border.color: theme.border

                                                property int pId: modelData.id
                                                property real pVal: modelData.value
                                                property string pName: modelData.name

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 4
                                                    spacing: 1

                                                    RowLayout {
                                                        Layout.fillWidth: true
                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: fxParamCard.pName
                                                            font.bold: true
                                                            font.pixelSize: 9
                                                            color: theme.textHeading
                                                            elide: Text.ElideRight
                                                        }
                                                        Text {
                                                            text: Math.round(fxParamCard.pVal * 100) + "%"
                                                            font.pixelSize: 9
                                                            color: theme.accent
                                                            font.bold: true
                                                        }
                                                    }

                                                    Slider {
                                                        id: fxSlider
                                                        Layout.fillWidth: true
                                                        from: 0.0
                                                        to: 1.0
                                                        value: fxParamCard.pVal
                                                        onMoved: {
                                                            if (engine) {
                                                                engine.setTrackFxParamValue(root.targetTrack, index, fxParamCard.pId, value)
                                                                fxParamCard.pVal = value
                                                            }
                                                        }
                                                        onValueChanged: {
                                                            if (pressed && engine) {
                                                                engine.setTrackFxParamValue(root.targetTrack, index, fxParamCard.pId, value)
                                                                fxParamCard.pVal = value
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
        }
    }

    Keys.onEscapePressed: root.close()
}

