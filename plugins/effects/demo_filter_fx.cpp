#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "clap/clap.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

const clap_plugin_descriptor_t s_demo_filter_descriptor = {
    CLAP_VERSION,
    "myms.demo-filter-fx-clap",
    "Demo Filter FX",
    "myms Studios",
    "https://github.com/myavuzuzun/myms",
    "",
    "",
    "1.0.0",
    "Stereo resonant lowpass filter with soft drive and dry/wet mix",
    (const char*[]){CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_FILTER, nullptr}
};

class DemoFilterFxClap {
public:
    DemoFilterFxClap(const clap_host_t* host) : m_host(host) {
        m_plugin.desc = &s_demo_filter_descriptor;
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

        m_params[0] = 0.75f; // Cutoff
        m_params[1] = 0.25f; // Resonance
        m_params[2] = 0.20f; // Drive
        m_params[3] = 1.00f; // Dry/Wet
    }

    const clap_plugin_t* getClapPlugin() const { return &m_plugin; }

    static bool initCallback(const clap_plugin_t* plugin) { return true; }
    static void destroyCallback(const clap_plugin_t* plugin) {
        delete static_cast<DemoFilterFxClap*>(plugin->plugin_data);
    }

    static bool activateCallback(const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) {
        auto* self = static_cast<DemoFilterFxClap*>(plugin->plugin_data);
        self->m_sampleRate = (sample_rate > 8000.0) ? static_cast<float>(sample_rate) : 48000.0f;
        return true;
    }

    static void deactivateCallback(const clap_plugin_t* plugin) {}
    static bool startProcessingCallback(const clap_plugin_t* plugin) { return true; }
    static void stopProcessingCallback(const clap_plugin_t* plugin) {}

    static void resetCallback(const clap_plugin_t* plugin) {
        auto* self = static_cast<DemoFilterFxClap*>(plugin->plugin_data);
        self->m_lpL = 0.0f;
        self->m_bpL = 0.0f;
        self->m_lpR = 0.0f;
        self->m_bpR = 0.0f;
    }

    static void onMainThreadCallback(const clap_plugin_t* plugin) {}

    static const void* getExtensionCallback(const clap_plugin_t* plugin, const char* id) {
        if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &s_params_ext;
        if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &s_audio_ports_ext;
        return nullptr;
    }

    static clap_process_status processCallback(const clap_plugin_t* plugin, const clap_process_t* process) {
        return static_cast<DemoFilterFxClap*>(plugin->plugin_data)->process(process);
    }

    clap_process_status process(const clap_process_t* process) {
        const clap_input_events_t* in_events = process->in_events;
        uint32_t num_events = in_events ? in_events->size(in_events) : 0;
        uint32_t event_index = 0;

        const float* inL = (process->audio_inputs_count > 0 && process->audio_inputs[0].channel_count > 0)
                           ? process->audio_inputs[0].data32[0] : nullptr;
        const float* inR = (process->audio_inputs_count > 0 && process->audio_inputs[0].channel_count > 1)
                           ? process->audio_inputs[0].data32[1] : inL;
        float* outL = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 0)
                      ? process->audio_outputs[0].data32[0] : nullptr;
        float* outR = (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count > 1)
                      ? process->audio_outputs[0].data32[1] : nullptr;

        uint32_t num_frames = process->frames_count;

        for (uint32_t f = 0; f < num_frames; ++f) {
            while (event_index < num_events) {
                const clap_event_header_t* hdr = in_events->get(in_events, event_index);
                if (!hdr || hdr->time > f) break;
                if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID && hdr->type == CLAP_EVENT_PARAM_VALUE) {
                    const auto* param_ev = reinterpret_cast<const clap_event_param_value_t*>(hdr);
                    if (param_ev->param_id < 4) {
                        m_params[param_ev->param_id] = std::clamp(static_cast<float>(param_ev->value), 0.0f, 1.0f);
                    }
                }
                event_index++;
            }

            float cutoffHz = 40.0f + m_params[0] * 12000.0f;
            float fCoeff = std::clamp(2.0f * std::sin(PI * cutoffHz / m_sampleRate), 0.002f, 0.85f);
            float q = std::clamp(1.0f - m_params[1] * 0.9f, 0.08f, 1.0f);
            float drive = 1.0f + m_params[2] * 4.0f;
            float wet = m_params[3];
            float dry = 1.0f - wet;

            float dryL = inL ? inL[f] : 0.0f;
            float dryR = inR ? inR[f] : 0.0f;

            float dL = std::tanh(dryL * drive);
            float hpL = dL - m_lpL - q * m_bpL;
            m_bpL += fCoeff * hpL;
            m_lpL += fCoeff * m_bpL;

            float dR = std::tanh(dryR * drive);
            float hpR = dR - m_lpR - q * m_bpR;
            m_bpR += fCoeff * hpR;
            m_lpR += fCoeff * m_bpR;

