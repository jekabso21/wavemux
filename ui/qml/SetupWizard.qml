import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: wizard
    color: "transparent"

    property int currentStep: 0
    property string selectedOutput: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 0

        // Progress indicator
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 4
            Layout.bottomMargin: 30
            spacing: 8

            Repeater {
                model: 3
                Rectangle {
                    Layout.fillWidth: true
                    height: 4
                    radius: 2
                    color: index <= currentStep ? "#e94560" : "#333355"
                }
            }
        }

        // Step content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: currentStep

            // Step 0: Welcome
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20

                    Label {
                        text: "Welcome to WaveMux"
                        font.pixelSize: 32
                        font.bold: true
                        color: "#ffffff"
                    }

                    Label {
                        text: "Let's set up your audio routing in a few quick steps."
                        font.pixelSize: 16
                        color: "#aaaaaa"
                    }

                    Label {
                        text: "WaveMux creates virtual audio channels that let you control " +
                              "which apps go to your headphones and which go to your stream."
                        font.pixelSize: 14
                        color: "#888888"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.topMargin: 20
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Step 1: Output Device
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15

                    Label {
                        text: "Select Output Device"
                        font.pixelSize: 28
                        font.bold: true
                        color: "#ffffff"
                    }

                    Label {
                        text: "Choose where you want to hear your audio (headphones/speakers)."
                        font.pixelSize: 14
                        color: "#aaaaaa"
                    }

                    // Device count debug
                    Label {
                        text: "Found " + daemon.outputDevices.length + " device(s)"
                        font.pixelSize: 12
                        color: "#666666"
                        visible: daemon.outputDevices.length === 0
                    }

                    // Scrollable device list
                    ScrollView {
                        id: deviceScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.topMargin: 10
                        clip: true
                        contentWidth: availableWidth

                        Column {
                            width: deviceScroll.availableWidth
                            spacing: 8

                            Repeater {
                                model: daemon.outputDevices

                                Rectangle {
                                    width: deviceScroll.availableWidth
                                    height: 60
                                    radius: 8
                                    color: selectedOutput === modelData.id ? "#e94560" : "#252542"
                                    border.color: selectedOutput === modelData.id ? "#e94560" : "#333355"
                                    border.width: 1

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            selectedOutput = modelData.id
                                            console.log("Selected device:", modelData.id)
                                        }
                                    }

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 15
                                        spacing: 15

                                        Rectangle {
                                            width: 30
                                            height: 30
                                            radius: 15
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: selectedOutput === modelData.id ? "#ffffff" : "#333355"

                                            Label {
                                                anchors.centerIn: parent
                                                text: selectedOutput === modelData.id ? "✓" : ""
                                                color: "#e94560"
                                                font.pixelSize: 16
                                                font.bold: true
                                            }
                                        }

                                        Column {
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: parent.width - 60
                                            spacing: 2

                                            Label {
                                                text: modelData.name || "Unknown Device"
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Label {
                                                text: modelData.description || ""
                                                font.pixelSize: 11
                                                color: "#888888"
                                                elide: Text.ElideRight
                                                width: parent.width
                                                visible: text !== "" && text !== modelData.name
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // No devices message
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: 8
                        color: "#252542"
                        visible: daemon.outputDevices.length === 0

                        Label {
                            anchors.centerIn: parent
                            text: "No output devices found.\nMake sure the daemon is running."
                            font.pixelSize: 14
                            color: "#888888"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            // Step 2: Done
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20

                    Label {
                        text: "All Set!"
                        font.pixelSize: 32
                        font.bold: true
                        color: "#ffffff"
                    }

                    Label {
                        text: "WaveMux is ready to use."
                        font.pixelSize: 16
                        color: "#aaaaaa"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: channelInfo.height + 40
                        radius: 12
                        color: "#252542"

                        ColumnLayout {
                            id: channelInfo
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 20
                            spacing: 12

                            Label {
                                text: "Your audio channels:"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#e94560"
                            }

                            Repeater {
                                model: ["Game - for games and gameplay",
                                        "Chat - for Discord, Zoom, etc.",
                                        "Media - for Spotify, YouTube, etc.",
                                        "AUX - for everything else"]
                                Label {
                                    text: "• " + modelData
                                    font.pixelSize: 13
                                    color: "#cccccc"
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Navigation buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.topMargin: 30
            spacing: 15

            Button {
                text: "Back"
                visible: currentStep > 0
                onClicked: currentStep--

                background: Rectangle {
                    color: "#333355"
                    radius: 6
                    implicitWidth: 100
                    implicitHeight: 40
                }
                contentItem: Label {
                    text: parent.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: currentStep === 2 ? "Get Started" : "Continue"
                enabled: currentStep !== 1 || selectedOutput !== ""

                onClicked: {
                    if (currentStep === 1 && selectedOutput) {
                        daemon.setOutputDevice(selectedOutput)
                    }
                    if (currentStep === 2) {
                        daemon.setSetupComplete(true)
                        daemon.saveConfig()
                    } else {
                        currentStep++
                    }
                }

                background: Rectangle {
                    color: parent.enabled ? "#e94560" : "#555555"
                    radius: 6
                    implicitWidth: 120
                    implicitHeight: 40
                }
                contentItem: Label {
                    text: parent.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("SetupWizard loaded, devices:", daemon.outputDevices.length)
        for (var i = 0; i < daemon.outputDevices.length; i++) {
            console.log("  Device:", daemon.outputDevices[i].name)
        }
    }
}
