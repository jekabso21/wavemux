#include "audiomanager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QThread>

namespace WaveMux {

namespace {
    const QStringList CHANNEL_IDS = {"game", "chat", "media", "aux"};
    const QHash<QString, QString> CHANNEL_NAMES = {
        {"game", "Game"},
        {"chat", "Chat"},
        {"media", "Media"},
        {"aux", "AUX"}
    };
}

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
{
}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::runCommand(const QString &command, QString *output) const {
    QProcess process;
    process.start("sh", {"-c", command});

    if (!process.waitForFinished(5000)) {
        qWarning() << "Command timed out:" << command;
        return false;
    }

    if (output) {
        *output = QString::fromUtf8(process.readAllStandardOutput());
    }

    if (process.exitCode() != 0) {
        qWarning() << "Command failed:" << command;
        qWarning() << "stderr:" << process.readAllStandardError();
        return false;
    }

    return true;
}

bool AudioManager::createVirtualSink(const QString &name, const QString &description) {
    // Replace spaces with dashes for PipeWire compatibility
    QString safeDesc = description;
    safeDesc.replace(' ', '-');

    // Explicit stereo configuration to ensure proper channel handling
    QString cmd = QString("pactl load-module module-null-sink sink_name=%1 "
                          "sink_properties=device.description=%2 "
                          "channel_map=front-left,front-right")
        .arg(name, safeDesc);

    QString output;
    if (!runCommand(cmd, &output)) {
        emit error(QString("Failed to create sink: %1").arg(name));
        return false;
    }

    qInfo() << "Created virtual sink:" << name << "module:" << output.trimmed();
    return true;
}

bool AudioManager::removeVirtualSink(uint32_t moduleId) {
    QString cmd = QString("pactl unload-module %1").arg(moduleId);
    return runCommand(cmd);
}

std::optional<SinkInfo> AudioManager::getSinkInfo(const QString &name) const {
    QString output;

    // Use short format for getting index
    if (!runCommand("pactl list sinks short", &output)) {
        return std::nullopt;
    }

    SinkInfo info;
    info.name = name;

    // Parse short format: INDEX NAME DRIVER FORMAT STATE
    for (const auto &line : output.split('\n', Qt::SkipEmptyParts)) {
        auto parts = line.split('\t');
        if (parts.size() >= 2 && parts[1] == name) {
            info.index = parts[0].toUInt();
            break;
        }
    }

    if (info.index == 0) {
        return std::nullopt;
    }

    // Get volume and mute state using pactl get commands
    QString volOutput;
    if (runCommand(QString("pactl get-sink-volume %1").arg(name), &volOutput)) {
        QRegularExpression volRe("(\\d+)%");
        auto match = volRe.match(volOutput);
        if (match.hasMatch()) {
            info.volume = match.captured(1).toInt();
        }
    }

    QString muteOutput;
    if (runCommand(QString("pactl get-sink-mute %1").arg(name), &muteOutput)) {
        info.muted = muteOutput.contains("yes");
    }

    return info;
}

bool AudioManager::setSinkVolume(const QString &sinkName, int volume) {
    volume = qBound(0, volume, 100);
    QString cmd = QString("pactl set-sink-volume %1 %2%")
        .arg(sinkName)
        .arg(volume);
    return runCommand(cmd);
}

bool AudioManager::setSinkMute(const QString &sinkName, bool muted) {
    QString cmd = QString("pactl set-sink-mute %1 %2")
        .arg(sinkName)
        .arg(muted ? "1" : "0");
    return runCommand(cmd);
}

bool AudioManager::createChannels() {
    for (const auto &id : CHANNEL_IDS) {
        QString sinkName = QString("wavemux_%1").arg(id);
        QString description = QString("WaveMux %1").arg(CHANNEL_NAMES.value(id));

        // Check if sink already exists
        auto existingInfo = getSinkInfo(sinkName);
        if (existingInfo) {
            qInfo() << "Virtual sink already exists:" << sinkName;
        } else {
            // Create new sink
            if (!createVirtualSink(sinkName, description)) {
                return false;
            }
        }

        // Reset volume to 100% with balanced stereo to prevent stream-restore issues
        setSinkVolume(sinkName, 100);

        auto info = getSinkInfo(sinkName);
        if (!info) {
            qWarning() << "Could not get sink info for:" << sinkName;
            continue;
        }

        ChannelState state;
        state.id = id;
        state.displayName = CHANNEL_NAMES.value(id);
        state.sinkName = sinkName;
        state.sinkIndex = info->index;
        state.volume = 100;
        state.muted = false;
        state.personalVolume = 100;
        state.streamVolume = 0;

        m_channels[id] = state;
    }

    return true;
}

bool AudioManager::createMixes() {
    // For MVP, we don't create separate mix sinks
    // Instead, we'll use loopback modules to route channel audio to hardware output
    // Personal mix = channels routed to headphones
    // Stream mix = OBS captures channel monitors directly

    qInfo() << "Mix routing will be configured in Phase 2 using loopback modules";
    return true;
}

void AudioManager::setupRouting() {
    // For MVP, routing is handled by moving sink-inputs
    // The include matrix determines which channels feed into which mixes
    // This will be implemented when we handle stream detection
    qInfo() << "Routing setup placeholder - will be implemented in Phase 2";
}

bool AudioManager::initialize() {
    if (m_initialized) {
        return true;
    }

    qInfo() << "Initializing audio manager...";

    // Remember current default sink before creating ours
    QString originalDefault;
    runCommand("pactl get-default-sink", &originalDefault);
    originalDefault = originalDefault.trimmed();
    qInfo() << "Current default sink:" << originalDefault;

    // Create the silent unassigned sink first
    // This sink captures all audio that isn't routed to a channel
    if (!createVirtualSink(m_unassignedSinkName, "WaveMux-Unassigned")) {
        emit error("Failed to create unassigned sink");
        return false;
    }
    // Get the module ID for the unassigned sink
    QString moduleOutput;
    if (runCommand(QString("pactl list modules short | grep %1 | cut -f1").arg(m_unassignedSinkName), &moduleOutput)) {
        m_unassignedSinkModule = moduleOutput.trimmed().toUInt();
        qInfo() << "Created unassigned sink, module:" << m_unassignedSinkModule;
    }
    // Mute the unassigned sink so it's completely silent
    setSinkMute(m_unassignedSinkName, true);

    if (!createChannels()) {
        emit error("Failed to create channel sinks");
        return false;
    }

    if (!createMixes()) {
        emit error("Failed to create mix sinks");
        return false;
    }

    // Restore original default sink (PipeWire may have changed it)
    if (!originalDefault.isEmpty() && !originalDefault.startsWith("wavemux_")) {
        runCommand(QString("pactl set-default-sink %1").arg(originalDefault));
        qInfo() << "Restored default sink to:" << originalDefault;
    }

    setupRouting();

    // Start monitoring for new audio streams
    startStreamMonitor();

    m_initialized = true;
    qInfo() << "Audio manager initialized successfully";
    emit channelsChanged();

    return true;
}

void AudioManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    qInfo() << "Shutting down audio manager...";

