#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <iostream>
#include "miniaudio.h"

namespace myms {

constexpr int NUM_DRUM_PADS = 8;
constexpr int VOICES_PER_PAD = 4;

struct DrumVoice {
    bool active = false;
    double framePos = 0.0;
    double frameSpeed = 1.0;
    float currentDecay = 1.0f;
    float decayFactor = 1.0f;
    float velocity = 1.0f;
};

struct DrumPadData {
    std::string name;
    std::string filePath;
    std::vector<float> pcmData; // Stereo interleaved: [L0, R0, L1, R1, ...]
    size_t totalFrames = 0;

    std::atomic<float> pitchSemi{0.0f};  // -24 to +24 semitones
    std::atomic<float> decaySec{0.5f};   // 0.01 to 2.0 seconds
    std::atomic<float> volume{0.9f};     // 0.0 to 1.0
    std::atomic<float> pan{0.0f};        // -1.0 to +1.0
    std::atomic<bool> reverse{false};

    DrumVoice voices[VOICES_PER_PAD];
    int nextVoice = 0;
};

class DrumSampler {
public:
    DrumSampler() : m_sampleRate(48000.0f) {}

    void init(float sampleRate) {
        m_sampleRate = sampleRate;

        // Default pad names
        const char* padNames[NUM_DRUM_PADS] = {
            "Kick 808", "Snare", "Clap", "Closed Hat", 
            "Open Hat", "Low Tom", "Rimshot", "Crash"
        };

        for (int i = 0; i < NUM_DRUM_PADS; ++i) {
            m_pads[i].name = padNames[i];
            m_pads[i].filePath = "[Built-in]";
        }

        generateDefaultKits();
    }

    void generateDefaultKits() {
        for (int i = 0; i < NUM_DRUM_PADS; ++i) {
            generatePadSynthSample(i);
        }
    }

    bool loadSample(int padIdx, const std::string& filepath) {
        if (padIdx < 0 || padIdx >= NUM_DRUM_PADS) return false;

        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, static_cast<ma_uint32>(m_sampleRate));
        ma_decoder decoder;

        if (ma_decoder_init_file(filepath.c_str(), &config, &decoder) != MA_SUCCESS) {
            std::cerr << "[myms] Failed to decode audio file: " << filepath << std::endl;
            return false;
        }

