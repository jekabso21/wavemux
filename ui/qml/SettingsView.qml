import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0d0d0d"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Settings panels
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 15

            // Output device
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#1a1a1a"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    Label {
                        text: "OUTPUT DEVICE"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1
                        color: "#666666"
                    }

                    ComboBox {
                        id: outputCombo
                        Layout.fillWidth: true
                        model: daemon.outputDevices
                        textRole: "name"

                        currentIndex: {
                            for (var i = 0; i < daemon.outputDevices.length; i++) {
                                if (daemon.outputDevices[i].id === daemon.outputDevice) {
                                    return i
                                }
                            }
                            return -1
                        }

                        onActivated: {
                            if (currentIndex >= 0) {
                                daemon.setOutputDevice(daemon.outputDevices[currentIndex].id)
                                daemon.saveConfig()
                            }
                        }

                        background: Rectangle {
                            implicitHeight: 40
                            radius: 4
                            color: "#0d0d0d"
                            border.color: "#333333"
                            border.width: 1
                        }

                        contentItem: Label {
                            leftPadding: 12
                            text: outputCombo.displayText || "Select device..."
                            font.pixelSize: 12
                            color: "#ffffff"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        indicator: Label {
                            x: outputCombo.width - width - 12
                            y: (outputCombo.height - height) / 2
                            text: "▼"
                            font.pixelSize: 8
                            color: "#666666"
                        }

                        popup: Popup {
                            y: outputCombo.height + 2
                            width: outputCombo.width
                            padding: 4

                            background: Rectangle {
                                color: "#1a1a1a"
                                radius: 4
                                border.color: "#333333"
                                border.width: 1
                            }

                            contentItem: ListView {
                                implicitHeight: Math.min(contentHeight, 200)
                                clip: true
                                model: outputCombo.popup.visible ? outputCombo.delegateModel : null

                                delegate: Rectangle {
                                    width: outputCombo.width - 8
                                    height: 36
                                    radius: 4
                                    color: mouseArea.containsMouse ? "#2a2a2a" : "transparent"

                                    Label {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        text: modelData.name
                                        font.pixelSize: 12
                                        color: "#ffffff"
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    MouseArea {
                                        id: mouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            outputCombo.currentIndex = index
                                            outputCombo.activated(index)
                                            outputCombo.popup.close()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Streamer mode
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: daemon.streamEnabled ? 200 : 100
                color: "#1a1a1a"
                radius: 8

                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "STREAMER MODE"
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 1
                            color: "#666666"
                        }

                        Item { Layout.fillWidth: true }

                        // Toggle switch
                        Rectangle {
                            id: streamToggle
                            width: 48
                            height: 26
                            radius: 13
                            color: daemon.streamEnabled ? "#9C27B0" : "#333333"

                            Behavior on color { ColorAnimation { duration: 150 } }

                            Rectangle {
                                id: toggleKnob
                                width: 22
                                height: 22
                                radius: 11
                                color: "#ffffff"
                                x: daemon.streamEnabled ? parent.width - width - 2 : 2
                                anchors.verticalCenter: parent.verticalCenter

                                Behavior on x { NumberAnimation { duration: 150 } }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    daemon.setStreamEnabled(!daemon.streamEnabled)
                                    daemon.saveConfig()
                                }
                            }
                        }
                    }

                    Label {
                        text: "Route audio to a second output device for streaming"
                        font.pixelSize: 11
                        color: "#666666"
                    }

                    // Stream output device selector (visible when enabled)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: daemon.streamEnabled
                        opacity: daemon.streamEnabled ? 1.0 : 0.0

                        Behavior on opacity { NumberAnimation { duration: 200 } }

                        Label {
                            text: "STREAM OUTPUT DEVICE"
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 1
                            color: "#666666"
                            Layout.topMargin: 10
                        }

                        ComboBox {
                            id: streamOutputCombo
                            Layout.fillWidth: true
                            model: daemon.outputDevices
                            textRole: "name"

                            currentIndex: {
                                for (var i = 0; i < daemon.outputDevices.length; i++) {
                                    if (daemon.outputDevices[i].id === daemon.streamOutputDevice) {
                                        return i
                                    }
                                }
                                return -1
                            }

                            onActivated: {
                                if (currentIndex >= 0) {
                                    daemon.setStreamOutputDevice(daemon.outputDevices[currentIndex].id)
                                    daemon.saveConfig()
                                }
                            }

                            background: Rectangle {
                                implicitHeight: 40
                                radius: 4
                                color: "#0d0d0d"
                                border.color: "#9C27B0"
                                border.width: 1
                            }

                            contentItem: Label {
                                leftPadding: 12
                                text: streamOutputCombo.displayText || "Select stream device..."
                                font.pixelSize: 12
                                color: "#ffffff"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            indicator: Label {
                                x: streamOutputCombo.width - width - 12
                                y: (streamOutputCombo.height - height) / 2
                                text: "▼"
                                font.pixelSize: 8
                                color: "#9C27B0"
                            }

                            popup: Popup {
                                y: streamOutputCombo.height + 2
                                width: streamOutputCombo.width
                                padding: 4

                                background: Rectangle {
                                    color: "#1a1a1a"
                                    radius: 4
                                    border.color: "#9C27B0"
                                    border.width: 1
                                }

                                contentItem: ListView {
                                    implicitHeight: Math.min(contentHeight, 200)
                                    clip: true
                                    model: streamOutputCombo.popup.visible ? streamOutputCombo.delegateModel : null

                                    delegate: Rectangle {
                                        width: streamOutputCombo.width - 8
                                        height: 36
                                        radius: 4
                                        color: streamMouseArea.containsMouse ? "#2a2a2a" : "transparent"

                                        Label {
                                            anchors.fill: parent
                                            anchors.leftMargin: 10
                                            text: modelData.name
                                            font.pixelSize: 12
                                            color: "#ffffff"
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }

                                        MouseArea {
                                            id: streamMouseArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onClicked: {
                                                streamOutputCombo.currentIndex = index
                                                streamOutputCombo.activated(index)
                                                streamOutputCombo.popup.close()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Routing rules
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1a1a1a"
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "AUTO-ROUTING RULES"
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 1
                            color: "#666666"
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: 80
                            height: 28
                            radius: 4
                            color: "#ff6b35"

                            Label {
                                anchors.centerIn: parent
                                text: "ADD RULE"
                                font.pixelSize: 9
                                font.bold: true
                                color: "#ffffff"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: addRuleDialog.open()
                            }
                        }
                    }

                    Label {
                        text: "Automatically assign apps to channels based on name"
                        font.pixelSize: 11
                        color: "#666666"
                    }

                    ListView {
                        id: rulesList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6

                        model: daemon.getRoutingRules()

                        delegate: Rectangle {
                            width: rulesList.width
                            height: 40
                            radius: 4
                            color: "#0d0d0d"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                Label {
                                    text: modelData.matchPattern
                                    font.pixelSize: 12
                                    font.family: "monospace"
                                    color: "#ff6b35"
                                    Layout.preferredWidth: 150
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: "→"
                                    font.pixelSize: 14
                                    color: "#444444"
                                }

                                Label {
                                    text: modelData.targetChannel.toUpperCase()
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#ffffff"
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 4
                                    color: "transparent"

                                    Label {
                                        anchors.centerIn: parent
                                        text: "×"
                                        font.pixelSize: 16
                                        color: "#666666"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            daemon.removeRoutingRule(modelData.matchPattern)
                                            daemon.saveConfig()
                                            rulesList.model = daemon.getRoutingRules()
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "No routing rules"
                            font.pixelSize: 12
                            color: "#444444"
                            visible: rulesList.count === 0
                        }
                    }
                }
            }
        }

        // About panel
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#1a1a1a"
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                Label {
                    text: "WAVEMUX"
                    font.pixelSize: 16
                    font.bold: true
                    font.letterSpacing: 2
                    color: "#ff6b35"
                }

                Label {
                    text: "Version 0.1.0"
                    font.pixelSize: 11
                    color: "#666666"
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 4
                    color: "#2a2a2a"

                    Label {
                        anchors.centerIn: parent
                        text: "RESET SETUP"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#888888"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            daemon.setSetupComplete(false)
                            daemon.saveConfig()
                        }
                    }
                }
            }
        }
    }

    // Add rule dialog
    Popup {
        id: addRuleDialog
        anchors.centerIn: parent
        width: 350
        height: 220
        modal: true

        background: Rectangle {
            color: "#1a1a1a"
            radius: 8
            border.color: "#333333"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            Label {
                text: "ADD ROUTING RULE"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1
                color: "#ffffff"
            }

            TextField {
                id: patternField
                Layout.fillWidth: true
                placeholderText: "App name pattern (e.g., discord, spotify)"

                background: Rectangle {
                    implicitHeight: 40
                    radius: 4
                    color: "#0d0d0d"
                    border.color: "#333333"
                    border.width: 1
                }
                color: "#ffffff"
                placeholderTextColor: "#555555"
                font.pixelSize: 12
                leftPadding: 12
            }

            ComboBox {
                id: channelField
                Layout.fillWidth: true
                model: ["Game", "Chat", "Media", "AUX"]

                background: Rectangle {
                    implicitHeight: 40
                    radius: 4
                    color: "#0d0d0d"
                    border.color: "#333333"
                    border.width: 1
                }

                contentItem: Label {
                    leftPadding: 12
                    text: channelField.displayText
                    font.pixelSize: 12
                    color: "#ffffff"
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 80
                    height: 36
                    radius: 4
                    color: "#2a2a2a"

                    Label {
                        anchors.centerIn: parent
                        text: "CANCEL"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#888888"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: addRuleDialog.close()
                    }
                }

                Rectangle {
                    width: 80
                    height: 36
                    radius: 4
                    color: "#ff6b35"

                    Label {
                        anchors.centerIn: parent
                        text: "ADD"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (patternField.text) {
                                var channelId = ["game", "chat", "media", "aux"][channelField.currentIndex]
                                daemon.addRoutingRule(patternField.text, channelId)
                                daemon.saveConfig()
                                rulesList.model = daemon.getRoutingRules()
                                patternField.text = ""
                                addRuleDialog.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
