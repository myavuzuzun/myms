#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "audio_engine.h"
#include <iostream>
#include <cstring>
#include <cmath>

namespace myms {

static void ma_audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    if (!pDevice || !pDevice->pUserData || !pOutput) return;

    auto* engine = static_cast<AudioEngine*>(pDevice->pUserData);
    engine->renderAudio(static_cast<float*>(pOutput), frameCount, pDevice->playback.channels);
}

AudioEngine& AudioEngine::instance() {
    static AudioEngine s_instance;
    return s_instance;
}

AudioEngine::AudioEngine() {
}

AudioEngine::~AudioEngine() {
    shutdown();
}

void AudioEngine::setupInitialInstruments() {
    m_instrumentNames.resize(MAX_INSTRUMENTS);
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        m_instrumentNames[i] = "Synth " + std::to_string(i + 1);
    }

    // Default versatile starting synth patch
    SynthParams initPatch;
    initPatch.osc1Wave = WAVE_SAW;
    initPatch.osc2Wave = WAVE_SQUARE;
    initPatch.osc2DetuneSemi = 0.0f;
    initPatch.osc2FineCents = 8.0f;
    initPatch.oscMix = 0.45f;
    initPatch.filterCutoff = 3500.0f;
    initPatch.filterResonance = 0.35f;
    initPatch.filterEnvAmount = 0.40f;
    initPatch.ampAttack = 0.005f;
    initPatch.ampDecay = 0.25f;
    initPatch.ampSustain = 0.65f;
    initPatch.ampRelease = 0.25f;

    for (size_t i = 0; i < m_instruments.size(); ++i) {
        m_instruments[i].setParams(initPatch);
    }
}

void AudioEngine::setInstrumentName(int inst, const std::string& name) {
    if (inst >= 0 && inst < static_cast<int>(m_instrumentNames.size())) {
        m_instrumentNames[inst] = name;
    }
}

std::string AudioEngine::getInstrumentName(int inst) {
    if (inst >= 0 && inst < static_cast<int>(m_instrumentNames.size())) {
        return m_instrumentNames[inst];
    }
    return "Synth";
}

void AudioEngine::loadDemoGroove() {
    clearAllTracks();

    // Ensure we have at least 4 tracks for demo
    while (getNumTracks() < 4) {
        addTrack();
    }

    m_instrumentNames[0] = "Lead";
    m_instrumentNames[1] = "Bass";
    m_instrumentNames[2] = "Drums";
    m_instrumentNames[3] = "SoundFont";

    setTrackType(0, TRACK_TYPE_SYNTH);
    setTrackType(1, TRACK_TYPE_SYNTH);
    setTrackType(2, TRACK_TYPE_DRUMS);
    setTrackType(3, TRACK_TYPE_SOUNDFONT);

    // Track 1: Synth Lead
    m_sequencer.setCell(0, 0, 60, 1, 255, 0, 0);
    m_sequencer.setCell(0, 4, 63, 1, 220, 0, 0);
    m_sequencer.setCell(0, 8, 67, 1, 255, 0x10, 0xBB);
    m_sequencer.setCell(0, 12, 70, 1, 200, 0, 0);
    m_sequencer.setCell(0, 14, 72, 1, 240, 0x10, 0xFF);
    m_sequencer.setCell(0, 15, NOTE_OFF, 1, 255, 0, 0);
    m_sequencer.setCell(0, 16, 60, 1, 255, 0, 0);
    m_sequencer.setCell(0, 20, 63, 1, 220, 0, 0);
    m_sequencer.setCell(0, 24, 65, 1, 255, 0, 0);
    m_sequencer.setCell(0, 28, 67, 1, 230, 0, 0);
    m_sequencer.setCell(0, 31, NOTE_OFF, 1, 255, 0, 0);

    // Track 2: Sub Bass
    m_sequencer.setCell(1, 0, 36, 2, 255, 0, 0);
    m_sequencer.setCell(1, 2, 36, 2, 180, 0, 0);
    m_sequencer.setCell(1, 6, 36, 2, 200, 0, 0);
    m_sequencer.setCell(1, 8, 39, 2, 255, 0, 0);
    m_sequencer.setCell(1, 10, 39, 2, 180, 0, 0);
    m_sequencer.setCell(1, 14, 41, 2, 220, 0, 0);
    m_sequencer.setCell(1, 16, 36, 2, 255, 0, 0);
    m_sequencer.setCell(1, 18, 36, 2, 180, 0, 0);
    m_sequencer.setCell(1, 22, 36, 2, 200, 0, 0);
    m_sequencer.setCell(1, 24, 44, 2, 255, 0, 0);
    m_sequencer.setCell(1, 28, 43, 2, 220, 0, 0);

    // Track 3: BeatPad Drums (Kick on 0, 8, 16, 24; Snare on 4, 12, 20, 28; Hats on evens)
    for (int r = 0; r < 32; r += 2) {
        m_sequencer.setCell(2, r, 39, 3, 180, 0, 0); // Hat (pad 3)
    }
    m_sequencer.setCell(2, 0, 36, 3, 255, 0, 0); // Kick
    m_sequencer.setCell(2, 4, 37, 3, 230, 0, 0); // Snare
    m_sequencer.setCell(2, 8, 36, 3, 255, 0, 0); // Kick
    m_sequencer.setCell(2, 12, 37, 3, 230, 0, 0); // Snare
    m_sequencer.setCell(2, 14, 36, 3, 190, 0, 0); // Ghost Kick
    m_sequencer.setCell(2, 16, 36, 3, 255, 0, 0); // Kick
    m_sequencer.setCell(2, 20, 37, 3, 230, 0, 0); // Snare
    m_sequencer.setCell(2, 24, 36, 3, 255, 0, 0); // Kick
    m_sequencer.setCell(2, 28, 37, 3, 230, 0, 0); // Snare

    // Track 4: SoundFont Chords / Arp
    m_sequencer.setCell(3, 0, 48, 4, 200, 0, 0);
    m_sequencer.setCell(3, 8, 51, 4, 200, 0, 0);
    m_sequencer.setCell(3, 16, 48, 4, 200, 0, 0);
    m_sequencer.setCell(3, 24, 53, 4, 200, 0, 0);
}

