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
    property var fxList: []
    property int selectedFxIndex: 0

    property var currentFx: (fxList && fxList.length > 0 && selectedFxIndex >= 0 && selectedFxIndex < fxList.length) ? fxList[selectedFxIndex] : null
    property int currentSlot: currentFx ? currentFx.slot : 0
    property var paramList: []

    function refreshParams() {
        if (!engine || !currentFx) {
            paramList = []
            return
        }
        paramList = engine.getTrackFxParams(targetTrack, currentSlot)
    }

    onTargetTrackChanged: refreshParams()
    onSelectedFxIndexChanged: refreshParams()
    onFxListChanged: refreshParams()

    function open(track) {
        targetTrack = track
        if (engine) engine.setSelectedInstrument(track)
        if (fxTrackNameInput && engine) {
            fxTrackNameInput.text = engine.getInstrumentName(track)
        }
        refreshSlots()
        if (fxList.length > 0 && selectedFxIndex >= fxList.length) {
            selectedFxIndex = 0
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
        if (selectedFxIndex >= fxList.length) {
            selectedFxIndex = Math.max(0, fxList.length - 1)
        }
        refreshParams()
    }

    Connections {
        target: engine
        function onTrackFxChanged(t, s) {
            if (t === root.targetTrack || t === -1) {
                root.refreshSlots()
            }
        }
        function onTrackFxParamsChanged(t, s) {
            if (t === root.targetTrack && s === root.currentSlot) {
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
        width: Math.min(parent.width - 30, 940)
        height: Math.min(parent.height - 30, 580)
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
                        text: "— Track Insert Effects Chain"
                        font.pixelSize: 11
                        color: theme.textMuted
                    }

                    // Active Count Badge
                    Rectangle {
                        height: 20
                        implicitWidth: countText.implicitWidth + 12
                        radius: 10
                        color: root.fxList.length > 0 ? theme.accent : theme.bgWell
                        border.color: theme.border
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: root.fxList.length + " ACTIVE"
                            font.bold: true
                            font.pixelSize: 9
                            color: root.fxList.length > 0 ? "#ffffff" : theme.textMuted
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
            // 2. MAIN TWO-PANE BODY
            // ==========================================
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // ------------------------------------------
                // LEFT PANE: SIGNAL CHAIN LIST
                // ------------------------------------------
                Rectangle {
                    Layout.preferredWidth: 260
                    Layout.fillHeight: true
                    color: theme.bgWell
                    border.color: theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        // Section Header
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "SIGNAL CHAIN"
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

                        // Scrollable List of Active Effects
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            ListView {
                                id: chainListView
                                width: parent.width
                                model: root.fxList
                                spacing: 6

                                delegate: Rectangle {
                                    id: fxItemCard
                                    width: chainListView.width
                                    height: 52
                                    radius: 4
                                    color: (index === root.selectedFxIndex) ? theme.bgCard : theme.bgHeader
                                    border.color: (index === root.selectedFxIndex) ? theme.accent : theme.border
                                    border.width: (index === root.selectedFxIndex) ? 2 : 1

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedFxIndex = index
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 8

                                        // Step Number
                                        Rectangle {
                                            width: 22
                                            height: 22
                                            radius: 11
                                            color: (index === root.selectedFxIndex) ? theme.accent : theme.bgWell
                                            Text {
                                                anchors.centerIn: parent
                                                text: (index + 1)
                                                font.bold: true
                                                font.pixelSize: 9
                                                color: (index === root.selectedFxIndex) ? "#ffffff" : theme.textMuted
                                            }
                                        }

                                        // Format Badge
                                        Rectangle {
                                            width: 38
                                            height: 18
                                            radius: 2
                                            color: {
                                                if (modelData.format === "VST3") return "#00b894"
                                                if (modelData.format === "CLAP") return "#6c5ce7"
                                                return "#e67e22"
                                            }
                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.format
                                                font.bold: true
                                                font.pixelSize: 8
                                                color: "#ffffff"
                                            }
                                        }

                                        // Effect Name & Params Count
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.name
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: (index === root.selectedFxIndex) ? theme.textHeading : theme.textMain
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
                                            width: 22
                                            height: 22
                                            radius: 3
                                            color: delMouse.containsMouse ? "#c0392b" : "transparent"
                                            border.color: delMouse.containsMouse ? "#e74c3c" : theme.border

                                            Text {
                                                anchors.centerIn: parent
                                                text: "✕"
                                                font.bold: true
                                                font.pixelSize: 10
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
                        }

                        // Add Effect Actions
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Button {
                                Layout.fillWidth: true
                                implicitHeight: 30
                                text: "➕ ADD INSERT EFFECT..."
                                font.bold: true
                                font.pixelSize: 10
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

                            Button {
                                Layout.fillWidth: true
                                implicitHeight: 24
                                text: "+ Add Demo Filter FX"
                                font.pixelSize: 9
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

                // ------------------------------------------
                // RIGHT PANE: SPACIOUS PARAMETER INSPECTOR
                // ------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: theme.bgCard

                    // Case A: Effect Selected & Loaded
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10
                        visible: root.fxList.length > 0 && root.selectedFxIndex >= 0 && root.selectedFxIndex < root.fxList.length

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
                                        visible: (root.engine && root.currentFx) ? root.engine.hasTrackFxNativeEditor(root.targetTrack, root.currentSlot) : false
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
                                            if (root.engine) root.engine.promptOpenTrackFxGui(root.targetTrack, root.currentSlot)
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
                                            if (root.engine) root.engine.promptOpenTrackFxDialog(root.targetTrack, root.currentSlot)
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
                                            if (root.engine) root.engine.removeTrackFx(root.targetTrack, root.currentSlot)
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
                                                            root.engine.setTrackFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                            let disp = root.engine.getTrackFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                            if (disp && disp.length > 0) {
                                                                fxParamCard.paramDisp = disp
                                                            } else {
                                                                fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
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
                                                            root.engine.setTrackFxParamValue(root.targetTrack, root.currentSlot, fxParamCard.paramId, newVal)
                                                            let disp = root.engine.getTrackFxParamDisplay(root.targetTrack, root.currentSlot, fxParamCard.paramId)
                                                            if (disp && disp.length > 0) {
                                                                fxParamCard.paramDisp = disp
                                                            } else {
                                                                fxParamCard.paramDisp = Math.round(newVal * 100) + "%"
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

                    // Case B: Empty State (No Effects Loaded on Track)
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: root.fxList.length === 0

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "🎛️"
                            font.pixelSize: 48
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "No Insert Effects Added to Track #" + (root.targetTrack + 1)
                            font.bold: true
                            font.pixelSize: 14
                            color: theme.textHeading
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Add as many VST3, CLAP, or VST2 effects as you want to this track."
                            font.pixelSize: 11
                            color: theme.textMuted
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 8

                            Button {
                                implicitHeight: 32
                                implicitWidth: 160
                                text: "➕ ADD EFFECT..."
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

    Keys.onEscapePressed: {
        root.close()
    }
}
