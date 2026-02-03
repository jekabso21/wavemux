#include "devicedbusadaptor.h"
#include "../audiomanager.h"

namespace WaveMux {

DeviceDBusAdaptor::DeviceDBusAdaptor(AudioManager *manager)
    : QDBusAbstractAdaptor(manager)
    , m_manager(manager)
{
}

QVariantList DeviceDBusAdaptor::ListOutputDevices() {
    QVariantList result;
    for (const auto &dev : m_manager->listOutputDevices()) {
        QVariantMap map;
        map["id"] = dev.id;
        map["name"] = dev.name;
        map["description"] = dev.description;
        result.append(map);
    }
    return result;
}

bool DeviceDBusAdaptor::SetOutputDevice(const QString &deviceId) {
    return m_manager->setOutputDevice(deviceId);
}

QString DeviceDBusAdaptor::GetOutputDevice() {
    return m_manager->getOutputDevice();
}

bool DeviceDBusAdaptor::SetStreamOutputDevice(const QString &deviceId) {
    return m_manager->setStreamOutputDevice(deviceId);
}

QString DeviceDBusAdaptor::GetStreamOutputDevice() {
    return m_manager->getStreamOutputDevice();
}

} // namespace WaveMux
