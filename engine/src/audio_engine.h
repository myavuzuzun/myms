#ifndef MYMS_AUDIO_ENGINE_H
#define MYMS_AUDIO_ENGINE_H

#include "synth_engine.h"
#include "soundfont_player.h"
#include "drum_sampler.h"
#include "macro_processor.h"
#include "tracker_sequencer.h"
#include <atomic>
#include <memory>
#include <vector>
#include <string>

struct ma_device;

namespace myms {

constexpr int MAX_INSTRUMENTS = 32;

enum TrackType {
    TRACK_TYPE_SYNTH = 0,
    TRACK_TYPE_SOUNDFONT = 1,
    TRACK_TYPE_DRUMS = 2
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

    SoundFontPlayer m_soundfont;
    DrumSampler m_drumSampler;

    MacroProcessor m_macros;
    TrackerSequencer m_sequencer;

    std::atomic<float> m_peakL{0.0f};
    std::atomic<float> m_peakR{0.0f};
    std::atomic<float> m_rms{0.0f};
    std::atomic<int> m_uiCurrentRow{0};
    std::atomic<int> m_uiCurrentPattern{0};
};

} // namespace myms

#endif // MYMS_AUDIO_ENGINE_H
