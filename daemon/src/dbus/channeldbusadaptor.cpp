#include "channeldbusadaptor.h"
#include "../audiomanager.h"

namespace WaveMux {

ChannelDBusAdaptor::ChannelDBusAdaptor(AudioManager *manager)
    : QDBusAbstractAdaptor(manager)
    , m_manager(manager)
{
    connect(m_manager, &AudioManager::channelsChanged,
            this, &ChannelDBusAdaptor::ChannelsChanged);
}

QVariantList ChannelDBusAdaptor::ListChannels() {
    QVariantList result;
    for (const auto &ch : m_manager->listChannels()) {
        QVariantMap map;
        map["id"] = ch.id;
        map["displayName"] = ch.displayName;
        map["sinkName"] = ch.sinkName;
        map["volume"] = ch.volume;
        map["muted"] = ch.muted;
        map["personalVolume"] = ch.personalVolume;
        map["streamVolume"] = ch.streamVolume;
        map["personalMuted"] = ch.personalMuted;
        map["streamMuted"] = ch.streamMuted;
        result.append(map);
    }
    return result;
}

bool ChannelDBusAdaptor::SetChannelVolume(const QString &channelId, int volume) {
    return m_manager->setChannelVolume(channelId, volume);
}

bool ChannelDBusAdaptor::SetChannelMute(const QString &channelId, bool muted) {
    return m_manager->setChannelMute(channelId, muted);
}

bool ChannelDBusAdaptor::SetChannelPersonalVolume(const QString &channelId, int volume) {
    return m_manager->setChannelPersonalVolume(channelId, volume);
}

bool ChannelDBusAdaptor::SetChannelStreamVolume(const QString &channelId, int volume) {
    return m_manager->setChannelStreamVolume(channelId, volume);
}

bool ChannelDBusAdaptor::SetChannelPersonalMute(const QString &channelId, bool muted) {
    return m_manager->setChannelPersonalMute(channelId, muted);
}

bool ChannelDBusAdaptor::SetChannelStreamMute(const QString &channelId, bool muted) {
    return m_manager->setChannelStreamMute(channelId, muted);
}

} // namespace WaveMux