bool AudioEngine::init(int sampleRate, int bufferSize) {
    if (m_initialized) return true;

    m_sampleRate = static_cast<float>(sampleRate);
    m_sequencer.init(m_sampleRate);
    m_macros.init(m_sampleRate);
    m_soundfont.init(m_sampleRate);
    m_drumSampler.init(m_sampleRate);

    m_instruments.clear();
    m_instruments.emplace_back();
    m_instruments.back().init(m_sampleRate);
    m_trackTypes.clear();
    m_trackTypes.push_back(TRACK_TYPE_SYNTH);
    m_trackMixers.clear();
    m_trackMixers.emplace_back();
    m_plugins.clear();
    m_plugins.emplace_back(std::make_unique<PluginInstance>());

    m_trackFx.clear();
    std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX> initFx;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        initFx[s] = std::make_unique<PluginInstance>();
    }
    m_trackFx.emplace_back(std::move(initFx));

    setupInitialInstruments();

    m_device = std::make_unique<ma_device>();
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = sampleRate;
    config.dataCallback      = ma_audio_callback;
    config.pUserData         = this;
    config.periodSizeInFrames = bufferSize;

    if (ma_device_init(nullptr, &config, m_device.get()) != MA_SUCCESS) {
        std::cerr << "[myms] Failed to initialize miniaudio device!" << std::endl;
        m_device.reset();
        return false;
    }

    if (ma_device_start(m_device.get()) != MA_SUCCESS) {
        std::cerr << "[myms] Failed to start miniaudio playback!" << std::endl;
        ma_device_uninit(m_device.get());
        m_device.reset();
        return false;
    }

    m_initialized = true;
    std::cout << "[myms] Audio engine initialized at " << sampleRate << "Hz, " 
              << bufferSize << " frames buffer." << std::endl;
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    if (m_device) {
        ma_device_stop(m_device.get());
        ma_device_uninit(m_device.get());
        m_device.reset();
    }
    m_initialized = false;
    std::cout << "[myms] Audio engine shut down cleanly." << std::endl;
}

void AudioEngine::play() {
    m_sequencer.play();
}

void AudioEngine::pause() {
    m_sequencer.pause();
    for (size_t i = 0; i < m_previewCountdowns.size(); ++i) {
        m_previewCountdowns[i] = 0;
    }
    allNotesOff();
    m_uiCurrentRow.store(m_sequencer.getCurrentRow(), std::memory_order_relaxed);
}

void AudioEngine::stop() {
    m_sequencer.stop();
    for (size_t i = 0; i < m_previewCountdowns.size(); ++i) {
        m_previewCountdowns[i] = 0;
    }
    allNotesOff();
    m_uiCurrentRow.store(0, std::memory_order_relaxed);
    m_uiCurrentPattern.store(m_sequencer.getCurrentPattern(), std::memory_order_relaxed);
    m_uiSongOrderIndex.store(m_sequencer.getSongOrderIndex(), std::memory_order_relaxed);
}

bool AudioEngine::isPlaying() const {
    return m_sequencer.isPlaying();
}

void AudioEngine::setBpm(float bpm) {
    m_sequencer.setBpm(bpm);
}

float AudioEngine::getBpm() const {
    return m_sequencer.getBpm();
}

int AudioEngine::getCurrentRow() const {
    return m_uiCurrentRow.load(std::memory_order_relaxed);
}

void AudioEngine::setCurrentRow(int row) {
    m_sequencer.setCurrentRow(row);
    m_uiCurrentRow.store(row, std::memory_order_relaxed);
}

void AudioEngine::setLooping(bool loop) {
    m_sequencer.setLooping(loop);
}

bool AudioEngine::isLooping() const {
    return m_sequencer.isLooping();
}

int AudioEngine::getNumRows() const {
    return m_sequencer.getNumRows();
}

void AudioEngine::setNumRows(int rows) {
    m_sequencer.setNumRows(rows);
}

int AudioEngine::getNumTracks() const {
    return m_sequencer.getNumTracks();
}

void AudioEngine::addTrack() {
    m_instruments.emplace_back();
    m_instruments.back().init(m_sampleRate);
    m_trackTypes.push_back(TRACK_TYPE_SYNTH);
    m_trackMixers.emplace_back();
    m_plugins.emplace_back(std::make_unique<PluginInstance>());

    std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX> fxSlots;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        fxSlots[s] = std::make_unique<PluginInstance>();
    }
    m_trackFx.emplace_back(std::move(fxSlots));

    // Initialize with default synth patch
    SynthParams p = m_instruments[0].getParams();
    m_instruments.back().setParams(p);

    m_sequencer.addTrack();
}

