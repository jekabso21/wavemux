# WaveMux — Development Roadmap

A Sonar‑style virtual audio mixer for Linux built with C++ + Qt on top of PipeWire.

Goal: deliver a simple, gamer‑friendly mixer with Personal Mix, Stream Mix, channel buses, mic routing, and master control — without pro‑audio complexity.

---

## 🎯 Core Product Vision

WaveMux should feel like SteelSeries Sonar:

• Virtual channels for apps (Game / Chat / Media / AUX / Mic)
• Two output mixes (Personal + Stream)
• Per‑channel inclusion into each mix
• Master volume controlling both mixes
• Persistent routing & profiles
• Works automatically on login

Non‑goals for MVP:
• Studio‑grade DSP
• Patchbay‑style routing UI
• Complex plugin chains

---

## 🧱 Audio Architecture (Final Model)

### Channels (Virtual App Inputs)

wavemux_game
wavemux_chat
wavemux_media
wavemux_aux
wavemux_mic

Apps route into these.

---

### Mix Outputs

wavemux_personal → user headphones
wavemux_stream → OBS / recording software

Each channel can be included in:
• Personal mix
• Stream mix
• Both
• Neither

---

### Master Control

Logical top‑level controller that scales:
• wavemux_personal volume
• wavemux_stream volume

together.

---

## 📆 Phase Roadmap

---

## Phase 1 — Audio Foundations

Goal: prove full routing graph works

Tasks:

* Create all channel buses
* Create Personal + Stream mix sinks
* Route channels into mixes
* Bind Personal mix to hardware output
* Validate volume + mute per node

Deliverable:
✔ Full Sonar‑style signal flow working in terminal

---

## Phase 2 — Core Daemon (C++)

Goal: central audio controller service

Tasks:

* Virtual sink creation manager
* Stream detection
* Routing engine
* Master control logic
* Volume/mute management
* JSON state persistence

Deliverable:
✔ Daemon recreates entire mixer on startup

---

## Phase 3 — DBus API

Goal: clean interface between daemon and UI

Methods:

* ListChannels
* ListStreams
* RouteStream
* SetChannelVolume
* SetMixVolume
* SetMasterVolume
* ToggleInclude(channel, mix)
* SaveProfile
* LoadProfile

Deliverable:
✔ Full audio system controllable remotely

---

## Phase 4 — Qt UI MVP

Goal: Sonar‑like clean interface with first-launch setup

### Setup Wizard (First Launch)

Shown when `~/.config/wavemux/config.json` doesn't exist or `setupComplete: false`

Steps:
1. Welcome — intro to WaveMux
2. Output Device — select headphones/speakers for Personal mix
3. Input Device — select microphone (optional, can skip)
4. Stream Setup — enable Stream mix for OBS (optional)
5. Auto-Configure — create buses, set system defaults, show progress
6. Done — transition to Main UI

### Main UI (Tabbed)

* **Mixer Tab** — channels + master, Personal/Stream toggles, app indicators
* **Apps Tab** — drag-drop routing, app list with channel assignment
* **Settings Tab** — device selection, profile manager, reset

Deliverable:
✔ Setup wizard configures system on first launch
✔ Zero terminal usage required

---

## Phase 5 — System Integration

Goal: always‑on experience

Tasks:

* systemd user service
* Auto restore on login
* Device reconnect handling
* Crash recovery

Deliverable:
✔ Works across reboots like native audio software

---

## Phase 6 — Mic Virtualization

Goal: Wave‑Link‑style mic workflow

Tasks:

* Virtual mic source
* Physical mic routing
* Stream mix default inclusion
* Optional monitoring into personal mix

Deliverable:
✔ Apps select “WaveMux Mic” instead of raw mic

---

## Phase 7 — Quality of Life

Tasks:

* Tray icon
* Quick mute buttons
* Global hotkeys
* Profile hotkeys
* Auto‑routing presets

Deliverable:
✔ Daily‑driver usability

---

## Phase 8 — Optional Enhancements

Future ideas:

* Noise suppression presets
* Limiter on master
* Game auto‑profiles
* Stream ducking
* Plugin architecture

---

## 🚀 Milestones

M1 — Virtual buses working
M2 — Full routing graph automated
M3 — UI MVP usable
M4 — Persistent system service
M5 — Mic workflow complete

---

## ✅ Definition of MVP Success

• Game/Chat/Media/AUX separation works
• Personal + Stream mixes function
• Master controls both mixes
• Routes persist
• Reboot safe
• No audio knowledge required

---

## 🧠 Product Philosophy

If users need to understand audio graphs — we failed.

If it feels like Sonar on Linux — we succeeded.
