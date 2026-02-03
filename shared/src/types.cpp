#include "wavemux/types.h"
#include <QMetaType>
#include <QDBusMetaType>

namespace WaveMux {

QDBusArgument& operator<<(QDBusArgument& arg, const Channel& channel) {
    arg.beginStructure();
    arg << channel.id
        << channel.displayName
        << channel.sinkName
        << channel.volume
        << channel.muted
        << channel.personalVolume
        << channel.streamVolume;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, Channel& channel) {
    arg.beginStructure();
    arg >> channel.id
        >> channel.displayName
        >> channel.sinkName
        >> channel.volume
        >> channel.muted
        >> channel.personalVolume
        >> channel.streamVolume;
    arg.endStructure();
    return arg;
}

QDBusArgument& operator<<(QDBusArgument& arg, const Stream& stream) {
    arg.beginStructure();
    arg << stream.id
        << stream.appName
        << stream.mediaName
        << stream.processName
        << stream.assignedChannel;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, Stream& stream) {
    arg.beginStructure();
    arg >> stream.id
        >> stream.appName
        >> stream.mediaName
        >> stream.processName
        >> stream.assignedChannel;
    arg.endStructure();
    return arg;
}

QDBusArgument& operator<<(QDBusArgument& arg, const Device& device) {
    arg.beginStructure();
    arg << device.id
        << device.name
        << device.description;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, Device& device) {
    arg.beginStructure();
    arg >> device.id
        >> device.name
        >> device.description;
    arg.endStructure();
    return arg;
}

void registerMetaTypes() {
    qRegisterMetaType<Channel>("WaveMux::Channel");
    qRegisterMetaType<QList<Channel>>("QList<WaveMux::Channel>");
    qDBusRegisterMetaType<Channel>();
    qDBusRegisterMetaType<QList<Channel>>();

    qRegisterMetaType<Stream>("WaveMux::Stream");
    qRegisterMetaType<QList<Stream>>("QList<WaveMux::Stream>");
    qDBusRegisterMetaType<Stream>();
    qDBusRegisterMetaType<QList<Stream>>();

    qRegisterMetaType<Device>("WaveMux::Device");
    qRegisterMetaType<QList<Device>>("QList<WaveMux::Device>");
    qDBusRegisterMetaType<Device>();
    qDBusRegisterMetaType<QList<Device>>();
}

} // namespace WaveMux
