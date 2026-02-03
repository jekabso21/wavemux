#include <gtest/gtest.h>
#include <QCoreApplication>
#include "wavemux/types.h"

class TypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        WaveMux::registerMetaTypes();
    }
};

TEST_F(TypesTest, ChannelDefaultValues) {
    WaveMux::Channel channel;
    EXPECT_EQ(channel.volume, 0);
    EXPECT_EQ(channel.muted, false);
    EXPECT_EQ(channel.personalVolume, 100);
    EXPECT_EQ(channel.streamVolume, 0);
    EXPECT_TRUE(channel.id.isEmpty());
    EXPECT_TRUE(channel.displayName.isEmpty());
    EXPECT_TRUE(channel.sinkName.isEmpty());
}

TEST_F(TypesTest, StreamDefaultValues) {
    WaveMux::Stream stream;
    EXPECT_EQ(stream.id, 0u);
    EXPECT_TRUE(stream.appName.isEmpty());
    EXPECT_TRUE(stream.mediaName.isEmpty());
    EXPECT_TRUE(stream.processName.isEmpty());
    EXPECT_TRUE(stream.assignedChannel.isEmpty());
}

TEST_F(TypesTest, DeviceDefaultValues) {
    WaveMux::Device device;
    EXPECT_TRUE(device.id.isEmpty());
    EXPECT_TRUE(device.name.isEmpty());
    EXPECT_TRUE(device.description.isEmpty());
}

TEST_F(TypesTest, RoutingRuleDefaultValues) {
    WaveMux::RoutingRule rule;
    EXPECT_TRUE(rule.matchPattern.isEmpty());
    EXPECT_TRUE(rule.targetChannel.isEmpty());
}

TEST_F(TypesTest, ProfileDefaultValues) {
    WaveMux::Profile profile;
    EXPECT_TRUE(profile.name.isEmpty());
    EXPECT_TRUE(profile.channels.isEmpty());
    EXPECT_TRUE(profile.rules.isEmpty());
}

TEST_F(TypesTest, ConfigDefaultValues) {
    WaveMux::Config config;
    EXPECT_EQ(config.setupComplete, false);
    EXPECT_TRUE(config.outputDevice.isEmpty());
    EXPECT_EQ(config.streamEnabled, false);
    EXPECT_TRUE(config.activeProfile.isEmpty());
    EXPECT_TRUE(config.profiles.isEmpty());
    EXPECT_TRUE(config.routingRules.isEmpty());
}

TEST_F(TypesTest, ChannelAssignment) {
    WaveMux::Channel channel;
    channel.id = "game";
    channel.displayName = "Game";
    channel.sinkName = "wavemux_game";
    channel.volume = 75;
    channel.muted = true;
    channel.personalVolume = 80;
    channel.streamVolume = 50;

    EXPECT_EQ(channel.id, "game");
    EXPECT_EQ(channel.displayName, "Game");
    EXPECT_EQ(channel.sinkName, "wavemux_game");
    EXPECT_EQ(channel.volume, 75);
    EXPECT_EQ(channel.muted, true);
    EXPECT_EQ(channel.personalVolume, 80);
    EXPECT_EQ(channel.streamVolume, 50);
}

TEST_F(TypesTest, StreamAssignment) {
    WaveMux::Stream stream;
    stream.id = 12345;
    stream.appName = "Discord";
    stream.mediaName = "Voice";
    stream.processName = "discord";
    stream.assignedChannel = "chat";

    EXPECT_EQ(stream.id, 12345u);
    EXPECT_EQ(stream.appName, "Discord");
    EXPECT_EQ(stream.mediaName, "Voice");
    EXPECT_EQ(stream.processName, "discord");
    EXPECT_EQ(stream.assignedChannel, "chat");
}

TEST_F(TypesTest, RoutingRuleAssignment) {
    WaveMux::RoutingRule rule;
    rule.matchPattern = "discord";
    rule.targetChannel = "chat";

    EXPECT_EQ(rule.matchPattern, "discord");
    EXPECT_EQ(rule.targetChannel, "chat");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