    // Stop stream monitor
    stopStreamMonitor();

    // Remove personal mix loopbacks
    removeAllLoopbacks();

    // Remove stream mix loopbacks
    removeAllStreamLoopbacks();

    // Remove channel sinks
    for (const auto &channel : m_channels) {
        if (channel.moduleId > 0) {
            removeVirtualSink(channel.moduleId);
        }
    }
    m_channels.clear();
    m_streamAssignments.clear();

    // Remove mix sinks
    if (m_personalMixModule > 0) {
        removeVirtualSink(m_personalMixModule);
        m_personalMixModule = 0;
    }
    if (m_streamMixModule > 0) {
        removeVirtualSink(m_streamMixModule);
        m_streamMixModule = 0;
    }

    // Remove unassigned sink
    if (m_unassignedSinkModule > 0) {
        removeVirtualSink(m_unassignedSinkModule);
        m_unassignedSinkModule = 0;
    }

    m_initialized = false;
}

QList<Channel> AudioManager::listChannels() const {
    QList<Channel> result;
    for (const auto &state : m_channels) {
        Channel ch;
        ch.id = state.id;
        ch.displayName = state.displayName;
        ch.sinkName = state.sinkName;
        ch.volume = state.volume;
        ch.muted = state.muted;
        ch.personalVolume = state.personalVolume;
        ch.streamVolume = state.streamVolume;
        result.append(ch);
    }
    return result;
}

bool AudioManager::setChannelVolume(const QString &channelId, int volume) {
    if (!m_channels.contains(channelId)) {
        return false;
    }

    volume = qBound(0, volume, 100);
    auto &channel = m_channels[channelId];
    if (setSinkVolume(channel.sinkName, volume)) {
        channel.volume = volume;
        emit channelsChanged();
        return true;
    }
    return false;
}

bool AudioManager::setChannelMute(const QString &channelId, bool muted) {
    if (!m_channels.contains(channelId)) {
        return false;
    }

    auto &channel = m_channels[channelId];
    if (setSinkMute(channel.sinkName, muted)) {
        channel.muted = muted;
        emit channelsChanged();
        return true;
    }
    return false;
}

bool AudioManager::setMixVolume(const QString &mixId, int volume) {
    QString sinkName = QString("wavemux_%1").arg(mixId);
    return setSinkVolume(sinkName, volume);
}

bool AudioManager::setMasterVolume(int volume) {
    m_masterVolume = qBound(0, volume, 100);

    // Master volume affects all loopback volumes
    applyMasterToLoopbacks();

    emit masterVolumeChanged(m_masterVolume);
    return true;
}

void AudioManager::applyMasterToLoopbacks() {
    // Set volume on all tracked personal mix loopback sink-inputs
    for (auto it = m_loopbackSinkInputs.begin(); it != m_loopbackSinkInputs.end(); ++it) {
        const QString &channelId = it.key();
        if (m_channels.contains(channelId)) {
            int effectiveVolume = (m_channels[channelId].personalVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(it.value()).arg(effectiveVolume));
        }
    }

    // Set volume on all tracked stream mix loopback sink-inputs
    for (auto it = m_streamLoopbackSinkInputs.begin(); it != m_streamLoopbackSinkInputs.end(); ++it) {
        const QString &channelId = it.key();
        if (m_channels.contains(channelId)) {
            int effectiveVolume = (m_channels[channelId].streamVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(it.value()).arg(effectiveVolume));
        }
    }
}

uint32_t AudioManager::findLoopbackSinkInput(uint32_t moduleId) const {
    // Find the sink-input created by a loopback module
    QString output;
    if (!runCommand("pactl list sink-inputs", &output)) {
        return 0;
    }

    uint32_t currentId = 0;
    bool inBlock = false;

    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("Sink Input #")) {
            currentId = trimmed.mid(12).toUInt();
            inBlock = true;
        }

        if (inBlock && trimmed.contains(QString("module.id = \"%1\"").arg(moduleId))) {
            return currentId;
        }
    }

    return 0;
}

