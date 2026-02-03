#pragma once

#include <QDBusAbstractAdaptor>
#include <QVariantList>
#include <QVariantMap>

namespace WaveMux {

class AudioManager;

class DeviceDBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.wavemux.Devices")

public:
    explicit DeviceDBusAdaptor(AudioManager *manager);

public slots:
    QVariantList ListOutputDevices();
    bool SetOutputDevice(const QString &deviceId);
    QString GetOutputDevice();
    bool SetStreamOutputDevice(const QString &deviceId);
    QString GetStreamOutputDevice();

private:
    AudioManager *m_manager;
};

} // namespace WaveMux
