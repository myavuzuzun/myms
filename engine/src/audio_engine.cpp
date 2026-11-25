#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "audio_engine.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdio>

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
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        m_trackBaseCutoff[i] = 3500.0f;
    }
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

    m_trackPreFx.clear();
    std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX> initPreFx;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        initPreFx[s] = std::make_unique<PluginInstance>();
    }
    m_trackPreFx.emplace_back(std::move(initPreFx));

    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        m_masterFx[s] = std::make_unique<PluginInstance>();
    }
    m_masterFxMaxSlot.store(0, std::memory_order_release);

    m_trackDelayBufL.resize(MAX_INSTRUMENTS);
    m_trackDelayBufR.resize(MAX_INSTRUMENTS);
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        m_trackCellFilterL[i].init(m_sampleRate);
        m_trackCellFilterR[i].init(m_sampleRate);
        m_trackCellFilterActive[i] = false;
        m_trackBaseCutoff[i] = 3500.0f;
        m_trackPitchBendSemi[i] = 0.0f;
        m_trackPitchSlidePerSample[i] = 0.0f;
        m_trackNoteCutCountdown[i] = -1;
        m_trackRetrigInterval[i] = -1;
        m_trackRetrigCountdown[i] = -1;
        m_trackLastNote[i] = -1;
        m_trackLastVel[i] = 1.0f;
        m_trackDelayBufL[i].assign(TRACK_DELAY_SIZE, 0.0f);
        m_trackDelayBufR[i].assign(TRACK_DELAY_SIZE, 0.0f);
        m_trackDelayWritePos[i] = 0;
        m_trackDelaySend[i] = 0.0f;
    }

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
    int curPat = getCurrentPattern();
    return m_sequencer.getPatternNumRows(curPat);
}

void AudioEngine::setNumRows(int rows) {
    int curPat = getCurrentPattern();
    m_sequencer.setPatternNumRows(curPat, rows);
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

    std::array<std::unique_ptr<PluginInstance>, MAX_TRACK_FX> preFxSlots;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        preFxSlots[s] = std::make_unique<PluginInstance>();
    }
    m_trackPreFx.emplace_back(std::move(preFxSlots));

    // Initialize with default synth patch
    SynthParams p = m_instruments[0].getParams();
    m_instruments.back().setParams(p);

    m_sequencer.addTrack();
}