void AudioEngine::removeTrack(int trackIdx) {
    if (m_instruments.size() <= 1 || trackIdx < 0 || trackIdx >= static_cast<int>(m_instruments.size())) return;
    m_instruments.erase(m_instruments.begin() + trackIdx);
    if (trackIdx < static_cast<int>(m_trackTypes.size())) {
        m_trackTypes.erase(m_trackTypes.begin() + trackIdx);
    }
    if (trackIdx < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers.erase(m_trackMixers.begin() + trackIdx);
    }
    if (trackIdx < static_cast<int>(m_plugins.size())) {
        m_plugins.erase(m_plugins.begin() + trackIdx);
    }
    if (trackIdx < static_cast<int>(m_trackFx.size())) {
        m_trackFx.erase(m_trackFx.begin() + trackIdx);
    }
    m_sequencer.removeTrack(trackIdx);
}

int AudioEngine::getNumInstruments() const {
    return static_cast<int>(m_instruments.size());
}

void AudioEngine::setTrackType(int track, int type) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].type = std::clamp(type, 0, 3);
    }
    if (track >= 0 && track < static_cast<int>(m_trackTypes.size())) {
        m_trackTypes[track] = std::clamp(type, 0, 3);
    }
}

int AudioEngine::getTrackType(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].type;
    }
    if (track >= 0 && track < static_cast<int>(m_trackTypes.size())) {
        return m_trackTypes[track];
    }
    return TRACK_TYPE_SYNTH;
}

// Track Mixer Controls
void AudioEngine::setTrackVolume(int track, float vol) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].volume = std::clamp(vol, 0.0f, 2.0f);
    }
}

float AudioEngine::getTrackVolume(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].volume;
    }
    return 1.0f;
}

void AudioEngine::setTrackPan(int track, float pan) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].pan = std::clamp(pan, 0.0f, 1.0f);
    }
}

float AudioEngine::getTrackPan(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].pan;
    }
    return 0.5f;
}

void AudioEngine::setTrackMute(int track, bool mute) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].mute = mute;
    }
}

bool AudioEngine::getTrackMute(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].mute;
    }
    return false;
}

void AudioEngine::setTrackSolo(int track, bool solo) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].solo = solo;
    }
}

bool AudioEngine::getTrackSolo(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].solo;
    }
    return false;
}

void AudioEngine::setTrackSf2Preset(int track, int presetIndex) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        m_trackMixers[track].sf2Preset = std::max(0, presetIndex);
        m_soundfont.setChannelPreset(track, m_trackMixers[track].sf2Preset);
    }
}

int AudioEngine::getTrackSf2Preset(int track) const {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        return m_trackMixers[track].sf2Preset;
    }
    return 0;
}

void AudioEngine::getTrackMeters(int track, float* outPeakL, float* outPeakR) {
    if (track >= 0 && track < static_cast<int>(m_trackMixers.size())) {
        if (outPeakL) *outPeakL = m_trackMixers[track].peakL.load(std::memory_order_relaxed);
        if (outPeakR) *outPeakR = m_trackMixers[track].peakR.load(std::memory_order_relaxed);
    } else {
        if (outPeakL) *outPeakL = 0.0f;
        if (outPeakR) *outPeakR = 0.0f;
    }
}

// Song Order Sequencer Methods
int AudioEngine::getSongOrderLength() const {
    return m_sequencer.getSongOrderLength();
}

int AudioEngine::getSongOrderStep(int step) const {
    return m_sequencer.getSongOrderStep(step);
}

void AudioEngine::setSongOrderStep(int step, int pat) {
    m_sequencer.setSongOrderStep(step, pat);
}

void AudioEngine::addSongOrderStep(int pat) {
    m_sequencer.addSongOrderStep(pat);
}

void AudioEngine::removeSongOrderStep(int step) {
    m_sequencer.removeSongOrderStep(step);
}

int AudioEngine::getSongOrderIndex() const {
    if (m_sequencer.isPlaying()) {
        return m_uiSongOrderIndex.load(std::memory_order_relaxed);
    }
    return m_sequencer.getSongOrderIndex();
}

void AudioEngine::setSongOrderIndex(int step) {
    m_sequencer.setSongOrderIndex(step);
    m_uiSongOrderIndex.store(step, std::memory_order_relaxed);
    m_uiCurrentPattern.store(m_sequencer.getCurrentPattern(), std::memory_order_relaxed);
}

void AudioEngine::noteOn(int inst, int note, float velocity) {
    if (!m_sequencer.isPlaying() && inst >= 0 && inst < MAX_INSTRUMENTS) {
        // Auto-release preview/audition notes after ~350ms so notes never drone while stopped
        m_previewCountdowns[inst] = static_cast<int>(0.35f * m_sampleRate);
    }

    int type = getTrackType(inst);
    if (type == TRACK_TYPE_SOUNDFONT) {
        int preset = getTrackSf2Preset(inst);
        m_soundfont.setChannelPreset(inst, preset);
        m_soundfont.noteOn(inst, note, velocity);
    } else if (type == TRACK_TYPE_DRUMS) {
        int pad = (note >= 36 && note <= 43) ? (note - 36) : (note % NUM_DRUM_PADS);
        m_drumSampler.triggerPad(pad, velocity);
    } else if (type == TRACK_TYPE_PLUGIN) {
        if (inst >= 0 && inst < static_cast<int>(m_plugins.size()) && m_plugins[inst]) {
            m_plugins[inst]->noteOn(note, velocity);
        }
    } else {
        if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
            m_instruments[inst].noteOn(note, velocity);
        }
    }
}

