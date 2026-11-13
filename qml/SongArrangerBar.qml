import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    StudioTheme { id: theme }

    color: theme.bgHeader
    height: 38
    border.color: theme.border
    border.width: 1

    property var engine

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        // Left Label & Song Mode Status
        RowLayout {
            spacing: 6
            Text {
                text: "ARRANGER"
                font.bold: true
                font.pixelSize: 10
                color: theme.textHeading
            }

            Rectangle {
                height: 18
                width: songStatusText.implicitWidth + 10
                radius: 2
                color: (engine && engine.isSongMode) ? theme.playAccent : theme.bgWell
                border.color: (engine && engine.isSongMode) ? theme.playAccent : theme.border

                Text {
                    id: songStatusText
                    anchors.centerIn: parent
                    text: (engine && engine.isSongMode) ? "SONG ACTIVE" : "PAT LOOP"
                    font.bold: true
                    font.pixelSize: 8
                    color: (engine && engine.isSongMode) ? "#ffffff" : theme.textMuted
                }
            }
        }

        Rectangle { width: 1; height: 18; color: theme.border }

        // Horizontal Scrollable Sequence Chain
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Row {
                spacing: 6
                anchors.verticalCenter: parent.verticalCenter

                Repeater {
                    id: stepRepeater
                    model: engine ? engine.songOrderLength : 1

                    // Single Song Step Block
                    Rectangle {
                        id: stepBlock
                        width: 78
                        height: 28
                        radius: 3
                        property int stepIdx: index
                        property int patNum: engine ? engine.getSongOrderStep(stepIdx) : 0
                        property bool isCurrentPlaybackStep: (engine && engine.isSongMode && engine.songOrderIndex === stepIdx)
                        property bool isSelectedPattern: (engine && engine.currentPattern === patNum)

                        color: isCurrentPlaybackStep ? theme.playRow : (isSelectedPattern ? theme.bgWellHover : theme.bgCard)
                        border.color: isCurrentPlaybackStep ? theme.playAccent : (isSelectedPattern ? theme.accent : theme.border)
                        border.width: isCurrentPlaybackStep ? 2 : (isSelectedPattern ? 1 : 1)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 4

                            // Step Number
                            Text {
                                text: "#" + (stepBlock.stepIdx + 1)
                                font.pixelSize: 8
                                font.bold: true
                                color: stepBlock.isCurrentPlaybackStep ? theme.playAccent : theme.textDim
                            }

                            // Pattern Badge
                            Rectangle {
                                Layout.fillWidth: true
                                height: 18
                                radius: 2
                                color: stepBlock.isCurrentPlaybackStep ? theme.playAccent : (stepBlock.isSelectedPattern ? theme.accent : theme.btnBg)

                                Text {
                                    anchors.centerIn: parent
                                    text: "PAT " + (stepBlock.patNum < 10 ? "0" + stepBlock.patNum : "" + stepBlock.patNum)
                                    font.bold: true
                                    font.pixelSize: 9
                                    font.family: "Monospace"
                                    color: (stepBlock.isCurrentPlaybackStep || stepBlock.isSelectedPattern) ? "#ffffff" : theme.textMain
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onWheel: (wheel) => {
                                        if (engine) {
                                            let delta = wheel.angleDelta.y > 0 ? 1 : -1
                                            let newPat = Math.max(0, Math.min(engine.numPatterns - 1, stepBlock.patNum + delta))
                                            engine.setSongOrderStep(stepBlock.stepIdx, newPat)
                                        }
                                    }
                                    onClicked: {
                                        if (engine) {
                                            engine.setSongOrderIndex(stepBlock.stepIdx)
                                            engine.setCurrentPattern(stepBlock.patNum)
                                        }
                                    }
                                }
                            }

                            // Remove Step button (visible if > 1 step)
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 2
                                visible: (engine && engine.songOrderLength > 1)
                                color: delMouse.containsMouse ? theme.stopAccent : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.bold: true
                                    font.pixelSize: 9
                                    color: theme.textMuted
                                }

                                MouseArea {
                                    id: delMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (engine) engine.removeSongOrderStep(stepBlock.stepIdx)
                                    }
                                }
                            }
                        }
                    }
                }

                // Arrow Connector / Add Step Button
                Button {
                    implicitWidth: 64
                    implicitHeight: 28
                    text: "+ STEP"
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
                }
            }
        }
    }
}
