#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <csignal>
#include "wavemux/types.h"
#include "audiomanager.h"
#include "configmanager.h"
#include "dbus/channeldbusadaptor.h"
#include "dbus/streamdbusadaptor.h"
#include "dbus/devicedbusadaptor.h"
#include "dbus/configdbusadaptor.h"

static WaveMux::AudioManager *g_audioManager = nullptr;
static WaveMux::ConfigManager *g_configManager = nullptr;

void signalHandler(int signal) {
    qInfo() << "Received signal" << signal << "- shutting down...";
    if (g_configManager) {
        g_configManager->save();
    }
    if (g_audioManager) {
        g_audioManager->shutdown();
    }
    QCoreApplication::quit();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("wavemuxd");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("WaveMux");

    WaveMux::registerMetaTypes();

    // Setup signal handlers for clean shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Connect to session bus
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical() << "Cannot connect to D-Bus session bus";
        return 1;
    }

    // Register service name
    if (!bus.registerService("com.wavemux.Daemon")) {
        qCritical() << "Cannot register D-Bus service - is another instance running?";
        return 1;
    }

    // Initialize audio manager
    WaveMux::AudioManager audioManager;
    g_audioManager = &audioManager;

    // Initialize config manager
    WaveMux::ConfigManager configManager(&audioManager);
    g_configManager = &configManager;

    // Create DBus adaptors (one per responsibility)
    new WaveMux::ChannelDBusAdaptor(&audioManager);
    new WaveMux::StreamDBusAdaptor(&audioManager);
    new WaveMux::DeviceDBusAdaptor(&audioManager);
    new WaveMux::ConfigDBusAdaptor(&audioManager, &configManager);

    // Register object on DBus
    if (!bus.registerObject("/", &audioManager)) {
        qCritical() << "Cannot register D-Bus object";
        return 1;
    }

    QObject::connect(&audioManager, &WaveMux::AudioManager::error,
        [](const QString &msg) {
            qCritical() << "Audio error:" << msg;
        });

    if (!audioManager.initialize()) {
        qCritical() << "Failed to initialize audio manager";
        return 1;
    }

    // Load saved configuration AFTER initialize (channels must exist first)
    configManager.load();

    // Connect auto-save (save settings when they change)
    configManager.connectAutoSave();

    qInfo() << "WaveMux daemon started";
    qInfo() << "D-Bus service: com.wavemux.Daemon";
    qInfo() << "Channels:" << audioManager.listChannels().size();
    qInfo() << "Output devices:" << audioManager.listOutputDevices().size();
    qInfo() << "Setup complete:" << configManager.isSetupComplete();

    int result = app.exec();

    g_configManager = nullptr;
    g_audioManager = nullptr;
    return result;
}
