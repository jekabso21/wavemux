#include "dbusclient.h"
#include <QDBusReply>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QDebug>
#include <QDateTime>

namespace {
    QVariantMap extractMap(const QVariant &v) {
        if (v.canConvert<QDBusArgument>()) {
            QDBusArgument arg = v.value<QDBusArgument>();
            QVariantMap map;
            arg >> map;
            return map;
        }
        return v.toMap();
    }
}

namespace WaveMux {

DBusClient::DBusClient(QObject *parent)
    : QObject(parent)
{
}

DBusClient::~DBusClient() {
    disconnectFromDaemon();
}

bool DBusClient::connectToDaemon() {
    if (m_connected) {
        return true;
    }

    const QString service = "com.wavemux.Daemon";
    const QString path = "/";

    // Create interface for each adaptor
    m_channelInterface = new QDBusInterface(service, path, "com.wavemux.Channels", QDBusConnection::sessionBus(), this);
    m_streamInterface = new QDBusInterface(service, path, "com.wavemux.Streams", QDBusConnection::sessionBus(), this);
    m_deviceInterface = new QDBusInterface(service, path, "com.wavemux.Devices", QDBusConnection::sessionBus(), this);
    m_configInterface = new QDBusInterface(service, path, "com.wavemux.Config", QDBusConnection::sessionBus(), this);

    if (!m_channelInterface->isValid()) {
        qWarning() << "Failed to connect to WaveMux daemon:" << m_channelInterface->lastError().message();
        disconnectFromDaemon();
        return false;
    }

    // Connect signals from Channels interface
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Channels",
        "ChannelsChanged", this, SLOT(onChannelsChanged()));

    // Connect signals from Streams interface
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Streams",
        "StreamsChanged", this, SLOT(onStreamsChanged()));
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Streams",
        "StreamAdded", this, SLOT(onStreamAdded(uint,QString)));
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Streams",
        "StreamRemoved", this, SLOT(onStreamRemoved(uint)));

    // Connect signals from Config interface
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Config",
        "Error", this, SLOT(onError(QString)));
    QDBusConnection::sessionBus().connect(service, path, "com.wavemux.Config",
        "StreamEnabledChanged", this, SLOT(onStreamEnabledChanged(bool)));

    m_connected = true;
    emit connectedChanged();

    // Fetch initial data
    refresh();

    qInfo() << "Connected to WaveMux daemon";
    return true;
}

void DBusClient::disconnectFromDaemon() {
    if (!m_connected) {
        return;
    }

    delete m_channelInterface;
    delete m_streamInterface;
    delete m_deviceInterface;
    delete m_configInterface;
    m_channelInterface = nullptr;
    m_streamInterface = nullptr;
    m_deviceInterface = nullptr;
    m_configInterface = nullptr;
    m_connected = false;
    emit connectedChanged();
}

void DBusClient::refresh() {
    if (!m_connected) return;
    fetchChannels();
    fetchStreams();
    fetchDevices();
    fetchConfig();
}

void DBusClient::fetchChannels() {
    QDBusReply<QVariantList> reply = m_channelInterface->call("ListChannels");
    if (!reply.isValid()) {
        qWarning() << "Failed to list channels:" << reply.error().message();
        return;
    }

    m_channels.clear();
    for (const auto &item : reply.value()) {
        QVariantMap map = extractMap(item);
        Channel ch;
        ch.id = map["id"].toString();
        ch.displayName = map["displayName"].toString();
        ch.sinkName = map["sinkName"].toString();
        ch.volume = map["volume"].toInt();
        ch.muted = map["muted"].toBool();
        ch.personalVolume = map["personalVolume"].toInt();
        ch.streamVolume = map["streamVolume"].toInt();
        m_channels.append(ch);
    }
    emit channelsChanged();
}

void DBusClient::fetchStreams() {
    QDBusReply<QVariantList> reply = m_streamInterface->call("ListStreams");
    if (!reply.isValid()) {
        qWarning() << "Failed to list streams:" << reply.error().message();
        return;
    }

    m_streams.clear();
    for (const auto &item : reply.value()) {
        QVariantMap map = extractMap(item);
        Stream stream;
        stream.id = map["id"].toUInt();
        stream.appName = map["appName"].toString();
        stream.mediaName = map["mediaName"].toString();
        stream.processName = map["processName"].toString();
        stream.assignedChannel = map["assignedChannel"].toString();
        m_streams.append(stream);
    }
    emit streamsChanged();
}

