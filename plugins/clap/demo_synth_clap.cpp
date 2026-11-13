#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <atomic>
#include "clap/clap.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

inline float note_to_freq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

const clap_plugin_descriptor_t s_demo_synth_descriptor = {
    CLAP_VERSION,
    "myms.demo-synth-clap",
    "DemoSynth CLAP",
    "myms Studios",
    "https://github.com/myavuzuzun/myms",
    "",
    "",
    "1.0.0",
    "A clean subtractive synthesizer for testing CLAP host",
    (const char*[]){CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr}
};

class DemoSynthClap {
public:
    DemoSynthClap(const clap_host_t* host) : m_host(host) {
        m_plugin.desc = &s_demo_synth_descriptor;
        m_plugin.plugin_data = this;
        m_plugin.init = initCallback;
        m_plugin.destroy = destroyCallback;
        m_plugin.activate = activateCallback;
        m_plugin.deactivate = deactivateCallback;
        m_plugin.start_processing = startProcessingCallback;
        m_plugin.stop_processing = stopProcessingCallback;
        m_plugin.reset = resetCallback;
        m_plugin.process = processCallback;
        m_plugin.get_extension = getExtensionCallback;
        m_plugin.on_main_thread = onMainThreadCallback;

        m_params[0] = 0.6f; // Cutoff (60%)
        m_params[1] = 0.3f; // Resonance (30%)
        m_params[2] = 0.4f; // Decay / Release
        m_params[3] = 0.0f; // Waveform (0 = Saw, 1 = Square)
    }

    const clap_plugin_t* getClapPlugin() const { return &m_plugin; }

    static bool initCallback(const clap_plugin_t* plugin) { return true; }
    static void destroyCallback(const clap_plugin_t* plugin) {
        DemoSynthClap* self = static_cast<DemoSynthClap*>(plugin->plugin_data);
        delete self;
    }

    static bool activateCallback(const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) {
        DemoSynthClap* self = static_cast<DemoSynthClap*>(plugin->plugin_data);
        self->m_sampleRate = (sample_rate > 8000.0) ? (float)sample_rate : 48000.0f;
        return true;
    }

    static void deactivateCallback(const clap_plugin_t* plugin) {}
    static bool startProcessingCallback(const clap_plugin_t* plugin) { return true; }
    static void stopProcessingCallback(const clap_plugin_t* plugin) {}
    static void resetCallback(const clap_plugin_t* plugin) {
        DemoSynthClap* self = static_cast<DemoSynthClap*>(plugin->plugin_data);
        self->m_env = 0.0f;
        self->m_gate = false;
        self->m_filterState = 0.0f;
    }

    static void onMainThreadCallback(const clap_plugin_t* plugin) {}

    static const void* getExtensionCallback(const clap_plugin_t* plugin, const char* id) {
        if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &s_params_ext;
        if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &s_audio_ports_ext;
        if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &s_note_ports_ext;
        return nullptr;
    }

    static clap_process_status processCallback(const clap_plugin_t* plugin, const clap_process_t* process) {
        DemoSynthClap* self = static_cast<DemoSynthClap*>(plugin->plugin_data);
        return self->process(process);
    }

