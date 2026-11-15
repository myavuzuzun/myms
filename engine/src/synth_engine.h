#ifndef MYMS_SYNTH_ENGINE_H
#define MYMS_SYNTH_ENGINE_H

#include "synth_voice.h"
#include <vector>
#include <array>

namespace myms {

constexpr int MAX_VOICES = 16;

class SynthEngine {
public:
    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        for (int i = 0; i < MAX_VOICES; ++i) {
            m_voices[i].init(sampleRate);
        }
    }

    void setParams(const SynthParams& params) {
        m_params = params;
    }

    SynthParams& getParams() {
        return m_params;
    }

    void noteOn(int note, float velocity) {
        // Tracker monophony per track: shut off any previous playing voice on this track
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].isActive()) {
                m_voices[i].reset();
            }
        }

        // Find free voice or steal oldest
        int targetVoice = -1;
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (!m_voices[i].isActive()) {
                targetVoice = i;
                break;
            }
        }

        if (targetVoice == -1) {
            targetVoice = m_voiceStealIndex;
            m_voiceStealIndex = (m_voiceStealIndex + 1) % MAX_VOICES;
        }

        m_voices[targetVoice].noteOn(note, velocity, m_params);
    }

    void noteOff(int note = -1) {
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].isActive()) {
                if (note < 0 || note == 254 || m_voices[i].getNote() == note) {
                    m_voices[i].noteOff();
                }
            }
        }
    }

    void allNotesOff() {
        for (int i = 0; i < MAX_VOICES; ++i) {
            m_voices[i].reset();
        }
    }

    void noteOffAll() {
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].isActive()) {
                m_voices[i].noteOff();
            }
        }
    }

    void setPitchBendSemi(float semi) {
        m_pitchBendSemi = semi;
    }

    float getPitchBendSemi() const {
        return m_pitchBendSemi;
    }

    // Process a block of stereo frames (L, R interleaved or separate)
    inline void processBlock(float* outL, float* outR, int numFrames) {
        for (int f = 0; f < numFrames; ++f) {
            float sum = 0.0f;
            for (int v = 0; v < MAX_VOICES; ++v) {
                if (m_voices[v].isActive()) {
                    sum += m_voices[v].renderSample(m_params, m_pitchBendSemi);
                }
            }
            // Scale gain slightly to leave headroom for polyphony
            sum *= 0.35f;
            outL[f] += sum;
            outR[f] += sum;
        }
    }

private:
    float m_sampleRate = 48000.0f;
    std::array<SynthVoice, MAX_VOICES> m_voices;
    SynthParams m_params;
    int m_voiceStealIndex = 0;
    float m_pitchBendSemi = 0.0f;
};

} // namespace myms

#endif // MYMS_SYNTH_ENGINE_H

