#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
#include "vst2/aeffectx.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

inline float note_to_freq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

class DemoSynthVst2 {
public:
    DemoSynthVst2(audioMasterCallback audioMaster) : m_audioMaster(audioMaster) {
        std::memset(&m_effect, 0, sizeof(AEffect));
        m_effect.magic = kEffectMagic;
        m_effect.dispatcher = dispatcherCallback;
        m_effect.process = nullptr;
        m_effect.processReplacing = processReplacingCallback;
        m_effect.setParameter = setParameterCallback;
        m_effect.getParameter = getParameterCallback;
        m_effect.numPrograms = 1;
        m_effect.numParams = 4;
        m_effect.numInputs = 0;
        m_effect.numOutputs = 2; // Stereo
        m_effect.flags = effFlagsCanReplacing | effFlagsIsSynth;
        m_effect.uniqueID = CCONST('D', 'S', 'Y', '2');
        m_effect.user = this;

        m_params[0] = 0.6f; // Cutoff (60%)
        m_params[1] = 0.3f; // Resonance (30%)
        m_params[2] = 0.4f; // Decay / Release
        m_params[3] = 0.0f; // Waveform (0 = Saw, 1 = Square)
    }

    AEffect* getAEffect() { return &m_effect; }

    static intptr_t VST_CALL_CONV dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        DemoSynthVst2* self = static_cast<DemoSynthVst2*>(effect->user);
        if (!self) return 0;
        return self->dispatcher(opcode, index, value, ptr, opt);
    }

    static void VST_CALL_CONV processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int sampleFrames) {
        DemoSynthVst2* self = static_cast<DemoSynthVst2*>(effect->user);
        if (self) self->processReplacing(inputs, outputs, sampleFrames);
    }

    static void VST_CALL_CONV setParameterCallback(AEffect* effect, int32_t index, float value) {
        DemoSynthVst2* self = static_cast<DemoSynthVst2*>(effect->user);
        if (self && index >= 0 && index < 4) {
            self->m_params[index] = std::clamp(value, 0.0f, 1.0f);
        }
    }

    static float VST_CALL_CONV getParameterCallback(AEffect* effect, int32_t index) {
        DemoSynthVst2* self = static_cast<DemoSynthVst2*>(effect->user);
        if (self && index >= 0 && index < 4) {
            return self->m_params[index];
        }
        return 0.0f;
    }

    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        switch (opcode) {
            case effOpen:
                return 1;
            case effClose:
                delete this;
                return 1;
            case effSetSampleRate:
                m_sampleRate = (opt > 8000.0f) ? opt : 48000.0f;
                return 1;
            case effSetBlockSize:
                return 1;
            case effMainsChanged:
                m_active = (value != 0);
                return 1;
            case effGetEffectName:
                if (ptr) std::strncpy((char*)ptr, "DemoSynth VST2", 32);
                return 1;
            case effGetVendorString:
                if (ptr) std::strncpy((char*)ptr, "myms Studios", 32);
                return 1;
            case effGetProductString:
                if (ptr) std::strncpy((char*)ptr, "myms Demo Synth", 32);
                return 1;
            case effGetParamName:
                if (ptr) {
                    if (index == 0) std::strncpy((char*)ptr, "Cutoff", 16);
                    else if (index == 1) std::strncpy((char*)ptr, "Resonance", 16);
                    else if (index == 2) std::strncpy((char*)ptr, "Decay", 16);
                    else if (index == 3) std::strncpy((char*)ptr, "Waveform", 16);
                }
                return 1;
            case effGetParamLabel:
                if (ptr) {
                    if (index == 0) std::strncpy((char*)ptr, "Hz", 8);
                    else if (index == 1) std::strncpy((char*)ptr, "%", 8);
                    else if (index == 2) std::strncpy((char*)ptr, "s", 8);
                    else if (index == 3) std::strncpy((char*)ptr, "type", 8);
                }
                return 1;
            case effGetParamDisplay:
                if (ptr) {
                    char buf[32];
                    if (index == 0) std::snprintf(buf, sizeof(buf), "%.0f", 20.0f + m_params[0] * 8000.0f);
                    else if (index == 1) std::snprintf(buf, sizeof(buf), "%.0f", m_params[1] * 100.0f);
                    else if (index == 2) std::snprintf(buf, sizeof(buf), "%.2f", 0.05f + m_params[2] * 2.0f);
                    else if (index == 3) std::snprintf(buf, sizeof(buf), "%s", (m_params[3] > 0.5f) ? "Square" : "Saw");
                    std::strncpy((char*)ptr, buf, 16);
                }
                return 1;
            case effProcessEvents: {
                VstEvents* evs = (VstEvents*)ptr;
                if (!evs) return 0;
                for (int32_t i = 0; i < evs->numEvents; ++i) {
                    VstMidiEvent* midi = (VstMidiEvent*)evs->events[i];
                    if (midi && midi->type == kVstMidiType) {
                        uint8_t status = (uint8_t)midi->midiData[0] & 0xF0;
                        uint8_t note = (uint8_t)midi->midiData[1];
                        uint8_t vel = (uint8_t)midi->midiData[2];
                        if (status == 0x90 && vel > 0) {
                            handleNoteOn(note, vel / 127.0f);
                        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
                            handleNoteOff(note);
                        }
                    }
                }
                return 1;
            }
            default:
                return 0;
        }
    }

    void handleNoteOn(int note, float vel) {
        m_note = note;
        m_freq = note_to_freq(note);
        m_velocity = vel;
        m_env = 1.0f;
        m_gate = true;
    }

    void handleNoteOff(int note) {
        if (m_note == note || note < 0) {
            m_gate = false;
        }
    }

    void processReplacing(float** inputs, float** outputs, int sampleFrames) {
        (void)inputs;
        float* outL = outputs[0];
        float* outR = outputs[1];

        float cutoffHz = 20.0f + m_params[0] * 8000.0f;
        float reso = std::clamp(m_params[1], 0.05f, 0.95f);
        float decayTime = std::max(0.02f, 0.05f + m_params[2] * 2.0f);
        float decayRate = 1.0f / (decayTime * m_sampleRate);
        bool isSquare = (m_params[3] > 0.5f);

        // Simple 1-pole filter coeff
        float w = 2.0f * PI * cutoffHz / m_sampleRate;
        float alpha = std::clamp(w / (1.0f + w), 0.001f, 0.999f);

        for (int i = 0; i < sampleFrames; ++i) {
            if (m_env > 0.0001f) {
                // Waveform generator
                float raw = isSquare ? ((m_phase < 0.5f) ? 0.8f : -0.8f) : (2.0f * m_phase - 1.0f);
                m_phase += m_freq / m_sampleRate;
                if (m_phase >= 1.0f) m_phase -= 1.0f;

                // Simple filter
                m_filterState += alpha * (raw - m_filterState);

                float sample = (m_filterState + (raw - m_filterState) * (1.0f - reso)) * m_env * m_velocity * 0.4f;

                if (!m_gate) {
                    m_env -= decayRate;
                    if (m_env < 0.0f) m_env = 0.0f;
                }

                outL[i] = sample;
                outR[i] = sample;
            } else {
                outL[i] = 0.0f;
                outR[i] = 0.0f;
            }
        }
    }

private:
    AEffect m_effect;
    audioMasterCallback m_audioMaster = nullptr;
    float m_sampleRate = 48000.0f;
    bool m_active = true;
    float m_params[4];

    // Synth state
    int m_note = -1;
    float m_freq = 440.0f;
    float m_velocity = 0.0f;
    float m_phase = 0.0f;
    float m_env = 0.0f;
    bool m_gate = false;
    float m_filterState = 0.0f;
};

} // namespace

extern "C" {

#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    if (!audioMaster) return nullptr;
    DemoSynthVst2* plugin = new DemoSynthVst2(audioMaster);
    return plugin->getAEffect();
}

#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
AEffect* main_plugin(audioMasterCallback audioMaster) {
    return VSTPluginMain(audioMaster);
}

} // extern "C"