    clap_process_status process(const clap_process_t* process) {
        // 1. Process Input Events
        const clap_input_events_t* in_events = process->in_events;
        uint32_t num_events = in_events ? in_events->size(in_events) : 0;
        uint32_t event_index = 0;

        float* outL = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 0)
                      ? process->audio_outputs[0].data32[0] : nullptr;
        float* outR = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 1)
                      ? process->audio_outputs[0].data32[1] : nullptr;

        uint32_t num_frames = process->frames_count;

        float cutoffHz = 20.0f + m_params[0] * 8000.0f;
        float reso = std::clamp(m_params[1], 0.05f, 0.95f);
        float decayTime = std::max(0.02f, 0.05f + m_params[2] * 2.0f);
        float decayRate = 1.0f / (decayTime * m_sampleRate);
        bool isSquare = (m_params[3] > 0.5f);

        float w = 2.0f * PI * cutoffHz / m_sampleRate;
        float alpha = std::clamp(w / (1.0f + w), 0.001f, 0.999f);

        for (uint32_t f = 0; f < num_frames; ++f) {
            // Handle events up to this frame
            while (event_index < num_events) {
                const clap_event_header_t* hdr = in_events->get(in_events, event_index);
                if (hdr->time > f) break;

                if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
                    if (hdr->type == CLAP_EVENT_NOTE_ON) {
                        const clap_event_note_t* note_ev = reinterpret_cast<const clap_event_note_t*>(hdr);
                        m_note = note_ev->key;
                        m_freq = note_to_freq(note_ev->key);
                        m_velocity = (float)note_ev->velocity;
                        m_env = 1.0f;
                        m_gate = true;
                    } else if (hdr->type == CLAP_EVENT_NOTE_OFF) {
                        const clap_event_note_t* note_ev = reinterpret_cast<const clap_event_note_t*>(hdr);
                        if (note_ev->key == m_note || note_ev->key < 0) {
                            m_gate = false;
                        }
                    } else if (hdr->type == CLAP_EVENT_PARAM_VALUE) {
                        const clap_event_param_value_t* param_ev = reinterpret_cast<const clap_event_param_value_t*>(hdr);
                        if (param_ev->param_id >= 0 && param_ev->param_id < 4) {
                            m_params[param_ev->param_id] = std::clamp((float)param_ev->value, 0.0f, 1.0f);
                        }
                    }
                }
                event_index++;
            }

            if (m_env > 0.0001f) {
                float raw = isSquare ? ((m_phase < 0.5f) ? 0.8f : -0.8f) : (2.0f * m_phase - 1.0f);
                m_phase += m_freq / m_sampleRate;
                if (m_phase >= 1.0f) m_phase -= 1.0f;

                m_filterState += alpha * (raw - m_filterState);
                float sample = (m_filterState + (raw - m_filterState) * (1.0f - reso)) * m_env * m_velocity * 0.4f;

                if (!m_gate) {
                    m_env -= decayRate;
                    if (m_env < 0.0f) m_env = 0.0f;
                }

                if (outL) outL[f] = sample;
                if (outR) outR[f] = sample;
            } else {
                if (outL) outL[f] = 0.0f;
                if (outR) outR[f] = 0.0f;
            }
        }

        return (m_env > 0.0001f) ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_SLEEP;
    }

    // Parameters extension callbacks
    static uint32_t paramsCount(const clap_plugin_t* plugin) { return 4; }
    static bool paramsGetInfo(const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* info) {
        if (index >= 4 || !info) return false;
        std::memset(info, 0, sizeof(clap_param_info_t));
        info->id = index;
        info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = (index == 0) ? 0.6 : ((index == 1) ? 0.3 : ((index == 2) ? 0.4 : 0.0));

        if (index == 0) std::strncpy(info->name, "Cutoff", sizeof(info->name));
        else if (index == 1) std::strncpy(info->name, "Resonance", sizeof(info->name));
        else if (index == 2) std::strncpy(info->name, "Decay", sizeof(info->name));
        else if (index == 3) std::strncpy(info->name, "Waveform", sizeof(info->name));

        return true;
    }

    static bool paramsGetValue(const clap_plugin_t* plugin, clap_id id, double* out_value) {
        DemoSynthClap* self = static_cast<DemoSynthClap*>(plugin->plugin_data);
        if (id < 4 && out_value) {
            *out_value = self->m_params[id];
            return true;
        }
        return false;
    }

    static bool paramsValueToText(const clap_plugin_t* plugin, clap_id id, double value, char* out_buffer, uint32_t out_buffer_capacity) {
        if (!out_buffer || out_buffer_capacity < 8) return false;
        if (id == 0) std::snprintf(out_buffer, out_buffer_capacity, "%.0f Hz", 20.0 + value * 8000.0);
        else if (id == 1) std::snprintf(out_buffer, out_buffer_capacity, "%.0f %%", value * 100.0);
        else if (id == 2) std::snprintf(out_buffer, out_buffer_capacity, "%.2f s", 0.05 + value * 2.0);
        else if (id == 3) std::snprintf(out_buffer, out_buffer_capacity, "%s", (value > 0.5) ? "Square" : "Saw");
        return true;
    }

    static bool paramsTextToValue(const clap_plugin_t* plugin, clap_id id, const char* display, double* out_value) { return false; }
    static void paramsFlush(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {}

    // Audio ports extension
    static uint32_t audioPortsCount(const clap_plugin_t* plugin, bool is_input) { return is_input ? 0 : 1; }
    static bool audioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info) {
        if (is_input || index != 0 || !info) return false;
        std::memset(info, 0, sizeof(clap_audio_port_info_t));
        info->id = 0;
        std::strncpy(info->name, "Main Output", sizeof(info->name));
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;
        return true;
    }

    // Note ports extension
    static uint32_t notePortsCount(const clap_plugin_t* plugin, bool is_input) { return is_input ? 1 : 0; }
    static bool notePortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_note_port_info_t* info) {
        if (!is_input || index != 0 || !info) return false;
        std::memset(info, 0, sizeof(clap_note_port_info_t));
        info->id = 0;
        std::strncpy(info->name, "MIDI In", sizeof(info->name));
        info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        return true;
    }