        ma_uint64 totalFrames = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) != MA_SUCCESS || totalFrames == 0) {
            totalFrames = static_cast<ma_uint64>(m_sampleRate * 10); // Cap fallback at 10s
        }

        std::vector<float> pcm(totalFrames * 2);
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&decoder, pcm.data(), totalFrames, &framesRead);
        ma_decoder_uninit(&decoder);

        if (framesRead == 0) return false;
        pcm.resize(framesRead * 2);

        {
            std::lock_guard<std::mutex> lock(m_padMutex[padIdx]);
            m_pads[padIdx].pcmData = std::move(pcm);
            m_pads[padIdx].totalFrames = static_cast<size_t>(framesRead);
            m_pads[padIdx].filePath = filepath;
            
            // Extract filename without path for display
            size_t slash = filepath.find_last_of("/\\");
            m_pads[padIdx].name = (slash != std::string::npos) ? filepath.substr(slash + 1) : filepath;
        }

        std::cout << "[myms] Loaded sample into pad " << (padIdx + 1) << ": " << m_pads[padIdx].name 
                  << " (" << framesRead << " frames)" << std::endl;
        return true;
    }

    void triggerPad(int padIdx, float velocity) {
        if (padIdx < 0 || padIdx >= NUM_DRUM_PADS) return;

        DrumPadData& pad = m_pads[padIdx];
        if (pad.totalFrames == 0) return;

        int vIdx = pad.nextVoice;
        pad.nextVoice = (pad.nextVoice + 1) % VOICES_PER_PAD;

        DrumVoice& voice = pad.voices[vIdx];
        bool isRev = pad.reverse.load(std::memory_order_relaxed);
        voice.framePos = isRev ? static_cast<double>(pad.totalFrames - 1) : 0.0;

        float pitch = pad.pitchSemi.load(std::memory_order_relaxed);
        voice.frameSpeed = std::pow(2.0, pitch / 12.0);
        if (isRev) voice.frameSpeed = -voice.frameSpeed;

        float decay = pad.decaySec.load(std::memory_order_relaxed);
        float decayFrames = std::max(10.0f, decay * m_sampleRate);
        voice.decayFactor = std::pow(0.001f, 1.0f / decayFrames); // -60dB over decay time
        voice.currentDecay = 1.0f;
        voice.velocity = std::clamp(velocity, 0.0f, 1.0f);
        voice.active = true;
    }

    void stopPad(int padIdx) {
        if (padIdx < 0 || padIdx >= NUM_DRUM_PADS) return;
        for (int v = 0; v < VOICES_PER_PAD; ++v) {
            m_pads[padIdx].voices[v].active = false;
        }
    }

    void allNotesOff() {
        for (int i = 0; i < NUM_DRUM_PADS; ++i) {
            stopPad(i);
        }
    }

    // Process a block of samples (mixes directly into outStereoInterleaved)
    void processBlock(float* outStereo, int frames) {
        for (int p = 0; p < NUM_DRUM_PADS; ++p) {
            DrumPadData& pad = m_pads[p];
            if (pad.totalFrames == 0) continue;

            float padVol = pad.volume.load(std::memory_order_relaxed);
            float padPan = pad.pan.load(std::memory_order_relaxed);
            float gainL = padVol * std::min(1.0f, 1.0f - padPan);
            float gainR = padVol * std::min(1.0f, 1.0f + padPan);

            const float* pcm = pad.pcmData.data();
            size_t totalF = pad.totalFrames;

            for (int v = 0; v < VOICES_PER_PAD; ++v) {
                DrumVoice& voice = pad.voices[v];
                if (!voice.active) continue;

                for (int f = 0; f < frames; ++f) {
                    if (!voice.active) break;

                    long pos0 = static_cast<long>(voice.framePos);
                    if (pos0 < 0 || pos0 >= static_cast<long>(totalF)) {
                        voice.active = false;
                        break;
                    }

                    long pos1 = pos0 + 1;
                    if (pos1 >= static_cast<long>(totalF)) pos1 = pos0;

                    float frac = static_cast<float>(voice.framePos - pos0);

                    float sampL = pcm[pos0 * 2 + 0] * (1.0f - frac) + pcm[pos1 * 2 + 0] * frac;
                    float sampR = pcm[pos0 * 2 + 1] * (1.0f - frac) + pcm[pos1 * 2 + 1] * frac;

                    float voiceGain = voice.currentDecay * voice.velocity;
                    outStereo[f * 2 + 0] += sampL * voiceGain * gainL;
                    outStereo[f * 2 + 1] += sampR * voiceGain * gainR;

                    voice.framePos += voice.frameSpeed;
                    voice.currentDecay *= voice.decayFactor;

                    if (voice.currentDecay < 0.0005f) {
                        voice.active = false;
                    }
                }
            }
        }
    }

    // Pad Parameter Setters & Getters
    void setPadPitch(int pad, float semi) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].pitchSemi.store(std::clamp(semi, -24.0f, 24.0f));
    }
    float getPadPitch(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].pitchSemi.load() : 0.0f;
    }

    void setPadDecay(int pad, float sec) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].decaySec.store(std::clamp(sec, 0.01f, 3.0f));
    }
    float getPadDecay(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].decaySec.load() : 0.5f;
    }

    void setPadVolume(int pad, float vol) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].volume.store(std::clamp(vol, 0.0f, 1.5f));
    }
    float getPadVolume(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].volume.load() : 0.9f;
    }

    void setPadPan(int pad, float pan) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].pan.store(std::clamp(pan, -1.0f, 1.0f));
    }
    float getPadPan(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].pan.load() : 0.0f;
    }

    void setPadReverse(int pad, bool rev) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].reverse.store(rev);
    }
    bool getPadReverse(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].reverse.load() : false;
    }

    std::string getPadName(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].name : "";
    }
    void setPadName(int pad, const std::string& name) {
        if (pad >= 0 && pad < NUM_DRUM_PADS) m_pads[pad].name = name;
    }

    std::string getPadFilePath(int pad) const {
        return (pad >= 0 && pad < NUM_DRUM_PADS) ? m_pads[pad].filePath : "";
    }