void AudioEngine::removeTrack(int trackIdx) {
    int oldCount = static_cast<int>(m_instruments.size());
    if (oldCount <= 1 || trackIdx < 0 || trackIdx >= oldCount) return;
    allNotesOff();
    m_instruments.erase(m_instruments.begin() + trackIdx);
    if (trackIdx < static_cast<int>(m_instrumentNames.size())) {
        m_instrumentNames.erase(m_instrumentNames.begin() + trackIdx);
    }
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
    if (trackIdx < static_cast<int>(m_trackPreFx.size())) {
        m_trackPreFx.erase(m_trackPreFx.begin() + trackIdx);
    }
    for (int i = trackIdx; i < oldCount - 1 && i + 1 < MAX_INSTRUMENTS; ++i) {
        m_trackBaseCutoff[i] = m_trackBaseCutoff[i + 1];
        m_trackFxMaxSlot[i].store(m_trackFxMaxSlot[i + 1].load(std::memory_order_relaxed), std::memory_order_release);
        m_trackPreFxMaxSlot[i].store(m_trackPreFxMaxSlot[i + 1].load(std::memory_order_relaxed), std::memory_order_release);
    }
    if (oldCount - 1 < MAX_INSTRUMENTS) {
        m_trackBaseCutoff[oldCount - 1] = 3500.0f;
        m_trackFxMaxSlot[oldCount - 1].store(0, std::memory_order_release);
        m_trackPreFxMaxSlot[oldCount - 1].store(0, std::memory_order_release);
    }
    m_sequencer.removeTrack(trackIdx);
    int newCount = static_cast<int>(m_instruments.size());
    for (int t = 0; t < newCount && t < static_cast<int>(m_trackMixers.size()); ++t) {
        m_soundfont.setChannelPreset(t, m_trackMixers[t].sf2Preset);
        m_soundfont.setChannelVolume(t, m_trackMixers[t].volume);
        m_soundfont.setChannelPan(t, m_trackMixers[t].pan);
        m_soundfont.setChannelMute(t, m_trackMixers[t].mute);
    }
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

bool AudioEngine::moveSongOrderStep(int fromStep, int toStep) {
    bool ok = m_sequencer.moveSongOrderStep(fromStep, toStep);
    if (ok) {
        m_uiSongOrderIndex.store(m_sequencer.getSongOrderIndex(), std::memory_order_relaxed);
    }
    return ok;
}

bool AudioEngine::swapSongOrderSteps(int stepA, int stepB) {
    bool ok = m_sequencer.swapSongOrderSteps(stepA, stepB);
    if (ok) {
        m_uiSongOrderIndex.store(m_sequencer.getSongOrderIndex(), std::memory_order_relaxed);
    }
    return ok;
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

    if (inst >= 0 && inst < static_cast<int>(m_trackPreFx.size())) {
        int maxPre = (inst < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[inst].load(std::memory_order_relaxed) : 0;
        for (int s = 0; s < maxPre; ++s) {
            if (m_trackPreFx[inst][s] && m_trackPreFx[inst][s]->isLoaded()) {
                m_trackPreFx[inst][s]->noteOn(note, velocity);
            }
        }
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

    if (inst >= 0 && inst < static_cast<int>(m_trackPreFx.size())) {
        int maxPre = (inst < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[inst].load(std::memory_order_relaxed) : 0;
        for (int s = 0; s < maxPre; ++s) {
            if (m_trackPreFx[inst][s] && m_trackPreFx[inst][s]->isLoaded()) {
                m_trackPreFx[inst][s]->noteOff(note);
            }
        }
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
    for (size_t t = 0; t < m_trackPreFx.size(); ++t) {
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_trackPreFx[t][s]) m_trackPreFx[t][s]->allNotesOff();
        }
    }
    for (size_t t = 0; t < m_trackFx.size(); ++t) {
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_trackFx[t][s]) m_trackFx[t][s]->allNotesOff();
        }
    }
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        if (m_masterFx[s]) m_masterFx[s]->allNotesOff();
    }
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        m_previewCountdowns[i] = 0;
        m_trackPitchBendSemi[i] = 0.0f;
        m_trackPitchSlidePerSample[i] = 0.0f;
        m_trackNoteCutCountdown[i] = -1;
        m_trackRetrigInterval[i] = -1;
        m_trackRetrigCountdown[i] = -1;
        m_trackCellFilterActive[i] = false;
        m_trackDelaySend[i] = 0.0f;
        m_soundfont.setChannelPitchWheel(i, 8192);
        if (i < static_cast<int>(m_instruments.size())) {
            m_instruments[i].setPitchBendSemi(0.0f);
            m_instruments[i].getParams().filterCutoff = m_trackBaseCutoff[i];
        }
        if (i < static_cast<int>(m_trackDelayBufL.size())) {
            std::fill(m_trackDelayBufL[i].begin(), m_trackDelayBufL[i].end(), 0.0f);
            std::fill(m_trackDelayBufR[i].begin(), m_trackDelayBufR[i].end(), 0.0f);
        }
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
        float c = std::clamp(cutoffHz, 20.0f, 20000.0f);
        m_instruments[inst].getParams().filterCutoff = c;
        if (inst < MAX_INSTRUMENTS) {
            m_trackBaseCutoff[inst] = c;
        }
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

int AudioEngine::getPatternNumRows(int pat) const {
    return m_sequencer.getPatternNumRows(pat);
}

void AudioEngine::setPatternNumRows(int pat, int rows) {
    m_sequencer.setPatternNumRows(pat, rows);
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

float AudioEngine::getCpuUsage() const {
    return m_cpuUsage.load(std::memory_order_relaxed);
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

void AudioEngine::renderProcessBlock(float* output, int frameCount, int channels, bool isPlayingNow, float* trkPeakL, float* trkPeakR) {
    int numTracks = m_sequencer.getNumTracks();

    // Determine solo state
    bool anySolo = false;
    for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
        if (m_trackMixers[t].solo) {
            anySolo = true;
            break;
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

            int samplesPerRow = m_sequencer.getSamplesPerRow();

            for (int t = 0; t < numTracks; ++t) {
                TrackerCell cell = m_sequencer.getPatternCell(triggeredPat, t, triggeredRow);
                // Option C: Cross-Track Instrument Routing / Multiplexing
                // If cell.instrument is specified (1-based, e.g. 01 for Track 1/Inst 0), route to that instrument; otherwise default to current track t.
                int targetInst = (cell.instrument > 0 && cell.instrument <= numTracks) ? (cell.instrument - 1) : t;
                int type = getTrackType(targetInst);

                // --- 1. Evaluate Cell Effects (Commands) ---
                if (cell.effectType != 0) {
                    switch (cell.effectType) {
                        case 0x10: { // Filter Cutoff
                            if (cell.effectParam >= 250) {
                                m_trackCellFilterActive[t] = false;
                                if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                                    m_instruments[targetInst].getParams().filterCutoff = 20000.0f;
                                }
                            } else {
                                m_trackCellFilterActive[t] = true;
                                float norm = cell.effectParam / 255.0f;
                                // Smooth logarithmic/exponential mapping: 70 Hz to 18,000 Hz
                                float cutoff = 70.0f * std::pow(18000.0f / 70.0f, norm);
                                float resonance = 0.50f;
                                m_trackCellFilterL[t].setCutoffAndResonance(cutoff, resonance);
                                m_trackCellFilterR[t].setCutoffAndResonance(cutoff, resonance);
                                if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                                    m_instruments[targetInst].getParams().filterCutoff = cutoff;
                                }
                            }
                            break;
                        }
                        case 0x01: { // Pitch Slide Up
                            float slideSemi = (cell.effectParam / 255.0f) * 12.0f;
                            m_trackPitchSlidePerSample[t] = (samplesPerRow > 0) ? (slideSemi / samplesPerRow) : 0.0f;
                            break;
                        }
                        case 0x02: { // Pitch Slide Down
                            float slideSemi = (cell.effectParam / 255.0f) * 12.0f;
                            m_trackPitchSlidePerSample[t] = (samplesPerRow > 0) ? (-slideSemi / samplesPerRow) : 0.0f;
                            break;
                        }
                        case 0x0C: { // Retrigger / Note Cut
                            if (cell.effectParam <= 16) {
                                int cutTicks = std::max(1, static_cast<int>(cell.effectParam));
                                m_trackNoteCutCountdown[t] = (cutTicks * samplesPerRow) / 16;
                                m_trackRetrigInterval[t] = -1;
                                m_trackRetrigCountdown[t] = -1;
                            } else {
                                int retrigs = std::clamp(static_cast<int>(cell.effectParam >> 4) + 1, 2, 8);
                                m_trackRetrigInterval[t] = samplesPerRow / retrigs;
                                m_trackRetrigCountdown[t] = m_trackRetrigInterval[t];
                                m_trackNoteCutCountdown[t] = -1;
                            }
                            break;
                        }
                        case 0x0E: { // Delay / Echo Send
                            m_trackDelaySend[t] = (cell.effectParam / 255.0f);
                            break;
                        }
                        default:
                            break;
                    }
                } else {
                    m_trackPitchSlidePerSample[t] = 0.0f;
                    m_trackNoteCutCountdown[t] = -1;
                    m_trackRetrigInterval[t] = -1;
                    m_trackRetrigCountdown[t] = -1;
                }

                // Per-step delay send reset: if cell doesn't set 0x0E, dry signal sends 0 to delay
                if (cell.effectType != 0x0E) {
                    m_trackDelaySend[t] = 0.0f;
                }

                // --- 2. Evaluate Note Events ---
                if (cell.note == NOTE_OFF) {
                    m_trackLastNote[t] = -1;
                    m_trackPitchBendSemi[t] = 0.0f;
                    m_trackPitchSlidePerSample[t] = 0.0f;
                    m_trackCellFilterActive[t] = false;
                    m_trackDelaySend[t] = 0.0f;
                    m_trackNoteCutCountdown[t] = -1;
                    m_trackRetrigInterval[t] = -1;
                    m_trackRetrigCountdown[t] = -1;
                    if (targetInst < static_cast<int>(m_trackPreFx.size())) {
                        int maxPre = (targetInst < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[targetInst].load(std::memory_order_relaxed) : 0;
                        for (int s = 0; s < maxPre; ++s) {
                            if (m_trackPreFx[targetInst][s] && m_trackPreFx[targetInst][s]->isLoaded()) {
                                m_trackPreFx[targetInst][s]->noteOff(cell.note);
                            }
                        }
                    }
                    if (type == TRACK_TYPE_SOUNDFONT) {
                        m_soundfont.noteOff(targetInst, cell.note);
                    } else if (type == TRACK_TYPE_PLUGIN) {
                        if (targetInst < static_cast<int>(m_plugins.size()) && m_plugins[targetInst]) {
                            m_plugins[targetInst]->noteOff(cell.note);
                        }
                    } else if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                        m_instruments[targetInst].getParams().filterCutoff = m_trackBaseCutoff[targetInst];
                        m_instruments[targetInst].setPitchBendSemi(0.0f);
                        m_instruments[targetInst].noteOff(cell.note);
                    }
                } else if (cell.note <= 127) {
                    // When a new note triggers, ensure previous note's effects do not carry over:
                    if (cell.effectType != 0x10) {
                        m_trackCellFilterActive[t] = false;
                        if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                            m_instruments[targetInst].getParams().filterCutoff = m_trackBaseCutoff[targetInst];
                        }
                    }
                    if (cell.effectType != 0x01 && cell.effectType != 0x02) {
                        m_trackPitchBendSemi[t] = 0.0f;
                        m_trackPitchSlidePerSample[t] = 0.0f;
                        if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                            m_instruments[targetInst].setPitchBendSemi(0.0f);
                        } else if (type == TRACK_TYPE_SOUNDFONT) {
                            m_soundfont.setChannelPitchWheel(targetInst, 8192);
                        }
                    }
                    if (cell.effectType != 0x0C) {
                        m_trackNoteCutCountdown[t] = -1;
                        m_trackRetrigInterval[t] = -1;
                        m_trackRetrigCountdown[t] = -1;
                    }

                    float vel = (cell.volume == 255) ? 1.0f : (cell.volume / 128.0f);
                    m_trackLastNote[t] = cell.note;
                    m_trackLastVel[t] = vel;
                    m_trackPitchBendSemi[t] = 0.0f;
                    if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                        m_instruments[targetInst].setPitchBendSemi(0.0f);
                    } else if (type == TRACK_TYPE_SOUNDFONT) {
                        m_soundfont.setChannelPitchWheel(targetInst, 8192);
                    }
                    
                    if (targetInst < static_cast<int>(m_trackPreFx.size())) {
                        int maxPre = (targetInst < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[targetInst].load(std::memory_order_relaxed) : 0;
                        for (int s = 0; s < maxPre; ++s) {
                            if (m_trackPreFx[targetInst][s] && m_trackPreFx[targetInst][s]->isLoaded()) {
                                m_trackPreFx[targetInst][s]->noteOn(cell.note, vel);
                            }
                        }
                    }

                    if (type == TRACK_TYPE_SOUNDFONT) {
                        int preset = getTrackSf2Preset(targetInst);
                        m_soundfont.setChannelPreset(targetInst, preset);
                        m_soundfont.noteOn(targetInst, cell.note, vel);
                    } else if (type == TRACK_TYPE_DRUMS) {
                        int pad = (cell.note >= 36 && cell.note <= 43) ? (cell.note - 36) : (cell.note % NUM_DRUM_PADS);
                        m_drumSampler.triggerPad(pad, vel);
                    } else if (type == TRACK_TYPE_PLUGIN) {
                        if (targetInst < static_cast<int>(m_plugins.size()) && m_plugins[targetInst]) {
                            m_plugins[targetInst]->noteOn(cell.note, vel);
                        }
                    } else {
                        if (targetInst < static_cast<int>(m_instruments.size())) {
                            m_instruments[targetInst].noteOn(cell.note, vel);
                        }
                    }
                }
            }
        }

        // --- 3. Per-Frame Effect Modulation (Note Cut, Retriggers, Pitch Slide) ---
        for (int t = 0; t < numTracks; ++t) {
            int targetInst = t;
            int type = getTrackType(targetInst);

            // Note Cut
            if (m_trackNoteCutCountdown[t] > 0) {
                --m_trackNoteCutCountdown[t];
                if (m_trackNoteCutCountdown[t] == 0) {
                    m_trackNoteCutCountdown[t] = -1;
                    if (type == TRACK_TYPE_SOUNDFONT) {
                        m_soundfont.noteOff(targetInst, m_trackLastNote[t]);
                    } else if (type == TRACK_TYPE_PLUGIN && targetInst < static_cast<int>(m_plugins.size()) && m_plugins[targetInst]) {
                        m_plugins[targetInst]->noteOff(m_trackLastNote[t]);
                    } else if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                        m_instruments[targetInst].noteOff();
                    }
                }
            }

            // Retrigger
            if (m_trackRetrigInterval[t] > 0 && m_trackLastNote[t] >= 0) {
                --m_trackRetrigCountdown[t];
                if (m_trackRetrigCountdown[t] <= 0) {
                    m_trackRetrigCountdown[t] = m_trackRetrigInterval[t];
                    if (type == TRACK_TYPE_SOUNDFONT) {
                        m_soundfont.noteOn(targetInst, m_trackLastNote[t], m_trackLastVel[t]);
                    } else if (type == TRACK_TYPE_DRUMS) {
                        int pad = (m_trackLastNote[t] >= 36 && m_trackLastNote[t] <= 43) ? (m_trackLastNote[t] - 36) : (m_trackLastNote[t] % NUM_DRUM_PADS);
                        m_drumSampler.triggerPad(pad, m_trackLastVel[t]);
                    } else if (type == TRACK_TYPE_PLUGIN && targetInst < static_cast<int>(m_plugins.size()) && m_plugins[targetInst]) {
                        m_plugins[targetInst]->noteOn(m_trackLastNote[t], m_trackLastVel[t]);
                    } else if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                        m_instruments[targetInst].noteOn(m_trackLastNote[t], m_trackLastVel[t]);
                    }
                }
            }

            // Pitch Slide
            if (m_trackPitchSlidePerSample[t] != 0.0f) {
                m_trackPitchBendSemi[t] = std::clamp(m_trackPitchBendSemi[t] + m_trackPitchSlidePerSample[t], -24.0f, 24.0f);
                if (type == TRACK_TYPE_SYNTH && targetInst < static_cast<int>(m_instruments.size())) {
                    m_instruments[targetInst].setPitchBendSemi(m_trackPitchBendSemi[t]);
                } else if (type == TRACK_TYPE_SOUNDFONT) {
                    int wheel = 8192 + static_cast<int>(std::clamp(m_trackPitchBendSemi[t] / 12.0f, -1.0f, 1.0f) * 8191.0f);
                    m_soundfont.setChannelPitchWheel(targetInst, wheel);
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
                
                // Track Pre-Instrument FX Chain (Plugins Before Instrument)
                if (t < static_cast<int>(m_trackPreFx.size())) {
                    int maxPre = (t < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxPre; ++s) {
                        if (m_trackPreFx[t][s] && m_trackPreFx[t][s]->isLoaded()) {
                            m_trackPreFx[t][s]->processSample(instL, instR);
                        }
                    }
                }

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(instL, instR);
                        }
                    }
                }

                // Track Cell Effects (Filter & Delay)
                applyTrackCellEffects(t, instL, instR);

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;
                
                instL *= effVol * panL;
                instR *= effVol * panR;
                sL += instL;
                sR += instR;

                if (trkPeakL && trkPeakR) {
                    float absL = std::abs(instL);
                    float absR = std::abs(instR);
                    if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                    if (absR > trkPeakR[t]) trkPeakR[t] = absR;
                }
            }
        }

        // 2. Process SoundFont tracks with Insert FX & per-track volume & panning
        for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_SOUNDFONT) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float sfL = 0.0f, sfR = 0.0f;
                m_soundfont.processChannelSample(t, sfL, sfR);

                // Track Pre-Instrument FX Chain
                if (t < static_cast<int>(m_trackPreFx.size())) {
                    int maxPre = (t < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxPre; ++s) {
                        if (m_trackPreFx[t][s] && m_trackPreFx[t][s]->isLoaded()) {
                            m_trackPreFx[t][s]->processSample(sfL, sfR);
                        }
                    }
                }

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(sfL, sfR);
                        }
                    }
                }

                // Track Cell Effects (Filter & Delay)
                applyTrackCellEffects(t, sfL, sfR);

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                sfL *= effVol * panL;
                sfR *= effVol * panR;
                sL += sfL;
                sR += sfR;

                if (trkPeakL && trkPeakR) {
                    float absL = std::abs(sfL);
                    float absR = std::abs(sfR);
                    if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                    if (absR > trkPeakR[t]) trkPeakR[t] = absR;
                }
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

                // Track Pre-Instrument FX Chain
                if (t < static_cast<int>(m_trackPreFx.size())) {
                    int maxPre = (t < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxPre; ++s) {
                        if (m_trackPreFx[t][s] && m_trackPreFx[t][s]->isLoaded()) {
                            m_trackPreFx[t][s]->processSample(dL, dR);
                        }
                    }
                }

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(dL, dR);
                        }
                    }
                }

                // Track Cell Effects (Filter & Delay)
                applyTrackCellEffects(t, dL, dR);

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                dL *= effVol * panL;
                dR *= effVol * panR;
                sL += dL;
                sR += dR;

                if (trkPeakL && trkPeakR) {
                    float absL = std::abs(dL);
                    float absR = std::abs(dR);
                    if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                    if (absR > trkPeakR[t]) trkPeakR[t] = absR;
                }
            }
        }

        // 4. Process Unified Plugin Instrument tracks (VST3, CLAP) with Insert FX
        for (int t = 0; t < numTracks && t < static_cast<int>(m_plugins.size()) && t < static_cast<int>(m_trackMixers.size()); ++t) {
            if (m_trackMixers[t].type == TRACK_TYPE_PLUGIN && m_plugins[t]->isLoaded()) {
                bool canSound = isPlayingNow || (t < MAX_INSTRUMENTS && m_previewCountdowns[t] > 0);
                if (!canSound) continue;
                float plgL = 0.0f, plgR = 0.0f;
                m_plugins[t]->processSample(plgL, plgR);

                // Track Pre-Instrument FX Chain
                if (t < static_cast<int>(m_trackPreFx.size())) {
                    int maxPre = (t < MAX_INSTRUMENTS) ? m_trackPreFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxPre; ++s) {
                        if (m_trackPreFx[t][s] && m_trackPreFx[t][s]->isLoaded()) {
                            m_trackPreFx[t][s]->processSample(plgL, plgR);
                        }
                    }
                }

                // Track Insert FX Chain
                if (t < static_cast<int>(m_trackFx.size())) {
                    int maxSlot = (t < MAX_INSTRUMENTS) ? m_trackFxMaxSlot[t].load(std::memory_order_relaxed) : 0;
                    for (int s = 0; s < maxSlot; ++s) {
                        if (m_trackFx[t][s] && m_trackFx[t][s]->isLoaded()) {
                            m_trackFx[t][s]->processEffectSample(plgL, plgR);
                        }
                    }
                }

                // Track Cell Effects (Filter & Delay)
                applyTrackCellEffects(t, plgL, plgR);

                bool audible = anySolo ? m_trackMixers[t].solo : !m_trackMixers[t].mute;
                float effVol = audible ? m_trackMixers[t].volume : 0.0f;
                float pan = m_trackMixers[t].pan;
                float panL = (pan <= 0.5f) ? 1.0f : (1.0f - pan) * 2.0f;
                float panR = (pan >= 0.5f) ? 1.0f : pan * 2.0f;

                plgL *= effVol * panL;
                plgR *= effVol * panR;
                sL += plgL;
                sR += plgR;

                if (trkPeakL && trkPeakR) {
                    float absL = std::abs(plgL);
                    float absR = std::abs(plgR);
                    if (absL > trkPeakL[t]) trkPeakL[t] = absL;
                    if (absR > trkPeakR[t]) trkPeakR[t] = absR;
                }
            }
        }

        // 5. Master Insert FX Chain
        int masterMax = m_masterFxMaxSlot.load(std::memory_order_relaxed);
        for (int s = 0; s < masterMax; ++s) {
            if (m_masterFx[s] && m_masterFx[s]->isLoaded()) {
                m_masterFx[s]->processEffectSample(sL, sR);
            }
        }

        // 6. Smart Macros & Master Soft Limiter
        m_macros.processSample(sL, sR);
        MacroProcessor::masterBusProcess(sL, sR);

        if (channels == 2) {
            output[f * 2 + 0] = sL;
            output[f * 2 + 1] = sR;
        } else {
            output[f] = 0.5f * (sL + sR);
        }
    }
}

