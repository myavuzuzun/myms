#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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
    m_instrumentNames[2] = "Pluck";
    m_instrumentNames[3] = "Pad";

    // Track 1
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

    // Track 2
    m_sequencer.setCell(1, 0, 36, 2, 255, 0, 0);
    m_sequencer.setCell(1, 2, 36, 2, 180, 0, 0);
    m_sequencer.setCell(1, 6, 36, 2, 200, 0, 0);
    m_sequencer.setCell(1, 8, 39, 2, 255, 0, 0);
    m_sequencer.setCell(1, 10, 39, 2, 180, 0, 0);
    m_sequencer.setCell(1, 14, 41, 2, 220, 0, 0);
    m_sequencer.setCell(1, 16, 36, 2, 255, 0, 0);
    m_sequencer.setCell(1, 18, 36, 2, 180, 0, 0);
    m_sequencer.setCell(1, 22, 36, 2, 200, 0, 0);
    m_sequencer.setCell(1, 24, 43, 2, 255, 0, 0);
    m_sequencer.setCell(1, 28, 41, 2, 220, 0, 0);

    // Track 3
    for (int r = 0; r < 32; r += 2) {
        int notes[] = { 60, 67, 72, 75 };
        m_sequencer.setCell(2, r, notes[(r / 2) % 4], 3, 160, 0, 0);
    }

    // Track 4
    m_sequencer.setCell(3, 0, 48, 4, 180, 0, 0);
    m_sequencer.setCell(3, 16, 44, 4, 180, 0, 0);
}

bool AudioEngine::init(int sampleRate, int bufferSize) {
    if (m_initialized) return true;

    m_sampleRate = static_cast<float>(sampleRate);

    m_instruments.resize(MAX_INSTRUMENTS);
    for (size_t i = 0; i < m_instruments.size(); ++i) {
        m_instruments[i].init(m_sampleRate);
    }
    m_macros.init(m_sampleRate);
    m_sequencer.init(m_sampleRate);

    setupInitialInstruments();
    // Starts with 1 empty track and clean empty pattern (zero presets hardcoded)

    m_device = std::make_unique<ma_device>();
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = sampleRate;
    config.dataCallback      = ma_audio_callback;
    config.pUserData         = this;
    config.periodSizeInFrames = bufferSize;

    if (ma_device_init(nullptr, &config, m_device.get()) != MA_SUCCESS) {
        std::cerr << "[myms] Failed to initialize audio playback device!" << std::endl;
        m_device.reset();
        return false;
    }

    if (ma_device_start(m_device.get()) != MA_SUCCESS) {
        std::cerr << "[myms] Failed to start audio playback device!" << std::endl;
        ma_device_uninit(m_device.get());
        m_device.reset();
        return false;
    }

    m_initialized = true;
    return true;
}

void AudioEngine::shutdown() {
    if (m_initialized && m_device) {
        ma_device_uninit(m_device.get());
        m_device.reset();
        m_initialized = false;
    }
}

void AudioEngine::play() {
    m_sequencer.play();
}

void AudioEngine::pause() {
    m_sequencer.pause();
    allNotesOff();
}

void AudioEngine::stop() {
    m_sequencer.stop();
    allNotesOff();
    m_uiCurrentRow.store(0, std::memory_order_relaxed);
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
    m_sequencer.addTrack();
}

void AudioEngine::removeTrack(int trackIdx) {
    m_sequencer.removeTrack(trackIdx);
}

int AudioEngine::getNumInstruments() const {
    return static_cast<int>(m_instruments.size());
}

void AudioEngine::noteOn(int inst, int note, float velocity) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].noteOn(note, velocity);
    }
}

void AudioEngine::noteOff(int inst, int note) {
    if (inst >= 0 && inst < static_cast<int>(m_instruments.size())) {
        m_instruments[inst].noteOff(note);
    }
}

void AudioEngine::allNotesOff() {
    for (size_t i = 0; i < m_instruments.size(); ++i) {
        m_instruments[i].allNotesOff();
    }
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

void AudioEngine::setMacroPunch(float val) {
    m_macros.getControls().punch = std::clamp(val, 0.0f, 1.0f);
}

void AudioEngine::setMacroWarmth(float val) {
    m_macros.getControls().warmth = std::clamp(val, 0.0f, 1.0f);
}

void AudioEngine::setMacroTone(float val) {
    m_macros.getControls().tone = std::clamp(val, 0.0f, 1.0f);
}

void AudioEngine::setMacroSpace(float val) {
    m_macros.getControls().space = std::clamp(val, 0.0f, 1.0f);
}

void AudioEngine::setMasterVolume(float val) {
    m_macros.getControls().volume = std::clamp(val, 0.0f, 2.0f);
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
    for (int r = 0; r < m_sequencer.getNumRows(); ++r) {
        m_sequencer.setCell(track, r, NOTE_EMPTY, 0, 255, 0, 0);
    }
}

void AudioEngine::clearAllTracks() {
    int numTracks = m_sequencer.getNumTracks();
    for (int t = 0; t < numTracks; ++t) {
        clearTrack(t);
    }
}

void AudioEngine::transposeTrack(int track, int semitones) {
    for (int r = 0; r < m_sequencer.getNumRows(); ++r) {
        TrackerCell c = m_sequencer.getCell(track, r);
        if (c.note <= 127) {
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

    for (int f = 0; f < frameCount; ++f) {
        int triggeredRow = -1;
        if (m_sequencer.advanceSample(triggeredRow)) {
            m_uiCurrentRow.store(triggeredRow, std::memory_order_relaxed);

            for (int t = 0; t < numTracks; ++t) {
                TrackerCell cell = m_sequencer.getCell(t, triggeredRow);
                
                int instIdx = (cell.instrument > 0 && cell.instrument <= static_cast<int>(m_instruments.size())) 
                              ? (cell.instrument - 1) 
                              : (t % m_instruments.size());

                if (cell.note == NOTE_OFF) {
                    m_instruments[instIdx].allNotesOff();
                } else if (cell.note <= 127) {
                    float vel = (cell.volume == 255) ? 1.0f : (cell.volume / 128.0f);
                    
                    if (cell.effectType == 0x10) {
                        float cutoff = 150.0f + (cell.effectParam / 255.0f) * 10000.0f;
                        m_instruments[instIdx].getParams().filterCutoff = cutoff;
                    }
                    m_instruments[instIdx].noteOn(cell.note, vel);
                }
            }
        }

        float sL = 0.0f;
        float sR = 0.0f;
        for (size_t inst = 0; inst < m_instruments.size(); ++inst) {
            m_instruments[inst].processBlock(&sL, &sR, 1);
        }

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

    float prevPeakL = m_peakL.load(std::memory_order_relaxed);
    float prevPeakR = m_peakR.load(std::memory_order_relaxed);
    m_peakL.store(std::max(peakL, prevPeakL * 0.92f), std::memory_order_relaxed);
    m_peakR.store(std::max(peakR, prevPeakR * 0.92f), std::memory_order_relaxed);

    float rms = std::sqrt(sumSq / std::max(1, frameCount));
    m_rms.store(rms, std::memory_order_relaxed);
}

} // namespace myms
