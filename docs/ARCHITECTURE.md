# WaveMux — Architecture

WaveMux is a Sonar‑style virtual audio mixer for Linux built with C++ + Qt on top of PipeWire.

The system is designed to feel like SteelSeries Sonar:

* Virtual app channels (Game / Chat / Media / AUX / Mic)
* Two mixes (Personal + Stream)
* Per‑channel inclusion per mix
* Master volume controlling both mixes
* Persistent routing rules and profiles

---

## 1. High‑Level Components

### 1.1 Daemon (wavemuxd)

A background service responsible for creating and maintaining the audio graph.

Responsibilities:

* Create and own virtual sinks (channels + mixes)
* Detect active application streams
* Route streams into channels
* Route channels into Personal/Stream mixes (include matrix)
* Apply master + mix volumes
* Persist and restore state (rules, volumes, profiles)
* Handle device reconnects and session restarts

Runs as: `systemd --user` service.

### 1.2 UI (WaveMux)

A Qt application (QML recommended) that controls the daemon.

#### Application Flow

```
App Start
    ↓
Check: ~/.config/wavemux/config.json exists && setupComplete?
    ↓
No  → Setup Wizard → Main UI
Yes → Main UI
```

#### Setup Wizard (First Launch)

Shown when config doesn't exist or `setupComplete: false`.

Steps:
1. **Welcome** — intro to WaveMux
2. **Output Device** — select headphones/speakers for Personal mix
3. **Input Device** — select microphone (optional, can skip)
4. **Stream Setup** — enable Stream mix for OBS (optional)
5. **Auto-Configure** — create buses, set system defaults, show progress
6. **Done** — mark `setupComplete: true`, transition to Main UI

The wizard calls daemon methods to:
* Enumerate available devices
* Create virtual sinks
* Set system audio defaults
* Persist initial configuration

#### Main UI (Tabbed, Sonar-style)

* **Mixer Tab** — channel strips (volume, mute), Personal/Stream toggles, app indicators
* **Apps Tab** — drag-drop routing, app list with channel assignment dropdown
* **Settings Tab** — device selection, profile manager, reset to run wizard again

Responsibilities:

* Display channel mixer (volume, mute)
* Display include toggles (Personal/Stream)
* Show active apps list with routing dropdown
* Allow selecting hardware output device
* Profile management
* Tray controls (later)

UI does not directly manipulate PipeWire. It always calls the daemon.

### 1.3 IPC (DBus)

The communication layer between UI and daemon.

Responsibilities:

* Provide a stable API
* Support UI and future integrations (CLI, scripts)
* Emit signals on state changes

---

## 2. Audio Model (Sonar‑Style)

WaveMux models audio as:

* Channels: virtual endpoints that apps route into
* Mixes: two outputs built from those channels
* Master: a logical top fader controlling both mixes

### 2.1 Channels (Virtual App Inputs)

Virtual sinks created by daemon:

* `wavemux_game`
* `wavemux_chat`
* `wavemux_media`
* `wavemux_aux`

Optional mic channel (see mic section):

* `wavemux_mic`

Apps route their output streams into these channel sinks.

### 2.2 Mix Outputs

Virtual sinks created by daemon:

* `wavemux_personal`
* `wavemux_stream`

`wavemux_personal` is connected to the user’s selected hardware output (e.g., Nova Pro).

`wavemux_stream` is exposed as the capture target for OBS/recording software.

### 2.3 Master

Master is a logical controller in the daemon/UI.

Definition:

* Master sets a scalar volume applied to both mixes
* Mix volumes can still exist separately (optional advanced trim)

WaveMux master must control:

* Personal mix overall loudness
* Stream mix overall loudness

Implementation:

* daemon computes effective mix volume = master * mixTrim

---

## 3. Routing Graph

### 3.1 Conceptual Flow

Apps → Channel → (include matrix) → Personal/Stream → Outputs

* Each channel may be included in Personal and/or Stream mix
* Inclusion is toggled per channel per mix

### 3.2 Include Matrix

For each channel C:

* If `include[C][Personal]` = true, route C into `wavemux_personal`
* If `include[C][Stream]`   = true, route C into `wavemux_stream`

