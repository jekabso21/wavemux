import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: strip
    color: "#252542"
    radius: 12

    property string channelId: ""
    property string channelName: ""
    property int volume: 100
    property bool muted: false
    property bool inPersonalMix: true
    property bool inStreamMix: false

    signal volumeAdjusted(int newVolume)
    signal muteToggled(bool isMuted)
    signal personalToggled(bool inPersonal)
    signal streamToggled(bool inStream)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 12

        // Channel name
        Label {
            text: channelName
            font.pixelSize: 16
            font.bold: true
            color: muted ? "#666666" : "#ffffff"
            Layout.alignment: Qt.AlignHCenter
        }

        // Volume slider (vertical)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Slider {
                id: volumeSlider
                anchors.centerIn: parent
                orientation: Qt.Vertical
                height: parent.height
                from: 0
                to: 100
                value: volume
                enabled: !muted

                onMoved: strip.volumeAdjusted(value)

                background: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding
                    width: 8
                    height: volumeSlider.availableHeight
                    radius: 4
                    color: "#333355"

                    Rectangle {
                        width: parent.width
                        height: volumeSlider.visualPosition * parent.height
                        y: parent.height - height
                        radius: 4
                        color: muted ? "#555555" : "#e94560"
                    }
                }

                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                    y: volumeSlider.topPadding + volumeSlider.visualPosition * (volumeSlider.availableHeight - height)
                    width: 24
                    height: 14
                    radius: 4
                    color: muted ? "#666666" : "#ffffff"
                }
            }
        }

        // Volume percentage
        Label {
            text: Math.round(volumeSlider.value) + "%"
            font.pixelSize: 14
            color: muted ? "#555555" : "#ffffff"
            Layout.alignment: Qt.AlignHCenter
        }

        // Mute button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            onClicked: strip.muteToggled(!muted)

            background: Rectangle {
                radius: 6
                color: muted ? "#e94560" : "#333355"
            }

            contentItem: Label {
                text: muted ? "MUTED" : "MUTE"
                font.pixelSize: 11
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Include toggles
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Personal mix toggle
            Rectangle {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 32
                radius: 6
                color: inPersonalMix ? "#4a9c6d" : "#333355"

                Label {
                    anchors.centerIn: parent
                    text: "P"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#ffffff"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: strip.personalToggled(!inPersonalMix)
                    cursorShape: Qt.PointingHandCursor
                }
            }

            // Stream mix toggle
            Rectangle {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 32
                radius: 6
                color: inStreamMix ? "#9c4a8c" : "#333355"

                Label {
                    anchors.centerIn: parent
                    text: "S"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#ffffff"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: strip.streamToggled(!inStreamMix)
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
}
