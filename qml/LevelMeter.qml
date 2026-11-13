import QtQuick

Item {
    id: root
    width: 20
    height: 28

    property real peakL: 0.0
    property real peakR: 0.0

    StudioTheme { id: theme }

    Row {
        anchors.fill: parent
        spacing: 3

        Item {
            width: 8
            height: parent.height

            Rectangle {
                anchors.fill: parent
                color: theme.bgWell
                radius: 2
                border.color: theme.border
                border.width: 1

                Rectangle {
                    width: parent.width - 2
                    height: Math.min(parent.height - 2, Math.max(0, (parent.height - 2) * root.peakL))
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 1
                    color: root.peakL > 0.95 ? theme.stopAccent : (root.peakL > 0.75 ? "#f39c12" : theme.playAccent)
                }
            }
        }

        Item {
            width: 8
            height: parent.height

            Rectangle {
                anchors.fill: parent
                color: theme.bgWell
                radius: 2
                border.color: theme.border
                border.width: 1

                Rectangle {
                    width: parent.width - 2
                    height: Math.min(parent.height - 2, Math.max(0, (parent.height - 2) * root.peakR))
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 1
                    color: root.peakR > 0.95 ? theme.stopAccent : (root.peakR > 0.75 ? "#f39c12" : theme.playAccent)
                }
            }
        }
    }
}
