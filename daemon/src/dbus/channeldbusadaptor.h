#pragma once

#include <QDBusAbstractAdaptor>
#include <QVariantList>
#include <QVariantMap>

namespace WaveMux {

class AudioManager;

class ChannelDBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.wavemux.Channels")

public:
    explicit ChannelDBusAdaptor(AudioManager *manager);

public slots:
    QVariantList ListChannels();
    bool SetChannelVolume(const QString &channelId, int volume);
    bool SetChannelMute(const QString &channelId, bool muted);
    bool SetChannelPersonalVolume(const QString &channelId, int volume);
    bool SetChannelStreamVolume(const QString &channelId, int volume);
    bool SetChannelPersonalMute(const QString &channelId, bool muted);
    bool SetChannelStreamMute(const QString &channelId, bool muted);

signals:
    void ChannelsChanged();

private:
    AudioManager *m_manager;
};

} // namespace WaveMux