bool AudioManager::setChannelPersonalVolume(const QString &channelId, int volume) {
    if (!m_channels.contains(channelId)) {
        return false;
    }

    volume = qBound(0, volume, 100);
    auto &channel = m_channels[channelId];
    channel.personalVolume = volume;

    // Handle loopback - keep loopback alive, just adjust volume (avoids screech from creation/destruction)
    if (!m_outputDevice.isEmpty()) {
        if (!m_loopbackSinkInputs.contains(channelId)) {
            // No loopback exists yet - create one
            addChannelLoopback(channelId);
        }

        if (m_loopbackSinkInputs.contains(channelId)) {
            // Update volume on existing loopback (0% = effectively silent)
            uint32_t sinkInputId = m_loopbackSinkInputs[channelId];
            int effectiveVolume = (volume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));
        }
    }

    emit channelsChanged();
    return true;
}

bool AudioManager::setChannelStreamVolume(const QString &channelId, int volume) {
    if (!m_channels.contains(channelId)) {
        return false;
    }

    volume = qBound(0, volume, 100);
    auto &channel = m_channels[channelId];
    channel.streamVolume = volume;

    // Handle stream loopback - keep loopback alive, just adjust volume (avoids screech from creation/destruction)
    if (m_streamEnabled && !m_streamOutputDevice.isEmpty()) {
        if (!m_streamLoopbackSinkInputs.contains(channelId)) {
            // No stream loopback exists yet - create one
            addStreamChannelLoopback(channelId);
        }

        if (m_streamLoopbackSinkInputs.contains(channelId)) {
            // Update volume on existing stream loopback (0% = effectively silent)
            uint32_t sinkInputId = m_streamLoopbackSinkInputs[channelId];
            int effectiveVolume = (volume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));
        }
    }

    emit channelsChanged();
    return true;
}

QList<Device> AudioManager::listOutputDevices() const {
    QList<Device> devices;
    QString output;

    if (!runCommand("pactl list sinks", &output)) {
        return devices;
    }

    Device current;
    bool inSink = false;

    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();

        if (line.startsWith("Sink #")) {
            // Save previous device if valid
            if (inSink && !current.id.isEmpty() && !current.id.startsWith("wavemux_")) {
                devices.append(current);
            }
            current = Device();
            inSink = true;
        }

        if (!inSink) continue;

        if (trimmed.startsWith("Name:")) {
            current.id = trimmed.mid(5).trimmed();
        } else if (trimmed.startsWith("Description:")) {
            current.description = trimmed.mid(12).trimmed();
            current.name = current.description;  // Use description as display name
        }
    }

    // Don't forget the last one
    if (inSink && !current.id.isEmpty() && !current.id.startsWith("wavemux_")) {
        devices.append(current);
    }

    return devices;
}


bool AudioManager::setOutputDevice(const QString &deviceId) {
    m_outputDevice = deviceId;
    qInfo() << "Output device set to:" << deviceId;
    // Update loopbacks to route to the new device
    if (m_initialized) {
        updateLoopbacks();
    }
    return true;
}

void AudioManager::startStreamMonitor() {
    if (m_monitorProcess) {
        return;
    }

    // Sync existing streams: restore assignments and apply routing rules
    syncExistingStreams();

    m_monitorProcess = new QProcess(this);
    connect(m_monitorProcess, &QProcess::readyReadStandardOutput,
            this, &AudioManager::handleMonitorOutput);

    m_monitorProcess->start("pactl", {"subscribe"});
    qInfo() << "Started stream monitor";
}

void AudioManager::syncExistingStreams() {
    // Build sink index -> channel ID map
    QHash<uint32_t, QString> sinkIndexToChannelId;
    for (const auto &channel : m_channels) {
        if (auto info = getSinkInfo(channel.sinkName)) {
            sinkIndexToChannelId[info->index] = channel.id;
        }
    }

    QString output;
    if (!runCommand("pactl list sink-inputs short", &output)) {
        return;
    }

    bool changed = false;

    for (const auto &line : output.split('\n', Qt::SkipEmptyParts)) {
        auto parts = line.split('\t');
        if (parts.size() < 2) {
            continue;
        }

        bool okStream = false;
        const uint32_t streamId = parts[0].toUInt(&okStream);
        if (!okStream) {
            continue;
        }

        bool okSink = false;
        const uint32_t sinkIndex = parts[1].toUInt(&okSink);
        if (!okSink) {
            continue;
        }

        // Record existing assignments on our channel sinks
        if (sinkIndexToChannelId.contains(sinkIndex)) {
            QString channelId = sinkIndexToChannelId[sinkIndex];
            if (m_streamAssignments.value(streamId) != channelId) {
                m_streamAssignments[streamId] = channelId;
                changed = true;
            }
            qInfo() << "Existing stream" << streamId << "on channel" << channelId;
            continue;
        }

        auto info = getStreamInfo(streamId);
        if (!info) {
            continue;
        }

        // Skip loopback and system streams
        if (info->appName.contains("Loopback", Qt::CaseInsensitive) ||
            info->processName.contains("loopback", Qt::CaseInsensitive) ||
            (info->appName.isEmpty() && info->processName.isEmpty())) {
            continue;
        }

        // Apply routing rules or move to silent sink
        bool routed = false;
        for (const auto &rule : m_routingRules) {
            QRegularExpression re(rule.matchPattern, QRegularExpression::CaseInsensitiveOption);
            if (re.match(info->appName).hasMatch() || re.match(info->processName).hasMatch()) {
                moveStreamToChannel(streamId, rule.targetChannel);
                changed = true;
                routed = true;
                break;
            }
        }

        if (!routed) {
            QString cmd = QString("pactl move-sink-input %1 %2").arg(streamId).arg(m_unassignedSinkName);
            if (runCommand(cmd)) {
                if (m_streamAssignments.contains(streamId)) {
                    m_streamAssignments.remove(streamId);
                    changed = true;
                }
                qInfo() << "Moved stream" << streamId << "(" << info->appName << ") to silent sink";
            }
        }
    }

    if (changed) {
        emit streamsChanged();
    }
}

