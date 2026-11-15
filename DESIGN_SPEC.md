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
  - `NOTE`: Musical note and octave (e.g. `C-4`, `F#3`, `OFF`, `---`).
  - `INST`: Instrument ID (`01`, `02`, `03`... corresponding in hex to Track 1, Track 2, Track 3...).
    - **Option C Cross-Track Instrument Routing / Multiplexing**: A cell's `INST` column targets that specific instrument engine (`01` = Instrument 1, `02` = Instrument 2, etc.) even when placed on another track. If `cell.instrument == 0`, it defaults to the current track's own instrument. Enables classic tracker polyphony (e.g. playing chords of Instrument 1 across multiple tracks) and cross-track multiplexing with zero CPU overhead on the real-time audio thread.
  - `VOL`: Step volume / velocity (`00`–`FF` in hex, `--` = 255 default max).
  - `FX`: Parameter locks & effects commands + value in hex (e.g. `102A` = Command `10` Filter Cutoff with value `2A` / 42; `01` Pitch Up, `02` Pitch Down, `0C` Retrigger/Roll, `0E` Delay).
- **Sub-Column Navigation & Hex Editing**:
  - Left / Right arrows navigate smoothly between the 4 sub-columns (`selectedColumn`: 0 = Note, 1 = Inst, 2 = Vol, 3 = FX) and cross track boundaries seamlessly.
  - Active sub-column is visibly highlighted with an elegant KDE Breeze highlight box.
  - Direct typing on sub-columns: `0-9` and `+`/`-` for Inst; `0-9, A-F` for Vol and FX.
  - Column-specific Delete key clears only the selected sub-column without erasing the entire cell (`Shift+Del` clears whole cell).
- **Consistent Fixed Track Width & 2D Synchronized Scrolling (Renoise / Polyend Style)**:
  - Tracks in the pattern matrix maintain a fixed, consistent width (160px) regardless of how many tracks exist. Tracks no longer squish or expand uncontrollably with screen dimensions.
  - **Pinned Left Column**: Pinned 38px row numbers column (`00`..`NN`) on the left with lockstep vertical scrolling (`y: -matrixListView.contentY`).
  - **Pinned Top Header**: Pinned 28px track headers bar at the top with lockstep horizontal scrolling (`x: -matrixListView.contentX`).
  - **Pinned Top-Left Corner**: Dedicated `P00` pattern badge anchored at (0, 0), staying frozen in place.
  - **Smooth Horizontal & Vertical Scrolling**: Full 2D scrolling (`Flickable.HorizontalAndVerticalFlick`) with `ScrollBar.horizontal` and `ScrollBar.vertical` appearing automatically when tracks exceed viewport width.
  - **Cursor Auto-Scroll**: Navigating across tracks with Left/Right keys or selecting instruments smoothly auto-scrolls horizontally (`ensureTrackVisible`) so the active track is always in view; Up/Down keys auto-scroll vertically.
- **Step Cell Inspector & Editor Modal (`StepCellEditorModal`)**:
  - Double-clicking any cell opens a full-featured in-app editor modal.
  - Interactive semitone buttons (`C`..`B`, `OFF`, `---`) with octave stepper and MIDI note number readout.
  - Hex Instrument ID stepper with track name preview (`01 · Synth 1`).
  - Volume slider with hex (`C8`) and percentage (`78%`) display and `DEFAULT` reset.
  - Human-readable Effect Command dropdown (Filter Cutoff, Pitch Slide Up/Down, Retrigger, Delay) with parameter slider, hex readout, and `CLEAR FX` button.
- **Arbitrary Pattern Row Length (1..512 Rows)**:
  - Eliminated hardcoded row presets (`16`, `32`, `64`).
  - Replaced with an interactive arbitrary row control `[ − ] [ 32 ] [ + ]` in the top transport toolbar.
  - Supports direct numeric typing of any row count between 1 and 512, plus single-step increment/decrement buttons (`−1` / `+1`).
