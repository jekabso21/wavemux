# WaveMux

**A Linux desktop audio mixer inspired by SteelSeries Sonar**

---

## Why WaveMux?

A few days ago, I made the switch from Windows to Linux. While I love the freedom and customization Linux offers, I quickly realized something was missing: **SteelSeries GG Sonar**.

For those unfamiliar, Sonar is an incredibly user-friendly audio mixer that lets you split your audio into separate channels (Game, Chat, Media, etc.) and create different mixes for your headphones vs. what goes to your stream/recording. No complicated audio routing knowledge required, it just works.

After searching for a Linux alternative and coming up empty, I decided to build one myself. Thus, **WaveMux** was born.

I still have a long way to go, but I believe that with continued work, this can become a solid open-source solution for Linux users who:
- Previously used SteelSeries GG Sonar and miss it
- Want simple audio routing without diving deep into PipeWire/PulseAudio internals
- Need separate audio mixes for streaming and personal listening

**I'd love to hear your suggestions and feature ideas!** This project is as much about learning as it is about filling a gap in the Linux audio ecosystem.

> **Fair warning:** I don't have extensive C++ experience, so you might find some... creative code in here. I'm learning as I go and trying to improve. Constructive feedback is always welcome!

---

## Current State

WaveMux is in early development. Here's what currently works:

- **Virtual audio channels**: Game, Chat, Media, and AUX sinks that applications can route to
- **Two output mixes:**
  - **Personal**: What you hear in your headphones
  - **Stream**: What OBS/recording software captures
- **Per-channel mix inclusion**: Control how much of each channel goes to Personal vs. Stream
- **Master volume**: Single fader that controls overall output
- **Basic UI**: Dark theme mixer interface with channel strips
- **App detection**: See running audio applications and assign them to channels
- **Configuration persistence**: Your settings survive reboots

What's missing (planned):
- Automatic routing rules based on app names
- Profile system for quick switching
- System tray integration
- Installer/packages for easy distribution
- Polish, polish, and more polish

---

## Screenshots

*Coming soon!*

---

## How It Works

WaveMux consists of two parts:

1. **wavemuxd** (daemon): Background service that creates virtual audio devices and handles all the routing magic via PipeWire
2. **wavemux** (UI): Qt/QML application that talks to the daemon over DBus

```
┌─────────────────────────────────────────────────────────────┐
│                        Applications                          │
│              (Games, Discord, Spotify, etc.)                 │
└─────────────────────┬───────────────────────────────────────┘
                      │ route audio to
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Virtual Channels                          │
│      wavemux_game   wavemux_chat   wavemux_media   wavemux_aux      │
└─────────────────────┬───────────────────────────────────────┘
                      │ mix into
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                      Output Mixes                            │
│    wavemux_personal (You)          wavemux_stream (OBS)     │
│         🎧 Headphones                  📡 Recording          │
└─────────────────────────────────────────────────────────────┘
```

---

## Requirements

- **Linux** with PipeWire (with PulseAudio compatibility layer)
- **Qt 6** (Core, DBus, Quick, QuickControls2)
- **CMake 3.16+**
- **GCC/Clang** with C++17 support

### Installing Dependencies

**Ubuntu/Debian:**
```bash
# Build dependencies
sudo apt install cmake qt6-base-dev qt6-declarative-dev libgl1-mesa-dev

# Runtime dependencies (for development)
sudo apt install qml6-module-qtquick-controls qml6-module-qtquick-templates \
                 qml6-module-qtquick-window qml6-module-qtquick-layouts
```

**Fedora:**
```bash
sudo dnf install cmake qt6-qtbase-devel qt6-qtdeclarative-devel mesa-libGL-devel
```

**Arch Linux:**
```bash
sudo pacman -S cmake qt6-base qt6-declarative
```

---

## Building

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/WaveMux.git
cd WaveMux

# Create build directory
mkdir -p build && cd build

# Configure
cmake ..

# Build everything
cmake --build .
```

### Build Targets

```bash
cmake --build . --target wavemuxd    # Daemon only
cmake --build . --target wavemux     # UI only
cmake --build .                      # Everything
```

---

## Running

**Start the daemon:**
```bash
./build/daemon/wavemuxd
```

**Start the UI (in a separate terminal):**
```bash
./build/ui/wavemux
```

### Running as a Service

WaveMux includes a systemd user service file:

```bash
# Copy service file
mkdir -p ~/.config/systemd/user
cp packaging/wavemux.service ~/.config/systemd/user/

