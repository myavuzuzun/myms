# myms — Modern Lightweight Linux Tracker & Studio
**Author**: myavuzuzun  
**Platform**: Linux (KDE Plasma 6 / Breeze Native)  
**Core Technologies**: C++ DSP Audio Engine (`miniaudio`, zero-allocation audio loop) + PyQt6 / Qt Quick QML  
**Version Control**: Git-backed (`phase-1`, `phase-2`, active development on `phase-3`)

---

## 1. Vision & Core Philosophy

- **Goal**: A lightweight, distraction-free, CPU-efficient Linux music creation environment blending the speed and power of a **modern tracker** (Renoise / Polyend style) with the intuitive visual flow of step sequencing, unified plugin hosting (VST3, CLAP, VST2), and automatic macro mixing.
- **Visual Design**: 100% compliant with the user's neutral dark **KDE Plasma 6 (Breeze)** desktop environment (`#474747` window background, `#393939` technical cards, KDE `#3daee9` sky blue accents, crisp `#f0f2f5` typography).
- **Clean Slate Startup**: Boots with exactly 1 empty track and no ghost notes or dummy instruments. Zero self-branding badges, logos, or author watermarks inside the UI.
- **Performance**:
  - Real-time C++ audio engine (`libmyms_audio.so`) running on ALSA / PipeWire via `miniaudio.h`.
  - Zero memory allocation, zero locks, zero file I/O on the real-time audio thread.
  - Sub-1% CPU idle and active playback footprint.

---

## 2. Workflow: Tracker-Forward Architecture

```
   ┌────────────────────────────────────────────────────────┐
   │                  myms Pattern Tracker                  │
   │                                                        │
   │  Row │ Track 1: Acid │ Track 2: Sub 808│ Track 3: Lead  │
   │  ════╪═══════════════╪═════════════════╪════════════════│
   │  00  │ C-4 01 FF ----│ C-2 02 C0 ----  │ --- -- -- ---- │
   │  01  │ --- -- -- ----│ --- -- -- ----  │ E-4 03 80 10AF │
   │  02  │ --- -- -- ----│ C-2 02 80 ----  │ G-4 03 80 10BF │
   │  03  │ --- -- -- ----│ --- -- -- ----  │ --- -- -- ---- │
   │  04  │ C-4 01 D0 ----│ D#2 02 C0 ----  │ A#4 03 A0 10FF │
   │      │[Note][Ins][Vol│[Note][Ins][Vol] │[Note][Ins][Fx] │
   └──────┴───────────────┴─────────────────┴────────────────┘
                                  ▲
                         Fast Keyboard Entry
             (Z/S/X/D/C = Notes | 0-9 = Values | Space = Play)
```

- **Tracker Columns per Track**:
  - `NOTE`: Musical note and octave (e.g. `C-4`, `F#3`, `OFF`).
  - `INST`: Instrument ID (`01`, `02`, `03`...).
  - `VOL`: Step volume / velocity (`00`–`FF`).
  - `FX`: Parameter locks & effects (`10` Cutoff, `01` Pitch Up, `02` Pitch Down, `0C` Retrigger/Roll, `0E` Delay).
- **2-Octave Piano Keyboard Entry**:
  - Native auto-detected **Turkish Q layout** (`A..Ğ` lower octave, `Q..I` upper octave, `ı`/`i` separation, `<`/`>` octave shifts) with live toggle to standard **US QWERTY**.
  - Jam Mode (audition without writing) vs. Edit Mode (stamp note into cell and auto-advance row).
- **Transport Controls**:
  - **True Pause**: Freezes sequencer clock on the exact row without rewinding.
  - **Paused Row Highlight**: Row glows in warm amber (`#544626`) with a 1px border (`#f39c12`) and `❚❚` row number badge.
  - **Stop**: Halts playback and immediately rewinds to row 0.
  - **Play from Cursor**: Click or double-click any row number to start playback directly from that step.
- **Tracker Monophony & Clean Cutoff**:
  - Each tracker track enforces strict monophony: any new note on a track immediately cuts the previous playing voice on that track.
  - Explicit `OFF` (254) events immediately release and silence active notes on all instruments (AuraSynth, SoundFont, BeatPad, and Plugin Instruments).

---

## 3. "Smart Macro" Mixing & Mastering Engine

- **Per-Track / Master DSP Chain**:
  - **Punch**: Dynamic transient designer + parallel punch compression.
  - **Warmth**: Tube and tape soft-saturation curve.
  - **Tone**: Linear tilt EQ (sub bass warmth through high-frequency air).
  - **Space**: Ducked algorithmic stereo diffusion and ambient decay.
- **Master Bus Soft Limiter**:
  - Zero-latency lookahead true-peak soft limiter preventing digital clipping cleanly.
  - Master stereo output VU meter with peak monitoring.

---

## 4. Multi-Instrument Sound Engine & UI Dock