void DBusClient::fetchDevices() {
    // Output devices
    QDBusReply<QVariantList> outReply = m_deviceInterface->call("ListOutputDevices");
    if (outReply.isValid()) {
        m_outputDevices.clear();
        for (const auto &item : outReply.value()) {
            QVariantMap map = extractMap(item);
            Device dev;
            dev.id = map["id"].toString();
            dev.name = map["name"].toString();
            dev.description = map["description"].toString();
            m_outputDevices.append(dev);
        }
    } else {
        qWarning() << "Failed to list output devices:" << outReply.error().message();
    }

    emit devicesChanged();
}

void DBusClient::fetchConfig() {
    // Setup complete
    QDBusReply<bool> setupReply = m_configInterface->call("IsSetupComplete");
    if (setupReply.isValid()) {
        bool oldValue = m_setupComplete;
        m_setupComplete = setupReply.value();
        if (oldValue != m_setupComplete) {
            emit setupCompleteChanged();
        }
    }

    // Output device
    QDBusReply<QString> deviceReply = m_deviceInterface->call("GetOutputDevice");
    if (deviceReply.isValid()) {
        QString oldValue = m_outputDevice;
        m_outputDevice = deviceReply.value();
        if (oldValue != m_outputDevice) {
            emit outputDeviceChanged();
        }
    }

    // Master volume
    QDBusReply<int> volReply = m_configInterface->call("GetMasterVolume");
    if (volReply.isValid()) {
        int oldValue = m_masterVolume;
        m_masterVolume = volReply.value();
        if (oldValue != m_masterVolume) {
            emit masterVolumeChanged();
        }
    }

    // Stream output device
    QDBusReply<QString> streamDeviceReply = m_deviceInterface->call("GetStreamOutputDevice");
    if (streamDeviceReply.isValid()) {
        QString oldValue = m_streamOutputDevice;
        m_streamOutputDevice = streamDeviceReply.value();
        if (oldValue != m_streamOutputDevice) {
            emit streamOutputDeviceChanged();
        }
    }

    // Stream enabled
    QDBusReply<bool> streamEnabledReply = m_configInterface->call("IsStreamEnabled");
    if (streamEnabledReply.isValid()) {
        bool oldValue = m_streamEnabled;
        m_streamEnabled = streamEnabledReply.value();
        if (oldValue != m_streamEnabled) {
            emit streamEnabledChanged();
        }
    }
}

bool DBusClient::setChannelVolume(const QString &channelId, int volume) {
    qDebug() << "DBusClient::setChannelVolume called:" << channelId << volume;
    if (!m_connected) {
        qDebug() << "Not connected!";
        return false;
    }

    // Set debounce timestamp to ignore incoming channelsChanged signals during drag
    m_lastVolumeChangeTime = QDateTime::currentMSecsSinceEpoch();

    // Send to daemon
    QDBusReply<bool> reply = m_channelInterface->call("SetChannelVolume", channelId, volume);
    qDebug() << "DBus reply valid:" << reply.isValid() << "value:" << (reply.isValid() ? reply.value() : false);
    return reply.isValid() && reply.value();
}

bool DBusClient::setChannelMute(const QString &channelId, bool muted) {
    if (!m_connected) return false;
    QDBusReply<bool> reply = m_channelInterface->call("SetChannelMute", channelId, muted);
    return reply.isValid() && reply.value();
}

bool DBusClient::setChannelPersonalVolume(const QString &channelId, int volume) {
    if (!m_connected) return false;
    // Set debounce timestamp to ignore incoming channelsChanged signals during drag
    m_lastVolumeChangeTime = QDateTime::currentMSecsSinceEpoch();
    QDBusReply<bool> reply = m_channelInterface->call("SetChannelPersonalVolume", channelId, volume);
    return reply.isValid() && reply.value();
}

bool DBusClient::setChannelStreamVolume(const QString &channelId, int volume) {
    if (!m_connected) return false;
    // Set debounce timestamp to ignore incoming channelsChanged signals during drag
    m_lastVolumeChangeTime = QDateTime::currentMSecsSinceEpoch();
    QDBusReply<bool> reply = m_channelInterface->call("SetChannelStreamVolume", channelId, volume);
    return reply.isValid() && reply.value();
}

bool DBusClient::moveStreamToChannel(uint streamId, const QString &channelId) {
    if (!m_connected) return false;
    QDBusReply<bool> reply = m_streamInterface->call("MoveStreamToChannel", streamId, channelId);
    return reply.isValid() && reply.value();
}