void AudioEngine::noteOff(int inst, int note) {
    if (inst >= 0 && inst < MAX_INSTRUMENTS) {
        m_previewCountdowns[inst] = 0;
    }

    int type = getTrackType(inst);
    if (type == TRACK_TYPE_SOUNDFONT) {
        m_soundfont.noteOff(inst, note);
    } else if (type == TRACK_TYPE_DRUMS) {
        // One-shot drum hits decay naturally
    } else if (type == TRACK_TYPE_PLUGIN) {
        if (inst >= 0 && inst < static_cast<int>(m_plugins.size()) && m_plugins[inst]) {
            m_plugins[inst]->noteOff(note);
        }
    } else {
        if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
            m_instruments[inst].noteOff(note);
        }
    }
}

void AudioEngine::allNotesOff() {
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        m_previewCountdowns[i] = 0;
    }
    for (size_t i = 0; i < m_instruments.size(); ++i) {
        m_instruments[i].allNotesOff();
    }
    m_soundfont.allNotesOff();
    m_drumSampler.allNotesOff();
    for (size_t i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i]) m_plugins[i]->allNotesOff();
    }
    m_macros.resetBuffers();
}

void AudioEngine::setInstrumentOsc1(int inst, int wave) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().osc1Wave = static_cast<Waveform>(wave);
    }
}

void AudioEngine::setInstrumentOsc2(int inst, int wave) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().osc2Wave = static_cast<Waveform>(wave);
    }
}

void AudioEngine::setInstrumentOscMix(int inst, float mix) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().oscMix = std::clamp(mix, 0.0f, 1.0f);
    }
}

void AudioEngine::setInstrumentDetune(int inst, float semi, float cents) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().osc2DetuneSemi = semi;
        m_instruments[inst].getParams().osc2FineCents = cents;
    }
}

void AudioEngine::setInstrumentFilter(int inst, float cutoffHz, float resonance, float envAmount) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().filterCutoff = std::clamp(cutoffHz, 20.0f, 20000.0f);
        m_instruments[inst].getParams().filterResonance = std::clamp(resonance, 0.0f, 0.98f);
        m_instruments[inst].getParams().filterEnvAmount = std::clamp(envAmount, 0.0f, 1.0f);
    }
}

void AudioEngine::setInstrumentAmpEnv(int inst, float a, float d, float s, float r) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().ampAttack = a;
        m_instruments[inst].getParams().ampDecay = d;
        m_instruments[inst].getParams().ampSustain = s;
        m_instruments[inst].getParams().ampRelease = r;
    }
}

void AudioEngine::setInstrumentFilterEnv(int inst, float a, float d, float s, float r) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].getParams().fltAttack = a;
        m_instruments[inst].getParams().fltDecay = d;
        m_instruments[inst].getParams().fltSustain = s;
        m_instruments[inst].getParams().fltRelease = r;
    }
}

SynthParams AudioEngine::getInstrumentParams(int inst) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        return m_instruments[inst].getParams();
    }
    return SynthParams();
}

// SoundFont Player Controls (FontBox)
bool AudioEngine::loadSoundFont(const std::string& filepath) {
    return m_soundfont.loadSoundFont(filepath);
}

bool AudioEngine::isSoundFontLoaded() const {
    return m_soundfont.isLoaded();
}

int AudioEngine::getSoundFontPresetCount() {
    return m_soundfont.getPresetCount();
}

std::string AudioEngine::getSoundFontPresetName(int index) {
    return m_soundfont.getPresetName(index);
}

void AudioEngine::setSoundFontPreset(int index) {
    m_soundfont.setPreset(index);
}

int AudioEngine::getSoundFontPreset() const {
    return m_soundfont.getPreset();
}

void AudioEngine::setSoundFontVolume(float vol) {
    m_soundfont.setVolume(vol);
}

float AudioEngine::getSoundFontVolume() const {
    return m_soundfont.getVolume();
}

std::string AudioEngine::getSoundFontPath() {
    return m_soundfont.getLoadedPath();
}

// Drum Sampler Controls (BeatPad)
bool AudioEngine::loadDrumSample(int pad, const std::string& filepath) {
    return m_drumSampler.loadSample(pad, filepath);
}

void AudioEngine::triggerDrumPad(int pad, float velocity) {
    m_drumSampler.triggerPad(pad, velocity);
}

void AudioEngine::setDrumPadPitch(int pad, float semi) {
    m_drumSampler.setPadPitch(pad, semi);
}

float AudioEngine::getDrumPadPitch(int pad) const {
    return m_drumSampler.getPadPitch(pad);
}

void AudioEngine::setDrumPadDecay(int pad, float sec) {
    m_drumSampler.setPadDecay(pad, sec);
}

float AudioEngine::getDrumPadDecay(int pad) const {
    return m_drumSampler.getPadDecay(pad);
}

void AudioEngine::setDrumPadVolume(int pad, float vol) {
    m_drumSampler.setPadVolume(pad, vol);
}

