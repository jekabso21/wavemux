#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "audiomanager.h"
#include "configmanager.h"
#include "wavemux/types.h"

class ConfigManagerTest : public ::testing::Test {
protected:
    WaveMux::AudioManager *manager = nullptr;
    WaveMux::ConfigManager *config = nullptr;
    QString testConfigPath;

    void SetUp() override {
        WaveMux::registerMetaTypes();
        cleanupSinks();

        manager = new WaveMux::AudioManager();
        config = new WaveMux::ConfigManager(manager);

        // Get and clean the config path
        testConfigPath = config->configPath();
        QFile::remove(testConfigPath);
    }

    void TearDown() override {
        if (manager) {
            manager->shutdown();
            delete manager;
            manager = nullptr;
        }
        delete config;
        config = nullptr;

        // Clean up test config
        QFile::remove(testConfigPath);
        cleanupSinks();
    }

    void cleanupSinks() {
        QProcess process;
        process.start("sh", {"-c", "pactl list modules short | grep null-sink | cut -f1 | xargs -r -n1 pactl unload-module"});
        process.waitForFinished(5000);
    }
};

TEST_F(ConfigManagerTest, InitialSetupNotComplete) {
    EXPECT_FALSE(config->isSetupComplete());
}

TEST_F(ConfigManagerTest, SetSetupComplete) {
    config->setSetupComplete(true);
    EXPECT_TRUE(config->isSetupComplete());
}

TEST_F(ConfigManagerTest, LoadNonexistentConfig) {
    // Should return false but not crash
    EXPECT_FALSE(config->load());
}

TEST_F(ConfigManagerTest, SaveCreatesConfigFile) {
    EXPECT_TRUE(manager->initialize());

    config->setSetupComplete(true);
    EXPECT_TRUE(config->save());

    EXPECT_TRUE(QFile::exists(testConfigPath));
}

TEST_F(ConfigManagerTest, SaveAndLoadConfig) {
    EXPECT_TRUE(manager->initialize());

    // Set some state
    config->setSetupComplete(true);
    manager->addRoutingRule("discord", "chat");
    manager->addRoutingRule("spotify", "media");

    // Save
    EXPECT_TRUE(config->save());

    // Create new config manager and load
    WaveMux::ConfigManager config2(manager);
    EXPECT_TRUE(config2.load());

    EXPECT_TRUE(config2.isSetupComplete());

    // Routing rules should be restored
    auto rules = manager->getRoutingRules();
    EXPECT_EQ(rules.size(), 2);
}

TEST_F(ConfigManagerTest, ConfigPathLocation) {
    QString path = config->configPath();
    EXPECT_TRUE(path.contains("wavemux"));
    EXPECT_TRUE(path.endsWith("config.json"));
}

TEST_F(ConfigManagerTest, SaveRoutingRules) {
    EXPECT_TRUE(manager->initialize());

    manager->addRoutingRule("firefox", "media");
    manager->addRoutingRule("zoom", "chat");

    EXPECT_TRUE(config->save());

    // Read config file directly
    QFile file(testConfigPath);
    EXPECT_TRUE(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    file.close();

    EXPECT_TRUE(data.contains("firefox"));
    EXPECT_TRUE(data.contains("media"));
    EXPECT_TRUE(data.contains("zoom"));
    EXPECT_TRUE(data.contains("chat"));
}

TEST_F(ConfigManagerTest, SaveChannelStates) {
    EXPECT_TRUE(manager->initialize());

    manager->setChannelVolume("game", 75);
    manager->setChannelMute("chat", true);

    EXPECT_TRUE(config->save());

    QFile file(testConfigPath);
    EXPECT_TRUE(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    file.close();

    // Verify channel data is in config
    EXPECT_TRUE(data.contains("channels"));
    EXPECT_TRUE(data.contains("game"));
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
