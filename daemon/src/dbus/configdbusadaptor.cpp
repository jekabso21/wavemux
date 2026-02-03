#include "configdbusadaptor.h"
#include "../audiomanager.h"
#include "../configmanager.h"

namespace WaveMux {

ConfigDBusAdaptor::ConfigDBusAdaptor(AudioManager *manager, ConfigManager *config)
    : QDBusAbstractAdaptor(manager)
    , m_manager(manager)
    , m_config(config)
{
    connect(m_manager, &AudioManager::error,
            this, &ConfigDBusAdaptor::Error);
}

bool ConfigDBusAdaptor::SetMasterVolume(int volume) {
    return m_manager->setMasterVolume(volume);
}

int ConfigDBusAdaptor::GetMasterVolume() {
    return m_manager->getMasterVolume();
}

bool ConfigDBusAdaptor::IsSetupComplete() {
    return m_config->isSetupComplete();
}

void ConfigDBusAdaptor::SetSetupComplete(bool complete) {
    m_config->setSetupComplete(complete);
}

void ConfigDBusAdaptor::SaveConfig() {
    m_config->save();
}

void ConfigDBusAdaptor::LoadConfig() {
    m_config->load();
}

bool ConfigDBusAdaptor::SetStreamEnabled(bool enabled) {
    bool result = m_manager->setStreamEnabled(enabled);
    if (result) {
        emit StreamEnabledChanged(enabled);
    }
    return result;
}

bool ConfigDBusAdaptor::IsStreamEnabled() {
    return m_manager->isStreamEnabled();
}

} // namespace WaveMux
