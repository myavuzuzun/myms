#ifndef MYMS_AUDIO_ENGINE_H
#define MYMS_AUDIO_ENGINE_H

#include "synth_engine.h"
#include "soundfont_player.h"
#include "drum_sampler.h"
#include "plugin_host.h"
#include "macro_processor.h"
#include "tracker_sequencer.h"
#include <atomic>
#include <memory>
#include <vector>
#include <string>

struct ma_device;

namespace myms {

constexpr int MAX_INSTRUMENTS = 32;
constexpr int MAX_TRACK_FX = 32;

enum TrackType {
    TRACK_TYPE_SYNTH = 0,
    TRACK_TYPE_SOUNDFONT = 1,
    TRACK_TYPE_DRUMS = 2,
    TRACK_TYPE_PLUGIN = 3
};

struct TrackMixerState {
    int type = TRACK_TYPE_SYNTH;
    int sf2Preset = 0;
    float volume = 1.0f;
    float pan = 0.5f;
    bool mute = false;
    bool solo = false;
    std::atomic<float> peakL{0.0f};
    std::atomic<float> peakR{0.0f};

    TrackMixerState() = default;
    TrackMixerState(const TrackMixerState& o)
        : type(o.type), sf2Preset(o.sf2Preset), volume(o.volume), pan(o.pan), mute(o.mute), solo(o.solo) {
        peakL.store(o.peakL.load(std::memory_order_relaxed));
        peakR.store(o.peakR.load(std::memory_order_relaxed));
    }
    TrackMixerState& operator=(const TrackMixerState& o) {
        if (this != &o) {
            type = o.type;
            sf2Preset = o.sf2Preset;
            volume = o.volume;
            pan = o.pan;
            mute = o.mute;
            solo = o.solo;
            peakL.store(o.peakL.load(std::memory_order_relaxed));
            peakR.store(o.peakR.load(std::memory_order_relaxed));
        }
        return *this;
    }
};

class AudioEngine {
public:
    static AudioEngine& instance();

    bool init(int sampleRate = 48000, int bufferSize = 256);
    void shutdown();

    // Sequencer controls
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setBpm(float bpm);
    float getBpm() const;
    int getCurrentRow() const;
    void setCurrentRow(int row);
    void setLooping(bool loop);
    bool isLooping() const;

    // Pattern length
    int getNumRows() const;
    void setNumRows(int rows);

    // Dynamic Tracks & Instruments
    int getNumTracks() const;
    void addTrack();
    void removeTrack(int trackIdx);
    int getNumInstruments() const;

    // Track Engine Types (0 = Synth, 1 = SoundFont, 2 = Drum Sampler)
    void setTrackType(int track, int type);
    int getTrackType(int track) const;

    // Per-Track Mixer Controls (MuseScore-style Channel Strip)
    void setTrackVolume(int track, float vol);
    float getTrackVolume(int track) const;
    void setTrackPan(int track, float pan);
    float getTrackPan(int track) const;
    void setTrackMute(int track, bool mute);
    bool getTrackMute(int track) const;
    void setTrackSolo(int track, bool solo);
    bool getTrackSolo(int track) const;
    void setTrackSf2Preset(int track, int presetIndex);
    int getTrackSf2Preset(int track) const;
    void getTrackMeters(int track, float* outPeakL, float* outPeakR);

    // Song Order Sequencer
    int getSongOrderLength() const;
    int getSongOrderStep(int step) const;
    void setSongOrderStep(int step, int pat);
    void addSongOrderStep(int pat);
    void removeSongOrderStep(int step);
    int getSongOrderIndex() const;
    void setSongOrderIndex(int step);

    // Custom Instrument Naming
    void setInstrumentName(int inst, const std::string& name);
    std::string getInstrumentName(int inst);

    // Live preview / Note triggers per instrument (inst: 0..MAX_INSTRUMENTS-1)
    void noteOn(int inst, int note, float velocity);
    void noteOff(int inst, int note);
    void allNotesOff();