void AudioEngine::renderAudio(float* output, int frameCount, int channels) {
    auto startCpuTime = std::chrono::steady_clock::now();
    std::memset(output, 0, frameCount * channels * sizeof(float));

    int numTracks = m_sequencer.getNumTracks();
    float trkPeakL[MAX_INSTRUMENTS] = {0.0f};
    float trkPeakR[MAX_INSTRUMENTS] = {0.0f};

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
            m_peakL.store(0.0f, std::memory_order_relaxed);
            m_peakR.store(0.0f, std::memory_order_relaxed);
            m_rms.store(0.0f, std::memory_order_relaxed);
            for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
                m_trackMixers[t].peakL.store(0.0f, std::memory_order_relaxed);
                m_trackMixers[t].peakR.store(0.0f, std::memory_order_relaxed);
            }
            float prevCpu = m_cpuUsage.load(std::memory_order_relaxed);
            m_cpuUsage.store(prevCpu * 0.90f, std::memory_order_relaxed);
            return;
        }
    }

    renderProcessBlock(output, frameCount, channels, isPlayingNow, trkPeakL, trkPeakR);

    // Compute peaks & RMS from the rendered output
    float peakL = 0.0f;
    float peakR = 0.0f;
    float sumSq = 0.0f;
    for (int f = 0; f < frameCount; ++f) {
        float sL = (channels == 2) ? output[f * 2 + 0] : output[f];
        float sR = (channels == 2) ? output[f * 2 + 1] : output[f];
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

    auto endCpuTime = std::chrono::steady_clock::now();
    double elapsedUs = std::chrono::duration<double, std::micro>(endCpuTime - startCpuTime).count();
    double bufferUs = (static_cast<double>(frameCount) / std::max(1.0, static_cast<double>(m_sampleRate))) * 1000000.0;
    float currentLoad = static_cast<float>((elapsedUs / std::max(1.0, bufferUs)) * 100.0);
    currentLoad = std::clamp(currentLoad, 0.0f, 100.0f);

    float prevCpu = m_cpuUsage.load(std::memory_order_relaxed);
    float smoothedCpu = (currentLoad > prevCpu) ? (prevCpu * 0.70f + currentLoad * 0.30f) : (prevCpu * 0.92f + currentLoad * 0.08f);
    m_cpuUsage.store(smoothedCpu, std::memory_order_relaxed);
}

