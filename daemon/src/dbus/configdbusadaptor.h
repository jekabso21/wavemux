#pragma once

#include <QDBusAbstractAdaptor>

namespace WaveMux {

class AudioManager;
class ConfigManager;

class ConfigDBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.wavemux.Config")

public:
    explicit ConfigDBusAdaptor(AudioManager *manager, ConfigManager *config);

public slots:
    // Master volume
    bool SetMasterVolume(int volume);
    int GetMasterVolume();

    // Setup
    bool IsSetupComplete();
    void SetSetupComplete(bool complete);

    // Stream mode
    bool SetStreamEnabled(bool enabled);
    bool IsStreamEnabled();

    // Persistence
    void SaveConfig();
    void LoadConfig();

signals:
    void Error(const QString &message);
    void StreamEnabledChanged(bool enabled);

private:
    AudioManager *m_manager;
    ConfigManager *m_config;
};

} // namespace WaveMux
