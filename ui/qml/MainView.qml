import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: mainView
    color: "#0d0d0d"

    property int currentTab: 0

    Component.onCompleted: {
        daemon.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#1a1a1a"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 30

                // Logo
                Label {
                    text: "WAVEMUX"
                    font.pixelSize: 18
                    font.bold: true
                    font.letterSpacing: 2
                    color: "#ff6b35"
                }

                // Tabs
                Row {
                    spacing: 0

                    Repeater {
                        model: ["MIXER", "APPS", "SETTINGS"]

                        Rectangle {
                            width: 80
                            height: 50
                            color: currentTab === index ? "#2a2a2a" : "transparent"

                            Label {
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 1
                                color: currentTab === index ? "#ffffff" : "#666666"
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 2
                                color: currentTab === index ? "#ff6b35" : "transparent"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: currentTab = index
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Master volume
                Row {
                    spacing: 15

                    Label {
                        text: "MASTER"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1
                        color: "#666666"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Slider {
                        id: masterSlider
                        width: 100
                        from: 0
                        to: 100
                        value: daemon.masterVolume
                        stepSize: 1
                        anchors.verticalCenter: parent.verticalCenter

                        onMoved: {
                            // Update volume in real-time while dragging
                            daemon.setMasterVolume(Math.round(value))
                        }

                        onPressedChanged: {
                            if (!pressed) {
                                value = Qt.binding(function() { return daemon.masterVolume })
                            }
                        }

                        background: Rectangle {
                            x: masterSlider.leftPadding
                            y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                            width: masterSlider.availableWidth
                            height: 4
                            radius: 2
                            color: "#333333"

                            Rectangle {
                                width: masterSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: "#ff6b35"
                            }
                        }

                        handle: Rectangle {
                            x: masterSlider.leftPadding + masterSlider.visualPosition * (masterSlider.availableWidth - width)
                            y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                            width: 14
                            height: 14
                            radius: 7
                            color: masterSlider.pressed ? "#ff6b35" : "#ffffff"
                        }
                    }

                    Label {
                        text: daemon.masterVolume
                        font.pixelSize: 12
                        font.bold: true
                        color: "#ffffff"
                        width: 30
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // Content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: currentTab

            MixerView {}
            AppsView {}
            SettingsView {}
        }
    }
}