void AudioManager::stopStreamMonitor() {
    if (m_monitorProcess) {
        m_monitorProcess->terminate();
        m_monitorProcess->waitForFinished(1000);
        delete m_monitorProcess;
        m_monitorProcess = nullptr;
        qInfo() << "Stopped stream monitor";
    }
}

void AudioManager::handleMonitorOutput() {
    while (m_monitorProcess->canReadLine()) {
        QString line = QString::fromUtf8(m_monitorProcess->readLine()).trimmed();

        // Parse events like: Event 'new' on sink-input #123
        // or: Event 'remove' on sink-input #123
        QRegularExpression re("Event '(\\w+)' on sink-input #(\\d+)");
        auto match = re.match(line);

        if (match.hasMatch()) {
            QString eventType = match.captured(1);
            uint32_t streamId = match.captured(2).toUInt();
            handleStreamEvent(eventType, streamId);
        }
    }
}

void AudioManager::handleStreamEvent(const QString &eventType, uint32_t id) {
    if (eventType == "new") {
        // Small delay to let stream properties settle
        QTimer::singleShot(100, this, [this, id]() {
            auto info = getStreamInfo(id);
            if (info) {
                // Skip loopback and system streams
                if (info->appName.contains("Loopback", Qt::CaseInsensitive) ||
                    info->processName.contains("loopback", Qt::CaseInsensitive) ||
                    info->mediaName.contains("Loopback", Qt::CaseInsensitive) ||
                    (info->appName.isEmpty() && info->processName.isEmpty())) {
                    qDebug() << "Ignoring system/loopback stream:" << id;
                    return;
                }

                // Skip if this is one of our own loopback sink-inputs
                for (auto it = m_loopbackSinkInputs.begin(); it != m_loopbackSinkInputs.end(); ++it) {
                    if (it.value() == id) {
                        qDebug() << "Ignoring our own personal loopback:" << id;
                        return;
                    }
                }
                for (auto it = m_streamLoopbackSinkInputs.begin(); it != m_streamLoopbackSinkInputs.end(); ++it) {
                    if (it.value() == id) {
                        qDebug() << "Ignoring our own stream loopback:" << id;
                        return;
                    }
                }

                qInfo() << "New stream:" << id << info->appName << info->processName;
                emit streamAdded(id, info->appName);

                // Apply routing rules
                applyRoutingRules(id, info->appName, info->processName);
            }
        });
    } else if (eventType == "remove") {
        qInfo() << "Stream removed:" << id;
        m_streamAssignments.remove(id);
        emit streamRemoved(id);
        emit streamsChanged();
    } else if (eventType == "change") {
        emit streamsChanged();
    }
}

std::optional<StreamInfo> AudioManager::getStreamInfo(uint32_t id) const {
    QString output;
    if (!runCommand(QString("pactl list sink-inputs"), &output)) {
        return std::nullopt;
    }

    StreamInfo info;
    info.id = id;
    bool foundSinkInput = false;
    bool inCorrectBlock = false;

    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("Sink Input #")) {
            uint32_t currentId = trimmed.mid(12).toUInt();
            inCorrectBlock = (currentId == id);
            foundSinkInput = inCorrectBlock;
        }

        if (!inCorrectBlock) continue;

        if (trimmed.startsWith("Sink:")) {
            // Extract sink index, then we need to get sink name
            QRegularExpression sinkRe("Sink: (\\d+)");
            auto match = sinkRe.match(trimmed);
            if (match.hasMatch()) {
                // We'll need another query to get sink name from index
                // For now store the index as string
                info.currentSink = match.captured(1);
            }
        } else if (trimmed.startsWith("application.name = ")) {
            info.appName = trimmed.mid(19).remove('"');
        } else if (trimmed.startsWith("media.name = ")) {
            info.mediaName = trimmed.mid(13).remove('"');
        } else if (trimmed.startsWith("application.process.binary = ")) {
            info.processName = trimmed.mid(29).remove('"');
        }
    }

    if (foundSinkInput) {
        return info;
    }
    return std::nullopt;
}

