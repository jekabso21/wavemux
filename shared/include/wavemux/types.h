#pragma once

#include <QString>
#include <QList>
#include <QDBusArgument>

namespace WaveMux {

struct Channel {
    QString id;
    QString displayName;
    QString sinkName;
    int volume = 0;
    bool muted = false;
    int personalVolume = 100;  // 0-100, mix level for personal output
    int streamVolume = 0;      // 0-100, mix level for stream output
    bool personalMuted = false;
    bool streamMuted = false;
};

struct Stream {
    uint32_t id = 0;
    QString appName;
    QString mediaName;
    QString processName;
    QString assignedChannel;
};

struct RoutingRule {
    QString matchPattern;   // app or process name pattern
    QString targetChannel;
};

struct Profile {
    QString name;
    QList<Channel> channels;
    QList<RoutingRule> rules;
};

struct Device {
    QString id;
    QString name;
    QString description;
};

struct Config {
    bool setupComplete = false;
    QString outputDevice;
    QString streamOutputDevice;
    bool streamEnabled = false;
    QString activeProfile;
    QList<Profile> profiles;
    QList<RoutingRule> routingRules;
};

// DBus serialization
QDBusArgument& operator<<(QDBusArgument& arg, const Channel& channel);
const QDBusArgument& operator>>(const QDBusArgument& arg, Channel& channel);

QDBusArgument& operator<<(QDBusArgument& arg, const Stream& stream);
const QDBusArgument& operator>>(const QDBusArgument& arg, Stream& stream);

QDBusArgument& operator<<(QDBusArgument& arg, const Device& device);
const QDBusArgument& operator>>(const QDBusArgument& arg, Device& device);

void registerMetaTypes();

} // namespace WaveMux
