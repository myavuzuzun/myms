#ifndef MYMS_DSP_UTILS_H
#define MYMS_DSP_UTILS_H

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace myms {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;

inline float fast_tanh(float x) {
    if (std::isnan(x)) return 0.0f;
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float note_to_freq(int note) {
    if (note < 0 || note > 127) return 440.0f;
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

// Zero-Delay Feedback (ZDF) Topology-Preserving State Variable Filter (SVF)
// 100% unconditionally stable at all cutoff frequencies and high resonance
class StateVariableFilter {
public:
    enum Type {
        LOWPASS = 0,
        BANDPASS,
        HIGHPASS,
        NOTCH
    };

    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        reset();
        setCutoffAndResonance(1000.0f, 0.2f);
    }

    void reset() {
        m_s1 = 0.0f;
        m_s2 = 0.0f;
    }

    void setCutoffAndResonance(float cutoffHz, float resonance) {
        // Safe clamping strictly below Nyquist
        cutoffHz = std::clamp(cutoffHz, 20.0f, m_sampleRate * 0.46f);
        
        // Pre-warping factor g
        float wd = 2.0f * PI * cutoffHz;
        float T = 1.0f / m_sampleRate;
        float wa = (2.0f / T) * std::tan(wd * T * 0.5f);
        m_g = std::clamp(wa * T * 0.5f, 0.0001f, 16.0f);

        // Resonance damping R in [0.05, 1.0]
        resonance = std::clamp(resonance, 0.0f, 0.98f);
        m_R = 1.0f - resonance * 0.92f;
    }

    inline float process(float input, Type type = LOWPASS) {
        // Prevent denormals / NaNs
        if (std::isnan(m_s1) || std::isinf(m_s1)) m_s1 = 0.0f;
        if (std::isnan(m_s2) || std::isinf(m_s2)) m_s2 = 0.0f;
        if (std::isnan(input) || std::isinf(input)) input = 0.0f;

        // ZDF SVF transfer calculation
        float denom = 1.0f + 2.0f * m_R * m_g + m_g * m_g;
        float hp = (input - (2.0f * m_R + m_g) * m_s1 - m_s2) / denom;
        float bp = m_g * hp + m_s1;
        float lp = m_g * bp + m_s2;

        // State update with trapezoidal integration
        m_s1 = m_g * hp + bp;
        m_s2 = m_g * bp + lp;

        // Internal saturation protection (keeps filter singing even at extreme settings)
        m_s1 = std::clamp(m_s1, -4.0f, 4.0f);
        m_s2 = std::clamp(m_s2, -4.0f, 4.0f);

        switch (type) {
            case LOWPASS:  return lp;
            case HIGHPASS: return hp;
            case BANDPASS: return bp;
            case NOTCH:    return hp + lp;
            default:       return lp;
        }
    }

private:
    float m_sampleRate = 48000.0f;
    float m_g = 0.1f;
    float m_R = 0.5f;
    float m_s1 = 0.0f;
    float m_s2 = 0.0f;
};

// Linear/Exponential Envelope Generator (ADSR)
class ADSREnvelope {
public:
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        reset();
    }

    void reset() {
        m_state = IDLE;
        m_currentValue = 0.0f;
    }

    void setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec) {
        m_attackTime = std::max(0.001f, attackSec);
        m_decayTime = std::max(0.001f, decaySec);
        m_sustainLevel = std::clamp(sustainLevel, 0.0f, 1.0f);
        m_releaseTime = std::max(0.001f, releaseSec);

        m_attackRate = 1.0f / (m_attackTime * m_sampleRate);
        m_decayRate = (1.0f - m_sustainLevel) / (m_decayTime * m_sampleRate);
        m_releaseRate = 1.0f / (m_releaseTime * m_sampleRate);
    }

    void noteOn() {
        m_state = ATTACK;
    }

    void noteOff() {
        if (m_state != IDLE) {
            m_state = RELEASE;
        }
    }

    bool isActive() const {
        return m_state != IDLE;
    }

    inline float process() {
        switch (m_state) {
            case IDLE:
                m_currentValue = 0.0f;
                break;
            case ATTACK:
                m_currentValue += m_attackRate;
                if (m_currentValue >= 1.0f) {
                    m_currentValue = 1.0f;
                    m_state = DECAY;
                }
                break;
            case DECAY:
                m_currentValue -= m_decayRate;
                if (m_currentValue <= m_sustainLevel) {
                    m_currentValue = m_sustainLevel;
                    m_state = SUSTAIN;
                }
                break;
            case SUSTAIN:
                m_currentValue = m_sustainLevel;
                break;
            case RELEASE:
                m_currentValue -= m_releaseRate;
                if (m_currentValue <= 0.0f) {
                    m_currentValue = 0.0f;
                    m_state = IDLE;
                }
                break;
        }
        return m_currentValue;
    }

    State getState() const { return m_state; }
    float getValue() const { return m_currentValue; }

private:
    float m_sampleRate = 48000.0f;
    State m_state = IDLE;
    float m_currentValue = 0.0f;
    float m_attackTime = 0.01f;
    float m_decayTime = 0.1f;
    float m_sustainLevel = 0.7f;
    float m_releaseTime = 0.2f;

    float m_attackRate = 0.0f;
    float m_decayRate = 0.0f;
    float m_releaseRate = 0.0f;
};

enum Waveform {
    WAVE_SAW = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SINE,
    WAVE_NOISE
};

} // namespace myms

#endif // MYMS_DSP_UTILS_H