QList<Stream> AudioManager::listStreams() const {
    QList<Stream> result;
    QString output;

    if (!runCommand("pactl list sink-inputs", &output)) {
        return result;
    }

    // List of apps/processes to filter out
    static const QStringList filteredApps = {
        "Loopback",
        "loopback",
        "speech-dispatcher",
        "speech_dispatcher",
        "spd-say",
        "PipeWire",
        "pipewire",
        "pw-loopback",
        "module-loopback",
        "PulseAudio Volume Control",
        "pavucontrol"
    };

    Stream current;
    bool inBlock = false;
    bool isLoopback = false;
    QString moduleName;

    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("Sink Input #")) {
            // Save previous stream if valid and not filtered
            if (inBlock && current.id > 0 && !isLoopback) {
                bool filtered = false;
                for (const auto &filter : filteredApps) {
                    if (current.appName.contains(filter, Qt::CaseInsensitive) ||
                        current.processName.contains(filter, Qt::CaseInsensitive)) {
                        filtered = true;
                        break;
                    }
                }
                // Also filter out streams with no app name and no process name (system streams)
                if (!filtered && (current.appName.isEmpty() && current.processName.isEmpty())) {
                    filtered = true;
                }
                if (!filtered) {
                    if (m_streamAssignments.contains(current.id)) {
                        current.assignedChannel = m_streamAssignments[current.id];
                    }
                    result.append(current);
                }
            }
            current = Stream();
            current.id = trimmed.mid(12).toUInt();
            inBlock = true;
            isLoopback = false;
            moduleName.clear();
        }

        if (!inBlock) continue;

        if (trimmed.startsWith("application.name = ")) {
            current.appName = trimmed.mid(19).remove('"');
        } else if (trimmed.startsWith("media.name = ")) {
            current.mediaName = trimmed.mid(13).remove('"');
            // Check if media name indicates loopback
            if (current.mediaName.contains("Loopback", Qt::CaseInsensitive) ||
                current.mediaName.contains("loopback", Qt::CaseInsensitive)) {
                isLoopback = true;
            }
        } else if (trimmed.startsWith("application.process.binary = ")) {
            current.processName = trimmed.mid(29).remove('"');
        } else if (trimmed.startsWith("module-stream-restore.id = ")) {
            moduleName = trimmed.mid(27).remove('"');
            // Check if this is a loopback module
            if (moduleName.contains("module-loopback")) {
                isLoopback = true;
            }
        } else if (trimmed.contains("module.id = ")) {
            // Check if this sink-input belongs to one of our loopback modules
            QRegularExpression moduleRe("module\\.id = \"(\\d+)\"");
            auto match = moduleRe.match(trimmed);
            if (match.hasMatch()) {
                uint32_t moduleId = match.captured(1).toUInt();
                for (auto it = m_loopbackModules.begin(); it != m_loopbackModules.end(); ++it) {
                    if (it.value() == moduleId) {
                        isLoopback = true;
                        break;
                    }
                }
            }
        }
    }

    // Don't forget the last one
    if (inBlock && current.id > 0 && !isLoopback) {
        bool filtered = false;
        for (const auto &filter : filteredApps) {
            if (current.appName.contains(filter, Qt::CaseInsensitive) ||
                current.processName.contains(filter, Qt::CaseInsensitive)) {
                filtered = true;
                break;
            }
        }
        // Also filter out streams with no app name and no process name (system streams)
        if (!filtered && (current.appName.isEmpty() && current.processName.isEmpty())) {
            filtered = true;
        }
        if (!filtered) {
            if (m_streamAssignments.contains(current.id)) {
                current.assignedChannel = m_streamAssignments[current.id];
            }
            result.append(current);
        }
    }

    return result;
}

bool AudioManager::moveStreamToChannel(uint32_t streamId, const QString &channelId) {
    if (!m_channels.contains(channelId)) {
        qWarning() << "Unknown channel:" << channelId;
        return false;
    }

    const auto &channel = m_channels[channelId];
    QString cmd = QString("pactl move-sink-input %1 %2")
        .arg(streamId)
        .arg(channel.sinkName);

    if (runCommand(cmd)) {
        m_streamAssignments[streamId] = channelId;
        qInfo() << "Moved stream" << streamId << "to channel" << channelId;

        // Auto-create routing rule based on app/process name
        auto streamInfo = getStreamInfo(streamId);
        if (streamInfo) {
            QString pattern;
            // Prefer process name for matching (more reliable across sessions)
            if (!streamInfo->processName.isEmpty()) {
                pattern = streamInfo->processName;
            } else if (!streamInfo->appName.isEmpty()) {
                pattern = streamInfo->appName;
            }

            if (!pattern.isEmpty()) {
                // Use case-insensitive exact match
                addRoutingRule(pattern, channelId);
                qInfo() << "Auto-created routing rule:" << pattern << "->" << channelId;
            }
        }

        emit streamsChanged();
        return true;
    }

    return false;
}

QString AudioManager::getStreamChannel(uint32_t streamId) const {
    return m_streamAssignments.value(streamId);
}

bool AudioManager::unassignStream(uint32_t streamId) {
    // Move stream to the silent unassigned sink
    // Unassigned streams should not produce audio
    QString cmd = QString("pactl move-sink-input %1 %2")
        .arg(streamId)
        .arg(m_unassignedSinkName);

    if (runCommand(cmd)) {
        m_streamAssignments.remove(streamId);
        qInfo() << "Unassigned stream" << streamId << "to silent sink";
        emit streamsChanged();
        return true;
    }

    return false;
}

void AudioManager::addRoutingRule(const QString &pattern, const QString &channelId) {
    // Remove existing rule for this pattern (without emitting signal)
    m_routingRules.erase(
        std::remove_if(m_routingRules.begin(), m_routingRules.end(),
            [&pattern](const RoutingRule &rule) {
                return rule.matchPattern == pattern;
            }),
        m_routingRules.end());

    RoutingRule rule;
    rule.matchPattern = pattern;
    rule.targetChannel = channelId;
    m_routingRules.append(rule);

    qInfo() << "Added routing rule:" << pattern << "->" << channelId;
    emit routingRulesChanged();
}