private:
    clap_plugin_t m_plugin;
    const clap_host_t* m_host = nullptr;
    float m_sampleRate = 48000.0f;
    float m_params[4];

    int m_note = -1;
    float m_freq = 440.0f;
    float m_velocity = 0.0f;
    float m_phase = 0.0f;
    float m_env = 0.0f;
    bool m_gate = false;
    float m_filterState = 0.0f;

    static const clap_plugin_params_t s_params_ext;
    static const clap_plugin_audio_ports_t s_audio_ports_ext;
    static const clap_plugin_note_ports_t s_note_ports_ext;
};

const clap_plugin_params_t DemoSynthClap::s_params_ext = {
    DemoSynthClap::paramsCount,
    DemoSynthClap::paramsGetInfo,
    DemoSynthClap::paramsGetValue,
    DemoSynthClap::paramsValueToText,
    DemoSynthClap::paramsTextToValue,
    DemoSynthClap::paramsFlush
};

const clap_plugin_audio_ports_t DemoSynthClap::s_audio_ports_ext = {
    DemoSynthClap::audioPortsCount,
    DemoSynthClap::audioPortsGet
};

const clap_plugin_note_ports_t DemoSynthClap::s_note_ports_ext = {
    DemoSynthClap::notePortsCount,
    DemoSynthClap::notePortsGet
};

// Factory
uint32_t clap_plugin_factory_get_plugin_count(const clap_plugin_factory_t* factory) { return 1; }
const clap_plugin_descriptor_t* clap_plugin_factory_get_plugin_descriptor(const clap_plugin_factory_t* factory, uint32_t index) {
    return (index == 0) ? &s_demo_synth_descriptor : nullptr;
}
const clap_plugin_t* clap_plugin_factory_create_plugin(const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id) {
    if (std::strcmp(plugin_id, s_demo_synth_descriptor.id) == 0) {
        DemoSynthClap* synth = new DemoSynthClap(host);
        return synth->getClapPlugin();
    }
    return nullptr;
}

const clap_plugin_factory_t s_demo_synth_factory = {
    clap_plugin_factory_get_plugin_count,
    clap_plugin_factory_get_plugin_descriptor,
    clap_plugin_factory_create_plugin
};

bool clap_entry_init(const char* plugin_path) { return true; }
void clap_entry_deinit() {}
const void* clap_entry_get_factory(const char* factory_id) {
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &s_demo_synth_factory;
    }
    return nullptr;
}

} // namespace

extern "C" {

#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    clap_entry_init,
    clap_entry_deinit,
    clap_entry_get_factory
};

} // extern "C"