- **Pattern Creation Isolation & Streamlined Arranger**:
  - Creating a new pattern (via `+` on the top toolbar PAT selector) or duplicating a pattern creates the pattern in the project without automatically appending it to the song arrangement structure (`m_songOrder`).
  - Removed duplicate toolbar toggle and redundant arranger buttons: the Song Arranger bar features a dedicated `[ PAT LOOP ]` / `[ SONG MODE ]` pill button and `[ + STEP ]` append action, keeping the interface uncluttered and distraction-free.
  - The song structure remains strictly author-controlled.
- **Track & Plugin Effect Reordering (Precision Step Buttons)**:
  - **Track Order Rearranging**: Tracks can be reordered via precision `◀` and `▶` step buttons (`moveTrack(from, to)`) in both the Tracker View column headers and Mixer channel strip headers. Reorders synth state, plugins, mixer controls, Pre/Post FX chains, and sequencer cells atomically.
  - **Plugin Effect Reordering**: Pre-FX and Post-FX in `TrackFxModal` can be reordered via precision `▲` and `▼` step buttons (`swapTrackFx`, `swapTrackPreFx`).
  - **Plugin Effect Power Bypass Toggle**: Each effect has a dedicated zero-allocation `[ ⏻ ]` bypass button. When bypassed, the real-time audio thread passes audio dry with zero CPU overhead. Bypass state is fully persisted in `.myms` project files.
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

- **Master Bus Smart Macros Modal (`SmartMacroModal`)**:
  - Removed the horizontal 26px dock tab bar header from the bottom dock to reclaim screen real estate for the mixer and tracker view.
  - Smart Macros are directly accessible via a dedicated `[ 🎛 MACROS ]` button on the Master channel strip in the Mixer deck.
  - Opens a clean KDE Breeze auto-mastering modal (580x220) with floating centered `✕` close button, PUNCH, WARMTH, TONE, SPACE, and MASTER knobs with live percentage readouts, and `↺ RESET` action.
- **Per-Track / Master DSP Chain**:
  - **Punch**: Dynamic transient designer + parallel punch compression.
  - **Warmth**: Tube and tape soft-saturation curve.
  - **Tone**: Linear tilt EQ (sub bass warmth through high-frequency air).
  - **Space**: Ducked algorithmic stereo diffusion and ambient decay.
- **Master Bus Soft Limiter & Real-Time Metering**:
  - Zero-latency lookahead true-peak soft limiter preventing digital clipping cleanly.
  - Master stereo output VU meter with peak monitoring and RMS level calculation.
  - **LMMS-Style Real-Time Audio DSP CPU Load Meter**:
    - Continuously measures exact audio callback rendering time relative to buffer duration: `load = (elapsedTimeUs / bufferDurationUs) * 100.0`.
    - Exponential moving average with fast attack and smooth release.
    - Rendered in top toolbar with mini horizontal load bar (green/yellow/red), percentage readout (`<1%`, `2.4%`), sample rate badge (`48kHz`), and real-time DSP load tooltip.

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

### 4.2. Pre-Instrument & Post-Instrument Device Chain with Unified In-App Signal Flow
- **Complete Signal Flow Architecture**:
  `[Tracker Sequencer / Keyboard] ➔ [1. Pre-Instrument FX Chain (0..31)] ➔ [2. The Instrument (Synth / SoundFont / BeatPad / Plugin)] ➔ [3. Post-Instrument FX Chain (0..31)] ➔ [Track Vol & Pan] ➔ [Master Bus FX Chain (0..31)] ➔ [Smart Macros & Limiter]`.
- **Pre-Instrument FX Chain (`m_trackPreFx`, up to 32 slots per track)**:
  - Users can load VST3, CLAP, and VST2 plugins *before* the sound generator.
  - Receives live and pattern MIDI note-on / note-off events directly from the tracker sequencer and virtual keyboard.
  - Any audio generated by pre-effects is summed with the sound generator before entering the post-insert effects chain.