### 4.1. Supported Sound Generators (4 Core Types)
1. **AuraSynth (`TRACK_TYPE_SYNTH = 0`)**: Polyphonic subtractive synthesizer with dual anti-aliased oscillators, ZDF SVF 24dB resonant filter, and dual ADSR envelopes.
2. **SoundFont2 (`TRACK_TYPE_SOUNDFONT = 1`, `FontBox`)**: Multi-channel GM SoundFont player supporting up to 32 independent MIDI channels with per-track preset selection.
3. **BeatPad (`TRACK_TYPE_DRUMS = 2`)**: 8-slot drum sampler with MPC pads, pitch (-24..+24 st), decay, volume, pan, reverse, and WAV/FLAC sample loading.
4. **Plugin Instrument (`TRACK_TYPE_PLUGIN = 3`, Unified Host)**:
   - Single unified engine selector option: `🔌 Plugin Instrument` (Matrix header badge: `PLG`).
   - Unified Plugin Host automatically detects plugin binary format:
     - **VST3**: `.vst3` directory bundles or shared objects (`GetPluginFactory`).
     - **CLAP**: `.clap` shared libraries (`clap_entry`).
     - **VST2**: `.so` shared libraries (`VSTPluginMain` / `main_plugin` / `main`).
   - Hosts native Graphical User Interfaces (GUIs) in dedicated floating KDE Breeze windows (`[ 🗔 OPEN GUI ]`).
   - Integrated parameter editor with smooth sliders for quick parameter manipulation and sound design.

### 4.2. Per-Track Plugin Insert Effects (FX Chain & Modal Window)
- Every track features an Insert FX chain with up to 4 dedicated slots (`MAX_TRACK_FX = 4`).
- Works on **any** track type (AuraSynth, SoundFont2, BeatPad, or Plugin Instrument).
- Signal path per track:
  `[Instrument Sound Generator] ➔ [Insert FX Slot 1..4] ➔ [Track Volume & Pan] ➔ [Master Bus]`.
- Mixer Channel Strip Integration:
  - Clean 3-button row: `[ MUTE ]`, `[ SOLO ]`, `[ FX ]` (or `[ FX (N) ]` glowing in KDE sky blue when effects are loaded).
  - Clicking `[ FX ]` opens the dedicated in-app `TrackFxModal` window.
- In-App Track Insert FX Modal (`TrackFxModal`):
  - Visual signal flow diagram from Generator through FX 1..4 to Mixer.
  - Dedicated cards for all 4 slots showing loaded format badges (`VST3`, `CLAP`, `VST2`), plugin names, vendors, paths, and status.
  - Action buttons: `[ 🗔 OPEN GUI ]`, `[ 📁 CHANGE... ]`, `[ ✕ REMOVE ]`, and quick demo presets.

### 4.3. Native Plugin GUI Window Host (`PluginNativeHostWindow`)
- Standalone top-level floating window wrapping an X11 container widget (`WA_NativeWindow`, `WA_PaintOnScreen`).
- Dynamically resizes to match plugin's native dimensions (`effEditGetRect`, `CLAP_EXT_GUI`, or `IPlugView::getSize`).
- Forwards mouse clicks and drags seamlessly to plugin editors with dedicated X11 event pumps.
- Title bar reflects plugin and track context: `myms — [Plugin Name] ([Format]) — Track [N]`.
- Closing the window cleanly invokes `effEditClose`, `clap_gui->destroy`, or `IPlugView::removed`.

### 4.4. Bundled Reference Plugins
1. **Interactive GUI Test Synth (`plugins/gui_synth/demo_gui_synth.so`)**:
   - Bundled lightweight Linux synthesizer featuring a custom interactive X11 graphical interface.
   - Real-time animated oscilloscope/waveform display, custom draggable knobs/sliders for Cutoff, Resonance, Decay, and Volume, and instant parameter feedback.
2. **Track Insert Filter FX (`plugins/effects/demo_filter_fx.so`)**:
   - Bundled stereo resonant multi-mode filter and saturation insert effect with interactive X11 GUI.
3. **Steinberg VST3 Reference Synth (`plugins/vst3/demo_synth_vst3.so`)**:
   - Native 64-bit Linux VST3 synthesizer built with official `vst3_c_api.h` implementing `IPluginFactory`, `IComponent`, `IAudioProcessor`, and `IEditController`.
4. **CLAP Reference Synth (`plugins/clap/demo_synth_clap.clap`)**:
   - Native CLAP synthesizer plugin implementing `clap_entry`, params, and note events.
5. **VST 2.4 Reference Synth (`plugins/vst2/demo_synth_vst2.so`)**:
   - Standard 64-bit Linux VST2 plugin.

### 4.5. Streamlined Dock System
- Dynamic splitter bar using global coordinate delta mapping (`mapToItem`) to smoothly adjust bottom deck height against the Tracker matrix without jitter.
- Simplified 2-Tab Dock: `🎚 TRACK MIXER` and `🎛 SMART MACROS`.
- Eliminates dock clutter: sound design and plugin instruments are edited exclusively in the dedicated in-app `InstrumentEditorModal` when clicking `[ ⚙ EDIT SOUND ]`.

---

## 5. Project Persistence & Desktop Integration