bool AudioEngine::exportRenderWav(const std::string& outputPath,
                                  bool songMode,
                                  int patternIdx,
                                  float tailSeconds,
                                  int patternLoops,
                                  AudioExportCallback callback,
                                  void* userData) {
    if (m_isExporting.exchange(true)) {
        return false; // Already exporting
    }

    bool wasPlaying = m_sequencer.isPlaying();
    stop();

    // Stop real-time audio device to avoid any race conditions during offline rendering
    if (m_device) {
        ma_device_stop(m_device.get());
    }

    // Save sequencer state
    int savedPat = m_sequencer.getCurrentPattern();
    int savedRow = m_sequencer.getCurrentRow();
    bool savedSongMode = m_sequencer.isSongMode();
    int savedSongOrderIdx = m_sequencer.getSongOrderIndex();
    bool savedLooping = m_sequencer.isLooping();

    // Calculate total frames
    int samplesPerRow = m_sequencer.getSamplesPerRow();
    int totalSongFrames = 0;

    int numPats = m_sequencer.getNumPatterns();
    if (songMode) {
        int songLen = m_sequencer.getSongOrderLength();
        for (int i = 0; i < songLen; ++i) {
            int p = m_sequencer.getSongOrderStep(i);
            int rows = (p >= 0 && p < numPats) ? m_sequencer.getPatternNumRows(p) : m_sequencer.getNumRows();
            totalSongFrames += rows * samplesPerRow;
        }
    } else {
        int targetP = (patternIdx >= 0 && patternIdx < numPats) ? patternIdx : savedPat;
        int rows = m_sequencer.getPatternNumRows(targetP);
        int loops = std::max(1, patternLoops);
        totalSongFrames = rows * samplesPerRow * loops;
    }

    int tailFrames = static_cast<int>(std::max(0.0f, tailSeconds) * m_sampleRate);
    int totalFrames = totalSongFrames + tailFrames;
    if (totalFrames <= 0) totalFrames = samplesPerRow * 16;

    // Setup sequencer for offline export
    if (songMode) {
        m_sequencer.setLooping(false);
        m_sequencer.setSongMode(true);
        m_sequencer.setSongOrderIndex(0);
        int firstPat = m_sequencer.getSongOrderStep(0);
        m_sequencer.setCurrentPattern(firstPat);
        m_sequencer.setCurrentRow(0);
    } else {
        int loops = std::max(1, patternLoops);
        m_sequencer.setLooping(loops > 1);
        m_sequencer.setSongMode(false);
        int targetP = (patternIdx >= 0 && patternIdx < numPats) ? patternIdx : savedPat;
        m_sequencer.setCurrentPattern(targetP);
        m_sequencer.setCurrentRow(0);
    }

    allNotesOff();
    m_sequencer.play();

    // Initialize miniaudio WAV encoder (32-bit float stereo WAV at native sampleRate)
    ma_encoder_config config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,
        2,
        static_cast<ma_uint32>(m_sampleRate)
    );
    ma_encoder encoder;
    ma_result res = ma_encoder_init_file(outputPath.c_str(), &config, &encoder);
    if (res != MA_SUCCESS) {
        // Restore state
        m_sequencer.setSongMode(savedSongMode);
        m_sequencer.setSongOrderIndex(savedSongOrderIdx);
        m_sequencer.setCurrentPattern(savedPat);
        m_sequencer.setCurrentRow(savedRow);
        m_sequencer.setLooping(savedLooping);
        if (m_device) ma_device_start(m_device.get());
        if (wasPlaying) play();
        m_isExporting.store(false);
        return false;
    }

    const int CHUNK_SIZE = 512;
    std::vector<float> chunk(CHUNK_SIZE * 2, 0.0f);
    int framesRendered = 0;
    bool cancelled = false;

    while (framesRendered < totalFrames && !cancelled) {
        int toRender = std::min(CHUNK_SIZE, totalFrames - framesRendered);

        // When all active pattern loops or song order steps have finished, stop sequencer so tail rings out
        if (framesRendered >= totalSongFrames && m_sequencer.isPlaying()) {
            m_sequencer.stop();
        }

        bool isPlayingNow = m_sequencer.isPlaying();
        renderProcessBlock(chunk.data(), toRender, 2, isPlayingNow, nullptr, nullptr);

        ma_uint64 written = 0;
        ma_encoder_write_pcm_frames(&encoder, chunk.data(), toRender, &written);

        framesRendered += toRender;

        if (callback && (framesRendered % (CHUNK_SIZE * 8) == 0 || framesRendered >= totalFrames)) {
            float pct = static_cast<float>(framesRendered) / static_cast<float>(totalFrames);
            if (!callback(pct, userData)) {
                cancelled = true;
            }
        }
    }

    ma_encoder_uninit(&encoder);

    // Restore state
    allNotesOff();
    m_sequencer.setSongMode(savedSongMode);
    m_sequencer.setSongOrderIndex(savedSongOrderIdx);
    m_sequencer.setCurrentPattern(savedPat);
    m_sequencer.setCurrentRow(savedRow);
    m_sequencer.setLooping(savedLooping);
    if (m_device) ma_device_start(m_device.get());
    if (wasPlaying) play();
    m_isExporting.store(false);

    if (cancelled) {
        std::remove(outputPath.c_str());
        return false;
    }

    return true;
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
// Track Insert FX Controls (VST3, CLAP)
// track == -1 targets the Master Bus
// ==========================================
bool AudioEngine::loadTrackFx(int track, int slot, const std::string& path) {
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (track == -1) {
        if (!m_masterFx[slot]) {
            m_masterFx[slot] = std::make_unique<PluginInstance>();
        }
        bool ok = m_masterFx[slot]->load(path, m_sampleRate, 256);
        if (ok) {
            int currentMax = m_masterFxMaxSlot.load(std::memory_order_relaxed);
            if (slot + 1 > currentMax) {
                m_masterFxMaxSlot.store(slot + 1, std::memory_order_release);
            }
            std::cout << "[myms] Loaded FX on Master Bus slot " << slot << ": "
                      << m_masterFx[slot]->getName() << " (" << m_masterFx[slot]->getFormatName() << ")" << std::endl;
        }
        return ok;
    }

    if (track < 0 || track >= static_cast<int>(m_trackFx.size())) return false;
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
    if (slot < 0 || slot >= MAX_TRACK_FX) return;
    if (track == -1) {
        if (m_masterFx[slot]) m_masterFx[slot]->unload();
        int maxSlot = 0;
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_masterFx[s] && m_masterFx[s]->isLoaded()) {
                maxSlot = s + 1;
            }
        }
        m_masterFxMaxSlot.store(maxSlot, std::memory_order_release);
        return;
    }

    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
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
    if (track == -1) {
        int count = 0;
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_masterFx[s] && m_masterFx[s]->isLoaded()) count++;
        }
        return count;
    }
    if (track < 0 || track >= static_cast<int>(m_trackFx.size())) return 0;
    int count = 0;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        if (m_trackFx[track][s] && m_trackFx[track][s]->isLoaded()) count++;
    }
    return count;
}

