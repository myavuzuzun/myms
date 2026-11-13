#ifndef MYMS_SYNTH_VOICE_H
#define MYMS_SYNTH_VOICE_H

#include "dsp_utils.h"
#include <cstdlib>

namespace myms {

struct SynthParams {
    Waveform osc1Wave = WAVE_SAW;
    Waveform osc2Wave = WAVE_SQUARE;
    float osc2DetuneSemi = 0.0f;
    float osc2FineCents = 7.0f;
    float oscMix = 0.5f;
    float pulseWidth = 0.5f;

    // Filter
    float filterCutoff = 2500.0f;
    float filterResonance = 0.3f;
    float filterEnvAmount = 0.5f;

    // Envelopes
    float ampAttack = 0.005f;
    float ampDecay = 0.2f;
    float ampSustain = 0.7f;
    float ampRelease = 0.25f;

    float fltAttack = 0.01f;
    float fltDecay = 0.3f;
    float fltSustain = 0.2f;
    float fltRelease = 0.2f;
};

class SynthVoice {
public:
    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        m_ampEnv.init(sampleRate);
        m_filterEnv.init(sampleRate);
        m_filter.init(sampleRate);
        reset();
    }

    void reset() {
        m_active = false;
        m_note = -1;
        m_velocity = 0.0f;
        m_phase1 = 0.0f;
        m_phase2 = 0.0f;
        m_ampEnv.reset();
        m_filterEnv.reset();
        m_filter.reset();
    }

    void noteOn(int note, float velocity, const SynthParams& params) {
        m_note = note;
        m_velocity = std::clamp(velocity, 0.0f, 1.0f);
        m_active = true;

        m_baseFreq = note_to_freq(note);
        m_phase1 = 0.0f;
        m_phase2 = 0.0f;

        m_ampEnv.setParameters(params.ampAttack, params.ampDecay, params.ampSustain, params.ampRelease);
        m_filterEnv.setParameters(params.fltAttack, params.fltDecay, params.fltSustain, params.fltRelease);

        m_ampEnv.noteOn();
        m_filterEnv.noteOn();
    }

    void noteOff() {
        m_ampEnv.noteOff();
        m_filterEnv.noteOff();
    }

    bool isActive() const {
        return m_active && m_ampEnv.isActive();
    }

    int getNote() const {
        return m_note;
    }

    inline float renderSample(const SynthParams& params) {
        if (!isActive()) {
            m_active = false;
            return 0.0f;
        }

        float ampEnvVal = m_ampEnv.process();
        float fltEnvVal = m_filterEnv.process();

        if (!m_ampEnv.isActive()) {
            m_active = false;
            return 0.0f;
        }

        float freq1 = m_baseFreq;
        float detuneFactor = std::pow(2.0f, (params.osc2DetuneSemi + params.osc2FineCents / 100.0f) / 12.0f);
        float freq2 = m_baseFreq * detuneFactor;

        float inc1 = freq1 / m_sampleRate;
        float inc2 = freq2 / m_sampleRate;

        float s1 = generateOsc(m_phase1, params.osc1Wave, params.pulseWidth);
        float s2 = generateOsc(m_phase2, params.osc2Wave, params.pulseWidth);

        m_phase1 += inc1;
        if (m_phase1 >= 1.0f) m_phase1 -= 1.0f;

        m_phase2 += inc2;
        if (m_phase2 >= 1.0f) m_phase2 -= 1.0f;

        float mixed = (1.0f - params.oscMix) * s1 + params.oscMix * s2;

        // Modulated Cutoff calculation (exponential & bounded)
        float cutoffMod = params.filterEnvAmount * fltEnvVal * 8000.0f;
        float modulatedCutoff = std::clamp(params.filterCutoff + cutoffMod, 20.0f, m_sampleRate * 0.45f);
        
        m_filter.setCutoffAndResonance(modulatedCutoff, params.filterResonance);
        float filtered = m_filter.process(mixed, StateVariableFilter::LOWPASS);

        // Safety anti-blowup check
        if (std::isnan(filtered) || std::isinf(filtered)) {
            m_filter.reset();
            filtered = 0.0f;
        }

        return filtered * ampEnvVal * m_velocity;
    }

private:
    inline float generateOsc(float phase, Waveform wave, float pw) {
        switch (wave) {
            case WAVE_SAW:
                return 2.0f * phase - 1.0f;
            case WAVE_SQUARE:
                return (phase < pw) ? 1.0f : -1.0f;
            case WAVE_TRIANGLE:
                return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            case WAVE_SINE:
                return std::sin(TWO_PI * phase);
            case WAVE_NOISE:
                return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            default:
                return 0.0f;
        }
    }

    float m_sampleRate = 48000.0f;
    bool m_active = false;
    int m_note = -1;
    float m_velocity = 0.0f;
    float m_baseFreq = 440.0f;

    float m_phase1 = 0.0f;
    float m_phase2 = 0.0f;

    ADSREnvelope m_ampEnv;
    ADSREnvelope m_filterEnv;
    StateVariableFilter m_filter;
};

} // namespace myms

#endif // MYMS_SYNTH_VOICE_H
