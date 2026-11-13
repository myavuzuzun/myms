#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <string>

#include "vst3/vst3_c_api.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

inline float note_to_freq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

// Unique Class ID for Demo VST3 Synth
static const Steinberg_TUID DemoVst3Synth_cid = SMTG_INLINE_UID(0x3B49E710, 0x88F24C21, 0x9B15C810, 0x5A6B29C1);

class DemoVst3Synth {
public:
    // Factory implementation
    struct FactoryImpl {
        Steinberg_IPluginFactoryVtbl vtbl;
        Steinberg_IPluginFactory iface;
        std::atomic<uint32_t> refCount{1};
    } factory;

    // Component / Processor / Controller implementation
    struct PluginImpl {
        Steinberg_Vst_IComponentVtbl compVtbl;
        Steinberg_Vst_IComponent compIface;

        Steinberg_Vst_IAudioProcessorVtbl procVtbl;
        Steinberg_Vst_IAudioProcessor procIface;

        Steinberg_Vst_IEditControllerVtbl ctrlVtbl;
        Steinberg_Vst_IEditController ctrlIface;

        DemoVst3Synth* parent = nullptr;
        std::atomic<uint32_t> refCount{1};
    } plugin;

    // DSP state
    float sampleRate = 48000.0f;
    float phase = 0.0f;
    float freq = 220.0f;
    float env = 0.0f;
    float velocity = 0.0f;
    int currentNote = -1;

    // Filter state
    float s1 = 0.0f;
    float s2 = 0.0f;

    // Parameters
    // 0: Cutoff (0..1)
    // 1: Resonance (0..1)
    // 2: Volume (0..1)
    float params[3] = { 0.70f, 0.35f, 0.80f };

    DemoVst3Synth() {
        initFactory();
        initPlugin();
    }

    void initFactory() {
        factory.vtbl.queryInterface = [](void* thisInterface, const Steinberg_TUID iid, void** obj) -> Steinberg_tresult {
            if (!obj) return Steinberg_kInvalidArgument;
            if (std::memcmp(iid, Steinberg_IPluginFactory_iid, sizeof(Steinberg_TUID)) == 0 ||
                std::memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0) {
                *obj = thisInterface;
                return Steinberg_kResultOk;
            }
            *obj = nullptr;
            return Steinberg_kNoInterface;
        };
        factory.vtbl.addRef = [](void* thisInterface) -> Steinberg_uint32 {
            auto* f = reinterpret_cast<FactoryImpl*>(static_cast<char*>(thisInterface) - offsetof(FactoryImpl, iface));
            return ++f->refCount;
        };
        factory.vtbl.release = [](void* thisInterface) -> Steinberg_uint32 {
            auto* f = reinterpret_cast<FactoryImpl*>(static_cast<char*>(thisInterface) - offsetof(FactoryImpl, iface));
            return --f->refCount;
        };
        factory.vtbl.getFactoryInfo = [](void* thisInterface, Steinberg_PFactoryInfo* info) -> Steinberg_tresult {
            if (!info) return Steinberg_kInvalidArgument;
            std::strncpy(info->vendor, "myms Studios", sizeof(info->vendor) - 1);
            std::strncpy(info->url, "https://github.com/myavuzuzun/myms", sizeof(info->url) - 1);
            std::strncpy(info->email, "info@myms.local", sizeof(info->email) - 1);
            info->flags = 0;
            return Steinberg_kResultOk;
        };
        factory.vtbl.countClasses = [](void* thisInterface) -> Steinberg_int32 {
            return 1;
        };
        factory.vtbl.getClassInfo = [](void* thisInterface, Steinberg_int32 index, Steinberg_PClassInfo* info) -> Steinberg_tresult {
            if (!info || index != 0) return Steinberg_kInvalidArgument;
            std::memcpy(info->cid, DemoVst3Synth_cid, sizeof(Steinberg_TUID));
            info->cardinality = 0x7FFFFFFF;
            std::strncpy(info->category, "Audio Module", sizeof(info->category) - 1);
            std::strncpy(info->name, "Demo VST3 Synth", sizeof(info->name) - 1);
            return Steinberg_kResultOk;
        };
        factory.vtbl.createInstance = [](void* thisInterface, Steinberg_FIDString cid, Steinberg_FIDString iid, void** obj) -> Steinberg_tresult {
            if (!obj) return Steinberg_kInvalidArgument;
            if (std::memcmp(cid, DemoVst3Synth_cid, sizeof(Steinberg_TUID)) != 0) {
                *obj = nullptr;
                return Steinberg_kInvalidArgument;
            }
            auto* f = reinterpret_cast<FactoryImpl*>(static_cast<char*>(thisInterface) - offsetof(FactoryImpl, iface));
            auto* synth = reinterpret_cast<DemoVst3Synth*>(static_cast<char*>(static_cast<void*>(f)) - offsetof(DemoVst3Synth, factory));

            return synth->plugin.compVtbl.queryInterface(&synth->plugin.compIface, reinterpret_cast<const char*>(iid), obj);
        };

        factory.iface.lpVtbl = &factory.vtbl;
    }