bool AudioEngine::isTrackFxLoaded(int track, int slot) const {
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (track == -1) {
        return m_masterFx[slot] && m_masterFx[slot]->isLoaded();
    }
    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
        return m_trackFx[track][slot] && m_trackFx[track][slot]->isLoaded();
    }
    return false;
}

std::string AudioEngine::getTrackFxName(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getName() : m_trackFx[track][slot]->getName();
    }
    return "";
}

std::string AudioEngine::getTrackFxVendor(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getVendor() : m_trackFx[track][slot]->getVendor();
    }
    return "";
}

std::string AudioEngine::getTrackFxPath(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getPath() : m_trackFx[track][slot]->getPath();
    }
    return "";
}

int AudioEngine::getTrackFxFormat(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return static_cast<int>((track == -1) ? m_masterFx[slot]->getFormat() : m_trackFx[track][slot]->getFormat());
    }
    return 0;
}

std::string AudioEngine::getTrackFxFormatName(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getFormatName() : m_trackFx[track][slot]->getFormatName();
    }
    return "NONE";
}

int AudioEngine::getTrackFxNumParams(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getNumParams() : m_trackFx[track][slot]->getNumParams();
    }
    return 0;
}

std::string AudioEngine::getTrackFxParamName(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        const PluginParamInfo* info = (track == -1) ? m_masterFx[slot]->getParamInfo(paramIdx) : m_trackFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->name;
    }
    return "";
}