- **The Instrument in the Signal Chain**:
  - The central instrument is visually displayed in the signal chain list between Pre-FX and Post-FX.
  - Features quick instrument type badges (`SYNTH`, `SOUNDFONT`, `BEATPAD`, `PLUGIN`), instant `[ ⚙ EDIT SOUND ]` trigger, and plugin GUI button (`[ 🗔 GUI ]`).
- **Post-Instrument FX Chain (`m_trackFx`, up to 32 slots per track)**:
  - Dynamic, unlimited Insert FX chain architecture processing stereo audio waveforms.
  - Works on **any** track type (AuraSynth, SoundFont2, BeatPad, or Plugin Instrument).
- **Master Bus Insert FX Chain (`m_masterFx`, up to 32 slots)**:
  - The Master Bus features its own dedicated insert effects chain for mastering limiters, compressors, EQs, and visualizers.
  - Addressed seamlessly across C++ engine, C API, and Python bridge using `track == -1`.
  - Accessible via a prominent `[ FX ]` / `[ FX (N) ]` button on the Master channel strip in the Mixer deck.
- **Floating Centered Modal Close Button (`✕`) & Modal Escape Key Handling**:
  - Internal `✓ DONE` buttons removed from both `InstrumentEditorModal` and `TrackFxModal`.
  - Replaced with a sleek circular `✕` close button floating directly *above* the modal window, centered horizontally (`anchors.bottom: modalDialog.top; anchors.bottomMargin: 10; anchors.horizontalCenter: modalDialog.horizontalCenter;`).
  - **Modal Escape Key Prioritization**: When any popup modal window is open, pressing `Esc` closes the top-most modal immediately and consumes the event without affecting audio playback. Pressing `Esc` when all modals are closed stops playback as normal.
- **In-App Master-Detail Track & Master FX Modal (`TrackFxModal`)**:
  - Spacious Two-Pane Master-Detail layout (960x600) compliant with KDE Plasma 6 Breeze design.
  - **Left Pane (Device Signal Chain List, 320px wide)**:
    - Pre-Instrument FX section with step badges (`PRE 1`, `PRE 2`), format badges (`VST3`, `CLAP`, `VST2`), plugin names, and `[ ➕ ADD PRE-EFFECT... ]`.
    - Distinct central Instrument card with icon, track name, engine type badge, and `[ ⚙ EDIT ]` button.
    - Post-Instrument FX section with step badges (`FX 1`, `FX 2`), format badges, plugin names, and `[ ➕ ADD POST-EFFECT... ]`.
    - For Master Bus (`targetTrack == -1`): Displays Master Bus Insert FX with `M1`, `M2` step badges and `[ ➕ ADD MASTER EFFECT... ]`.
  - **Right Pane (Dedicated Effect & Instrument Inspector)**:
    - Shows selected effect details (format badge, full name, vendor, filesystem path), action buttons (`[ 🗔 OPEN GUI ]`, `[ 📁 CHANGE... ]`, `[ ✕ REMOVE ]`), and a spacious 2-column parameter grid featuring custom rock-solid `DawSlider` controls (`preventStealing: true`) and real-time formatted text readouts with zero cramping.
    - When the central Instrument card is selected: Displays instrument summary card with `[ ⚙ SOUND STUDIO EDITOR... ]` and `[ 🗔 OPEN GUI ]` actions.
- **Bi-directional Parameter Synchronization**:
  - VST2 `audioMasterAutomate` and VST3 `HostVst3ComponentHandler` continuously synchronize changes made inside native GUI windows back to DAW sliders and vice-versa at 60Hz without locks or audio glitches.