            if (outL) outL[f] = dryL * dry + m_lpL * wet;
            if (outR) outR[f] = dryR * dry + m_lpR * wet;
        }

        return CLAP_PROCESS_CONTINUE;
    }

    static uint32_t paramsCount(const clap_plugin_t* plugin) { return 4; }
    static bool paramsGetInfo(const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* info) {
        if (index >= 4 || !info) return false;
        std::memset(info, 0, sizeof(clap_param_info_t));
        info->id = index;
        info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        info->min_value = 0.0;
        info->max_value = 1.0;
        if (index == 0) {
            info->default_value = 0.75;
            std::strncpy(info->name, "Cutoff", sizeof(info->name));
        } else if (index == 1) {
            info->default_value = 0.25;
            std::strncpy(info->name, "Resonance", sizeof(info->name));
        } else if (index == 2) {
            info->default_value = 0.20;
            std::strncpy(info->name, "Drive", sizeof(info->name));
        } else {
            info->default_value = 1.0;
            std::strncpy(info->name, "Dry/Wet", sizeof(info->name));
        }
        return true;
    }

    static bool paramsGetValue(const clap_plugin_t* plugin, clap_id id, double* out_value) {
        auto* self = static_cast<DemoFilterFxClap*>(plugin->plugin_data);
        if (id < 4 && out_value) {
            *out_value = self->m_params[id];
            return true;
        }
        return false;
    }

    static bool paramsValueToText(const clap_plugin_t* plugin, clap_id id, double value, char* out_buffer, uint32_t out_buffer_capacity) {
        if (!out_buffer || out_buffer_capacity < 8) return false;
        if (id == 0) std::snprintf(out_buffer, out_buffer_capacity, "%.0f Hz", 40.0 + value * 12000.0);
        else if (id == 1) std::snprintf(out_buffer, out_buffer_capacity, "%.0f %%", value * 100.0);
        else if (id == 2) std::snprintf(out_buffer, out_buffer_capacity, "%.1f x", 1.0 + value * 4.0);
        else std::snprintf(out_buffer, out_buffer_capacity, "%.0f %%", value * 100.0);
        return true;
    }

    static bool paramsTextToValue(const clap_plugin_t* plugin, clap_id id, const char* display, double* out_value) { return false; }
    static void paramsFlush(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {
        auto* self = static_cast<DemoFilterFxClap*>(plugin->plugin_data);
        if (!in) return;
        uint32_t count = in->size(in);
        for (uint32_t i = 0; i < count; ++i) {
            const clap_event_header_t* hdr = in->get(in, i);
            if (hdr && hdr->space_id == CLAP_CORE_EVENT_SPACE_ID && hdr->type == CLAP_EVENT_PARAM_VALUE) {
                const auto* param_ev = reinterpret_cast<const clap_event_param_value_t*>(hdr);
                if (param_ev->param_id < 4) {
                    self->m_params[param_ev->param_id] = std::clamp(static_cast<float>(param_ev->value), 0.0f, 1.0f);
                }
            }
        }
    }

    static uint32_t audioPortsCount(const clap_plugin_t* plugin, bool is_input) { return 1; }
    static bool audioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info) {
        if (index != 0 || !info) return false;
        std::memset(info, 0, sizeof(clap_audio_port_info_t));
        info->id = 0;
        std::strncpy(info->name, is_input ? "Main Input" : "Main Output", sizeof(info->name));
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = 0;
        return true;
    }

private:
    clap_plugin_t m_plugin;
    const clap_host_t* m_host = nullptr;
    float m_sampleRate = 48000.0f;
    float m_params[4];
    float m_lpL = 0.0f;
    float m_bpL = 0.0f;
    float m_lpR = 0.0f;
    float m_bpR = 0.0f;

    static const clap_plugin_params_t s_params_ext;
    static const clap_plugin_audio_ports_t s_audio_ports_ext;
};

const clap_plugin_params_t DemoFilterFxClap::s_params_ext = {
    DemoFilterFxClap::paramsCount,
    DemoFilterFxClap::paramsGetInfo,
    DemoFilterFxClap::paramsGetValue,
    DemoFilterFxClap::paramsValueToText,
    DemoFilterFxClap::paramsTextToValue,
    DemoFilterFxClap::paramsFlush
};

const clap_plugin_audio_ports_t DemoFilterFxClap::s_audio_ports_ext = {
    DemoFilterFxClap::audioPortsCount,
    DemoFilterFxClap::audioPortsGet
};

uint32_t clap_plugin_factory_get_plugin_count(const clap_plugin_factory_t* factory) { return 1; }
const clap_plugin_descriptor_t* clap_plugin_factory_get_plugin_descriptor(const clap_plugin_factory_t* factory, uint32_t index) {
    return (index == 0) ? &s_demo_filter_descriptor : nullptr;
}
const clap_plugin_t* clap_plugin_factory_create_plugin(const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id) {
    if (std::strcmp(plugin_id, s_demo_filter_descriptor.id) == 0) {
        return (new DemoFilterFxClap(host))->getClapPlugin();
    }
    return nullptr;
}

const clap_plugin_factory_t s_demo_filter_factory = {
    clap_plugin_factory_get_plugin_count,
    clap_plugin_factory_get_plugin_descriptor,
    clap_plugin_factory_create_plugin
};

bool clap_entry_init(const char* plugin_path) { return true; }
void clap_entry_deinit() {}
const void* clap_entry_get_factory(const char* factory_id) {
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &s_demo_filter_factory;
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