# Enable and start
systemctl --user enable wavemux
systemctl --user start wavemux
```

---

## Usage

### First Launch

When you first run WaveMux, a setup wizard will guide you through:

1. Selecting your output device (headphones/speakers)
2. Enabling the Stream mix for OBS (optional)

### Mixer View

The main interface shows your audio channels as vertical strips:

- **Drag the sliders** to adjust volume
- **Click the mute button** to mute a channel
- **Personal/Stream sliders** control how much of each channel goes to each mix

### Apps View

See all applications currently playing audio:

- **Click a channel button** to route that app to a specific channel
- **Unassigned apps** show with a red border as a reminder

---

## Project Structure

```
WaveMux/
├── daemon/           # Background service (wavemuxd)
│   └── src/
│       ├── main.cpp
│       ├── audiomanager.cpp/h    # PipeWire/pactl interface
│       ├── configmanager.cpp/h   # Settings persistence
│       └── dbus/                 # DBus service adaptors
├── ui/               # Qt/QML application (wavemux)
│   ├── src/
│   │   └── main.cpp
│   └── qml/          # UI components
│       ├── Main.qml
│       ├── MixerView.qml
│       ├── ChannelStrip.qml
│       └── ...
├── shared/           # Common code (IPC types, DBus client)
├── tests/            # Unit tests
├── packaging/        # systemd service files
└── CMakeLists.txt
```

---

## Running Tests

```bash
cd build
cmake --build .
ctest --output-on-failure
```

Note: Audio tests require PipeWire to be running.

---

## Configuration

Configuration is stored at `~/.config/wavemux/config.json` and includes:

- Device selections
- Channel volumes
- Mix levels
- Routing rules
- Profiles

---

## Contributing

Contributions are welcome! Whether it's:

- **Bug reports**: Something not working? Let me know!
- **Feature requests**: What would make WaveMux better for you?
- **Code contributions**: PRs are appreciated
- **Documentation**: Help make this README better
- **Testing**: Try it out and share your experience

### Ideas I'd Love Help With

- Better app icon detection
- Flatpak/Snap packaging
- Integration with popular streaming software
- Audio ducking implementation (automatically lower music when voice detected)
- Noise suppression algorithms (RNNoise integration?)
- VST/LV2 plugin hosting
- JACK audio support
- Latency optimization
- Multi-device output (play to multiple devices simultaneously)

---

## Roadmap

### Phase 1: Foundation (Current)
- [x] Virtual channel creation
- [x] Personal/Stream mix routing
- [x] Basic mixer UI
- [x] App detection and manual routing
- [x] Configuration persistence

### Phase 2: Core Features
- [ ] Automatic routing rules (by app name patterns)
- [ ] Profile system (gaming, streaming, music, etc.)
- [ ] System tray with quick controls
- [ ] Better error handling and device recovery
- [ ] Keyboard shortcuts for volume control

### Phase 3: Microphone Support
- [ ] Microphone channel with dedicated controls
- [ ] Mic monitoring (hear yourself)
- [ ] Noise gate (cut background noise below threshold)
- [ ] Noise suppression (AI-based background noise removal)
- [ ] Mic EQ (bass, mid, treble adjustment)
- [ ] Compressor/limiter (prevent clipping, consistent volume)
- [ ] Separate mic routing to Personal vs Stream mix

### Phase 4: Advanced Audio Processing
- [ ] Per-channel EQ (parametric equalizer)
- [ ] Per-channel compressor/limiter
- [ ] Audio ducking (auto-lower music when someone talks in Chat)
- [ ] Spatial audio / virtual surround
- [ ] Audio visualization (spectrum analyzer, VU meters)

### Phase 5: Polish & Distribution
- [ ] Native PipeWire API (replace pactl/wpctl for better performance)
- [ ] .deb and .rpm packages
- [ ] Flatpak/Snap support
- [ ] AUR package for Arch users
- [ ] Auto-updater
- [ ] Proper application icon and branding

### Phase 6: Pro Features (Long-term)
- [ ] VST/LV2 plugin support
- [ ] Recording functionality (record any channel)
- [ ] Soundboard integration
- [ ] Stream deck / macro pad support
- [ ] Multi-device output (clone audio to multiple outputs)
- [ ] Bluetooth device handling improvements
- [ ] Per-application EQ presets

---

## Acknowledgments

- **SteelSeries** for creating Sonar and inspiring this project
- The **PipeWire** and **Qt** teams for excellent documentation
- Everyone who's contributed feedback and ideas

---

## License

This project is licensed under the **GNU General Public License v3.0** [GPL-3.0](LICENSE).

This means you're free to use, modify, and distribute this software, but any derivative works must also be open source under the same license.

**If you find WaveMux useful, consider starring the repo! It helps others discover the project.**
