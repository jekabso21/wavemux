#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QProcess>
#include <optional>
#include "wavemux/types.h"

class QProcess;

namespace WaveMux {

struct SinkInfo {
    uint32_t index = 0;
    uint32_t moduleId = 0;
    QString name;
    int volume = 0;
    bool muted = false;
};

struct StreamInfo {
    uint32_t id = 0;
    QString appName;
    QString mediaName;
    QString processName;
    QString currentSink;
};

class AudioManager : public QObject {
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager();

    bool initialize();
    void shutdown();

    // Channel management
    QList<Channel> listChannels() const;
    bool setChannelVolume(const QString &channelId, int volume);
    bool setChannelMute(const QString &channelId, bool muted);

    // Mix management
    bool setMixVolume(const QString &mixId, int volume);
    bool setMasterVolume(int volume);

    // Mix volumes per channel
    bool setChannelPersonalVolume(const QString &channelId, int volume);
    bool setChannelStreamVolume(const QString &channelId, int volume);
    bool setChannelPersonalMute(const QString &channelId, bool muted);
    bool setChannelStreamMute(const QString &channelId, bool muted);

    // Device management
    QList<Device> listOutputDevices() const;
    bool setOutputDevice(const QString &deviceId);
    QString getOutputDevice() const { return m_outputDevice; }

    // Master volume
    int getMasterVolume() const { return m_masterVolume; }

    // Stream management
    QList<Stream> listStreams() const;
    bool moveStreamToChannel(uint32_t streamId, const QString &channelId);
    bool unassignStream(uint32_t streamId);
    QString getStreamChannel(uint32_t streamId) const;

    // Routing rules
    void addRoutingRule(const QString &pattern, const QString &channelId);
    void removeRoutingRule(const QString &pattern);
    QList<RoutingRule> getRoutingRules() const;
    void applyRoutingRulesToExistingStreams();

    // Loopback routing
    bool updateLoopbacks();

    // Stream mix management
    bool setStreamOutputDevice(const QString &deviceId);
    QString getStreamOutputDevice() const { return m_streamOutputDevice; }
    bool setStreamEnabled(bool enabled);
    bool isStreamEnabled() const { return m_streamEnabled; }
    bool updateStreamLoopbacks();

signals:
    void channelsChanged();
    void mixesChanged();
    void streamsChanged();
    void streamAdded(uint32_t streamId, const QString &appName);
    void streamRemoved(uint32_t streamId);
    void masterVolumeChanged(int volume);
    void routingRulesChanged();
    void error(const QString &message);

private:
    struct ChannelState {
        QString id;
        QString displayName;
        QString sinkName;
        uint32_t sinkIndex = 0;
        uint32_t moduleId = 0;
        int volume = 100;
        bool muted = false;
        int personalVolume = 100;  // 0-100, mix level for personal output
        int streamVolume = 0;      // 0-100, mix level for stream output
        bool personalMuted = false;
        bool streamMuted = false;
    };

    bool createVirtualSink(const QString &name, const QString &description);
    bool removeVirtualSink(uint32_t moduleId);
    std::optional<SinkInfo> getSinkInfo(const QString &name) const;
    bool setSinkVolume(const QString &sinkName, int volume);
    bool setSinkMute(const QString &sinkName, bool muted);

    bool runCommand(const QString &command, QString *output = nullptr) const;
    bool createChannels();
    bool createMixes();
    void setupRouting();

    // Stream detection
    void startStreamMonitor();
    void stopStreamMonitor();
    void handleMonitorOutput();
    void handleStreamEvent(const QString &eventType, uint32_t id);
    std::optional<StreamInfo> getStreamInfo(uint32_t id) const;
    void applyRoutingRules(uint32_t streamId, const QString &appName, const QString &processName);
    void syncExistingStreams();

    // Loopback management
    bool createLoopback(const QString &sourceSink, const QString &targetSink);
    bool removeLoopback(uint32_t moduleId);
    void removeAllLoopbacks();
    void applyMasterToLoopbacks();
    uint32_t findLoopbackSinkInput(uint32_t moduleId) const;
    bool addChannelLoopback(const QString &channelId);
    bool removeChannelLoopback(const QString &channelId);

    // Stream loopback management
    bool addStreamChannelLoopback(const QString &channelId);
    bool removeStreamChannelLoopback(const QString &channelId);
    void removeAllStreamLoopbacks();

    QHash<QString, ChannelState> m_channels;
    QHash<uint32_t, QString> m_streamAssignments;  // streamId -> channelId
    QList<RoutingRule> m_routingRules;
    QHash<QString, uint32_t> m_loopbackModules;    // channelId -> moduleId (personal mix)
    QHash<QString, uint32_t> m_loopbackSinkInputs; // channelId -> sink-input ID (personal mix)
    QHash<QString, uint32_t> m_streamLoopbackModules;    // channelId -> moduleId (stream mix)
    QHash<QString, uint32_t> m_streamLoopbackSinkInputs; // channelId -> sink-input ID (stream mix)
    uint32_t m_unassignedSinkModule = 0;                 // Module ID for the silent unassigned sink
    QString m_unassignedSinkName = "wavemux_unassigned"; // Sink where unassigned streams go (silent)
    QProcess *m_monitorProcess = nullptr;
    uint32_t m_personalMixModule = 0;
    uint32_t m_streamMixModule = 0;
    int m_masterVolume = 100;
    QString m_outputDevice;
    QString m_streamOutputDevice;
    bool m_streamEnabled = false;
    bool m_initialized = false;
};

} // namespace WaveMux