bool DBusClient::unassignStream(uint streamId) {
    if (!m_connected) return false;
    QDBusReply<bool> reply = m_streamInterface->call("UnassignStream", streamId);
    return reply.isValid() && reply.value();
}

void DBusClient::addRoutingRule(const QString &pattern, const QString &channelId) {
    if (!m_connected) return;
    m_streamInterface->call("AddRoutingRule", pattern, channelId);
}

void DBusClient::removeRoutingRule(const QString &pattern) {
    if (!m_connected) return;
    m_streamInterface->call("RemoveRoutingRule", pattern);
}

QList<RoutingRule> DBusClient::getRoutingRules() {
    QList<RoutingRule> result;
    if (!m_connected) return result;

    QDBusReply<QVariantList> reply = m_streamInterface->call("GetRoutingRules");
    if (reply.isValid()) {
        for (const auto &item : reply.value()) {
            QVariantMap map = extractMap(item);
            RoutingRule rule;
            rule.matchPattern = map["matchPattern"].toString();
            rule.targetChannel = map["targetChannel"].toString();
            result.append(rule);
        }
    }
    return result;
}

void DBusClient::setOutputDevice(const QString &deviceId) {
    if (!m_connected) return;
    m_deviceInterface->call("SetOutputDevice", deviceId);
    m_outputDevice = deviceId;
    emit outputDeviceChanged();
}

void DBusClient::setStreamOutputDevice(const QString &deviceId) {
    if (!m_connected) return;
    m_deviceInterface->call("SetStreamOutputDevice", deviceId);
    m_streamOutputDevice = deviceId;
    emit streamOutputDeviceChanged();
}

void DBusClient::setStreamEnabled(bool enabled) {
    if (!m_connected) return;
    m_configInterface->call("SetStreamEnabled", enabled);
    m_streamEnabled = enabled;
    emit streamEnabledChanged();
}

void DBusClient::setMasterVolume(int volume) {
    if (!m_connected) return;
    m_configInterface->call("SetMasterVolume", volume);
    m_masterVolume = volume;
    emit masterVolumeChanged();
}

void DBusClient::setSetupComplete(bool complete) {
    if (!m_connected) return;
    m_configInterface->call("SetSetupComplete", complete);
    m_setupComplete = complete;
    emit setupCompleteChanged();
}

void DBusClient::saveConfig() {
    if (!m_connected) return;
    m_configInterface->call("SaveConfig");
}

void DBusClient::onChannelsChanged() {
    // Skip if we recently changed volume (debounce to prevent UI jitter during dragging)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastVolumeChangeTime < VOLUME_CHANGE_DEBOUNCE_MS) {
        return;
    }
    fetchChannels();
}

void DBusClient::onStreamsChanged() {
    fetchStreams();
}

void DBusClient::onStreamAdded(uint streamId, const QString &appName) {
    emit streamAdded(streamId, appName);
    fetchStreams();
}

void DBusClient::onStreamRemoved(uint streamId) {
    emit streamRemoved(streamId);
    fetchStreams();
}

void DBusClient::onError(const QString &message) {
    emit error(message);
}

void DBusClient::onStreamEnabledChanged(bool enabled) {
    m_streamEnabled = enabled;
    emit streamEnabledChanged();
}

QVariantList DBusClient::channelsVariant() const {
    QVariantList result;
    for (const auto &ch : m_channels) {
        QVariantMap map;
        map["id"] = ch.id;
        map["displayName"] = ch.displayName;
        map["sinkName"] = ch.sinkName;
        map["volume"] = ch.volume;
        map["muted"] = ch.muted;
        map["personalVolume"] = ch.personalVolume;
        map["streamVolume"] = ch.streamVolume;
        result.append(map);
    }
    return result;
}

QVariantList DBusClient::streamsVariant() const {
    QVariantList result;
    for (const auto &stream : m_streams) {
        QVariantMap map;
        map["id"] = stream.id;
        map["appName"] = stream.appName;
        map["mediaName"] = stream.mediaName;
        map["processName"] = stream.processName;
        map["assignedChannel"] = stream.assignedChannel;
        result.append(map);
    }
    return result;
}

QVariantList DBusClient::outputDevicesVariant() const {
    QVariantList result;
    for (const auto &dev : m_outputDevices) {
        QVariantMap map;
        map["id"] = dev.id;
        map["name"] = dev.name;
        map["description"] = dev.description;
        result.append(map);
    }
    return result;
}

} // namespace WaveMux