float AudioEngine::getDrumPadVolume(int pad) const {
    return m_drumSampler.getPadVolume(pad);
}

void AudioEngine::setDrumPadPan(int pad, float pan) {
    m_drumSampler.setPadPan(pad, pan);
}

float AudioEngine::getDrumPadPan(int pad) const {
    return m_drumSampler.getPadPan(pad);
}

void AudioEngine::setDrumPadReverse(int pad, bool rev) {
    m_drumSampler.setPadReverse(pad, rev);
}

bool AudioEngine::getDrumPadReverse(int pad) const {
    return m_drumSampler.getPadReverse(pad);
}

std::string AudioEngine::getDrumPadName(int pad) const {
    return m_drumSampler.getPadName(pad);
}

std::string AudioEngine::getDrumPadFilePath(int pad) const {
    return m_drumSampler.getPadFilePath(pad);
}

// Pattern & Song Management
int AudioEngine::getNumPatterns() const {
    return m_sequencer.getNumPatterns();
}

int AudioEngine::getCurrentPattern() const {
    if (m_sequencer.isPlaying()) {
        return m_uiCurrentPattern.load(std::memory_order_relaxed);
    }
    return m_sequencer.getCurrentPattern();
}

void AudioEngine::setCurrentPattern(int pat) {
    m_sequencer.setCurrentPattern(pat);
    m_uiCurrentPattern.store(m_sequencer.getCurrentPattern(), std::memory_order_relaxed);
}

int AudioEngine::addPattern() {
    int idx = m_sequencer.addPattern();
    m_uiCurrentPattern.store(idx, std::memory_order_relaxed);
    return idx;
}

int AudioEngine::duplicatePattern(int sourcePat) {
    int idx = m_sequencer.duplicatePattern(sourcePat);
    m_uiCurrentPattern.store(idx, std::memory_order_relaxed);
    return idx;
}

bool AudioEngine::removePattern(int pat) {
    bool ok = m_sequencer.removePattern(pat);
    m_uiCurrentPattern.store(m_sequencer.getCurrentPattern(), std::memory_order_relaxed);
    return ok;
}

void AudioEngine::setSongMode(bool songMode) {
    m_sequencer.setSongMode(songMode);
}

bool AudioEngine::isSongMode() const {
    return m_sequencer.isSongMode();
}

void AudioEngine::setPatternCell(int pat, int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam) {
    m_sequencer.setPatternCell(pat, track, row, note, instrument, volume, effectType, effectParam);
}

TrackerCell AudioEngine::getPatternCell(int pat, int track, int row) const {
    return m_sequencer.getPatternCell(pat, track, row);
}

void AudioEngine::setMacroPunch(float val) {
    m_macros.getControls().punch = val;
}

void AudioEngine::setMacroWarmth(float val) {
    m_macros.getControls().warmth = val;
}

void AudioEngine::setMacroTone(float val) {
    m_macros.getControls().tone = val;
}

void AudioEngine::setMacroSpace(float val) {
    m_macros.getControls().space = val;
}

void AudioEngine::setMasterVolume(float val) {
    m_macros.getControls().volume = val;
}

float AudioEngine::getMasterPeakL() const {
    return m_peakL.load(std::memory_order_relaxed);
}

float AudioEngine::getMasterPeakR() const {
    return m_peakR.load(std::memory_order_relaxed);
}

float AudioEngine::getMasterRms() const {
    return m_rms.load(std::memory_order_relaxed);
}

void AudioEngine::setTrackerCell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam) {
    m_sequencer.setCell(track, row, note, instrument, volume, effectType, effectParam);
}

TrackerCell AudioEngine::getTrackerCell(int track, int row) {
    return m_sequencer.getCell(track, row);
}

void AudioEngine::clearTrack(int track) {
    int nRows = m_sequencer.getNumRows();
    for (int r = 0; r < nRows; ++r) {
        m_sequencer.setCell(track, r, NO_NOTE, 0, 255, 0, 0);
    }
}

void AudioEngine::clearAllTracks() {
    int nTracks = m_sequencer.getNumTracks();
    for (int t = 0; t < nTracks; ++t) {
        clearTrack(t);
    }
}

void AudioEngine::transposeTrack(int track, int semitones) {
    int nRows = m_sequencer.getNumRows();
    for (int r = 0; r < nRows; ++r) {
        TrackerCell c = m_sequencer.getCell(track, r);
        if (c.note != NO_NOTE && c.note != NOTE_OFF) {
            int newNote = std::clamp(static_cast<int>(c.note) + semitones, 0, 127);
            m_sequencer.setCell(track, r, static_cast<uint8_t>(newNote), c.instrument, c.volume, c.effectType, c.effectParam);
        }
    }
}