private:
    float m_sampleRate = 48000.0f;
    DrumPadData m_pads[NUM_DRUM_PADS];
    std::mutex m_padMutex[NUM_DRUM_PADS];

    // High quality mathematical drum synthesis for out-of-the-box sounds
    void generatePadSynthSample(int padIdx) {
        int lengthFrames = static_cast<int>(m_sampleRate * 0.45); // ~450ms default
        std::vector<float> pcm;

        switch (padIdx) {
            case 0: { // 0: Kick 808
                lengthFrames = static_cast<int>(m_sampleRate * 0.42);
                pcm.resize(lengthFrames * 2);
                double phase = 0.0;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    double freq = 45.0 + 130.0 * std::exp(-t / 0.038);
                    phase += 2.0 * M_PI * freq / m_sampleRate;
                    double env = std::exp(-t / 0.14);
                    double click = (i < 48) ? (1.0 - i / 48.0) * 0.45 : 0.0;
                    double val = std::sin(phase) * env + click;
                    val = std::tanh(val * 1.45); // Tube style saturation
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[0].decaySec.store(0.40f);
                break;
            }
            case 1: { // 1: Snare
                lengthFrames = static_cast<int>(m_sampleRate * 0.28);
                pcm.resize(lengthFrames * 2);
                double phase = 0.0;
                uint32_t rng = 12345;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float noise = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    phase += 2.0 * M_PI * 185.0 / m_sampleRate;
                    double tone = std::sin(phase) * std::exp(-t / 0.045);
                    double snareNoise = noise * std::exp(-t / 0.095);
                    double val = tone * 0.45 + snareNoise * 0.65;
                    val = std::tanh(val * 1.2);
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[1].decaySec.store(0.25f);
                break;
            }
            case 2: { // 2: Clap
                lengthFrames = static_cast<int>(m_sampleRate * 0.32);
                pcm.resize(lengthFrames * 2);
                uint32_t rng = 889922;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float noise = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    double env = 0.0;
                    if (t < 0.012) env = std::exp(-t / 0.0035);
                    else if (t < 0.024) env = std::exp(-(t - 0.012) / 0.0035);
                    else if (t < 0.036) env = std::exp(-(t - 0.024) / 0.0035);
                    else env = std::exp(-(t - 0.036) / 0.075);
                    double val = noise * env * 0.9;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[2].decaySec.store(0.30f);
                break;
            }
            case 3: { // 3: Closed Hat
                lengthFrames = static_cast<int>(m_sampleRate * 0.06);
                pcm.resize(lengthFrames * 2);
                uint32_t rng = 445566;
                float hpMem = 0.0f;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float raw = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    float hp = raw - hpMem;
                    hpMem = raw * 0.35f + hpMem * 0.65f;
                    double env = std::exp(-t / 0.016);
                    double val = hp * env * 0.75;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[3].decaySec.store(0.05f);
                break;
            }
            case 4: { // 4: Open Hat
                lengthFrames = static_cast<int>(m_sampleRate * 0.42);
                pcm.resize(lengthFrames * 2);
                uint32_t rng = 998877;
                float hpMem = 0.0f;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float raw = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    float hp = raw - hpMem;
                    hpMem = raw * 0.38f + hpMem * 0.62f;
                    double env = std::exp(-t / 0.11);
                    double val = hp * env * 0.75;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[4].decaySec.store(0.38f);
                break;
            }
            case 5: { // 5: Low Tom
                lengthFrames = static_cast<int>(m_sampleRate * 0.32);
                pcm.resize(lengthFrames * 2);
                double phase = 0.0;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    double freq = 75.0 + 85.0 * std::exp(-t / 0.045);
                    phase += 2.0 * M_PI * freq / m_sampleRate;
                    double env = std::exp(-t / 0.11);
                    double val = std::sin(phase) * env * 0.85;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[5].decaySec.store(0.30f);
                break;
            }
            case 6: { // 6: Rimshot
                lengthFrames = static_cast<int>(m_sampleRate * 0.09);
                pcm.resize(lengthFrames * 2);
                double phase = 0.0;
                uint32_t rng = 331177;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float noise = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    phase += 2.0 * M_PI * 880.0 / m_sampleRate;
                    double env = std::exp(-t / 0.018);
                    double val = (std::sin(phase) * 0.65 + noise * 0.35) * env * 0.85;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[6].decaySec.store(0.08f);
                break;
            }
            case 7: { // 7: Crash
                lengthFrames = static_cast<int>(m_sampleRate * 1.1);
                pcm.resize(lengthFrames * 2);
                uint32_t rng = 771122;
                float bpMem1 = 0.0f, bpMem2 = 0.0f;
                for (int i = 0; i < lengthFrames; ++i) {
                    double t = static_cast<double>(i) / m_sampleRate;
                    rng = rng * 1664525 + 1013904223;
                    float raw = (static_cast<float>(rng) / 4294967296.0f) * 2.0f - 1.0f;
                    bpMem1 = raw * 0.5f + bpMem1 * 0.5f;
                    bpMem2 = (raw - bpMem1) * 0.4f + bpMem2 * 0.6f;
                    double env = std::exp(-t / 0.38);
                    double val = bpMem2 * env * 0.65;
                    pcm[i * 2 + 0] = static_cast<float>(val);
                    pcm[i * 2 + 1] = static_cast<float>(val);
                }
                m_pads[7].decaySec.store(1.0f);
                break;
            }
        }

        m_pads[padIdx].pcmData = std::move(pcm);
        m_pads[padIdx].totalFrames = lengthFrames;
    }
};

} // namespace myms