void AudioManager::removeRoutingRule(const QString &pattern) {
    int sizeBefore = m_routingRules.size();
    m_routingRules.erase(
        std::remove_if(m_routingRules.begin(), m_routingRules.end(),
            [&pattern](const RoutingRule &rule) {
                return rule.matchPattern == pattern;
            }),
        m_routingRules.end());

    if (m_routingRules.size() != sizeBefore) {
        emit routingRulesChanged();
    }
}

QList<RoutingRule> AudioManager::getRoutingRules() const {
    return m_routingRules;
}

void AudioManager::applyRoutingRulesToExistingStreams() {
    if (m_routingRules.isEmpty()) {
        qInfo() << "No routing rules to apply";
        return;
    }

    qInfo() << "Applying routing rules to existing streams...";

    QString output;
    if (!runCommand("pactl list sink-inputs", &output)) {
        return;
    }

    struct StreamData {
        uint32_t id = 0;
        QString appName;
        QString processName;
        QString mediaName;
    };

    QList<StreamData> streams;
    StreamData current;
    bool inBlock = false;

    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("Sink Input #")) {
            if (inBlock && current.id > 0) {
                streams.append(current);
            }
            current = StreamData();
            current.id = trimmed.mid(12).toUInt();
            inBlock = true;
        }

        if (!inBlock) continue;

        if (trimmed.startsWith("application.name = ")) {
            current.appName = trimmed.mid(19).remove('"');
        } else if (trimmed.startsWith("media.name = ")) {
            current.mediaName = trimmed.mid(13).remove('"');
        } else if (trimmed.startsWith("application.process.binary = ")) {
            current.processName = trimmed.mid(29).remove('"');
        }
    }

    if (inBlock && current.id > 0) {
        streams.append(current);
    }

    for (const auto &stream : streams) {
        if (stream.appName.contains("Loopback", Qt::CaseInsensitive) ||
            stream.processName.contains("loopback", Qt::CaseInsensitive) ||
            stream.mediaName.contains("Loopback", Qt::CaseInsensitive) ||
            (stream.appName.isEmpty() && stream.processName.isEmpty())) {
            continue;
        }

        for (const auto &rule : m_routingRules) {
            QRegularExpression re(rule.matchPattern, QRegularExpression::CaseInsensitiveOption);
            if (re.match(stream.appName).hasMatch() || re.match(stream.processName).hasMatch()) {
                if (m_channels.contains(rule.targetChannel)) {
                    const auto &channel = m_channels[rule.targetChannel];
                    QString cmd = QString("pactl move-sink-input %1 %2")
                        .arg(stream.id)
                        .arg(channel.sinkName);
                    if (runCommand(cmd)) {
                        m_streamAssignments[stream.id] = rule.targetChannel;
                        qInfo() << "Routed" << stream.appName << "to" << rule.targetChannel;
                    }
                }
                break;
            }
        }
    }
}

void AudioManager::applyRoutingRules(uint32_t streamId, const QString &appName, const QString &processName) {
    for (const auto &rule : m_routingRules) {
        QRegularExpression re(rule.matchPattern, QRegularExpression::CaseInsensitiveOption);

        if (re.match(appName).hasMatch() || re.match(processName).hasMatch()) {
            qInfo() << "Auto-routing stream" << streamId << "to" << rule.targetChannel
                    << "(matched:" << rule.matchPattern << ")";
            moveStreamToChannel(streamId, rule.targetChannel);
            return;
        }
    }

    // No routing rule matched - move to the silent unassigned sink
    // This ensures unrouted streams don't play through the default output
    QString cmd = QString("pactl move-sink-input %1 %2").arg(streamId).arg(m_unassignedSinkName);
    if (runCommand(cmd)) {
        qInfo() << "Moved unassigned stream" << streamId << "to silent sink";
    }
}

bool AudioManager::createLoopback(const QString &sourceSink, const QString &targetSink) {
    QString cmd = QString("pactl load-module module-loopback source=%1.monitor sink=%2 latency_msec=50")
        .arg(sourceSink, targetSink);

    QString output;
    if (runCommand(cmd, &output)) {
        uint32_t moduleId = output.trimmed().toUInt();
        qInfo() << "Created loopback from" << sourceSink << "to" << targetSink << "module:" << moduleId;
        return true;
    }

    return false;
}

bool AudioManager::removeLoopback(uint32_t moduleId) {
    QString cmd = QString("pactl unload-module %1").arg(moduleId);
    return runCommand(cmd);
}

void AudioManager::removeAllLoopbacks() {
    for (auto it = m_loopbackModules.begin(); it != m_loopbackModules.end(); ++it) {
        removeLoopback(it.value());
    }
    m_loopbackModules.clear();
    m_loopbackSinkInputs.clear();
}