This is the Sonar core concept.

---

## 4. Mic Architecture

WaveMux includes a mic workflow similar to Sonar/Wave Link.

### 4.1 Physical Mic Input

Example physical sources:

* Elgato Wave XLR

### 4.2 Virtual Mic Output (for applications)

WaveMux should provide a virtual mic source that applications can select:

* “WaveMux Mic”

Goal:

* Apps (Discord, game voice, etc.) select “WaveMux Mic” rather than the raw hardware mic.

### 4.3 Mic Routing

Physical mic → (optional processing later) → WaveMux Mic

Optional:

* Monitor mic into Personal mix (sidetone)
* Always include mic into Stream mix

---

## 5. Implementation Strategy (MVP)

### 5.1 PipeWire Integration Strategy

MVP uses PipeWire’s PulseAudio compatibility layer for maximum distro compatibility.

Primary mechanisms:

* Create virtual sinks via `module-null-sink`
* Move app sink-inputs via `pactl move-sink-input`
* Control volume/mute via `wpctl` (or pactl)
* Detect changes via `pactl subscribe`

Later we can migrate to native PipeWire API without changing UI/DBus.

### 5.2 Why a Daemon

A daemon is required to:

* keep the graph alive even when UI is closed
* auto‑restore on login
* apply routing rules as apps start
* recover from device reconnects

---

## 6. State, Profiles, and Persistence

### 6.1 Configuration & State Storage

File location:

* `~/.config/wavemux/config.json`

Stores:

* `setupComplete` — bool, triggers wizard if false
* `outputDevice` — selected hardware output
* `inputDevice` — selected microphone (optional)
* `streamEnabled` — whether Stream mix is active
* created module IDs (for cleanup/recreate)
* channel volumes/mute
* mix trims
* master level
* include matrix
* per‑app routing rules
* profiles[]

### 6.2 Profiles

Profiles store snapshots of:

* channel volumes
* include matrix
* master/mix trims
* routing rules (optional)

Example profiles:

* Default
* Gaming
* Work Calls
* Night

---

## 7. DBus API (Draft)

### 7.1 Methods

**Setup & Configuration:**

* GetConfig() -> Config
* IsSetupComplete() -> bool
* ListOutputDevices() -> Device[]
* ListInputDevices() -> Device[]
* RunSetup(outputDevice, inputDevice?, streamEnabled) — creates buses, sets defaults
* ResetSetup() — clears config, allows re-running wizard

**Channels & Mixes:**

* ListChannels() -> Channel[]
* ListMixes() -> Mix[]
* ListStreams() -> Stream[]
* RouteStream(streamId, channelId)
* SetChannelVolume(channelId, volume)
* SetChannelMute(channelId, muted)
* SetInclude(channelId, mixId, included)
* SetMasterVolume(volume)
* SetMixTrim(mixId, volume)
* SetHardwareOutput(deviceId)

**Profiles:**

* SaveProfile(name)
* LoadProfile(name)
* DeleteProfile(name)
* ListProfiles() -> string[]

### 7.2 Signals

* ChannelsChanged
* StreamsChanged
* MixesChanged
* StateChanged

Signals should be emitted whenever the daemon detects changes (new apps, device changes, volume changes).

---

## 8. Failure Handling

WaveMux must handle:

* PipeWire restart
* Device disconnect/reconnect
* Daemon crash/restart

Strategies:

* systemd user service with restart policy
* periodic self‑check
* idempotent “ensure graph” operations

---

## 9. Performance & UX Constraints

* Keep latency low (no heavy DSP in MVP)
* Avoid constantly spawning external commands (cache state, batch updates)
* UI should update live but not spam DBus

---

## 10. Future Enhancements

* Native PipeWire client API
* Optional DSP toggles (RNNoise, limiter)
* Auto‑routing presets
* Hotkeys & tray
* OBS helper / plugin

---

## 11. Definitions

Channel: a virtual sink representing an app category.
Mix: a virtual sink representing an output mix (Personal/Stream).
Master: a logical fader controlling both mixes.
Stream: an application playback stream (sink-input).
Include Matrix: mapping of which channels are present in which mixes.
