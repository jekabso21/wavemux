#include "configmanager.h"
#include "audiomanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>

namespace WaveMux {

ConfigManager::ConfigManager(AudioManager *manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
    // Create a debounced save timer (saves 2 seconds after last change)
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(2000);
    connect(m_saveTimer, &QTimer::timeout, this, &ConfigManager::save);
}

void ConfigManager::connectAutoSave() {
    // Connect to AudioManager signals for auto-save
    connect(m_manager, &AudioManager::channelsChanged, this, &ConfigManager::onSettingsChanged);
    connect(m_manager, &AudioManager::masterVolumeChanged, this, &ConfigManager::onSettingsChanged);
    connect(m_manager, &AudioManager::routingRulesChanged, this, &ConfigManager::onSettingsChanged);
    qInfo() << "Auto-save connected";
}

void ConfigManager::onSettingsChanged() {
    if (!m_loading) {
        scheduleSave();
    }
}

void ConfigManager::scheduleSave() {
    // Restart the timer - this debounces rapid changes
    m_saveTimer->start();
}

QString ConfigManager::configPath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return configDir + "/wavemux/config.json";
}

bool ConfigManager::load() {
    QString path = configPath();
    QFile file(path);

    if (!file.exists()) {
        qInfo() << "No config file found at" << path;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse config:" << error.errorString();
        return false;
    }

    QJsonObject root = doc.object();

    m_config.setupComplete = root["setupComplete"].toBool(false);
    m_config.outputDevice = root["outputDevice"].toString();
    m_config.streamOutputDevice = root["streamOutputDevice"].toString();
    m_config.streamEnabled = root["streamEnabled"].toBool(false);

    // Load routing rules
    m_config.routingRules.clear();
    QJsonArray rulesArray = root["routingRules"].toArray();
    for (const auto &ruleVal : rulesArray) {
        QJsonObject ruleObj = ruleVal.toObject();
        RoutingRule rule;
        rule.matchPattern = ruleObj["pattern"].toString();
        rule.targetChannel = ruleObj["channel"].toString();
        m_config.routingRules.append(rule);
    }

    m_channelStates.clear();
    QJsonArray channelsArray = root["channels"].toArray();
    for (const auto &chVal : channelsArray) {
        QJsonObject chObj = chVal.toObject();
        QString channelId = chObj["id"].toString();
        ChannelConfig chConfig;
        chConfig.volume = chObj["volume"].toInt(100);
        chConfig.muted = chObj["muted"].toBool(false);
        chConfig.personalVolume = chObj["personalVolume"].toInt(100);
        chConfig.streamVolume = chObj["streamVolume"].toInt(0);
        chConfig.personalMuted = chObj["personalMuted"].toBool(false);
        chConfig.streamMuted = chObj["streamMuted"].toBool(false);
        m_channelStates[channelId] = chConfig;
    }

    m_masterVolume = root["masterVolume"].toInt(100);

    qInfo() << "Loaded config with" << m_channelStates.size() << "channels," << m_config.routingRules.size() << "rules";

    // Prevent auto-save during config application
    m_loading = true;
    applyConfig();
    m_loading = false;

    emit configChanged();

    return true;
}

bool ConfigManager::save() {
    QString path = configPath();

    // Don't save if channels are empty (likely called after shutdown)
    auto channels = m_manager->listChannels();
    if (channels.isEmpty()) {
        qWarning() << "Skipping save - no channels (daemon likely shutting down)";
        return false;
    }

    // Ensure directory exists
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Gather current state from AudioManager
    m_config.outputDevice = m_manager->getOutputDevice();
    m_config.streamOutputDevice = m_manager->getStreamOutputDevice();
    m_config.streamEnabled = m_manager->isStreamEnabled();
    m_config.routingRules = m_manager->getRoutingRules();

    QJsonObject root;
    root["setupComplete"] = m_config.setupComplete;
    root["outputDevice"] = m_config.outputDevice;
    root["streamOutputDevice"] = m_config.streamOutputDevice;
    root["streamEnabled"] = m_config.streamEnabled;

    // Save routing rules
    QJsonArray rulesArray;
    for (const auto &rule : m_config.routingRules) {
        QJsonObject ruleObj;
        ruleObj["pattern"] = rule.matchPattern;
        ruleObj["channel"] = rule.targetChannel;
        rulesArray.append(ruleObj);
    }
    root["routingRules"] = rulesArray;

    // Save channel states
    QJsonArray channelsArray;
    for (const auto &ch : channels) {
        QJsonObject chObj;
        chObj["id"] = ch.id;
        chObj["volume"] = ch.volume;
        chObj["muted"] = ch.muted;
        chObj["personalVolume"] = ch.personalVolume;
        chObj["streamVolume"] = ch.streamVolume;
        chObj["personalMuted"] = ch.personalMuted;
        chObj["streamMuted"] = ch.streamMuted;
        channelsArray.append(chObj);
    }
    root["channels"] = channelsArray;

    root["masterVolume"] = m_manager->getMasterVolume();

    QJsonDocument doc(root);
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to write config file:" << path;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qInfo() << "Saved config to" << path;
    return true;
}

void ConfigManager::setSetupComplete(bool complete) {
    m_config.setupComplete = complete;
    save();
    emit configChanged();
}

void ConfigManager::applyConfig() {
    // Apply channel states first (before setting up loopbacks)
    for (auto it = m_channelStates.begin(); it != m_channelStates.end(); ++it) {
        const QString &channelId = it.key();
        const ChannelConfig &chConfig = it.value();

        m_manager->setChannelVolume(channelId, chConfig.volume);
        m_manager->setChannelMute(channelId, chConfig.muted);
        // Set mix volumes (loopbacks will be created when output devices are set)
        m_manager->setChannelPersonalVolume(channelId, chConfig.personalVolume);
        m_manager->setChannelStreamVolume(channelId, chConfig.streamVolume);
    }

    // Apply master volume
    m_manager->setMasterVolume(m_masterVolume);

    // Apply output device (this creates personal loopbacks)
    if (!m_config.outputDevice.isEmpty()) {
        m_manager->setOutputDevice(m_config.outputDevice);
    }

    // Apply stream output device
    if (!m_config.streamOutputDevice.isEmpty()) {
        m_manager->setStreamOutputDevice(m_config.streamOutputDevice);
    }

    // Apply stream enabled state (this creates stream loopbacks)
    m_manager->setStreamEnabled(m_config.streamEnabled);

    // Apply per-mix mute states (after loopbacks are created)
    for (auto it = m_channelStates.begin(); it != m_channelStates.end(); ++it) {
        const QString &channelId = it.key();
        const ChannelConfig &chConfig = it.value();
        m_manager->setChannelPersonalMute(channelId, chConfig.personalMuted);
        m_manager->setChannelStreamMute(channelId, chConfig.streamMuted);
    }

    // Apply routing rules
    for (const auto &rule : m_config.routingRules) {
        m_manager->addRoutingRule(rule.matchPattern, rule.targetChannel);
    }

    // Re-apply routing rules to existing streams (they were moved to silent sink
    // before config was loaded)
    m_manager->applyRoutingRulesToExistingStreams();

    qInfo() << "Applied config: outputDevice=" << m_config.outputDevice
            << "streamOutputDevice=" << m_config.streamOutputDevice
            << "streamEnabled=" << m_config.streamEnabled
            << "channels=" << m_channelStates.size()
            << "rules=" << m_config.routingRules.size();
}

} // namespace WaveMux
