import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0f0f0f"

    // Channel configuration with colors and icons
    readonly property var channelConfig: ({
        "game": { color: "#ff6b35", icon: "🎮", name: "Game" },
        "chat": { color: "#4CAF50", icon: "🎧", name: "Chat" },
        "media": { color: "#2196F3", icon: "🎵", name: "Media" },
        "aux": { color: "#9C27B0", icon: "🔊", name: "AUX" }
    })

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        // Channel strips section
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // Channel strips
            Repeater {
                model: daemon.channels

                Rectangle {
                    id: channelStrip
                    Layout.fillHeight: true
                    Layout.preferredWidth: 140
                    radius: 16
                    color: "#1a1a1a"

                    property string chId: modelData.id
                    property var config: channelConfig[chId] || { color: "#666", icon: "?", name: "Unknown" }

                    // Colored top accent bar
                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 4
                        radius: 16
                        color: channelStrip.config.color

                        // Only round top corners
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 2
                            color: channelStrip.config.color
                        }
                    }

                    // Glow effect when channel has audio
                    Rectangle {
                        anchors.fill: parent
                        radius: 16
                        color: "transparent"
                        border.color: channelStrip.config.color
                        border.width: modelData.volume > 0 && !modelData.muted ? 1 : 0
                        opacity: 0.3
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        anchors.topMargin: 20
                        spacing: 12

                        // Channel icon and name
                        Column {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 4

                            // Icon circle
                            Rectangle {
                                width: 48
                                height: 48
                                radius: 24
                                color: Qt.rgba(
                                    parseInt(channelStrip.config.color.substr(1,2), 16)/255,
                                    parseInt(channelStrip.config.color.substr(3,2), 16)/255,
                                    parseInt(channelStrip.config.color.substr(5,2), 16)/255,
                                    0.15
                                )
                                anchors.horizontalCenter: parent.horizontalCenter

                                Label {
                                    anchors.centerIn: parent
                                    text: channelStrip.config.icon
                                    font.pixelSize: 22
                                }
                            }

                            Label {
                                text: channelStrip.config.name
                                font.pixelSize: 14
                                font.bold: true
                                font.letterSpacing: 0.5
                                color: modelData.muted ? "#555" : "#fff"
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }

                        // Two vertical sliders side by side (Personal + Stream)
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            // Personal mix slider
                            Item {
                                id: personalSlider
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                // Use separate drag value to avoid breaking binding
                                property bool dragging: false
                                property int dragVolume: 0
                                property int displayVolume: dragging ? dragVolume : modelData.personalVolume

                                Column {
                                    anchors.fill: parent
                                    spacing: 4

                                    // Label
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "🎧"
                                        font.pixelSize: 12
                                    }

                                    // Slider track
                                    Item {
                                        width: parent.width
                                        height: parent.height - 60

                                        Rectangle {
                                            id: personalTrack
                                            anchors.centerIn: parent
                                            width: 10
                                            height: parent.height
                                            radius: 5
                                            color: "#252525"

                                            // Fill
                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                width: parent.width
                                                height: parent.height * (personalSlider.displayVolume / 100)
                                                radius: 5
                                                color: "#4CAF50"
                                            }
                                        }

                                        // Handle
                                        Rectangle {
                                            width: 28
                                            height: 14
                                            radius: 7
                                            color: personalMouse.pressed ? "#4CAF50" : "#fff"
                                            x: (parent.width - width) / 2
                                            y: personalTrack.y + personalTrack.height * (1 - personalSlider.displayVolume / 100) - height / 2

                                            Rectangle {
                                                anchors.fill: parent
                                                anchors.margins: -1
                                                radius: 8
                                                color: "#000"
                                                opacity: 0.2
                                                z: -1
                                            }
                                        }

                                        MouseArea {
                                            id: personalMouse
                                            anchors.fill: parent
                                            onPressed: (mouse) => {
                                                personalSlider.dragging = true
                                                updateVol()
                                            }
                                            onPositionChanged: (mouse) => {
                                                if (pressed) updateVol()
                                            }
                                            onReleased: (mouse) => {
                                                personalSlider.dragging = false
                                            }

                                            function updateVol() {
                                                let relY = Math.max(0, Math.min(personalTrack.height, personalMouse.mouseY - personalTrack.y))
                                                let vol = Math.round((1 - relY / personalTrack.height) * 100)
                                                personalSlider.dragVolume = vol
                                                daemon.setChannelPersonalVolume(modelData.id, vol)
                                            }
                                        }
                                    }

                                    // Value
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: personalSlider.displayVolume
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: "#4CAF50"
                                    }
                                }
                            }

                            // Stream mix slider (visible when enabled)
                            Item {
                                id: streamSlider
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                visible: daemon.streamEnabled
                                opacity: daemon.streamEnabled ? 1.0 : 0.0

                                // Use separate drag value to avoid breaking binding
                                property bool dragging: false
                                property int dragVolume: 0
                                property int displayVolume: dragging ? dragVolume : modelData.streamVolume

                                Behavior on opacity { NumberAnimation { duration: 200 } }

                                Column {
                                    anchors.fill: parent
                                    spacing: 4

                                    // Label
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "📡"
                                        font.pixelSize: 12
                                    }

                                    // Slider track
                                    Item {
                                        width: parent.width
                                        height: parent.height - 60

                                        Rectangle {
                                            id: streamTrack
                                            anchors.centerIn: parent
                                            width: 10
                                            height: parent.height
                                            radius: 5
                                            color: "#252525"

                                            // Fill
                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                width: parent.width
                                                height: parent.height * (streamSlider.displayVolume / 100)
                                                radius: 5
                                                color: "#9C27B0"
                                            }
                                        }

                                        // Handle
                                        Rectangle {
                                            width: 28
                                            height: 14
                                            radius: 7
                                            color: streamMouse.pressed ? "#9C27B0" : "#fff"
                                            x: (parent.width - width) / 2
                                            y: streamTrack.y + streamTrack.height * (1 - streamSlider.displayVolume / 100) - height / 2

                                            Rectangle {
                                                anchors.fill: parent
                                                anchors.margins: -1
                                                radius: 8
                                                color: "#000"
                                                opacity: 0.2
                                                z: -1
                                            }
                                        }

                                        MouseArea {
                                            id: streamMouse
                                            anchors.fill: parent
                                            onPressed: (mouse) => {
                                                streamSlider.dragging = true
                                                updateVol()
                                            }
                                            onPositionChanged: (mouse) => {
                                                if (pressed) updateVol()
                                            }
                                            onReleased: (mouse) => {
                                                streamSlider.dragging = false
                                            }

                                            function updateVol() {
                                                let relY = Math.max(0, Math.min(streamTrack.height, streamMouse.mouseY - streamTrack.y))
                                                let vol = Math.round((1 - relY / streamTrack.height) * 100)
                                                streamSlider.dragVolume = vol
                                                daemon.setChannelStreamVolume(modelData.id, vol)
                                            }
                                        }
                                    }

                                    // Value
                                    Label {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: streamSlider.displayVolume
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: "#9C27B0"
                                    }
                                }
                            }
                        }

                        // Mute button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            radius: 8
                            color: modelData.muted ? channelStrip.config.color : "#252525"

                            Behavior on color { ColorAnimation { duration: 150 } }

                            Label {
                                anchors.centerIn: parent
                                text: modelData.muted ? "🔇 MUTED" : "🔈 MUTE"
                                font.pixelSize: 10
                                font.bold: true
                                color: "#fff"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: daemon.setChannelMute(modelData.id, !modelData.muted)
                            }
                        }
                    }

                    // Drop zone for apps
                    DropArea {
                        anchors.fill: parent
                        z: -1
                        keys: ["application/x-wavemux-stream"]

                        onEntered: (drag) => {
                            channelStrip.color = Qt.lighter("#1a1a1a", 1.4)
                        }

                        onExited: {
                            channelStrip.color = "#1a1a1a"
                        }

                        onDropped: (drop) => {
                            channelStrip.color = "#1a1a1a"
                            daemon.moveStreamToChannel(parseInt(drop.text), channelStrip.chId)
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Compact output info column
            ColumnLayout {
                Layout.fillHeight: true
                Layout.preferredWidth: 120
                spacing: 8

                // Connection status dot
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: daemon.connected ? "#4CAF50" : "#f44"
                    }

                    Label {
                        text: daemon.connected ? "Online" : "Offline"
                        font.pixelSize: 10
                        color: daemon.connected ? "#8f8" : "#f88"
                    }
                }

                // Personal output
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    radius: 10
                    color: "#1a1a1a"
                    border.color: "#4CAF50"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        Row {
                            spacing: 6
                            Label {
                                text: "🎧"
                                font.pixelSize: 12
                            }
                            Label {
                                text: "Personal"
                                font.pixelSize: 10
                                font.bold: true
                                color: "#4CAF50"
                            }
                        }

                        Label {
                            width: parent.width
                            text: {
                                let dev = daemon.outputDevice
                                if (!dev) return "Not set"
                                return dev.replace("alsa_output.", "").split(".")[0].replace(/-/g, " ")
                            }
                            font.pixelSize: 9
                            color: "#aaa"
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                        }
                    }
                }

                // Stream output (visible when enabled)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: daemon.streamEnabled ? 70 : 0
                    radius: 10
                    color: "#1a1a1a"
                    border.color: "#9C27B0"
                    border.width: 1
                    visible: daemon.streamEnabled
                    opacity: daemon.streamEnabled ? 1.0 : 0.0
                    clip: true

                    Behavior on Layout.preferredHeight { NumberAnimation { duration: 200 } }
                    Behavior on opacity { NumberAnimation { duration: 200 } }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        Row {
                            spacing: 6
                            Label {
                                text: "📡"
                                font.pixelSize: 12
                            }
                            Label {
                                text: "Stream"
                                font.pixelSize: 10
                                font.bold: true
                                color: "#9C27B0"
                            }
                        }

                        Label {
                            width: parent.width
                            text: {
                                let dev = daemon.streamOutputDevice
                                if (!dev) return "Not set"
                                return dev.replace("alsa_output.", "").split(".")[0].replace(/-/g, " ")
                            }
                            font.pixelSize: 9
                            color: daemon.streamOutputDevice ? "#aaa" : "#f66"
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Apps section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            radius: 16
            color: "#1a1a1a"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // Header row
                RowLayout {
                    Layout.fillWidth: true

                    Row {
                        spacing: 8
                        Label {
                            text: "📱"
                            font.pixelSize: 16
                        }
                        Label {
                            text: "APPS"
                            font.pixelSize: 12
                            font.bold: true
                            font.letterSpacing: 1
                            color: "#888"
                        }
                    }

                    Label {
                        text: "Drag apps to channels or click to assign"
                        font.pixelSize: 10
                        color: "#555"
                        Layout.leftMargin: 16
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 80
                        height: 28
                        radius: 8
                        color: refreshHover.containsMouse ? "#333" : "#252525"

                        Row {
                            anchors.centerIn: parent
                            spacing: 6

                            Label {
                                text: "🔄"
                                font.pixelSize: 12
                            }
                            Label {
                                text: "Refresh"
                                font.pixelSize: 10
                                color: "#888"
                            }
                        }

                        MouseArea {
                            id: refreshHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: daemon.refresh()
                        }
                    }
                }

                // App cards
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                    clip: true

                    Row {
                        spacing: 12
                        height: parent.height

                        Repeater {
                            model: daemon.streams

                            Rectangle {
                                id: appCard
                                width: 180
                                height: parent.height - 8
                                radius: 12
                                color: "#252525"

                                property int streamId: modelData.id
                                property string assignedCh: modelData.assignedChannel || ""
                                property var chConfig: channelConfig[assignedCh]

                                // Colored left border when assigned
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    width: 4
                                    radius: 12
                                    color: appCard.chConfig ? appCard.chConfig.color : "transparent"

                                    Rectangle {
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: 2
                                        color: appCard.chConfig ? appCard.chConfig.color : "transparent"
                                    }
                                }

                                Drag.active: dragArea.drag.active
                                Drag.keys: ["application/x-wavemux-stream"]
                                Drag.mimeData: {"text/plain": streamId.toString()}

                                MouseArea {
                                    id: dragArea
                                    anchors.fill: parent
                                    drag.target: parent
                                    cursorShape: Qt.OpenHandCursor

                                    onPressed: {
                                        appCard.Drag.hotSpot = Qt.point(mouseX, mouseY)
                                        cursorShape = Qt.ClosedHandCursor
                                    }

                                    onReleased: {
                                        cursorShape = Qt.OpenHandCursor
                                        appCard.x = 0
                                        appCard.y = 0
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    anchors.leftMargin: 16
                                    spacing: 8

                                    // App info
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 10

                                        // App icon
                                        Rectangle {
                                            width: 36
                                            height: 36
                                            radius: 10
                                            color: appCard.chConfig ? appCard.chConfig.color : "#333"
                                            border.color: appCard.assignedCh ? "transparent" : "#f44"
                                            border.width: appCard.assignedCh ? 0 : 2

                                            Label {
                                                anchors.centerIn: parent
                                                text: appCard.chConfig ? appCard.chConfig.icon : "!"
                                                font.pixelSize: appCard.assignedCh ? 16 : 18
                                                font.bold: !appCard.assignedCh
                                                color: appCard.assignedCh ? "#fff" : "#f44"
                                            }
                                        }

                                        Column {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Label {
                                                text: modelData.appName || modelData.processName || "Unknown"
                                                font.pixelSize: 12
                                                font.bold: true
                                                color: "#fff"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Label {
                                                text: modelData.mediaName || (appCard.assignedCh ? channelConfig[appCard.assignedCh].name : "Unassigned")
                                                font.pixelSize: 10
                                                color: appCard.assignedCh ? "#888" : "#f66"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                        }
                                    }

                                    // Channel assignment buttons
                                    Row {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: 6

                                        Repeater {
                                            model: ["game", "chat", "media", "aux"]

                                            Rectangle {
                                                width: 36
                                                height: 28
                                                radius: 8
                                                color: appCard.assignedCh === modelData ? channelConfig[modelData].color : "#1a1a1a"
                                                border.color: channelConfig[modelData].color
                                                border.width: appCard.assignedCh === modelData ? 0 : 1

                                                Behavior on color { ColorAnimation { duration: 150 } }

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: channelConfig[modelData].icon
                                                    font.pixelSize: 14
                                                }

                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        if (appCard.assignedCh === modelData) {
                                                            daemon.unassignStream(appCard.streamId)
                                                        } else {
                                                            daemon.moveStreamToChannel(appCard.streamId, modelData)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Empty state
                        Rectangle {
                            width: 200
                            height: parent.height - 8
                            radius: 12
                            color: "#1f1f1f"
                            visible: daemon.streams.length === 0

                            Column {
                                anchors.centerIn: parent
                                spacing: 8

                                Label {
                                    text: "🔇"
                                    font.pixelSize: 24
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Label {
                                    text: "No apps playing audio"
                                    font.pixelSize: 11
                                    color: "#555"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