### 4.3. Native Plugin GUI Window Host (`PluginNativeHostWindow`)
- Standalone top-level floating window wrapping an X11 container widget (`WA_NativeWindow`, `WA_PaintOnScreen`).
- **Wayland / XWayland Compatibility**: Forces `QT_QPA_PLATFORM=xcb` at line 1 before any PyQt6 module loads to guarantee `winId()` returns a true X11 Window ID instead of an incompatible Wayland surface handle.
- **Modal Window Independence**: Instantiates with `super().__init__(None, Qt.WindowType.Window)` and raises over full-screen modal overlays (`InstrumentEditorModal`, `TrackFxModal`) so plugin GUIs never get occluded or suppressed by modal backdrop scrims.
- Dynamically resizes to match plugin's native dimensions (`effEditGetRect`, `CLAP_EXT_GUI`, or `IPlugView::getSize`).
- Forwards mouse clicks and drags seamlessly to plugin editors with dedicated X11 event pumps.
- Title bar reflects plugin and track context: `myms — [Plugin Name] ([Format]) — Track [N]`.
- Closing the window cleanly invokes `effEditClose`, `clap_gui->destroy`, or `IPlugView::removed`.

### 4.4. Bundled Reference Plugins
1. **Interactive GUI Test Synth (`plugins/gui_synth/demo_gui_synth.so`)**:
   - Bundled lightweight Linux synthesizer featuring a custom interactive X11 graphical interface.
   - Real-time animated oscilloscope/waveform display, custom draggable knobs/sliders for Cutoff, Resonance, Decay, and Volume, and instant parameter feedback.
2. **Steinberg VST3 Reference Synth (`plugins/vst3/demo_synth_vst3.so`)**:
   - Native 64-bit Linux VST3 synthesizer built with official `vst3_c_api.h` implementing `IPluginFactory`, `IComponent`, `IAudioProcessor`, and `IEditController`.
   - Equipped with complete Steinberg `IPlugView` native X11 graphical interface with emerald header, animated waveform visualizer, and draggable Cutoff, Resonance, and Volume controls.
3. **Track Insert Filter FX (`plugins/effects/demo_filter_fx.so`)**:
   - Bundled stereo resonant multi-mode filter and saturation insert effect with interactive X11 GUI.
4. **CLAP Reference Synth (`plugins/clap/demo_synth_clap.clap`)**:
   - Native CLAP synthesizer plugin implementing `clap_entry`, params, and note events.
5. **VST 2.4 Reference Synth (`plugins/vst2/demo_synth_vst2.so`)**:
   - Standard 64-bit Linux VST2 plugin.

### 4.5. Streamlined Dock & Dedicated Synth Editor
- Dynamic splitter bar using global coordinate delta mapping (`mapToItem`) to smoothly adjust bottom deck height against the Tracker matrix without jitter.
- Simplified 2-Tab Dock: `🎚 TRACK MIXER` and `🎛 SMART MACROS`.
- Eliminates dock clutter: sound design and plugin instruments are edited exclusively in the dedicated in-app `InstrumentEditorModal` when clicking `[ ⚙ EDIT SOUND ]`.
- **AuraSynth Studio Card (`AuraSynthView.qml`)**: Removed redundant track switcher tabs (`T1, T2...`) and `+` add track button. The synth view focuses purely on sound design (Dual Oscillators, ZDF Resonant Filter, ADSR Envelope, and `Init Patch`). Track management remains strictly in the Mixer and Tracker Matrix headers.

## 5. Project Persistence & Desktop Integration

- **Native `.myms` Project Schema (JSON v3.1)**:
  - Stores tempo (`bpm`), pattern length (`numRows`), looping, base octave, keyboard layout.
  - Full Smart Macro values (`punch`, `warmth`, `tone`, `space`, `master_volume`).
  - Dynamic track roster with custom instrument names, mixer parameters (vol, pan, mute, solo, sf2_preset).
  - **Strict Engine Type & SoundFont Persistence**: Only serializes plugin instrument paths and parameters if the track is actively set to Plugin (`TRACK_TYPE_PLUGIN = 3`). Unloads inactive plugins and strictly preserves SoundFont (`TRACK_TYPE_SOUNDFONT = 1`), BeatPad (`TRACK_TYPE_DRUMS = 2`), AuraSynth (`TRACK_TYPE_SYNTH = 0`), and Plugin (`TRACK_TYPE_PLUGIN = 3`) engines when opening projects.
  - Per-track Pre-Instrument FX chains (`pre_fx`) and Post-Instrument FX chains (`fx_chain`).
  - Master Bus Insert FX chain (`master_fx`).
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