bool AudioManager::updateLoopbacks() {
    if (m_outputDevice.isEmpty()) {
        qWarning() << "No output device set, cannot create loopbacks";
        return false;
    }

    // Mute output device before creating loopbacks to prevent startup noise
    runCommand(QString("pactl set-sink-mute %1 1").arg(m_outputDevice));

    // Remove existing loopbacks
    removeAllLoopbacks();

    // Small delay after removing old loopbacks
    QThread::msleep(50);

    // Create loopbacks for ALL channels (volume 0% = silent, no screech from create/destroy)
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        const auto &channel = it.value();

        // Create loopback with adjust_time=0 to prevent automatic volume adjustments
        QString cmd = QString("pactl load-module module-loopback source=%1.monitor sink=%2 "
                              "latency_msec=150 source_dont_move=true sink_dont_move=true remix=false adjust_time=0")
            .arg(channel.sinkName, m_outputDevice);

        QString output;
        if (runCommand(cmd, &output)) {
            uint32_t moduleId = output.trimmed().toUInt();
            m_loopbackModules[channel.id] = moduleId;
            qInfo() << "Created loopback for" << channel.id << "module:" << moduleId;

            // Find and track the sink-input for this loopback
            QThread::msleep(100);
            uint32_t sinkInputId = findLoopbackSinkInput(moduleId);
            if (sinkInputId > 0) {
                m_loopbackSinkInputs[channel.id] = sinkInputId;
                qInfo() << "Loopback" << channel.id << "sink-input:" << sinkInputId;

                // Set volume to 0% and mute to prevent startup noise
                runCommand(QString("pactl set-sink-input-volume %1 0%").arg(sinkInputId));
                runCommand(QString("pactl set-sink-input-mute %1 1").arg(sinkInputId));
            }
        } else {
            qWarning() << "Failed to create loopback for" << channel.id;
        }
    }

    // Wait for all loopbacks to fully stabilize
    QThread::msleep(250);

    // Set volumes while still muted
    for (auto it = m_loopbackSinkInputs.begin(); it != m_loopbackSinkInputs.end(); ++it) {
        const QString &chId = it.key();
        uint32_t sinkInputId = it.value();
        if (m_channels.contains(chId)) {
            int effectiveVolume = (m_channels[chId].personalVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));
        }
    }

    // Small delay before unmuting loopbacks
    QThread::msleep(100);

    // Unmute all loopback sink-inputs
    for (auto it = m_loopbackSinkInputs.begin(); it != m_loopbackSinkInputs.end(); ++it) {
        runCommand(QString("pactl set-sink-input-mute %1 0").arg(it.value()));
    }

    // Small delay then unmute output device
    QThread::msleep(50);
    runCommand(QString("pactl set-sink-mute %1 0").arg(m_outputDevice));

    return true;
}

bool AudioManager::addChannelLoopback(const QString &channelId) {
    if (!m_channels.contains(channelId) || m_outputDevice.isEmpty()) {
        return false;
    }

    // Remove existing loopback if any
    removeChannelLoopback(channelId);

    const auto &channel = m_channels[channelId];

    // Create loopback with adjust_time=0 to prevent automatic volume adjustments
    QString cmd = QString("pactl load-module module-loopback source=%1.monitor sink=%2 "
                          "latency_msec=150 source_dont_move=true sink_dont_move=true remix=false adjust_time=0")
        .arg(channel.sinkName, m_outputDevice);

    QString output;
    if (runCommand(cmd, &output)) {
        uint32_t moduleId = output.trimmed().toUInt();
        m_loopbackModules[channelId] = moduleId;
        qInfo() << "Created loopback for" << channelId << "module:" << moduleId;

        // Find and track the sink-input
        QThread::msleep(100);
        uint32_t sinkInputId = findLoopbackSinkInput(moduleId);
        if (sinkInputId > 0) {
            m_loopbackSinkInputs[channelId] = sinkInputId;

            // Set volume to 0% and mute to prevent any startup noise
            runCommand(QString("pactl set-sink-input-volume %1 0%").arg(sinkInputId));
            runCommand(QString("pactl set-sink-input-mute %1 1").arg(sinkInputId));

            // Wait for loopback to fully stabilize
            QThread::msleep(150);

            // Now set the target volume while still muted
            int effectiveVolume = (channel.personalVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));

            // Small delay then unmute
            QThread::msleep(50);
            runCommand(QString("pactl set-sink-input-mute %1 0").arg(sinkInputId));
        }
        return true;
    }

    qWarning() << "Failed to create loopback for" << channelId;
    return false;
}

bool AudioManager::removeChannelLoopback(const QString &channelId) {
    if (m_loopbackModules.contains(channelId)) {
        uint32_t moduleId = m_loopbackModules[channelId];
        if (removeLoopback(moduleId)) {
            m_loopbackModules.remove(channelId);
            m_loopbackSinkInputs.remove(channelId);
            qInfo() << "Removed loopback for" << channelId;
            return true;
        }
    }
    return false;
}

bool AudioManager::setStreamOutputDevice(const QString &deviceId) {
    m_streamOutputDevice = deviceId;
    qInfo() << "Stream output device set to:" << deviceId;
    // Update stream loopbacks to route to the new device
    if (m_initialized && m_streamEnabled) {
        updateStreamLoopbacks();
    }
    return true;
}

bool AudioManager::setStreamEnabled(bool enabled) {
    if (m_streamEnabled == enabled) {
        return true;
    }

    m_streamEnabled = enabled;
    qInfo() << "Stream enabled:" << enabled;

    if (enabled) {
        // Create stream loopbacks if we have a stream output device
        if (!m_streamOutputDevice.isEmpty()) {
            updateStreamLoopbacks();
        }
    } else {
        // Remove all stream loopbacks
        removeAllStreamLoopbacks();
    }

    return true;
}

