#ifndef MYMS_MACRO_PROCESSOR_H
#define MYMS_MACRO_PROCESSOR_H

#include "dsp_utils.h"
#include <cmath>
#include <algorithm>

namespace myms {

struct MacroControls {
    float punch = 0.2f;
    float warmth = 0.25f;
    float tone = 0.5f;
    float space = 0.15f;
    float volume = 0.85f;
};

class MacroProcessor {
public:
    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        m_delayBufferSize = static_cast<int>(sampleRate * 0.25f);
        m_delayBufferL.assign(m_delayBufferSize, 0.0f);
        m_delayBufferR.assign(m_delayBufferSize, 0.0f);
        m_delayWritePos = 0;

        m_fastEnv = 0.0f;
        m_slowEnv = 0.0f;

        m_tiltLowL = 0.0f;
        m_tiltLowR = 0.0f;
    }

    void setControls(const MacroControls& controls) {
        m_controls = controls;
    }

    MacroControls& getControls() {
        return m_controls;
    }

    inline void processSample(float& sampleL, float& sampleR) {
        if (std::isnan(sampleL) || std::isinf(sampleL)) sampleL = 0.0f;
        if (std::isnan(sampleR) || std::isinf(sampleR)) sampleR = 0.0f;

        // 1. PUNCH (Transient Snap)
        if (m_controls.punch > 0.01f) {
            float absMax = std::max(std::abs(sampleL), std::abs(sampleR));
            m_fastEnv += 0.2f * (absMax - m_fastEnv);
            m_slowEnv += 0.005f * (absMax - m_slowEnv);

            float transient = std::max(0.0f, m_fastEnv - m_slowEnv);
            float punchGain = 1.0f + (transient * m_controls.punch * 2.0f);

            sampleL *= punchGain;
            sampleR *= punchGain;
        }

        // 2. WARMTH (Analog Saturation)
        if (m_controls.warmth > 0.01f) {
            float drive = 1.0f + m_controls.warmth * 2.2f;
            sampleL = fast_tanh(sampleL * drive);
            sampleR = fast_tanh(sampleR * drive);
        }

        // 3. TONE (Tilt EQ)
        float tiltFactor = (m_controls.tone - 0.5f) * 2.0f;
        float lpAlpha = 0.15f;
        m_tiltLowL += lpAlpha * (sampleL - m_tiltLowL);
        m_tiltLowR += lpAlpha * (sampleR - m_tiltLowR);
        float highL = sampleL - m_tiltLowL;
        float highR = sampleR - m_tiltLowR;

        if (tiltFactor > 0.0f) {
            sampleL = m_tiltLowL * (1.0f - tiltFactor * 0.35f) + highL * (1.0f + tiltFactor * 1.3f);
            sampleR = m_tiltLowR * (1.0f - tiltFactor * 0.35f) + highR * (1.0f + tiltFactor * 1.3f);
        } else {
            float negFactor = -tiltFactor;
            sampleL = m_tiltLowL * (1.0f + negFactor * 1.3f) + highL * (1.0f - negFactor * 0.35f);
            sampleR = m_tiltLowR * (1.0f + negFactor * 1.3f) + highR * (1.0f - negFactor * 0.35f);
        }

        // 4. SPACE (Ambient Diffusion Send)
        if (m_controls.space > 0.01f) {
            int delaySamplesL = static_cast<int>(m_sampleRate * 0.11f);
            int delaySamplesR = static_cast<int>(m_sampleRate * 0.16f);

            int readPosL = (m_delayWritePos - delaySamplesL + m_delayBufferSize) % m_delayBufferSize;
            int readPosR = (m_delayWritePos - delaySamplesR + m_delayBufferSize) % m_delayBufferSize;

            float wetL = m_delayBufferL[readPosL];
            float wetR = m_delayBufferR[readPosR];

            float feedback = 0.40f * m_controls.space;
            m_delayBufferL[m_delayWritePos] = sampleL + wetR * feedback;
            m_delayBufferR[m_delayWritePos] = sampleR + wetL * feedback;

            m_delayWritePos = (m_delayWritePos + 1) % m_delayBufferSize;

            float spaceWet = m_controls.space * 0.35f;
            sampleL += wetL * spaceWet;
            sampleR += wetR * spaceWet;
        }

        sampleL *= m_controls.volume;
        sampleR *= m_controls.volume;
    }

    static inline void masterBusProcess(float& masterL, float& masterR) {
        if (std::isnan(masterL) || std::isinf(masterL)) masterL = 0.0f;
        if (std::isnan(masterR) || std::isinf(masterR)) masterR = 0.0f;

        float ceiling = 0.95f;
        float peak = std::max(std::abs(masterL), std::abs(masterR));
        if (peak > 0.85f) {
            float compRatio = ceiling / (1.0f + (peak - 0.85f) * 2.0f);
            masterL *= compRatio;
            masterR *= compRatio;
        }
        masterL = std::clamp(masterL, -ceiling, ceiling);
        masterR = std::clamp(masterR, -ceiling, ceiling);
    }

private:
    float m_sampleRate = 48000.0f;
    MacroControls m_controls;

    float m_fastEnv = 0.0f;
    float m_slowEnv = 0.0f;

    float m_tiltLowL = 0.0f;
    float m_tiltLowR = 0.0f;

    int m_delayBufferSize = 48000;
    int m_delayWritePos = 0;
    std::vector<float> m_delayBufferL;
    std::vector<float> m_delayBufferR;
};

} // namespace myms

#endif // MYMS_MACRO_PROCESSOR_H