float AudioEngine::getTrackFxParamValue(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->getParamValue(paramIdx) : m_trackFx[track][slot]->getParamValue(paramIdx);
    }
    return 0.0f;
}

void AudioEngine::setTrackFxParamValue(int track, int slot, int paramIdx, float val) {
    if (isTrackFxLoaded(track, slot)) {
        if (track == -1) m_masterFx[slot]->setParamValue(paramIdx, val);
        else m_trackFx[track][slot]->setParamValue(paramIdx, val);
    }
}

std::string AudioEngine::getTrackFxParamDisplay(int track, int slot, int paramIdx) const {
    if (isTrackFxLoaded(track, slot)) {
        const PluginParamInfo* info = (track == -1) ? m_masterFx[slot]->getParamInfo(paramIdx) : m_trackFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->display;
    }
    return "";
}

bool AudioEngine::isTrackFxParamsDirty(int track, int slot) {
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (track == -1) {
        if (m_masterFx[slot]) return m_masterFx[slot]->checkAndClearParamsDirty();
        return false;
    }
    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
        if (m_trackFx[track][slot]) return m_trackFx[track][slot]->checkAndClearParamsDirty();
    }
    return false;
}

bool AudioEngine::hasTrackFxEditor(int track, int slot) const {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->hasNativeEditor() : m_trackFx[track][slot]->hasNativeEditor();
    }
    return false;
}

bool AudioEngine::openTrackFxEditor(int track, int slot, void* parentWindow) {
    if (isTrackFxLoaded(track, slot)) {
        return (track == -1) ? m_masterFx[slot]->openNativeEditor(parentWindow) : m_trackFx[track][slot]->openNativeEditor(parentWindow);
    }
    return false;
}

void AudioEngine::closeTrackFxEditor(int track, int slot) {
    if (isTrackFxLoaded(track, slot)) {
        if (track == -1) m_masterFx[slot]->closeNativeEditor();
        else m_trackFx[track][slot]->closeNativeEditor();
    }
}

// ==========================================
// Track Pre-Instrument FX Controls (Plugins Before Instrument)
// ==========================================
bool AudioEngine::loadTrackPreFx(int track, int slot, const std::string& path) {
    if (track < 0 || track >= static_cast<int>(m_trackPreFx.size())) return false;
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (!m_trackPreFx[track][slot]) {
        m_trackPreFx[track][slot] = std::make_unique<PluginInstance>();
    }
    bool ok = m_trackPreFx[track][slot]->load(path, m_sampleRate, 256);
    if (ok) {
        if (track < MAX_INSTRUMENTS) {
            int currentMax = m_trackPreFxMaxSlot[track].load(std::memory_order_relaxed);
            if (slot + 1 > currentMax) {
                m_trackPreFxMaxSlot[track].store(slot + 1, std::memory_order_release);
            }
        }
        std::cout << "[myms] Loaded Pre-FX on track " << track << " slot " << slot << ": "
                  << m_trackPreFx[track][slot]->getName() << " (" << m_trackPreFx[track][slot]->getFormatName() << ")" << std::endl;
    }
    return ok;
}