    // Per-Instrument Synth Parameters
    void setInstrumentOsc1(int inst, int wave);
    void setInstrumentOsc2(int inst, int wave);
    void setInstrumentOscMix(int inst, float mix);
    void setInstrumentDetune(int inst, float semi, float cents);
    void setInstrumentFilter(int inst, float cutoffHz, float resonance, float envAmount);
    void setInstrumentAmpEnv(int inst, float a, float d, float s, float r);
    void setInstrumentFilterEnv(int inst, float a, float d, float s, float r);
    SynthParams getInstrumentParams(int inst);

    // SoundFont Player Controls (FontBox)
    bool loadSoundFont(const std::string& filepath);
    bool isSoundFontLoaded() const;
    int getSoundFontPresetCount();
    std::string getSoundFontPresetName(int index);
    void setSoundFontPreset(int index);
    int getSoundFontPreset() const;
    void setSoundFontVolume(float vol);
    float getSoundFontVolume() const;
    std::string getSoundFontPath();

    // Drum Sampler Controls (BeatPad)
    bool loadDrumSample(int pad, const std::string& filepath);
    void triggerDrumPad(int pad, float velocity);
    void setDrumPadPitch(int pad, float semi);
    float getDrumPadPitch(int pad) const;
    void setDrumPadDecay(int pad, float sec);
    float getDrumPadDecay(int pad) const;
    void setDrumPadVolume(int pad, float vol);
    float getDrumPadVolume(int pad) const;
    void setDrumPadPan(int pad, float pan);
    float getDrumPadPan(int pad) const;
    void setDrumPadReverse(int pad, bool rev);
    bool getDrumPadReverse(int pad) const;
    std::string getDrumPadName(int pad) const;
    std::string getDrumPadFilePath(int pad) const;

    // Plugin Host Controls (VST3, CLAP, VST2)
    bool loadTrackPlugin(int track, const std::string& path);
    void unloadTrackPlugin(int track);
    bool isTrackPluginLoaded(int track) const;
    std::string getTrackPluginName(int track) const;
    std::string getTrackPluginVendor(int track) const;
    std::string getTrackPluginPath(int track) const;
    int getTrackPluginFormat(int track) const;
    std::string getTrackPluginFormatName(int track) const;
    int getTrackPluginNumParams(int track) const;
    std::string getTrackPluginParamName(int track, int paramIdx) const;
    std::string getTrackPluginParamDisplay(int track, int paramIdx) const;
    std::string getTrackPluginParamLabel(int track, int paramIdx) const;
    float getTrackPluginParamValue(int track, int paramIdx) const;
    void setTrackPluginParamValue(int track, int paramIdx, float val);
    bool isTrackPluginParamsDirty(int track);
    bool hasTrackPluginNativeEditor(int track) const;
    bool openTrackPluginNativeEditor(int track, void* parentWindow);
    void closeTrackPluginNativeEditor(int track);

    // Track Insert FX Controls (VST3, CLAP, VST2)
    bool loadTrackFx(int track, int slot, const std::string& path);
    void unloadTrackFx(int track, int slot);
    bool isTrackFxLoaded(int track, int slot) const;
    int getTrackFxCount(int track) const;
    std::string getTrackFxName(int track, int slot) const;
    std::string getTrackFxVendor(int track, int slot) const;
    std::string getTrackFxPath(int track, int slot) const;
    int getTrackFxFormat(int track, int slot) const;
    std::string getTrackFxFormatName(int track, int slot) const;
    int getTrackFxNumParams(int track, int slot) const;
    std::string getTrackFxParamName(int track, int slot, int paramIdx) const;
    std::string getTrackFxParamDisplay(int track, int slot, int paramIdx) const;
    float getTrackFxParamValue(int track, int slot, int paramIdx) const;
    void setTrackFxParamValue(int track, int slot, int paramIdx, float val);
    bool isTrackFxParamsDirty(int track, int slot);
    bool hasTrackFxEditor(int track, int slot) const;
    bool openTrackFxEditor(int track, int slot, void* parentWindow);
    void closeTrackFxEditor(int track, int slot);