bool AudioManager::updateStreamLoopbacks() {
    if (m_streamOutputDevice.isEmpty()) {
        qWarning() << "No stream output device set, cannot create stream loopbacks";
        return false;
    }

    if (!m_streamEnabled) {
        qInfo() << "Stream not enabled, skipping stream loopback update";
        return true;
    }

    // Mute stream output device before creating loopbacks to prevent startup noise
    runCommand(QString("pactl set-sink-mute %1 1").arg(m_streamOutputDevice));

    // Remove existing stream loopbacks
    removeAllStreamLoopbacks();

    // Small delay after removing old loopbacks
    QThread::msleep(50);

    // Create loopbacks for ALL channels (volume 0% = silent, no screech from create/destroy)
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        const auto &channel = it.value();

        // Create loopback with adjust_time=0 to prevent automatic volume adjustments
        QString cmd = QString("pactl load-module module-loopback source=%1.monitor sink=%2 "
                              "latency_msec=150 source_dont_move=true sink_dont_move=true remix=false adjust_time=0")
            .arg(channel.sinkName, m_streamOutputDevice);

        QString output;
        if (runCommand(cmd, &output)) {
            uint32_t moduleId = output.trimmed().toUInt();
            m_streamLoopbackModules[channel.id] = moduleId;
            qInfo() << "Created stream loopback for" << channel.id << "module:" << moduleId;

            // Find and track the sink-input for this loopback
            QThread::msleep(100);
            uint32_t sinkInputId = findLoopbackSinkInput(moduleId);
            if (sinkInputId > 0) {
                m_streamLoopbackSinkInputs[channel.id] = sinkInputId;
                qInfo() << "Stream loopback" << channel.id << "sink-input:" << sinkInputId;

                // Set volume to 0% and mute to prevent startup noise
                runCommand(QString("pactl set-sink-input-volume %1 0%").arg(sinkInputId));
                runCommand(QString("pactl set-sink-input-mute %1 1").arg(sinkInputId));
            }
        } else {
            qWarning() << "Failed to create stream loopback for" << channel.id;
        }
    }

    // Wait for all loopbacks to fully stabilize
    QThread::msleep(250);

    // Set volumes while still muted
    for (auto it = m_streamLoopbackSinkInputs.begin(); it != m_streamLoopbackSinkInputs.end(); ++it) {
        const QString &chId = it.key();
        uint32_t sinkInputId = it.value();
        if (m_channels.contains(chId)) {
            int effectiveVolume = (m_channels[chId].streamVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));
        }
    }

    // Small delay before unmuting loopbacks
    QThread::msleep(100);

    // Unmute all loopback sink-inputs
    for (auto it = m_streamLoopbackSinkInputs.begin(); it != m_streamLoopbackSinkInputs.end(); ++it) {
        runCommand(QString("pactl set-sink-input-mute %1 0").arg(it.value()));
    }

    // Small delay then unmute stream output device
    QThread::msleep(50);
    runCommand(QString("pactl set-sink-mute %1 0").arg(m_streamOutputDevice));

    return true;
}

bool AudioManager::addStreamChannelLoopback(const QString &channelId) {
    if (!m_channels.contains(channelId) || m_streamOutputDevice.isEmpty() || !m_streamEnabled) {
        return false;
    }

    // Remove existing stream loopback if any
    removeStreamChannelLoopback(channelId);

    const auto &channel = m_channels[channelId];

    // Create loopback with adjust_time=0 to prevent automatic volume adjustments
    QString cmd = QString("pactl load-module module-loopback source=%1.monitor sink=%2 "
                          "latency_msec=150 source_dont_move=true sink_dont_move=true remix=false adjust_time=0")
        .arg(channel.sinkName, m_streamOutputDevice);

    QString output;
    if (runCommand(cmd, &output)) {
        uint32_t moduleId = output.trimmed().toUInt();
        m_streamLoopbackModules[channelId] = moduleId;
        qInfo() << "Created stream loopback for" << channelId << "module:" << moduleId;

        // Find and track the sink-input
        QThread::msleep(100);
        uint32_t sinkInputId = findLoopbackSinkInput(moduleId);
        if (sinkInputId > 0) {
            m_streamLoopbackSinkInputs[channelId] = sinkInputId;

            // Set volume to 0% and mute to prevent any startup noise
            runCommand(QString("pactl set-sink-input-volume %1 0%").arg(sinkInputId));
            runCommand(QString("pactl set-sink-input-mute %1 1").arg(sinkInputId));

            // Wait for loopback to fully stabilize
            QThread::msleep(150);

            // Now set the target volume while still muted
            int effectiveVolume = (channel.streamVolume * m_masterVolume) / 100;
            runCommand(QString("pactl set-sink-input-volume %1 %2%").arg(sinkInputId).arg(effectiveVolume));

            // Small delay then unmute
            QThread::msleep(50);
            runCommand(QString("pactl set-sink-input-mute %1 0").arg(sinkInputId));
        }
        return true;
    }

    qWarning() << "Failed to create stream loopback for" << channelId;
    return false;
}

bool AudioManager::removeStreamChannelLoopback(const QString &channelId) {
    if (m_streamLoopbackModules.contains(channelId)) {
        uint32_t moduleId = m_streamLoopbackModules[channelId];
        if (removeLoopback(moduleId)) {
            m_streamLoopbackModules.remove(channelId);
            m_streamLoopbackSinkInputs.remove(channelId);
            qInfo() << "Removed stream loopback for" << channelId;
            return true;
        }
    }
    return false;
}

void AudioManager::removeAllStreamLoopbacks() {
    for (auto it = m_streamLoopbackModules.begin(); it != m_streamLoopbackModules.end(); ++it) {
        removeLoopback(it.value());
    }
    m_streamLoopbackModules.clear();
    m_streamLoopbackSinkInputs.clear();
}

} // namespace WaveMux