void AudioEngine::unloadTrackPreFx(int track, int slot) {
    if (track >= 0 && track < static_cast<int>(m_trackPreFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        if (m_trackPreFx[track][slot]) m_trackPreFx[track][slot]->unload();
        if (track < MAX_INSTRUMENTS) {
            int maxSlot = 0;
            for (int s = 0; s < MAX_TRACK_FX; ++s) {
                if (m_trackPreFx[track][s] && m_trackPreFx[track][s]->isLoaded()) {
                    maxSlot = s + 1;
                }
            }
            m_trackPreFxMaxSlot[track].store(maxSlot, std::memory_order_release);
        }
    }
}

int AudioEngine::getTrackPreFxCount(int track) const {
    if (track < 0 || track >= static_cast<int>(m_trackPreFx.size())) return 0;
    int count = 0;
    for (int s = 0; s < MAX_TRACK_FX; ++s) {
        if (m_trackPreFx[track][s] && m_trackPreFx[track][s]->isLoaded()) count++;
    }
    return count;
}

bool AudioEngine::isTrackPreFxLoaded(int track, int slot) const {
    if (track >= 0 && track < static_cast<int>(m_trackPreFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        return m_trackPreFx[track][slot] && m_trackPreFx[track][slot]->isLoaded();
    }
    return false;
}

std::string AudioEngine::getTrackPreFxName(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return m_trackPreFx[track][slot]->getName();
    return "";
}

std::string AudioEngine::getTrackPreFxVendor(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return m_trackPreFx[track][slot]->getVendor();
    return "";
}

std::string AudioEngine::getTrackPreFxPath(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return m_trackPreFx[track][slot]->getPath();
    return "";
}

int AudioEngine::getTrackPreFxFormat(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return static_cast<int>(m_trackPreFx[track][slot]->getFormat());
    return 0;
}

std::string AudioEngine::getTrackPreFxFormatName(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return m_trackPreFx[track][slot]->getFormatName();
    return "NONE";
}

int AudioEngine::getTrackPreFxNumParams(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) return m_trackPreFx[track][slot]->getNumParams();
    return 0;
}

std::string AudioEngine::getTrackPreFxParamName(int track, int slot, int paramIdx) const {
    if (isTrackPreFxLoaded(track, slot)) {
        const PluginParamInfo* info = m_trackPreFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->name;
    }
    return "";
}

float AudioEngine::getTrackPreFxParamValue(int track, int slot, int paramIdx) const {
    if (isTrackPreFxLoaded(track, slot)) {
        return m_trackPreFx[track][slot]->getParamValue(paramIdx);
    }
    return 0.0f;
}

void AudioEngine::setTrackPreFxParamValue(int track, int slot, int paramIdx, float val) {
    if (isTrackPreFxLoaded(track, slot)) {
        m_trackPreFx[track][slot]->setParamValue(paramIdx, val);
    }
}

std::string AudioEngine::getTrackPreFxParamDisplay(int track, int slot, int paramIdx) const {
    if (isTrackPreFxLoaded(track, slot)) {
        const PluginParamInfo* info = m_trackPreFx[track][slot]->getParamInfo(paramIdx);
        if (info) return info->display;
    }
    return "";
}

bool AudioEngine::isTrackPreFxParamsDirty(int track, int slot) {
    if (track >= 0 && track < static_cast<int>(m_trackPreFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        if (m_trackPreFx[track][slot]) return m_trackPreFx[track][slot]->checkAndClearParamsDirty();
    }
    return false;
}

bool AudioEngine::hasTrackPreFxEditor(int track, int slot) const {
    if (isTrackPreFxLoaded(track, slot)) {
        return m_trackPreFx[track][slot]->hasNativeEditor();
    }
    return false;
}

bool AudioEngine::openTrackPreFxEditor(int track, int slot, void* parentWindow) {
    if (isTrackPreFxLoaded(track, slot)) {
        return m_trackPreFx[track][slot]->openNativeEditor(parentWindow);
    }
    return false;
}

void AudioEngine::closeTrackPreFxEditor(int track, int slot) {
    if (isTrackPreFxLoaded(track, slot)) {
        m_trackPreFx[track][slot]->closeNativeEditor();
    }
}

// Track & FX Rearranging and Bypass
bool AudioEngine::isTrackFxEnabled(int track, int slot) const {
    if (slot < 0 || slot >= MAX_TRACK_FX) return false;
    if (track == -1) {
        return m_masterFx[slot] && m_masterFx[slot]->isEnabled();
    }
    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
        return m_trackFx[track][slot] && m_trackFx[track][slot]->isEnabled();
    }
    return false;
}

void AudioEngine::setTrackFxEnabled(int track, int slot, bool enabled) {
    if (slot < 0 || slot >= MAX_TRACK_FX) return;
    if (track == -1) {
        if (m_masterFx[slot]) m_masterFx[slot]->setEnabled(enabled);
        return;
    }
    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
        if (m_trackFx[track][slot]) m_trackFx[track][slot]->setEnabled(enabled);
    }
}

bool AudioEngine::isTrackPreFxEnabled(int track, int slot) const {
    if (track >= 0 && track < static_cast<int>(m_trackPreFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        return m_trackPreFx[track][slot] && m_trackPreFx[track][slot]->isEnabled();
    }
    return false;
}

void AudioEngine::setTrackPreFxEnabled(int track, int slot, bool enabled) {
    if (track >= 0 && track < static_cast<int>(m_trackPreFx.size()) && slot >= 0 && slot < MAX_TRACK_FX) {
        if (m_trackPreFx[track][slot]) m_trackPreFx[track][slot]->setEnabled(enabled);
    }
}

bool AudioEngine::swapTrackFx(int track, int slotA, int slotB) {
    if (slotA < 0 || slotA >= MAX_TRACK_FX || slotB < 0 || slotB >= MAX_TRACK_FX || slotA == slotB) {
        return false;
    }
    if (track == -1) {
        std::swap(m_masterFx[slotA], m_masterFx[slotB]);
        int maxSlot = 0;
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_masterFx[s] && m_masterFx[s]->isLoaded()) {
                maxSlot = s + 1;
            }
        }
        m_masterFxMaxSlot.store(maxSlot, std::memory_order_release);
        return true;
    }
    if (track >= 0 && track < static_cast<int>(m_trackFx.size())) {
        std::swap(m_trackFx[track][slotA], m_trackFx[track][slotB]);
        if (track < MAX_INSTRUMENTS) {
            int maxSlot = 0;
            for (int s = 0; s < MAX_TRACK_FX; ++s) {
                if (m_trackFx[track][s] && m_trackFx[track][s]->isLoaded()) {
                    maxSlot = s + 1;
                }
            }
            m_trackFxMaxSlot[track].store(maxSlot, std::memory_order_release);
        }
        return true;
    }
    return false;
}

