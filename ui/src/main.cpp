#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDBusConnection>
#include <QDebug>
#include <QTimer>
#include "wavemux/types.h"
#include "dbusclient.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("WaveMux");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("WaveMux");

    QQuickStyle::setStyle("Fusion");

    WaveMux::registerMetaTypes();

    // Create DBus client
    WaveMux::DBusClient client;

    // Try to connect to daemon
    if (!client.connectToDaemon()) {
        qWarning() << "Could not connect to WaveMux daemon - is it running?";
        // Continue anyway, UI will show connection status
    }

    QQmlApplicationEngine engine;

    // Expose client to QML
    engine.rootContext()->setContextProperty("daemon", &client);

    const QUrl url(QStringLiteral("qrc:/WaveMux/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    return app.exec();
}