    void initPlugin() {
        plugin.parent = this;

        // Component vtbl
        plugin.compVtbl.queryInterface = [](void* thisInterface, const Steinberg_TUID iid, void** obj) -> Steinberg_tresult {
            if (!obj) return Steinberg_kInvalidArgument;
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, compIface));

            if (std::memcmp(iid, Steinberg_Vst_IComponent_iid, sizeof(Steinberg_TUID)) == 0 ||
                std::memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0) {
                *obj = &p->compIface;
                p->refCount++;
                return Steinberg_kResultOk;
            }
            if (std::memcmp(iid, Steinberg_Vst_IAudioProcessor_iid, sizeof(Steinberg_TUID)) == 0) {
                *obj = &p->procIface;
                p->refCount++;
                return Steinberg_kResultOk;
            }
            if (std::memcmp(iid, Steinberg_Vst_IEditController_iid, sizeof(Steinberg_TUID)) == 0) {
                *obj = &p->ctrlIface;
                p->refCount++;
                return Steinberg_kResultOk;
            }
            *obj = nullptr;
            return Steinberg_kNoInterface;
        };
        plugin.compVtbl.addRef = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, compIface));
            return ++p->refCount;
        };
        plugin.compVtbl.release = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, compIface));
            return --p->refCount;
        };
        plugin.compVtbl.initialize = [](void* thisInterface, Steinberg_FUnknown* context) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.terminate = [](void* thisInterface) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.getControllerClassId = [](void* thisInterface, Steinberg_TUID classId) -> Steinberg_tresult {
            std::memcpy(classId, DemoVst3Synth_cid, sizeof(Steinberg_TUID));
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.setIoMode = [](void* thisInterface, Steinberg_Vst_IoMode mode) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.getBusCount = [](void* thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir) -> Steinberg_int32 {
            if (type == 0 /* kAudio */ && dir == 1 /* kOutput */) return 1;
            return 0;
        };
        plugin.compVtbl.getBusInfo = [](void* thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, Steinberg_Vst_BusInfo* bus) -> Steinberg_tresult {
            if (!bus) return Steinberg_kInvalidArgument;
            if (type == 0 && dir == 1 && index == 0) {
                bus->mediaType = 0;
                bus->direction = 1;
                bus->channelCount = 2; // Stereo
                bus->busType = 0; // kMain
                bus->flags = 1; // kDefaultActive
                return Steinberg_kResultOk;
            }
            return Steinberg_kResultFalse;
        };
        plugin.compVtbl.getRoutingInfo = [](void* thisInterface, Steinberg_Vst_RoutingInfo* inInfo, Steinberg_Vst_RoutingInfo* outInfo) -> Steinberg_tresult {
            return Steinberg_kResultFalse;
        };
        plugin.compVtbl.activateBus = [](void* thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, Steinberg_TBool state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.setActive = [](void* thisInterface, Steinberg_TBool state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.setState = [](void* thisInterface, Steinberg_IBStream* state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.compVtbl.getState = [](void* thisInterface, Steinberg_IBStream* state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };

        plugin.compIface.lpVtbl = &plugin.compVtbl;

        // Processor vtbl
        plugin.procVtbl.queryInterface = [](void* thisInterface, const Steinberg_TUID iid, void** obj) -> Steinberg_tresult {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, procIface));
            return p->compVtbl.queryInterface(&p->compIface, iid, obj);
        };
        plugin.procVtbl.addRef = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, procIface));
            return ++p->refCount;
        };
        plugin.procVtbl.release = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, procIface));
            return --p->refCount;
        };
        plugin.procVtbl.setBusArrangements = [](void* thisInterface, Steinberg_Vst_SpeakerArrangement* inputs, Steinberg_int32 numIns, Steinberg_Vst_SpeakerArrangement* outputs, Steinberg_int32 numOuts) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.procVtbl.getBusArrangement = [](void* thisInterface, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, Steinberg_Vst_SpeakerArrangement* arr) -> Steinberg_tresult {
            if (arr && dir == 1 && index == 0) {
                *arr = 3; // Stereo (L + R)
                return Steinberg_kResultOk;
            }
            return Steinberg_kResultFalse;
        };
        plugin.procVtbl.canProcessSampleSize = [](void* thisInterface, Steinberg_int32 symbolicSampleSize) -> Steinberg_tresult {
            return (symbolicSampleSize == 0 /* kSample32 */) ? Steinberg_kResultOk : Steinberg_kResultFalse;
        };
        plugin.procVtbl.getLatencySamples = [](void* thisInterface) -> Steinberg_uint32 {
            return 0;
        };
        plugin.procVtbl.setupProcessing = [](void* thisInterface, Steinberg_Vst_ProcessSetup* setup) -> Steinberg_tresult {
            if (!setup) return Steinberg_kInvalidArgument;
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, procIface));
            p->parent->sampleRate = static_cast<float>(setup->sampleRate);
            return Steinberg_kResultOk;
        };
        plugin.procVtbl.setProcessing = [](void* thisInterface, Steinberg_TBool state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.procVtbl.process = [](void* thisInterface, Steinberg_Vst_ProcessData* data) -> Steinberg_tresult {
            if (!data) return Steinberg_kInvalidArgument;
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, procIface));
            auto* synth = p->parent;

            // Process Input Events
            if (data->inputEvents && data->inputEvents->lpVtbl) {
                int evCount = data->inputEvents->lpVtbl->getEventCount(data->inputEvents);
                for (int i = 0; i < evCount; ++i) {
                    Steinberg_Vst_Event ev;
                    if (data->inputEvents->lpVtbl->getEvent(data->inputEvents, i, &ev) == Steinberg_kResultOk) {
                        if (ev.type == 0 /* kNoteOnEvent */ && ev.Steinberg_Vst_Event_noteOn.velocity > 0.0f) {
                            synth->currentNote = ev.Steinberg_Vst_Event_noteOn.pitch;
                            synth->freq = note_to_freq(synth->currentNote);
                            synth->velocity = ev.Steinberg_Vst_Event_noteOn.velocity;
                            synth->env = 1.0f;
                        } else if (ev.type == 1 /* kNoteOffEvent */ || (ev.type == 0 && ev.Steinberg_Vst_Event_noteOn.velocity == 0.0f)) {
                            synth->env = 0.0f;
                        }
                    }
                }
            }

            // Process Input Parameter Changes
            if (data->inputParameterChanges && data->inputParameterChanges->lpVtbl) {
                int pCount = data->inputParameterChanges->lpVtbl->getParameterCount(data->inputParameterChanges);
                for (int i = 0; i < pCount; ++i) {
                    Steinberg_Vst_IParamValueQueue* queue = data->inputParameterChanges->lpVtbl->getParameterData(data->inputParameterChanges, i);
                    if (queue && queue->lpVtbl) {
                        Steinberg_Vst_ParamID pid = queue->lpVtbl->getParameterId(queue);
                        int ptCount = queue->lpVtbl->getPointCount(queue);
                        if (ptCount > 0) {
                            Steinberg_int32 offset = 0;
                            Steinberg_Vst_ParamValue val = 0.0;
                            if (queue->lpVtbl->getPoint(queue, ptCount - 1, &offset, &val) == Steinberg_kResultOk) {
                                if (pid >= 0 && pid < 3) {
                                    synth->params[pid] = static_cast<float>(val);
                                }
                            }
                        }
                    }
                }
            }

            // Render audio
            if (data->numOutputs > 0 && data->numSamples > 0) {
                float* outL = data->outputs[0].Steinberg_Vst_AudioBusBuffers_channelBuffers32[0];
                float* outR = data->outputs[0].Steinberg_Vst_AudioBusBuffers_channelBuffers32[1];

                float cutoff = synth->params[0];
                float reso = synth->params[1];
                float volume = synth->params[2];

                float fCut = 100.0f + cutoff * cutoff * 12000.0f;
                float w0 = 2.0f * PI * fCut / synth->sampleRate;
                float q = 0.5f + reso * 8.0f;
                float alpha = std::sin(w0) / (2.0f * q);
                float b0 = (1.0f - std::cos(w0)) * 0.5f;

                float phaseInc = synth->freq / synth->sampleRate;

                for (int s = 0; s < data->numSamples; ++s) {
                    float sig = 0.0f;
                    if (synth->env > 0.0001f) {
                        sig = (synth->phase * 2.0f - 1.0f) * synth->velocity * volume;
                        synth->phase += phaseInc;
                        if (synth->phase >= 1.0f) synth->phase -= 1.0f;
                    }

                    // Low-pass filter
                    synth->s1 = synth->s1 + alpha * (sig - synth->s1 - synth->s2);
                    synth->s2 = synth->s2 + alpha * synth->s1;
                    float filtered = synth->s2;

                    outL[s] = filtered;
                    outR[s] = filtered;
                }
            }
            return Steinberg_kResultOk;
        };
        plugin.procVtbl.getTailSamples = [](void* thisInterface) -> Steinberg_uint32 {
            return 0;
        };

        plugin.procIface.lpVtbl = &plugin.procVtbl;

        // Controller vtbl
        plugin.ctrlVtbl.queryInterface = [](void* thisInterface, const Steinberg_TUID iid, void** obj) -> Steinberg_tresult {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, ctrlIface));
            return p->compVtbl.queryInterface(&p->compIface, iid, obj);
        };
        plugin.ctrlVtbl.addRef = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, ctrlIface));
            return ++p->refCount;
        };
        plugin.ctrlVtbl.release = [](void* thisInterface) -> Steinberg_uint32 {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, ctrlIface));
            return --p->refCount;
        };
        plugin.ctrlVtbl.initialize = [](void* thisInterface, Steinberg_FUnknown* context) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.terminate = [](void* thisInterface) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.setComponentState = [](void* thisInterface, Steinberg_IBStream* state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.setState = [](void* thisInterface, Steinberg_IBStream* state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.getState = [](void* thisInterface, Steinberg_IBStream* state) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.getParameterCount = [](void* thisInterface) -> Steinberg_int32 {
            return 3;
        };
        plugin.ctrlVtbl.getParameterInfo = [](void* thisInterface, Steinberg_int32 paramIndex, Steinberg_Vst_ParameterInfo* info) -> Steinberg_tresult {
            if (!info) return Steinberg_kInvalidArgument;
            std::memset(info, 0, sizeof(Steinberg_Vst_ParameterInfo));
            info->id = paramIndex;
            info->stepCount = 0; // Continuous
            info->flags = 1; // kCanAutomate

            const char* names[3] = { "Cutoff", "Resonance", "Volume" };
            float defaults[3] = { 0.70f, 0.35f, 0.80f };

            if (paramIndex >= 0 && paramIndex < 3) {
                for (int i = 0; names[paramIndex][i] != 0 && i < 127; ++i) {
                    info->title[i] = static_cast<Steinberg_Vst_TChar>(names[paramIndex][i]);
                }
                info->defaultNormalizedValue = defaults[paramIndex];
                return Steinberg_kResultOk;
            }
            return Steinberg_kInvalidArgument;
        };
        plugin.ctrlVtbl.getParamStringByValue = [](void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue valueNormalized, Steinberg_Vst_String128 string) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.getParamValueByString = [](void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_TChar* string, Steinberg_Vst_ParamValue* valueNormalized) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.normalizedParamToPlain = [](void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue valueNormalized) -> Steinberg_Vst_ParamValue {
            return valueNormalized;
        };
        plugin.ctrlVtbl.plainParamToNormalized = [](void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue plainValue) -> Steinberg_Vst_ParamValue {
            return plainValue;
        };
        plugin.ctrlVtbl.getParamNormalized = [](void* thisInterface, Steinberg_Vst_ParamID id) -> Steinberg_Vst_ParamValue {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, ctrlIface));
            if (id >= 0 && id < 3) return p->parent->params[id];
            return 0.0;
        };
        plugin.ctrlVtbl.setParamNormalized = [](void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue value) -> Steinberg_tresult {
            auto* p = reinterpret_cast<PluginImpl*>(static_cast<char*>(thisInterface) - offsetof(PluginImpl, ctrlIface));
            if (id >= 0 && id < 3) {
                p->parent->params[id] = static_cast<float>(value);
                return Steinberg_kResultOk;
            }
            return Steinberg_kInvalidArgument;
        };
        plugin.ctrlVtbl.setComponentHandler = [](void* thisInterface, Steinberg_Vst_IComponentHandler* handler) -> Steinberg_tresult {
            return Steinberg_kResultOk;
        };
        plugin.ctrlVtbl.createView = [](void* thisInterface, Steinberg_FIDString name) -> Steinberg_IPlugView* {
            return nullptr;
        };

        plugin.ctrlIface.lpVtbl = &plugin.ctrlVtbl;
    }
};

static DemoVst3Synth g_synth;

} // namespace

extern "C" __attribute__((visibility("default")))
Steinberg_IPluginFactory* GetPluginFactory() {
    return &g_synth.factory.iface;
}
