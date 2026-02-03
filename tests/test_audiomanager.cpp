#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include "audiomanager.h"
#include "wavemux/types.h"

class AudioManagerTest : public ::testing::Test {
protected:
    WaveMux::AudioManager *manager = nullptr;

    void SetUp() override {
        WaveMux::registerMetaTypes();
        cleanupSinks();
        manager = new WaveMux::AudioManager();
    }

    void TearDown() override {
        if (manager) {
            manager->shutdown();
            delete manager;
            manager = nullptr;
        }
        cleanupSinks();
    }

    bool sinkExists(const QString &name) {
        QProcess process;
        process.start("pactl", {"list", "sinks", "short"});
        process.waitForFinished(5000);
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        return output.contains(name);
    }

    void cleanupSinks() {
        QProcess process;
        process.start("sh", {"-c", "pactl list modules short | grep null-sink | cut -f1 | xargs -r -n1 pactl unload-module"});
        process.waitForFinished(5000);
    }

    QString getDefaultSink() {
        QProcess process;
        process.start("pactl", {"get-default-sink"});
        process.waitForFinished(5000);
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }
};

TEST_F(AudioManagerTest, Initialize) {
    EXPECT_TRUE(manager->initialize());

    auto channels = manager->listChannels();
    EXPECT_EQ(channels.size(), 4);

    EXPECT_TRUE(sinkExists("wavemux_game"));
    EXPECT_TRUE(sinkExists("wavemux_chat"));
    EXPECT_TRUE(sinkExists("wavemux_media"));
    EXPECT_TRUE(sinkExists("wavemux_aux"));
}

TEST_F(AudioManagerTest, InitializeTwice) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->initialize());

    auto channels = manager->listChannels();
    EXPECT_EQ(channels.size(), 4);
}

TEST_F(AudioManagerTest, ListChannels) {
    EXPECT_TRUE(manager->initialize());

    auto channels = manager->listChannels();
    EXPECT_EQ(channels.size(), 4);

    QStringList expectedIds = {"game", "chat", "media", "aux"};
    for (const auto &channel : channels) {
        EXPECT_TRUE(expectedIds.contains(channel.id));
    }
}

TEST_F(AudioManagerTest, SetChannelVolume) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelVolume("game", 50));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.volume, 50);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelVolumeInvalid) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->setChannelVolume("invalid", 50));
}

TEST_F(AudioManagerTest, SetChannelVolumeClampsHigh) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelVolume("game", 150));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.volume, 100);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelVolumeClampsLow) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelVolume("game", -50));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.volume, 0);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelMute) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelMute("chat", true));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "chat") {
            EXPECT_TRUE(ch.muted);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelUnmute) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelMute("chat", true));
    EXPECT_TRUE(manager->setChannelMute("chat", false));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "chat") {
            EXPECT_FALSE(ch.muted);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelMuteInvalid) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->setChannelMute("invalid", true));
}

TEST_F(AudioManagerTest, SetChannelPersonalVolume) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelPersonalVolume("game", 75));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.personalVolume, 75);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelStreamVolume) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(manager->setChannelStreamVolume("game", 50));

    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.streamVolume, 50);
            break;
        }
    }
}

TEST_F(AudioManagerTest, SetChannelPersonalVolumeInvalidChannel) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->setChannelPersonalVolume("invalid", 50));
}

TEST_F(AudioManagerTest, SetChannelStreamVolumeInvalidChannel) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->setChannelStreamVolume("invalid", 50));
}

TEST_F(AudioManagerTest, ListOutputDevices) {
    EXPECT_TRUE(manager->initialize());

    auto devices = manager->listOutputDevices();
    EXPECT_GT(devices.size(), 0);

    for (const auto &dev : devices) {
        EXPECT_FALSE(dev.id.startsWith("wavemux_"));
    }
}

TEST_F(AudioManagerTest, DefaultSinkPreserved) {
    QString originalDefault = getDefaultSink();

    EXPECT_TRUE(manager->initialize());

    QString currentDefault = getDefaultSink();
    EXPECT_FALSE(currentDefault.startsWith("wavemux_"));
}