- [x] **Phase 3: Unified Plugin Instrument (VST3 / CLAP / VST2), Native GUI Windows, Pre/Post FX Chains & Master FX** *(Completed)*
  - **Unified Plugin Instrument (`TRACK_TYPE_PLUGIN = 3`)**: Consolidated VST2, VST3, and CLAP plugins under a single `🔌 Plugin Instrument` engine option across Mixer, Tracker Matrix (`PLG`), Dock, and Studio Modal.
  - **VST3 Host Architecture**: Full Steinberg VST3 host loader via official C API single header (`vst3_c_api.h`), supporting `.vst3` directory bundles and Linux shared objects.
  - **DAW Non-GUI Parameter Control Engine**:
    - Real-time modulation via `Steinberg_Vst_IParameterChanges` / `Steinberg_Vst_IParamValueQueue` in VST3 process block and `setParamNormalized` in edit controller.
    - Queued CLAP `CLAP_EVENT_PARAM_VALUE` events using plugin parameter IDs and VST2 `setParameter` integration.
    - In-app parameter sliders with dynamic formatted text display strings (`effGetParamDisplay`, percentage, or units).
    - Bi-directional synchronization updating native plugin GUI windows in real time when DAW controls change.
  - **Native Plugin GUI Window Host (`PluginNativeHostWindow`)**: Standalone floating KDE Breeze windows with dedicated X11 native container (`WA_NativeWindow`, `WA_PaintOnScreen`) and `[ 🗔 OPEN GUI ]` controls supporting mouse and keyboard interactivity.
  - **Bundled GUI Reference Plugins**:
    - `plugins/gui_synth/demo_gui_synth.so`: Interactive Linux synthesizer with custom X11 GUI, animated oscilloscope, and draggable parameter sliders.
    - `plugins/effects/demo_filter_fx.so`: Stereo resonant multi-mode filter and soft saturation insert effect with interactive X11 GUI.
    - `plugins/vst3/demo_synth_vst3.so`: Steinberg VST3 reference synth with parameter changes support.
    - `plugins/clap/demo_synth_clap.clap`: Linux CLAP reference synthesizer.
    - `plugins/vst2/demo_synth_vst2.so`: Linux VST 2.4 reference synthesizer.
  - **Pre-Instrument FX Chain & The Instrument in the Signal Flow**:
    - Pre-Instrument FX chain (`m_trackPreFx`, up to 32 slots) receiving live & pattern MIDI notes before sound synthesis.
    - Central Instrument Card clearly represented in the signal flow: `Pre-FX ➔ Instrument ➔ Post-FX`.
    - Instant sound editor access (`[ ⚙ EDIT SOUND ]`) and plugin GUI button (`[ 🗔 GUI ]`).
  - **Post-Instrument FX Chain & SoundFont Routing**:
    - Dynamic insert FX chain per track (`m_trackFx`, up to 32 slots).
    - Full per-channel SoundFont (`TinySoundFont`) insert FX routing: zero-allocation channel splitting routes each SoundFont track through its insert FX chain and track volume/pan before summing to master.
  - **Master Bus Insert FX Chain (`m_masterFx`)**:
    - Dedicated mastering insert FX chain on the Master Bus (`track == -1`) with up to 32 slots.
    - Prominent `[ FX ]` / `[ FX (N) ]` button on Master channel strip in the Mixer deck.
  - **Floating Centered Modal Close Buttons (`✕`)**:
    - Removed cluttered internal `✓ DONE` buttons.
    - Sleek circular `✕` close button floating above modal windows, centered horizontally.
  - **Tracker Voice Silencing & Audition Cutoff**:
    - Immediate silence of all sounding voices (`allNotesOff()`) upon stopping or pausing playback.
    - Stop-mode preview/audition notes automatically release after a safety window (~350ms) to prevent droning.
  - **Universal Track Renaming**:
    - Direct inline renaming in Mixer channel strip headers, Tracker matrix track headers, `InstrumentEditorModal` title bar, and `TrackFxModal` title bar.
    - Dynamic fallback naming based on instrument type (`SoundFont 1`, `BeatPad 2`, `Plugin 3`, `Synth 4`).
  - **Unified Studio Modal (`InstrumentEditorModal`)**: Refactored to 4 core tabs (`SYNTH`, `SOUNDFONT`, `BEATPAD`, `PLUGIN`) matching the 4 engine sound generator types.
  - **Project Format v3.1 Persistence**: Schema v3.1 serializing unified plugin paths, formats, parameter arrays, per-track Pre-FX chains, Post-FX chains, and Master Bus FX chain.
  - Git checkpoint commit and `phase-3` tag.
    - Stop-mode preview/audition notes automatically release after a safety window (~350ms) to prevent droning.
  - **Universal Track Renaming**:
    - Direct inline renaming in Mixer channel strip headers, Tracker matrix track headers, `InstrumentEditorModal` title bar, and `TrackFxModal` title bar.
    - Removed redundant track name input field from `AuraSynthView` to prevent UI clutter and confusion.
    - Dynamic fallback naming based on instrument type (`SoundFont 1`, `BeatPad 2`, `Plugin 3`, `Synth 4`).
  - **Unified Studio Modal (`InstrumentEditorModal`)**: Refactored to 4 core tabs (`SYNTH`, `SOUNDFONT`, `BEATPAD`, `PLUGIN`) matching the 4 engine sound generator types.
  - **Project Format v3 Persistence**: Schema v3 serializing unified plugin paths, formats, parameter arrays, and per-track insert FX chains.
  - Git checkpoint commit and `phase-3` tag.

