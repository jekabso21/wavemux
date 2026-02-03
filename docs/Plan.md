# WaveMux — Development Plan (C++ + Qt)

## Goal

Build a lightweight desktop mixer similar to SteelSeries Sonar for Linux using C++ and Qt.

Core focus:

* Simple virtual channels (Game / Chat / Media / AUX / Mic)
* Two output mixes (Personal + Stream)
* Clean mixer UI with tabbed interface
* First-launch setup wizard
* Per-app routing via drag-drop
* Persistent rules and profiles
* Works reliably with PipeWire

Non-goals for MVP:

* Advanced DSP (EQ, noise suppression, compressors)
* Complex audio patchbays

---

## Application Flow

### First Launch

```
App Start
    ↓
Check: ~/.config/wavemux/config.json exists?
    ↓
No → Setup Wizard
Yes → Main UI
```

### Setup Wizard

Step-by-step onboarding (Sonar-style):

**Step 1 — Welcome**
* Brief intro to WaveMux
* "Let's set up your audio"

**Step 2 — Output Device**
* List available playback devices
* User selects primary output (headphones/speakers)
* This becomes the Personal mix destination

**Step 3 — Input Device (Optional)**
* List available input devices
* User selects microphone
* Can skip if no mic needed

**Step 4 — Stream Output (Optional)**
* Enable Stream mix for OBS/recording
* Select virtual device or skip

**Step 5 — Auto-Configure**
* Create virtual buses
* Set system defaults
* Route Personal mix to selected output
* Show progress/confirmation

**Step 6 — Done**
* "Setup complete"
* Launch into Main UI

### Main UI

Tabbed interface (Sonar-style):

**Mixer Tab (Default)**
* All channels visible: Master, Game, Chat, Media, AUX, Mic
* Vertical sliders with mute buttons
* Personal/Stream mix toggles per channel
* Active apps shown under their assigned channel

**Apps Tab**
* List all audio-producing applications
* Drag-drop to assign to channels
* Or click app → select channel dropdown
* Search/filter apps

**Settings Tab**
* Output device selection
* Input device selection
* Profile management (save/load/delete)
* Reset to defaults
* About/version

---

## High-Level Architecture

### 1. Background Daemon (C++)

Responsible for:

* Creating virtual audio sinks (channels + mixes)
* Detecting active audio streams
* Routing streams to channels
* Managing volume & mute
* Saving/restoring state

Runs as a systemd user service.

### 2. Qt UI (C++ / QML)

* Setup wizard on first launch
* Tabbed mixer interface
* Drag-drop app routing
* Profile management
* Communicates with daemon via DBus

---

## Audio Strategy (MVP)

Use PipeWire with PulseAudio compatibility layer.

Core tools/APIs:

* pactl (create virtual sinks, move streams)
* wpctl (volume/mute)
* pactl subscribe (stream events)

Later: migrate to native PipeWire API.

---

## Virtual Audio Nodes

### Channels (App Inputs)

* wavemux_game
* wavemux_chat
* wavemux_media
* wavemux_aux
* wavemux_mic

### Mix Outputs

* wavemux_personal → user headphones
* wavemux_stream → OBS / recording

Each channel can route to:
* Personal mix only
* Stream mix only
* Both
* Neither

---

## Data Model

### Channel
* id
* displayName
* sinkName
* volume
* muted
* inPersonalMix
* inStreamMix

### Stream (App audio)
* id
* appName
* mediaName
* processName
* assignedChannel

### RoutingRule
* match (app/process name pattern)
* targetChannel

### Profile
* name
* channel volumes
* muted states
* mix assignments
* routing rules

### Config
* setupComplete (bool)
* outputDevice
* inputDevice
* streamEnabled
* activeProfile
* profiles[]
* routingRules[]

---

## Configuration Storage

Location: `~/.config/wavemux/config.json`

---

## UI Component Hierarchy

```
MainWindow
├── SetupWizard (shown if !setupComplete)
│   ├── WelcomePage
│   ├── OutputDevicePage
│   ├── InputDevicePage
│   ├── StreamSetupPage
│   ├── ConfigurePage (progress)
│   └── CompletePage
│
└── MainView (shown if setupComplete)
    ├── TabBar
    │   ├── Mixer
    │   ├── Apps
    │   └── Settings
    │
    ├── MixerView
    │   ├── MasterChannel
    │   ├── ChannelStrip (x5)
    │   └── AppIndicators
    │
    ├── AppsView
    │   ├── AppList
    │   └── ChannelDropTargets
    │
    └── SettingsView
        ├── DeviceSelector
        ├── ProfileManager
        └── ResetButton
```

---

## Phased Work Plan

### Phase 0 — Repo & Build System

* CMake setup
* Directory structure: daemon/, ui/, shared/, packaging/
* Deliverable: project builds cleanly

### Phase 1 — Audio Foundations

* Create all channel buses
* Create Personal + Stream mix sinks
* Route channels into mixes
* Bind Personal mix to hardware output
* Volume + mute control
* Deliverable: full signal flow working via CLI

### Phase 2 — Core Daemon

* Virtual sink creation manager
* Stream detection
* Routing engine
* Master control logic
* JSON state persistence
* Deliverable: daemon recreates mixer on startup

### Phase 3 — DBus API

Methods:
* ListChannels
* ListStreams
* RouteStream
* SetChannelVolume
* SetMixVolume
* SetMasterVolume
* ToggleInclude(channel, mix)
* SaveProfile / LoadProfile
* GetConfig / SetConfig

Deliverable: full audio system controllable remotely

### Phase 4 — Qt UI

* Setup wizard (first-launch flow)
* Tabbed main interface
* Mixer view with channel strips
* Apps view with drag-drop routing
* Settings view
* Deliverable: zero terminal usage required

### Phase 5 — System Integration

* systemd user service
* Auto restore on login
* Device reconnect handling
* Crash recovery
* Deliverable: works across reboots

### Phase 6 — Mic Virtualization

* Virtual mic source
* Physical mic routing
* Stream mix inclusion
* Optional monitoring
* Deliverable: apps select "WaveMux Mic"

### Phase 7 — Quality of Life

* System tray icon
* Quick mute buttons
* Global hotkeys
* Profile hotkeys
* Deliverable: daily-driver usability

---

## Success Criteria

MVP success means:

* Setup wizard configures system on first launch
* Game/Chat/Media/AUX separation works
* Personal + Stream mixes function
* Apps can be routed via UI (drag-drop)
* Volumes and routes persist
* Works after reboot
* No terminal usage required

---

## Philosophy

Simple first. Stable always.

If it feels like a sound engineer tool — it failed.
If it feels like Sonar — it succeeded.