bool AudioEngine::swapTrackPreFx(int track, int slotA, int slotB) {
    if (track < 0 || track >= static_cast<int>(m_trackPreFx.size())) return false;
    if (slotA < 0 || slotA >= MAX_TRACK_FX || slotB < 0 || slotB >= MAX_TRACK_FX || slotA == slotB) {
        return false;
    }
    std::swap(m_trackPreFx[track][slotA], m_trackPreFx[track][slotB]);
    if (track < MAX_INSTRUMENTS) {
        int maxSlot = 0;
        for (int s = 0; s < MAX_TRACK_FX; ++s) {
            if (m_trackPreFx[track][s] && m_trackPreFx[track][s]->isLoaded()) {
                maxSlot = s + 1;
            }
        }
        m_trackPreFxMaxSlot[track].store(maxSlot, std::memory_order_release);
    }
    return true;
}

bool AudioEngine::moveTrack(int fromIdx, int toIdx) {
    int numTracks = static_cast<int>(m_instruments.size());
    if (fromIdx < 0 || fromIdx >= numTracks || toIdx < 0 || toIdx >= numTracks || fromIdx == toIdx) {
        return false;
    }
    allNotesOff();

    // 1. Move instrument synth
    SynthEngine inst = std::move(m_instruments[fromIdx]);
    m_instruments.erase(m_instruments.begin() + fromIdx);
    m_instruments.insert(m_instruments.begin() + toIdx, std::move(inst));

    // 1b. Move instrument name
    if (fromIdx < static_cast<int>(m_instrumentNames.size()) && toIdx < static_cast<int>(m_instrumentNames.size())) {
        std::string name = m_instrumentNames[fromIdx];
        m_instrumentNames.erase(m_instrumentNames.begin() + fromIdx);
        m_instrumentNames.insert(m_instrumentNames.begin() + toIdx, name);
    }

    // 2. Move track type
    if (fromIdx < static_cast<int>(m_trackTypes.size()) && toIdx < static_cast<int>(m_trackTypes.size())) {
        int tt = m_trackTypes[fromIdx];
        m_trackTypes.erase(m_trackTypes.begin() + fromIdx);
        m_trackTypes.insert(m_trackTypes.begin() + toIdx, tt);
    }

    // 3. Move track mixer state
    if (fromIdx < static_cast<int>(m_trackMixers.size()) && toIdx < static_cast<int>(m_trackMixers.size())) {
        TrackMixerState ms = m_trackMixers[fromIdx];
        m_trackMixers.erase(m_trackMixers.begin() + fromIdx);
        m_trackMixers.insert(m_trackMixers.begin() + toIdx, ms);
    }

    // 4. Move plugin instrument
    if (fromIdx < static_cast<int>(m_plugins.size()) && toIdx < static_cast<int>(m_plugins.size())) {
        auto plug = std::move(m_plugins[fromIdx]);
        m_plugins.erase(m_plugins.begin() + fromIdx);
        m_plugins.insert(m_plugins.begin() + toIdx, std::move(plug));
    }

    // 5. Move Post-FX chain
    if (fromIdx < static_cast<int>(m_trackFx.size()) && toIdx < static_cast<int>(m_trackFx.size())) {
        auto fx = std::move(m_trackFx[fromIdx]);
        m_trackFx.erase(m_trackFx.begin() + fromIdx);
        m_trackFx.insert(m_trackFx.begin() + toIdx, std::move(fx));
    }

    // 6. Move Pre-FX chain
    if (fromIdx < static_cast<int>(m_trackPreFx.size()) && toIdx < static_cast<int>(m_trackPreFx.size())) {
        auto preFx = std::move(m_trackPreFx[fromIdx]);
        m_trackPreFx.erase(m_trackPreFx.begin() + fromIdx);
        m_trackPreFx.insert(m_trackPreFx.begin() + toIdx, std::move(preFx));
    }

    // 7. Move slot max atomics and synth baseline cutoffs
    int fxMax[MAX_INSTRUMENTS];
    int preMax[MAX_INSTRUMENTS];
    float baseCutoffs[MAX_INSTRUMENTS];
    for (int i = 0; i < numTracks && i < MAX_INSTRUMENTS; ++i) {
        fxMax[i] = m_trackFxMaxSlot[i].load(std::memory_order_relaxed);
        preMax[i] = m_trackPreFxMaxSlot[i].load(std::memory_order_relaxed);
        baseCutoffs[i] = m_trackBaseCutoff[i];
    }
    int savedFx = fxMax[fromIdx];
    int savedPre = preMax[fromIdx];
    float savedCutoff = baseCutoffs[fromIdx];
    if (fromIdx < toIdx) {
        for (int i = fromIdx; i < toIdx; ++i) {
            fxMax[i] = fxMax[i + 1];
            preMax[i] = preMax[i + 1];
            baseCutoffs[i] = baseCutoffs[i + 1];
        }
    } else {
        for (int i = fromIdx; i > toIdx; --i) {
            fxMax[i] = fxMax[i - 1];
            preMax[i] = preMax[i - 1];
            baseCutoffs[i] = baseCutoffs[i - 1];
        }
    }
    fxMax[toIdx] = savedFx;
    preMax[toIdx] = savedPre;
    baseCutoffs[toIdx] = savedCutoff;
    for (int i = 0; i < numTracks && i < MAX_INSTRUMENTS; ++i) {
        m_trackFxMaxSlot[i].store(fxMax[i], std::memory_order_release);
        m_trackPreFxMaxSlot[i].store(preMax[i], std::memory_order_release);
        m_trackBaseCutoff[i] = baseCutoffs[i];
    }

    // 8. Move sequencer pattern cells & remap instrument references
    m_sequencer.moveTrack(fromIdx, toIdx);

    // 9. Sync SoundFont channel state (preset, volume, pan, mute)
    for (int t = 0; t < numTracks && t < static_cast<int>(m_trackMixers.size()); ++t) {
        m_soundfont.setChannelPreset(t, m_trackMixers[t].sf2Preset);
        m_soundfont.setChannelVolume(t, m_trackMixers[t].volume);
        m_soundfont.setChannelPan(t, m_trackMixers[t].pan);
        m_soundfont.setChannelMute(t, m_trackMixers[t].mute);
    }

    return true;
}

} // namespace myms