- [x] **Phase 4: Non-realtime Audio Export (WAV, MP3, OGG, FLAC) & Final Polish** *(Completed)*
  - **Offline DSP Render Architecture**:
    - Chunked offline rendering via C++ `AudioEngine::exportRenderWav` using `miniaudio.h`'s `ma_encoder` writing 32-bit float stereo WAV at native 48kHz with zero locks and zero heap allocations in inner render loops.
    - Safe device management: temporarily stops the real-time `ma_device` audio callback during offline export to prevent race conditions, then restores state seamlessly.
    - Clean sequencer state preservation: captures and restores song mode, order indices, current pattern, row, and looping configurations.
    - Configurable Reverb/Delay Tail: renders user-selected audio tail duration (0.0s, 1.0s, 2.0s default, 4.0s) allowing reflections and decays to fade naturally into silence.
  - **Multi-Format Container & Codec Pipeline**:
    - **WAV**: Broadcast quality uncompressed PCM supporting 16-bit PCM (`pcm_s16le`), 24-bit studio standard (`pcm_s24le`), and 32-bit float (`pcm_f32le`) at 44.1 kHz, 48.0 kHz, and 96.0 kHz.
    - **MP3**: Universal distribution audio via FFmpeg `libmp3lame` supporting 128 kbps, 192 kbps, 256 kbps, and 320 kbps bitrates at 44.1 kHz and 48.0 kHz.
    - **OGG Vorbis**: High-efficiency Vorbis audio via FFmpeg `libvorbis` supporting variable quality levels Q4 (~128k), Q6 (~192k), Q8 (~256k, default), and Q10 (~500k) at 44.1 kHz and 48.0 kHz.
    - **FLAC**: Free Lossless Audio Codec supporting 16-bit and 24-bit depths (`-sample_fmt s16` / `s32`) with selectable compression levels (Level 5 Fast, Level 8 Maximum) at 44.1 kHz, 48.0 kHz, and 96.0 kHz.
  - **Background Worker & Reactive Progress Architecture**:
    - `AudioExportWorker` running on a separate `QThread` preventing UI stutter or blocking.
    - C++ to Python progress callback pipeline (`PROGRESS_CB`) scaling rendering (0%–85%) and container encoding (85%–100%) with live percentage readouts.
    - Instant cancellation and cleanup: `cancelExport()` immediately signals the C++ DSP chunk loop to stop, closes encoders, and deletes partial temporary files.
  - **KDE Plasma 6 Breeze Audio Export UI (`ExportAudioModal.qml`)**:
    - Modal dialog adhering strictly to KDE dark palette (`#474747`, `#393939`, `#2c2c2c`, `#3daee9`).
    - Floating centered circular close button (`✕`) above the window frame with hover highlight and <kbd>Esc</kbd> shortcut support.
    - Range selector: Song Mode (full arrangement sequence across patterns) vs Current Pattern Mode.
    - Interactive format selector pills adapting options dynamically (bit depth, sample rate, bitrate, quality, compression).
    - Reverb/Delay tail duration selector pills (0.0s, 1.0s, 2.0s, 4.0s).
    - Output file path with direct inline editing and native KDE file dialog via `[ 📁 BROWSE... ]`.
    - Live progress bar, status notifications, and `[ 📁 SHOW IN DOLPHIN / FOLDER ]` integration using native `QDesktopServices`.
    - Main window integration: File Menu `&Export Audio...` (`Ctrl+E`) and top toolbar `[ ⤓ EXPORT ]` trigger button.
  - **Song Structure & Tracker UI Enhancements**:
    - **Removed Obsolete Engine Badges**: Eliminated the legacy `SYN` / `SF2` / `DRUM` / `PLG` indicators from `TrackerView.qml` track headers. With Option C cross-track instrument routing, tracks are universal channel columns, and removing the badges provides 36px more horizontal space for crisp, unclipped track titles.
    - **Song Structure Pattern Rearranging (`SongArrangerBar.qml`)**:
      - Added responsive `◀` and `▶` move buttons to each step card on the song timeline, allowing steps to be moved earlier or later across the song sequence.
      - Implemented C++ DSP sequencer APIs: `tracker_sequencer.h::moveSongOrderStep(fromStep, toStep)` and `swapSongOrderSteps(stepA, stepB)`, keeping active playback step tracking synchronized.
      - Added quick mouse wheel pattern cycling on the `PAT XX ▾` pill to rapidly step through patterns without opening menus.
      - Added right-click context menu on each arranger step card: `◀ Move Left`, `▶ Move Right`, `Duplicate Step`, and `Remove Step (×)`.
    - **Streamlined Menu Actions (`main.py`)**:
      - Replaced the 8 hardcoded "Select Track / Instrument" actions in the **Tracks** menu with a single clean **Remove Selected Track** action (`Ctrl+Shift+T`).
      - Removed redundant "Increase/Decrease Tempo" menu actions from the **Playback** menu (tempo is managed directly via the toolbar BPM widget).
    - **Project-Aware Export Naming & Pattern Looping (`ExportAudioModal.qml`)**:
      - Output filename dynamically defaults to the active project's name (`<ProjectName>.<fmt>`) or `untitled.<fmt>` if unsaved, refreshed freshly on every modal open rather than preserving the previous export's path.
      - Added interactive **Pattern Loop** controls when Pattern Mode is active: toggle `[ ⟳ LOOP PATTERN ]`, stepper `[-] [count] [+]`, and quick preset pills (`1x`, `2x`, `4x`, `8x`).
      - C++ DSP engine loops the pattern seamlessly for the specified repeat count before gracefully stopping the sequencer and capturing natural reverb/delay tail decay.
  - **Comprehensive Verification Suite**:
    - 12 dedicated unit and integration tests in `tests/test_phase4_audio_export.py` validating file integrity, headers, sample rates, channel counts, stream bit depths, song order reordering, pattern looping, and track removal.
    - Full test suite passing across all phases (34/34 unit tests OK).
  - Git checkpoint commit and `phase-4` branch.