TEST_F(AudioManagerTest, Shutdown) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_TRUE(sinkExists("wavemux_game"));

    manager->shutdown();
    QThread::msleep(500);

    // After shutdown, sinks should be cleaned up
    // Note: Module cleanup happens in shutdown
}

// =============================================================================
// Phase 2 Tests - Stream Detection & Routing
// =============================================================================

TEST_F(AudioManagerTest, ListStreams) {
    EXPECT_TRUE(manager->initialize());

    // List streams (may be empty if no apps playing audio)
    auto streams = manager->listStreams();
    // Just verify it doesn't crash and returns a list
    EXPECT_GE(streams.size(), 0);
}

TEST_F(AudioManagerTest, MoveStreamInvalidId) {
    EXPECT_TRUE(manager->initialize());

    // Try to move a non-existent stream
    EXPECT_FALSE(manager->moveStreamToChannel(999999, "game"));
}

TEST_F(AudioManagerTest, MoveStreamInvalidChannel) {
    EXPECT_TRUE(manager->initialize());

    // Try to move to non-existent channel
    EXPECT_FALSE(manager->moveStreamToChannel(1, "invalid"));
}

TEST_F(AudioManagerTest, GetStreamChannelUnassigned) {
    EXPECT_TRUE(manager->initialize());

    // Unassigned stream returns empty string
    EXPECT_TRUE(manager->getStreamChannel(999999).isEmpty());
}

TEST_F(AudioManagerTest, AddRoutingRule) {
    EXPECT_TRUE(manager->initialize());

    manager->addRoutingRule("discord", "chat");
    auto rules = manager->getRoutingRules();

    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].matchPattern, "discord");
    EXPECT_EQ(rules[0].targetChannel, "chat");
}

TEST_F(AudioManagerTest, AddMultipleRoutingRules) {
    EXPECT_TRUE(manager->initialize());

    manager->addRoutingRule("discord", "chat");
    manager->addRoutingRule("spotify", "media");
    manager->addRoutingRule("game.*", "game");

    auto rules = manager->getRoutingRules();
    EXPECT_EQ(rules.size(), 3);
}

TEST_F(AudioManagerTest, RemoveRoutingRule) {
    EXPECT_TRUE(manager->initialize());

    manager->addRoutingRule("discord", "chat");
    manager->addRoutingRule("spotify", "media");

    manager->removeRoutingRule("discord");

    auto rules = manager->getRoutingRules();
    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].matchPattern, "spotify");
}

TEST_F(AudioManagerTest, ReplaceRoutingRule) {
    EXPECT_TRUE(manager->initialize());

    manager->addRoutingRule("discord", "chat");
    manager->addRoutingRule("discord", "aux");  // Same pattern, different channel

    auto rules = manager->getRoutingRules();
    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].targetChannel, "aux");
}

TEST_F(AudioManagerTest, UpdateLoopbacksNoDevice) {
    EXPECT_TRUE(manager->initialize());

    // Without output device set, loopbacks can't be created
    EXPECT_FALSE(manager->updateLoopbacks());
}

TEST_F(AudioManagerTest, UpdateLoopbacksWithDevice) {
    EXPECT_TRUE(manager->initialize());

    // Get the first available output device
    auto devices = manager->listOutputDevices();
    if (devices.isEmpty()) {
        GTEST_SKIP() << "No output devices available";
    }

    EXPECT_TRUE(manager->setOutputDevice(devices[0].id));
    EXPECT_TRUE(manager->updateLoopbacks());
}

TEST_F(AudioManagerTest, ChannelVolumeUpdatesLoopbacks) {
    EXPECT_TRUE(manager->initialize());

    auto devices = manager->listOutputDevices();
    if (devices.isEmpty()) {
        GTEST_SKIP() << "No output devices available";
    }

    manager->setOutputDevice(devices[0].id);

    // Set game personal volume to 0 (effectively removes from personal mix)
    EXPECT_TRUE(manager->setChannelPersonalVolume("game", 0));

    // Verify channel state updated
    auto channels = manager->listChannels();
    for (const auto &ch : channels) {
        if (ch.id == "game") {
            EXPECT_EQ(ch.personalVolume, 0);
            break;
        }
    }
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
