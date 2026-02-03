import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 900
    height: 650
    minimumWidth: 700
    minimumHeight: 500
    visible: true
    title: "WaveMux"
    color: "#1a1a2e"

    Component.onCompleted: {
        console.log("Main window loaded")
        console.log("  daemon.connected:", daemon.connected)
        console.log("  daemon.setupComplete:", daemon.setupComplete)
    }

    Connections {
        target: daemon
        function onSetupCompleteChanged() {
            console.log("Setup complete changed to:", daemon.setupComplete)
        }
        function onConnectedChanged() {
            console.log("Connected changed to:", daemon.connected)
        }
    }

    // Show wizard if not setup complete, otherwise show main UI
    Loader {
        id: mainLoader
        anchors.fill: parent
        source: {
            console.log("Loader deciding: connected=" + daemon.connected + " setupComplete=" + daemon.setupComplete)
            if (!daemon.connected) {
                return "ConnectionError.qml"
            } else if (!daemon.setupComplete) {
                return "SetupWizard.qml"
            } else {
                return "MainView.qml"
            }
        }

        onLoaded: {
            console.log("Loaded:", source)
        }
    }
}
