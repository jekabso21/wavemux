#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QTimer>
#include "wavemux/types.h"

namespace WaveMux {

class AudioManager;

struct ChannelConfig {
    int volume = 100;
    bool muted = false;
    int personalVolume = 100;
    int streamVolume = 0;
};

class ConfigManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(AudioManager *manager, QObject *parent = nullptr);

    bool load();
    bool save();

    bool isSetupComplete() const { return m_config.setupComplete; }
    void setSetupComplete(bool complete);

    QString configPath() const;

    // Call this after AudioManager is initialized to connect auto-save signals
    void connectAutoSave();

signals:
    void configChanged();

private slots:
    void onSettingsChanged();

private:
    void applyConfig();
    void scheduleSave();

    AudioManager *m_manager;
    Config m_config;
    QHash<QString, ChannelConfig> m_channelStates;
    int m_masterVolume = 100;
    QTimer *m_saveTimer = nullptr;
    bool m_loading = false;  // Prevent save during load
};

} // namespace WaveMux