    // Track Pre-Instrument FX Controls (Plugins Before Instrument)
    bool loadTrackPreFx(int track, int slot, const std::string& path);
    void unloadTrackPreFx(int track, int slot);
    bool isTrackPreFxLoaded(int track, int slot) const;
    int getTrackPreFxCount(int track) const;
    std::string getTrackPreFxName(int track, int slot) const;
    std::string getTrackPreFxVendor(int track, int slot) const;
    std::string getTrackPreFxPath(int track, int slot) const;
    int getTrackPreFxFormat(int track, int slot) const;
    std::string getTrackPreFxFormatName(int track, int slot) const;
    int getTrackPreFxNumParams(int track, int slot) const;
    std::string getTrackPreFxParamName(int track, int slot, int paramIdx) const;
    std::string getTrackPreFxParamDisplay(int track, int slot, int paramIdx) const;
    float getTrackPreFxParamValue(int track, int slot, int paramIdx) const;
    void setTrackPreFxParamValue(int track, int slot, int paramIdx, float val);
    bool isTrackPreFxParamsDirty(int track, int slot);
    bool hasTrackPreFxEditor(int track, int slot) const;
    bool openTrackPreFxEditor(int track, int slot, void* parentWindow);
    void closeTrackPreFxEditor(int track, int slot);

    // Pattern & Song Management
    int getNumPatterns() const;
    int getCurrentPattern() const;
    void setCurrentPattern(int pat);
    int addPattern();
    int duplicatePattern(int sourcePat);
    bool removePattern(int pat);
    void setSongMode(bool songMode);
    bool isSongMode() const;
    void setPatternCell(int pat, int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam);
    TrackerCell getPatternCell(int pat, int track, int row) const;

    // Smart Macro Controls
    void setMacroPunch(float val);
    void setMacroWarmth(float val);
    void setMacroTone(float val);
    void setMacroSpace(float val);
    void setMasterVolume(float val);

    // Metering & Visualization (Real-time read from UI)
    float getMasterPeakL() const;
    float getMasterPeakR() const;
    float getMasterRms() const;
    float getCpuUsage() const;

    // Tracker Grid Data Access
    void setTrackerCell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam);
    TrackerCell getTrackerCell(int track, int row);
    void clearTrack(int track);
    void clearAllTracks();
    void transposeTrack(int track, int semitones);
    void loadDemoGroove();

    // Real-time audio render callback (invoked by miniaudio)
    void renderAudio(float* output, int frameCount, int channels);

private:
    AudioEngine();
    ~AudioEngine();

    void setupInitialInstruments();

    float m_sampleRate = 48000.0f;
    std::unique_ptr<ma_device> m_device;
    bool m_initialized = false;

    std::vector<SynthEngine> m_instruments;
    std::vector<std::string> m_instrumentNames;
    std::vector<int> m_trackTypes;
    std::vector<TrackMixerState> m_trackMixers;

    SoundFontPlayer m_soundfont;
    DrumSampler m_drumSampler;
    std::vector<std::unique_ptr<PluginInstance>> m_plugins;
    std::vector<std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX>> m_trackFx;
    std::vector<std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX>> m_trackPreFx;

    std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX> m_masterFx;
    std::atomic<int> m_masterFxMaxSlot{0};

    MacroProcessor m_macros;
    TrackerSequencer m_sequencer;

    std::atomic<float> m_peakL{0.0f};
    std::atomic<float> m_peakR{0.0f};
    std::atomic<float> m_rms{0.0f};
    std::atomic<float> m_cpuUsage{0.0f};
    std::atomic<int> m_uiCurrentRow{0};
    std::atomic<int> m_uiCurrentPattern{0};
    std::atomic<int> m_uiSongOrderIndex{0};

    bool m_wasPlaying = false;
    std::array<int, MAX_INSTRUMENTS> m_previewCountdowns{};
    std::array<std::atomic<int>, MAX_INSTRUMENTS> m_trackFxMaxSlot{};
    std::array<std::atomic<int>, MAX_INSTRUMENTS> m_trackPreFxMaxSlot{};
};

} // namespace myms

#endif // MYMS_AUDIO_ENGINE_H
