import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgHeader
    height: 44
    border.color: theme.border
    border.width: 1

    property var engine

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        // 1. Arranger Mode & Title
        RowLayout {
            spacing: 8

            Text {
                text: "SONG ARRANGER"
                font.bold: true
                font.pixelSize: 11
                color: theme.textHeading
            }

            // Mode Selector Pill (PAT vs SONG)
            Button {
                id: modeToggleBtn
                implicitHeight: 24
                implicitWidth: (engine && engine.isSongMode) ? 105 : 95
                checkable: true
                checked: engine ? engine.isSongMode : false
                onClicked: {
                    if (engine) engine.toggleSongMode()
                }

                background: Rectangle {
                    color: (engine && engine.isSongMode) ? theme.playAccent : theme.bgWell
                    radius: 3
                    border.color: (engine && engine.isSongMode) ? theme.playAccent : theme.border
                }

                contentItem: Row {
                    anchors.centerIn: parent
                    spacing: 4
                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        color: (engine && engine.isSongMode) ? "#ffffff" : theme.stopAccent
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: (engine && engine.isSongMode) ? "SONG MODE" : "PAT LOOP"
                        font.bold: true
                        font.pixelSize: 10
                        color: (engine && engine.isSongMode) ? "#ffffff" : theme.textMuted
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: (engine && engine.isSongMode) ? "Currently playing entire song sequence" : "Currently looping single active pattern"
                ToolTip.delay: 400
            }
        }

        Rectangle { width: 1; height: 22; color: theme.border }

        // 2. Horizontal Sequence Timeline
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Row {
                id: sequenceRow
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                Repeater {
                    id: stepRepeater
                    model: engine ? engine.songOrderLength : 1

                    // Single Arranger Step Card
                    Rectangle {
                        id: stepCard
                        width: 104
                        height: 32
                        radius: 4

                        property int stepIdx: index
                        property int patNum: engine ? engine.getSongOrderStep(stepIdx) : 0
                        property bool isPlayingStep: (engine && engine.isSongMode && engine.isPlaying && engine.songOrderIndex === stepIdx)
                        property bool isSelectedStep: (engine && engine.songOrderIndex === stepIdx)

                        color: isPlayingStep ? theme.playRow : (isSelectedStep ? theme.bgWellHover : theme.bgCard)
                        border.color: isPlayingStep ? theme.playAccent : (isSelectedStep ? theme.accent : theme.border)
                        border.width: (isPlayingStep || isSelectedStep) ? 2 : 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 4

                            // Step Order Label
                            Rectangle {
                                width: 22
                                height: parent.height
                                color: "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "#" + (stepCard.stepIdx < 9 ? "0" + (stepCard.stepIdx + 1) : "" + (stepCard.stepIdx + 1))
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: stepCard.isPlayingStep ? theme.playAccent : (stepCard.isSelectedStep ? theme.accent : theme.textDim)
                                }
                            }

                            // Pattern Dropdown Button
                            Rectangle {
                                Layout.fillWidth: true
                                height: 24
                                radius: 2
                                color: stepCard.isPlayingStep ? theme.playAccent : (stepCard.isSelectedStep ? theme.accent : (patMouse.containsMouse ? theme.btnHover : theme.btnBg))
                                border.color: theme.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    anchors.rightMargin: 4
                                    spacing: 2

                                    Text {
                                        Layout.fillWidth: true
                                        text: "PAT " + (stepCard.patNum < 10 ? "0" + stepCard.patNum : "" + stepCard.patNum)
                                        font.bold: true
                                        font.pixelSize: 10
                                        font.family: "Monospace"
                                        color: (stepCard.isPlayingStep || stepCard.isSelectedStep) ? "#ffffff" : theme.textMain
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Text {
                                        text: "▾"
                                        font.pixelSize: 8
                                        color: (stepCard.isPlayingStep || stepCard.isSelectedStep) ? "#ffffff" : theme.textMuted
                                    }
                                }

                                MouseArea {
                                    id: patMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (engine) {
                                            engine.setSongOrderIndex(stepCard.stepIdx)
                                            engine.setCurrentPattern(stepCard.patNum)
                                        }
                                        patMenu.open()
                                    }
                                }

                                Menu {
                                    id: patMenu
                                    Repeater {
                                        model: engine ? engine.numPatterns : 1
                                        MenuItem {
                                            text: "Pattern " + (index < 10 ? "0" + index : "" + index)
                                            onTriggered: {
                                                if (engine) {
                                                    engine.setSongOrderStep(stepCard.stepIdx, index)
                                                    engine.setSongOrderIndex(stepCard.stepIdx)
                                                    engine.setCurrentPattern(index)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Remove Step button
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 2
                                visible: (engine && engine.songOrderLength > 1)
                                color: delMouse.containsMouse ? theme.stopAccent : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.bold: true
                                    font.pixelSize: 10
                                    color: (delMouse.containsMouse) ? "#ffffff" : theme.textMuted
                                }

                                MouseArea {
                                    id: delMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (engine) engine.removeSongOrderStep(stepCard.stepIdx)
                                    }
                                }
                            }
                        }

                        // Clicking anywhere on card selects this step & pattern
                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: {
                                if (engine) {
                                    engine.setSongOrderIndex(stepCard.stepIdx)
                                    engine.setCurrentPattern(stepCard.patNum)
                                }
                            }
                        }
                    }
                }

                // Add Duplicate Step Button
                Button {
                    implicitWidth: 72
                    implicitHeight: 28
                    text: "+ STEP"
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
                        color: theme.accent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (engine) {
                            let cur = engine.currentPattern
                            engine.addSongOrderStep(cur)
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: "Append duplicate step into song"
                    ToolTip.delay: 400
                }

                // Add Fresh Blank Pattern Button
                Button {
                    implicitWidth: 86
                    implicitHeight: 28
                    text: "+ NEW PAT"
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
                        color: parent.parent.hovered ? "#ffffff" : theme.playAccent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (engine) {
                            let newPat = engine.addPattern()
                            engine.setSongOrderIndex(engine.songOrderLength - 1)
                            engine.setCurrentPattern(newPat)
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: "Create fresh blank pattern and add to song"
                    ToolTip.delay: 400
                }
            }
        }

        Rectangle { width: 1; height: 22; color: theme.border }

        // Total song steps readout
        Text {
            text: (engine ? engine.songOrderLength : 1) + " Steps in Song"
            font.pixelSize: 10
            font.bold: true
            color: theme.textMuted
        }
    }
}
