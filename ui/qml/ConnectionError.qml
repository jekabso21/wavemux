import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "transparent"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            text: "Cannot Connect to Daemon"
            font.pixelSize: 24
            font.bold: true
            color: "#e94560"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "The WaveMux daemon is not running."
            font.pixelSize: 14
            color: "#cccccc"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Start it with: wavemuxd"
            font.pixelSize: 12
            font.family: "monospace"
            color: "#888888"
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Retry Connection"
            Layout.alignment: Qt.AlignHCenter
            onClicked: daemon.connectToDaemon()
        }
    }
}
