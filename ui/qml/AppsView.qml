import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0d0d0d"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Apps list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1a1a"
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                // Header
                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "ACTIVE APPS"
                        font.pixelSize: 12
                        font.bold: true
                        font.letterSpacing: 1
                        color: "#666666"
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 70
                        height: 28
                        radius: 4
                        color: "#2a2a2a"

                        Label {
                            anchors.centerIn: parent
                            text: "REFRESH"
                            font.pixelSize: 9
                            font.bold: true
                            color: "#888888"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: daemon.refresh()
                        }
                    }
                }

                // App list
                ListView {
                    id: appList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8

                    model: daemon.streams

                    delegate: Rectangle {
                        id: streamDelegate
                        width: appList.width
                        height: 50
                        radius: 6
                        color: "#0d0d0d"

                        // Store stream data for inner repeater access
                        property int streamId: modelData.id
                        property string streamChannel: modelData.assignedChannel || ""
                        property string streamAppName: modelData.appName || ""
                        property string streamProcessName: modelData.processName || ""
                        property string streamMediaName: modelData.mediaName || ""

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            // App icon placeholder
                            Rectangle {
                                width: 32
                                height: 32
                                radius: 6
                                color: "#2a2a2a"

                                Label {
                                    anchors.centerIn: parent
                                    text: (streamDelegate.streamAppName || streamDelegate.streamProcessName || "?").charAt(0).toUpperCase()
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#ff6b35"
                                }
                            }

                            // App name
                            Column {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: streamDelegate.streamAppName || streamDelegate.streamProcessName || "Unknown"
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: "#ffffff"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Label {
                                    text: streamDelegate.streamMediaName
                                    font.pixelSize: 10
                                    color: "#666666"
                                    elide: Text.ElideRight
                                    width: parent.width
                                    visible: text !== ""
                                }
                            }

                            // Channel buttons
                            Row {
                                spacing: 6

                                Repeater {
                                    model: [
                                        {chId: "game", label: "G", color: "#ff6b35"},
                                        {chId: "chat", label: "C", color: "#4CAF50"},
                                        {chId: "media", label: "M", color: "#2196F3"},
                                        {chId: "aux", label: "A", color: "#9C27B0"}
                                    ]

                                    Rectangle {
                                        width: 28
                                        height: 28
                                        radius: 4
                                        color: streamDelegate.streamChannel === modelData.chId ? modelData.color : "#2a2a2a"
                                        border.color: modelData.color
                                        border.width: streamDelegate.streamChannel === modelData.chId ? 0 : 1

                                        Label {
                                            anchors.centerIn: parent
                                            text: modelData.label
                                            font.pixelSize: 11
                                            font.bold: true
                                            color: streamDelegate.streamChannel === modelData.chId ? "#ffffff" : modelData.color
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                console.log("Moving stream", streamDelegate.streamId, "to", modelData.chId)
                                                daemon.moveStreamToChannel(streamDelegate.streamId, modelData.chId)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        text: "No apps playing audio"
                        font.pixelSize: 13
                        color: "#444444"
                        visible: appList.count === 0
                    }
                }
            }
        }

        // Legend / Help panel
        Rectangle {
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: "#1a1a1a"
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                Label {
                    text: "CHANNELS"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 1
                    color: "#666666"
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 10

                    Repeater {
                        model: [
                            {id: "game", label: "Game", desc: "Games & gameplay", color: "#ff6b35"},
                            {id: "chat", label: "Chat", desc: "Discord, Zoom", color: "#4CAF50"},
                            {id: "media", label: "Media", desc: "Music, videos", color: "#2196F3"},
                            {id: "aux", label: "AUX", desc: "Everything else", color: "#9C27B0"}
                        ]

                        Row {
                            width: parent.width
                            spacing: 10

                            Rectangle {
                                width: 24
                                height: 24
                                radius: 4
                                color: modelData.color

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.label.charAt(0)
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#ffffff"
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 1

                                Label {
                                    text: modelData.label
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#ffffff"
                                }

                                Label {
                                    text: modelData.desc
                                    font.pixelSize: 9
                                    color: "#666666"
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Label {
                    text: "Click a channel button to assign an app"
                    font.pixelSize: 10
                    color: "#444444"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }
}