void AudioEngine::renderAudio(float* output, int frameCount, int channels) {
    std::memset(output, 0, frameCount * channels * sizeof(float));

    float peakL = 0.0f;
    float peakR = 0.0f;
    float sumSq = 0.0f;

    int numTracks = m_sequencer.getNumTracks();
    float trkPeakL[MAX_INSTRUMENTS] = {0.0f};
    float trkPeakR[MAX_INSTRUMENTS] = {0.0f};

    // Determine solo state
    bool anySolo = false;
    for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
        if (m_trackMixers[t].solo) {
            anySolo = true;
            break;
        }
    }

    // Playback state transition & preview note auto-cutoff when playback is stopped
    bool isPlayingNow = m_sequencer.isPlaying();
    if (m_wasPlaying && !isPlayingNow) {
        allNotesOff();
    }
    m_wasPlaying = isPlayingNow;

    if (!isPlayingNow) {
        for (int t = 0; t < numTracks && t < MAX_INSTRUMENTS; ++t) {
            if (m_previewCountdowns[t] > 0) {
                m_previewCountdowns[t] -= frameCount;
                if (m_previewCountdowns[t] <= 0) {
                    m_previewCountdowns[t] = 0;
                    noteOff(t, -1);
                }
            }
        }

        bool anyPreview = false;
        for (int t = 0; t < numTracks && t < MAX_INSTRUMENTS; ++t) {
            if (m_previewCountdowns[t] > 0) {
                anyPreview = true;
                break;
            }
        }
        if (!anyPreview) {
            std::memset(output, 0, frameCount * channels * sizeof(float));
            m_peakL.store(0.0f, std::memory_order_relaxed);
            m_peakR.store(0.0f, std::memory_order_relaxed);
            m_rms.store(0.0f, std::memory_order_relaxed);
            for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
                m_trackMixers[t].peakL.store(0.0f, std::memory_order_relaxed);
                m_trackMixers[t].peakR.store(0.0f, std::memory_order_relaxed);
            }
            return;
        }
    }

    // Configure SoundFont channel volumes and pans
    for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
        if (m_trackMixers[t].type == TRACK_TYPE_SOUNDFONT) {
            bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
            float effVol = audible ? m_trackMixers[t].volume : 0.0f;
            m_soundfont.setChannelVolume(t, effVol);
            m_soundfont.setChannelPan(t, m_trackMixers[t].pan);
        }
    }

    for (int f = 0; f < frameCount; ++f) {
        int triggeredRow = -1;
        int triggeredPat = -1;
        int triggeredSongOrder = -1;
        if (m_sequencer.advanceSample(triggeredRow, triggeredPat, triggeredSongOrder)) {
            m_uiCurrentRow.store(triggeredRow, std::memory_order_relaxed);
            m_uiCurrentPattern.store(triggeredPat, std::memory_order_relaxed);
            m_uiSongOrderIndex.store(triggeredSongOrder, std::memory_order_relaxed);

            for (int t = 0; t < numTracks; ++t) {
                TrackerCell cell = m_sequencer.getPatternCell(triggeredPat, t, triggeredRow);
                int type = getTrackType(t);

                if (cell.note == NOTE_OFF) {
                    if (type == TRACK_TYPE_SOUNDFONT) {
                        m_soundfont.noteOff(t, cell.note);
                    } else if (type == TRACK_TYPE_PLUGIN) {
                        if (t < static_cast<int>(m_plugins.size()) && m_plugins[t]) {
                            m_plugins[t]->noteOff(cell.note);
                        }
                    } else if (type == TRACK_TYPE_SYNTH && t < static_cast<int>(m_instruments.size())) {
                        m_instruments[t].noteOff(cell.note);
                    }
                } else if (cell.note <= 127) {
                    float vel = (cell.volume == 255) ? 1.0f : (cell.volume / 128.0f);
                    
                    if (type == TRACK_TYPE_SOUNDFONT) {
                        int preset = getTrackSf2Preset(t);
                        m_soundfont.setChannelPreset(t, preset);
                        m_soundfont.noteOn(t, cell.note, vel);
                    } else if (type == TRACK_TYPE_DRUMS) {
                        int pad = (cell.note >= 36 && cell.note <= 43) ? (cell.note - 36) : (cell.note % NUM_DRUM_PADS);
                        m_drumSampler.triggerPad(pad, vel);
                    } else if (type == TRACK_TYPE_PLUGIN) {
                        if (t < static_cast<int>(m_plugins.size()) && m_plugins[t]) {
                            m_plugins[t]->noteOn(cell.note, vel);
                        }
                    } else {
                        if (cell.effectType == 0x10 && t < static_cast<int>(m_instruments.size())) {
                            float cutoff = 150.0f + (cell.effectParam / 255.0f) * 10000.0f;
                            m_instruments[t].getParams().filterCutoff = cutoff;
                        }
                        if (t < static_cast<int>(m_instruments.size())) {
                            m_instruments[t].noteOn(cell.note, vel);
                        }
                    }
                }
            }
        }

        float sL = 0.0f;
        float sR = 0.0f;

        // 1. Process AuraSynth tracks with Insert FX & per-track volume & panning
        for (int t = 0; t < numTracks && t < static_cast<int>(m_instruments.size()) && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_SYNTH) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float instL = 0.0f, instR = 0.0f;
                m_instruments[t].processBlock(&instL, &instR, 1);
                
                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(instL, instR);
                        }
                    }
                }

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;
                
                instL *= effVol * panL;
                instR *= effVol * panR;
                sL += instL;
                sR += instR;

                float absL = std::abs(instL);
                float absR = std::abs(instR);
                if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                if (absR > trkPeakR[t]) trkPeakR[t] = absR;
            }
        }

        // 2. Process SoundFont tracks with Insert FX & per-track volume & panning
        for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_SOUNDFONT) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float sfL = 0.0f, sfR = 0.0f;
                m_soundfont.processChannelSample(t, sfL, sfR);

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(sfL, sfR);
                        }
                    }
                }

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                sfL *= effVol * panL;
                sfR *= effVol * panR;
                sL += sfL;
                sR += sfR;

                float absL = std::abs(sfL);
                float absR = std::abs(sfR);
                if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                if (absR > trkPeakR[t]) trkPeakR[t] = absR;
            }
        }

        // 3. Process BeatPad Drum Sampler with Insert FX
        float drumBuf[2] = {0.0f, 0.0f};
        m_drumSampler.processBlock(drumBuf, 1);
        for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_DRUMS) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float dL = drumBuf[0];
                float dR = drumBuf[1];

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(dL, dR);
                        }
                    }
                }

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                dL *= effVol * panL;
                dR *= effVol * panR;
                sL += dL;
                sR += dR;

                float absL = std::abs(dL);
                float absR = std::abs(dR);
                if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                if (absR > trkPeakR[t]) trkPeakR[t] = absR;
            }
        }

        // 4. Process Unified Plugin Instrument tracks (VST3, CLAP, VST2) with Insert FX
        for (int t = 0; t < numTracks && t < static_cast<int>(m_plugins.size()) && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_PLUGIN && m_plugins[t]->isLoaded()) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float plgL = 0.0f, plgR = 0.0f;
                m_plugins[t]->processSample(plgL, plgR);

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(plgL, plgR);
                        }
                    }
                }

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                plgL *= effVol * panL;
                plgR *= effVol * panR;
                sL += plgL;
                sR += plgR;

                float absL = std::abs(plgL);
                float absR = std::abs(plgR);
                if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                if (absR > trkPeakR[t]) trkPeakR[t] = absR;
            }
        }

        // 5. Smart Macros & Master Soft Limiter
        m_macros.processSample(sL, sR);
        MacroProcessor::masterBusProcess(sL, sR);

        if (channels == 2) {
            output[f * 2 + 0] = sL;
            output[f * 2 + 1] = sR;
        } else {
            output[f] = 0.5f * (sL + sR);
        }

        float absL = std::abs(sL);
        float absR = std::abs(sR);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;
        sumSq += (sL * sL + sR * sR) * 0.5f;
    }

    // Update track peak meters with smooth decay
    for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
        float prevL = m_trackMixers[t].peakL.load(std::memory_order_relaxed);
        float prevR = m_trackMixers[t].peakR.load(std::memory_order_relaxed);
        m_trackMixers[t].peakL.store(std::max(trkPeakL[t], prevL * 0.90f), std::memory_order_relaxed);
        m_trackMixers[t].peakR.store(std::max(trkPeakR[t], prevR * 0.90f), std::memory_order_relaxed);
    }

    float prevPeakL = m_peakL.load(std::memory_order_relaxed);
    float prevPeakR = m_peakR.load(std::memory_order_relaxed);
    m_peakL.store(std::max(peakL, prevPeakL * 0.92f), std::memory_order_relaxed);
    m_peakR.store(std::max(peakR, prevPeakR * 0.92f), std::memory_order_relaxed);

    float rms = std::sqrt(sumSq / std::max(1, frameCount));
    m_rms.store(rms, std::memory_order_relaxed);
}

