#pragma once

#include <QDBusAbstractAdaptor>
#include <QVariantList>
#include <QVariantMap>

namespace WaveMux {

class AudioManager;

class StreamDBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.wavemux.Streams")

public:
    explicit StreamDBusAdaptor(AudioManager *manager);

public slots:
    QVariantList ListStreams();
    bool MoveStreamToChannel(uint streamId, const QString &channelId);
    bool UnassignStream(uint streamId);

    // Routing rules
    void AddRoutingRule(const QString &pattern, const QString &channelId);
    void RemoveRoutingRule(const QString &pattern);
    QVariantList GetRoutingRules();

signals:
    void StreamsChanged();
    void StreamAdded(uint streamId, const QString &appName);
    void StreamRemoved(uint streamId);

private:
    AudioManager *m_manager;
};

} // namespace WaveMux
