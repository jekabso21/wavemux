cmake_minimum_required(VERSION 3.16)
project(WaveMux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Options
option(BUILD_DAEMON "Build the WaveMux daemon" ON)
option(BUILD_UI "Build the WaveMux UI" ON)
option(BUILD_TESTS "Build tests" ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core DBus)

# =============================================================================
# Shared Library
# =============================================================================
add_library(wavemux-shared STATIC
    shared/src/types.cpp
    shared/src/dbusclient.cpp
    shared/src/dbusclient.h
)
target_include_directories(wavemux-shared PUBLIC shared/include shared/src)
target_link_libraries(wavemux-shared PUBLIC Qt6::Core Qt6::DBus)

# =============================================================================
# Daemon
# =============================================================================
if(BUILD_DAEMON)
    add_executable(wavemuxd
        daemon/src/main.cpp
        daemon/src/audiomanager.cpp
        daemon/src/audiomanager.h
        daemon/src/configmanager.cpp
        daemon/src/configmanager.h
        daemon/src/dbus/channeldbusadaptor.cpp
        daemon/src/dbus/channeldbusadaptor.h
        daemon/src/dbus/streamdbusadaptor.cpp
        daemon/src/dbus/streamdbusadaptor.h
        daemon/src/dbus/devicedbusadaptor.cpp
        daemon/src/dbus/devicedbusadaptor.h
        daemon/src/dbus/configdbusadaptor.cpp
        daemon/src/dbus/configdbusadaptor.h
    )
    target_include_directories(wavemuxd PRIVATE daemon/src)
    target_link_libraries(wavemuxd PRIVATE wavemux-shared Qt6::Core Qt6::DBus)

    install(TARGETS wavemuxd RUNTIME DESTINATION bin COMPONENT daemon)
endif()

# =============================================================================
# UI
# =============================================================================
if(BUILD_UI)
    find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)

    qt_add_executable(wavemux
        ui/src/main.cpp
        ui/qml/qml.qrc
    )
    target_link_libraries(wavemux PRIVATE
        wavemux-shared
        Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus
    )

    install(TARGETS wavemux RUNTIME DESTINATION bin COMPONENT ui)
endif()

# =============================================================================
# Tests
# =============================================================================
if(BUILD_TESTS)
    find_package(GTest QUIET)

    if(GTest_FOUND)
        enable_testing()
        include(GoogleTest)

        # Types tests
        add_executable(test_types tests/test_types.cpp)
        target_link_libraries(test_types PRIVATE wavemux-shared Qt6::Core Qt6::DBus GTest::gtest GTest::gtest_main)
        gtest_discover_tests(test_types)

        # AudioManager tests
        add_executable(test_audiomanager
            tests/test_audiomanager.cpp
            daemon/src/audiomanager.cpp
            daemon/src/audiomanager.h
        )
        target_include_directories(test_audiomanager PRIVATE daemon/src)
        target_link_libraries(test_audiomanager PRIVATE wavemux-shared Qt6::Core Qt6::DBus GTest::gtest GTest::gtest_main)
        gtest_discover_tests(test_audiomanager)

        # ConfigManager tests
        add_executable(test_config
            tests/test_config.cpp
            daemon/src/audiomanager.cpp
            daemon/src/audiomanager.h
            daemon/src/configmanager.cpp
            daemon/src/configmanager.h
        )
        target_include_directories(test_config PRIVATE daemon/src)
        target_link_libraries(test_config PRIVATE wavemux-shared Qt6::Core Qt6::DBus GTest::gtest GTest::gtest_main)
        gtest_discover_tests(test_config)
    else()
        message(STATUS "GTest not found - tests disabled. Install with: sudo apt install libgtest-dev")
    endif()
endif()

# =============================================================================
# Install
# =============================================================================
install(FILES packaging/wavemux.service DESTINATION lib/systemd/user COMPONENT daemon)
