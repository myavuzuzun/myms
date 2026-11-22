# myms

A lightweight, real-time Linux tracker and music creation studio built with a zero-allocation C++20 audio engine (`miniaudio`) and a PyQt6 / Qt Quick UI.

## Features
- **Tracker Sequencer**: Keyboard-driven note, instrument, volume, and per-cell effect editing (`0x01`/`0x02` pitch slide, `0x0C` note cut/retrigger, `0x0E` delay send, `0x10` filter cutoff), rectangular chunk selection (`Ctrl+C`/`X`/`V`/`A`), and arbitrary per-pattern row counts.
- **Song Arranger**: Pattern/Song playback mode switch, drag-and-drop step reordering, step insertion/duplication, and auto-scrolling playhead tracking.
- **Built-in Instruments**:
  - **Synth**: Polyphonic subtractive synthesizer with dual oscillators, resonant filter, and ADSR envelopes.
  - **FontBox**: SoundFont2 (`.sf2`) GM player.
  - **BeatPad**: 8-slot drum sampler with per-pad pitch, decay, pan, volume, and reverse.
- **Plugin Hosting (VST3 & CLAP)**: Load 64-bit Linux VST3 (`.vst3`) and CLAP (`.clap`) instruments, Pre-FX, Post-FX, and Master Bus FX with native X11 editor window hosting.
- **Undo / Redo**: Full-project undo/redo history (`Ctrl+Z` / `Ctrl+Shift+Z` / `Ctrl+Y`) with smart coalescing for continuous knob/slider drags.
- **Audio Export**: Offline rendering to **WAV** (built-in) and **FLAC / OGG / MP3** (via FFmpeg).

## Dependencies

### Required
- Python 3 + **PyQt6** (with Qt Quick / QML modules)
- `g++` (only needed on first run if `libmyms_audio.so` is not pre-built)

### Optional (Recommended)
- **FFmpeg** (`ffmpeg`) — required for FLAC, OGG, and MP3 audio export (WAV export works without FFmpeg).
- **General MIDI SoundFont** (`fluid-soundfont-gm` / `timgm6mb-soundfont`) — auto-detected from standard `/usr/share/sounds/sf2/` and `/usr/share/soundfonts/` locations.

### Installing Dependencies
- **Ubuntu / Debian**:
  ```bash
  sudo apt install python3-pyqt6 python3-pyqt6.qtquick g++ ffmpeg fluid-soundfont-gm
  ```
- **Arch Linux**:
  ```bash
  sudo pacman -S python-pyqt6 qt6-declarative gcc ffmpeg soundfont-fluid
  ```
- **Fedora**:
  ```bash
  sudo dnf install python3-qt6 qt6-qtdeclarative gcc-c++ ffmpeg fluid-soundfont-gm
  ```

## Running
```bash
./run.sh
```

## Desktop Entry
`myms.desktop` uses `Exec=myms`. If installing system-wide or to `~/.local/bin`, symlink or install `run.sh` (or a wrapper script) as `myms` in your `PATH`, and copy `myms.desktop` to `~/.local/share/applications/`.

