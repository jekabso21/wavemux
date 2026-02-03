#include "streamdbusadaptor.h"
#include "../audiomanager.h"

namespace WaveMux {

StreamDBusAdaptor::StreamDBusAdaptor(AudioManager *manager)
    : QDBusAbstractAdaptor(manager)
    , m_manager(manager)
{
    connect(m_manager, &AudioManager::streamsChanged,
            this, &StreamDBusAdaptor::StreamsChanged);
    connect(m_manager, &AudioManager::streamAdded,
            this, &StreamDBusAdaptor::StreamAdded);
    connect(m_manager, &AudioManager::streamRemoved,
            this, &StreamDBusAdaptor::StreamRemoved);
}

QVariantList StreamDBusAdaptor::ListStreams() {
    QVariantList result;
    for (const auto &stream : m_manager->listStreams()) {
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

bool StreamDBusAdaptor::MoveStreamToChannel(uint streamId, const QString &channelId) {
    return m_manager->moveStreamToChannel(streamId, channelId);
}

bool StreamDBusAdaptor::UnassignStream(uint streamId) {
    return m_manager->unassignStream(streamId);
}

void StreamDBusAdaptor::AddRoutingRule(const QString &pattern, const QString &channelId) {
    m_manager->addRoutingRule(pattern, channelId);
}

void StreamDBusAdaptor::RemoveRoutingRule(const QString &pattern) {
    m_manager->removeRoutingRule(pattern);
}

QVariantList StreamDBusAdaptor::GetRoutingRules() {
    QVariantList result;
    for (const auto &rule : m_manager->getRoutingRules()) {
        QVariantMap map;
        map["matchPattern"] = rule.matchPattern;
        map["targetChannel"] = rule.targetChannel;
        result.append(map);
    }
    return result;
}

} // namespace WaveMux
