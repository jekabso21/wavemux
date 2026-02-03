#pragma once

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QVariantList>
#include <QVariantMap>
#include "wavemux/types.h"

namespace WaveMux {

class DBusClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool setupComplete READ isSetupComplete NOTIFY setupCompleteChanged)
    Q_PROPERTY(QVariantList channels READ channelsVariant NOTIFY channelsChanged)
    Q_PROPERTY(QVariantList streams READ streamsVariant NOTIFY streamsChanged)
    Q_PROPERTY(QVariantList outputDevices READ outputDevicesVariant NOTIFY devicesChanged)
    Q_PROPERTY(QString outputDevice READ outputDevice WRITE setOutputDevice NOTIFY outputDeviceChanged)
    Q_PROPERTY(QString streamOutputDevice READ streamOutputDevice WRITE setStreamOutputDevice NOTIFY streamOutputDeviceChanged)
    Q_PROPERTY(bool streamEnabled READ isStreamEnabled WRITE setStreamEnabled NOTIFY streamEnabledChanged)
    Q_PROPERTY(int masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)

public:
    explicit DBusClient(QObject *parent = nullptr);
    ~DBusClient();

    bool isConnected() const { return m_connected; }
    bool isSetupComplete() const { return m_setupComplete; }

    QList<Channel> channels() const { return m_channels; }
    QList<Stream> streams() const { return m_streams; }
    QList<Device> outputDevices() const { return m_outputDevices; }

    // QML-friendly variants
    QVariantList channelsVariant() const;
    QVariantList streamsVariant() const;
    QVariantList outputDevicesVariant() const;

    QString outputDevice() const { return m_outputDevice; }
    QString streamOutputDevice() const { return m_streamOutputDevice; }
    bool isStreamEnabled() const { return m_streamEnabled; }
    int masterVolume() const { return m_masterVolume; }

public slots:
    bool connectToDaemon();
    void disconnectFromDaemon();

    // Channel operations
    bool setChannelVolume(const QString &channelId, int volume);
    bool setChannelMute(const QString &channelId, bool muted);
    bool setChannelPersonalVolume(const QString &channelId, int volume);
    bool setChannelStreamVolume(const QString &channelId, int volume);

    // Stream operations
    bool moveStreamToChannel(uint streamId, const QString &channelId);
    bool unassignStream(uint streamId);

    // Routing rules
    void addRoutingRule(const QString &pattern, const QString &channelId);
    void removeRoutingRule(const QString &pattern);
    QList<RoutingRule> getRoutingRules();

    // Device selection
    void setOutputDevice(const QString &deviceId);
    void setStreamOutputDevice(const QString &deviceId);

    // Stream mode
    void setStreamEnabled(bool enabled);

    // Master volume
    void setMasterVolume(int volume);

    // Config
    void setSetupComplete(bool complete);
    void saveConfig();

    // Refresh data from daemon
    void refresh();

signals:
    void connectedChanged();
    void setupCompleteChanged();
    void channelsChanged();
    void streamsChanged();
    void devicesChanged();
    void outputDeviceChanged();
    void streamOutputDeviceChanged();
    void streamEnabledChanged();
    void masterVolumeChanged();
    void streamAdded(uint streamId, const QString &appName);
    void streamRemoved(uint streamId);
    void error(const QString &message);

private slots:
    void onChannelsChanged();
    void onStreamsChanged();
    void onStreamAdded(uint streamId, const QString &appName);
    void onStreamRemoved(uint streamId);
    void onError(const QString &message);
    void onStreamEnabledChanged(bool enabled);

private:
    void fetchChannels();
    void fetchStreams();
    void fetchDevices();
    void fetchConfig();

    // Separate interfaces for each DBus adaptor
    QDBusInterface *m_channelInterface = nullptr;
    QDBusInterface *m_streamInterface = nullptr;
    QDBusInterface *m_deviceInterface = nullptr;
    QDBusInterface *m_configInterface = nullptr;

    bool m_connected = false;
    bool m_setupComplete = false;
    QList<Channel> m_channels;
    QList<Stream> m_streams;
    QList<Device> m_outputDevices;
    QString m_outputDevice;
    QString m_streamOutputDevice;
    bool m_streamEnabled = false;
    int m_masterVolume = 100;

    // Debounce channel updates during user interaction
    qint64 m_lastVolumeChangeTime = 0;
    static constexpr qint64 VOLUME_CHANGE_DEBOUNCE_MS = 500;
};

} // namespace WaveMux