// ==========================================
// Plugin Host Control Implementations
// ==========================================
bool AudioEngine::loadTrackPlugin(int track, const std::string& path) {
    if (track < 0 || track >= static_cast<int>(m_plugins.size())) return false;
    bool ok = m_plugins[track]->load(path, m_sampleRate, 256);
    if (ok) {
        setTrackType(track, TRACK_TYPE_PLUGIN);
        std::cout << "[myms] Loaded plugin on track " << track << ": " 
                  << m_plugins[track]->getName() << " (" << m_plugins[track]->getFormatName() << ")" << std::endl;
    }
    return ok;
}

void AudioEngine::unloadTrackPlugin(int track) {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        m_plugins[track]->unload();
    }
}

bool AudioEngine::isTrackPluginLoaded(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->isLoaded();
    }
    return false;
}

std::string AudioEngine::getTrackPluginName(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getName();
    }
    return "";
}

std::string AudioEngine::getTrackPluginVendor(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getVendor();
    }
    return "";
}

std::string AudioEngine::getTrackPluginPath(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getPath();
    }
    return "";
}

int AudioEngine::getTrackPluginFormat(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return static_cast<int>(m_plugins[track]->getFormat());
    }
    return 0;
}

std::string AudioEngine::getTrackPluginFormatName(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getFormatName();
    }
    return "NONE";
}

int AudioEngine::getTrackPluginNumParams(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getNumParams();
    }
    return 0;
}

std::string AudioEngine::getTrackPluginParamName(int track, int paramIdx) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        const PluginParamInfo* info = m_plugins[track]->getParamInfo(paramIdx);
        if (info) return info->name;
    }
    return "";
}

std::string AudioEngine::getTrackPluginParamDisplay(int track, int paramIdx) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        const PluginParamInfo* info = m_plugins[track]->getParamInfo(paramIdx);
        if (info) return info->display;
    }
    return "";
}

std::string AudioEngine::getTrackPluginParamLabel(int track, int paramIdx) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        const PluginParamInfo* info = m_plugins[track]->getParamInfo(paramIdx);
        if (info) return info->label;
    }
    return "";
}

float AudioEngine::getTrackPluginParamValue(int track, int paramIdx) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->getParamValue(paramIdx);
    }
    return 0.0f;
}