- **Native `.myms` Project Schema (JSON v3)**:
  - Stores tempo (`bpm`), pattern length (`numRows`), looping, base octave, keyboard layout.
  - Full Smart Macro values (`punch`, `warmth`, `tone`, `space`, `master_volume`).
  - Dynamic track roster with custom instrument names, mixer parameters (vol, pan, mute, solo, sf2_preset).
  - Plugin instrument paths, format types, and saved parameter arrays.
  - Track insert FX paths and parameter states.
  - Multi-pattern data and song arrangement step sequence.
- **KDE Plasma 6 Global Menu (`libdbusmenu-qt6`) & Clean Header**:
  - Native `QMainWindow` + `QMenuBar` docking into the KDE Plasma panel.
  - File (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`), Edit, Playback, and Tracks menus.
  - Current Project Name cleanly displayed in the toolbar header (left blank when untitled).
  - Dynamic window title: `myms — [ProjectName]` (or `myms` when untitled).

---

## 6. Phased Implementation Roadmap

- [x] **Phase 1: Minimal Viable Sound Engine & Tracker UI** *(Completed)*
  - Zero-allocation C++ DSP engine with `miniaudio.h` backend.
  - Dynamic track creation & removal.
  - ZDF SVF 24dB filter and polyphonic synth voices.
  - Smart Macros auto-mastering engine.
  - Native Turkish Q & US keyboard tracker entry.
  - True pause transport with visual amber paused row highlight.
  - Editable BPM text input.
  - Resizable bottom dock.
  - Native `.myms` project file saving/opening & KDE Global Menu.
  - Git repository initialization, checkpoint commit, and `phase-1` tag.

- [x] **Phase 2: SoundFont2 (`TinySoundFont`), 8-Slot Drum Sampler & MuseScore-Style Track Mixer** *(Completed)*
  - **MuseScore-Style Track Mixer (`MixerView`)**: Docked mixer with dedicated channel strips for each track featuring track naming, engine selector, inline preset selector, volume fader (-inf to +6dB) with live dual-channel VU meters, pan control (`L 100%` .. `C` .. `R 100%`), mute `[M]`, solo `[S]`, and `[⚙ EDIT]` button.
  - **Multi-Channel SoundFont2 Routing (`FontBox`)**: Up to 32 independent MIDI channels mapped to tracks.
  - **8-Slot Drum Sampler (`BeatPad`)**: Responsive MPC pads with synthesized punchy kits and WAV/MP3/FLAC sample loader.
  - **Song Sequence Arranger Bar (`SongArrangerBar`)**: Visual sequence timeline (`[ 00 ] -> [ 01 ] -> [ 00 ] ... [ + STEP ]`).
  - **Real-Time Matrix Pattern Synchronization**: Seamless pattern switching in both PAT and SONG modes.
  - **Project Format v2 Persistence**: Complete serialization of mixer states, patterns, and song arrangement.
  - Git checkpoint commit and `phase-2` tag.

- [x] **Phase 3: Unified Plugin Instrument (VST3 / CLAP / VST2), Native GUI Windows & Track Insert Effects** *(Completed)*
  - **Unified Plugin Instrument (`TRACK_TYPE_PLUGIN = 3`)**: Consolidated VST2, VST3, and CLAP plugins under a single `🔌 Plugin Instrument` engine option across Mixer, Tracker Matrix (`PLG`), Dock, and Studio Modal.
  - **VST3 Host Architecture**: Full Steinberg VST3 host loader via official C API single header (`vst3_c_api.h`), supporting `.vst3` directory bundles and Linux shared objects.
  - **Native Plugin GUI Window Host (`PluginNativeHostWindow`)**: Standalone floating KDE Breeze windows with dedicated X11 native container (`WA_NativeWindow`, `WA_PaintOnScreen`) and `[ 🗔 OPEN GUI ]` controls supporting mouse and keyboard interactivity.
  - **Bundled GUI Reference Plugins**:
    - `plugins/gui_synth/demo_gui_synth.so`: Interactive Linux synthesizer with custom X11 GUI, animated oscilloscope, and draggable parameter sliders.
    - `plugins/effects/demo_filter_fx.so`: Stereo resonant multi-mode filter and soft saturation insert effect with interactive X11 GUI.
  - **Per-Track Plugin Insert Effects**: 2-slot insert FX chain per track (`MAX_TRACK_FX = 2`) with real-time routing (`Instrument -> FX1 -> FX2 -> Vol/Pan -> Master`), Mixer slot controls (`[ + FX ]` / `[ FX Name ] [ 🗔 ] [ ✕ ]`), and automated parameter persistence.
  - **Unified Studio Modal (`InstrumentEditorModal`)**: Refactored to 4 core tabs (`SYNTH`, `SOUNDFONT`, `BEATPAD`, `PLUGIN`) matching the 4 engine sound generator types.
  - **Project Format v3 Persistence**: Schema v3 serializing unified plugin paths, formats, parameter arrays, and per-track insert FX chains.
  - Git checkpoint commit and `phase-3` tag.

- [ ] **Phase 4: Non-realtime Audio Export & Final Polish**
  - Non-realtime 32-bit float / 24-bit / 16-bit PCM WAV audio export.
  - Master bounce options (entire song sequence or current pattern loop).