void AudioEngine::setTrackPluginParamValue(int track, int paramIdx, float val) {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        m_plugins[track]->setParamValue(paramIdx, val);
    }
}

bool AudioEngine::isTrackPluginParamsDirty(int track) {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->checkAndClearParamsDirty();
    }
    return false;
}

bool AudioEngine::hasTrackPluginNativeEditor(int track) const {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->hasNativeEditor();
    }
    return false;
}

bool AudioEngine::openTrackPluginNativeEditor(int track, void* parentWindow) {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        return m_plugins[track]->openNativeEditor(parentWindow);
    }
    return false;
}

void AudioEngine::closeTrackPluginNativeEditor(int track) {
    if (track >= 0 && track < static_cast<int>(m_plugins.size())) {
        m_plugins[track]->closeNativeEditor();
    }
}

// ==========================================
// Track Insert FX Controls (VST3, CLAP, VST2)
// ==========================================
bool AudioEngine::loadTrackFx(int track, int slot, const std::string& path) {
    if (track < 0 || track >= static_cast<int>(m_trackFx.size())) return false;
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (!m_trackFx[track][slot]) {
        m_trackFx[track][slot] = std::make_unique<PluginInstance>();
    }
    bool ok = m_trackFx[track][slot]->load(path, m_sampleRate, 256);
    if (ok) {
        if (track < MAX_INSTRUMENTS) {
            int currentMax = m_trackFxMaxSlot[track].load(std::memory_order_relaxed);
            if (slot + 1 > currentMax) {
                m_trackFxMaxSlot[track].store(slot + 1, std::memory_order_release);
            }
        }
        std::cout << "[myms] Loaded FX on track " << track << " slot " << slot << ": "
                  << m_trackFx[track][slot]->getName() << " (" << m_trackFx[track][slot]->getFormatName() << ")" << std::endl;
    }
    return ok;
}

void AudioEngine::unloadTrackFx(int track, int slot) {
    if (track >= 0 && track < static_cast<int>(m_trackFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        if (m_trackFx[track][slot]) m_trackFx[track][slot]->unload();
        if (track < MAX_INSTRUMENTS) {
            int maxSlot = 0;
            for (int s = 0; s < MAX_TRACK_FX; ++s) {
                if (m_trackFx[track][s] && m_trackFx[track][s]->isLoaded()) {
                    maxSlot = s + 1;
                }
            }
            m_trackFxMaxSlot[track].store(maxSlot, std::memory_order_release);
        }
    }
}

int AudioEngine::getTrackFxCount(int track) const {
    if (track < 0 || track >= static_cast<int>(m_trackFx.size())) return 0;
    int count = 0;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        if (m_trackFx[track][s] && m_trackFx[track][s]->isLoaded()) count++;
    }
    return count;
}

bool AudioEngine::isTrackFxLoaded(int track, int slot) const {
    if (track >= 0 && track < static_cast<int>(m_trackFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        return m_trackFx[track][slot] && m_trackFx[track][slot]->isLoaded();
    }
    return false;
}

std::string AudioEngine::getTrackFxName(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return m_trackFx[track][slot]->getName();
    return "";
}

std::string AudioEngine::getTrackFxVendor(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return m_trackFx[track][slot]->getVendor();
    return "";
}

std::string AudioEngine::getTrackFxPath(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return m_trackFx[track][slot]->getPath();
    return "";
}

int AudioEngine::getTrackFxFormat(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return static_cast<int>(m_trackFx[track][slot]->getFormat());
    return 0;
}

std::string AudioEngine::getTrackFxFormatName(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return m_trackFx[track][slot]->getFormatName();
    return "NONE";
}

int AudioEngine::getTrackFxNumParams(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) return m_trackFx[track][slot]->getNumParams();
    return 0;
}

std::string AudioEngine::getTrackFxParamName(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        const PluginParamInfo* info = m_trackFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->name;
    }
    return "";
}

float AudioEngine::getTrackFxParamValue(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        return m_trackFx[track][slot]->getParamValue(paramIdx);
    }
    return 0.0f;
}

void AudioEngine::setTrackFxParamValue(int track, int slot, int paramIdx, float val) {
    if (isTrackFxLoaded(track, slot)) {
        m_trackFx[track][slot]->setParamValue(paramIdx, val);
    }
}

std::string AudioEngine::getTrackFxParamDisplay(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        const PluginParamInfo* info = m_trackFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->display;
    }
    return "";
}

bool AudioEngine::isTrackFxParamsDirty(int track, int slot) {
    if (track >= 0 && track < static_cast<int>(m_trackFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        if (m_trackFx[track][slot]) return m_trackFx[track][slot]->checkAndClearParamsDirty();
    }
    return false;
}

bool AudioEngine::hasTrackFxEditor(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return m_trackFx[track][slot]->hasNativeEditor();
    }
    return false;
}

bool AudioEngine::openTrackFxEditor(int track, int slot, void* parentWindow) {
    if (isTrackFxLoaded(track, slot)) {
        return m_trackFx[track][slot]->openNativeEditor(parentWindow);
    }
    return false;
}

void AudioEngine::closeTrackFxEditor(int track, int slot) {
    if (isTrackFxLoaded(track, slot)) {
        m_trackFx[track][slot]->closeNativeEditor();
    }
}

} // namespace myms
