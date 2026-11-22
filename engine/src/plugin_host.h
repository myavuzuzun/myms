#ifndef MYMS_PLUGIN_HOST_H
#define MYMS_PLUGIN_HOST_H

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>
#include <unistd.h>

#include "clap/clap.h"
#include "vst3/vst3_c_api.h"

namespace myms {

enum PluginFormat {
    PLUGIN_FORMAT_NONE = 0,
    PLUGIN_FORMAT_CLAP = 2,
    PLUGIN_FORMAT_VST3 = 3
};

struct PluginParamInfo {
    int id = 0;
    std::string name;
    std::string display;
    std::string label;
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
};

// Event structure for thread-safe lock-free audio thread event passing
struct QueuedMidiEvent {
    enum Type { NOTE_ON, NOTE_OFF, ALL_OFF, PARAM_CHANGE };
    Type type;
    int note = 0;
    float velocity = 0.0f;
    int paramId = 0;
    float paramValue = 0.0f;
};

constexpr int MAX_QUEUED_EVENTS = 64;

// Host VST3 Event List Helper
struct HostVst3EventList {
    Steinberg_Vst_IEventList iface;
    Steinberg_Vst_IEventListVtbl vtbl;
    std::array<Steinberg_Vst_Event, MAX_QUEUED_EVENTS> events;
    int count = 0;

    HostVst3EventList() {
        vtbl.queryInterface = [](void*, const Steinberg_TUID, void**) -> Steinberg_tresult { return Steinberg_kNoInterface; };
        vtbl.addRef = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.release = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.getEventCount = [](void* self) -> Steinberg_int32 {
            auto* p = reinterpret_cast<HostVst3EventList*>(self);
            return p->count;
        };
        vtbl.getEvent = [](void* self, Steinberg_int32 idx, Steinberg_Vst_Event* e) -> Steinberg_tresult {
            auto* p = reinterpret_cast<HostVst3EventList*>(self);
            if (idx >= 0 && idx < p->count) {
                *e = p->events[idx];
                return Steinberg_kResultOk;
            }
            return Steinberg_kInvalidArgument;
        };
        vtbl.addEvent = [](void* self, Steinberg_Vst_Event* e) -> Steinberg_tresult {
            auto* p = reinterpret_cast<HostVst3EventList*>(self);
            if (p->count < MAX_QUEUED_EVENTS) {
                p->events[p->count++] = *e;
                return Steinberg_kResultOk;
            }
            return Steinberg_kResultFalse;
        };
        iface.lpVtbl = &vtbl;
    }

    void clear() { count = 0; }
    void addNoteOn(int note, float velocity, int offset = 0) {
        if (count >= MAX_QUEUED_EVENTS) return;
        Steinberg_Vst_Event& ev = events[count++];
        std::memset(&ev, 0, sizeof(Steinberg_Vst_Event));
        ev.type = 0; // kNoteOnEvent
        ev.sampleOffset = offset;
        ev.Steinberg_Vst_Event_noteOn.channel = 0;
        ev.Steinberg_Vst_Event_noteOn.pitch = static_cast<Steinberg_int16>(note);
        ev.Steinberg_Vst_Event_noteOn.velocity = velocity;
        ev.Steinberg_Vst_Event_noteOn.noteId = -1;
    }
    void addNoteOff(int note, int offset = 0) {
        if (count >= MAX_QUEUED_EVENTS) return;
        Steinberg_Vst_Event& ev = events[count++];
        std::memset(&ev, 0, sizeof(Steinberg_Vst_Event));
        ev.type = 1; // kNoteOffEvent
        ev.sampleOffset = offset;
        ev.Steinberg_Vst_Event_noteOff.channel = 0;
        ev.Steinberg_Vst_Event_noteOff.pitch = static_cast<Steinberg_int16>(note);
        ev.Steinberg_Vst_Event_noteOff.velocity = 0.0f;
        ev.Steinberg_Vst_Event_noteOff.noteId = -1;
    }
};

// Host VST3 Parameter Changes Helper
struct HostVst3ParamQueue {
    Steinberg_Vst_IParamValueQueue iface;
    Steinberg_Vst_IParamValueQueueVtbl vtbl;
    Steinberg_Vst_ParamID paramId = 0;
    Steinberg_Vst_ParamValue value = 0.0;
    int pointCount = 0;

    HostVst3ParamQueue() {
        vtbl.queryInterface = [](void*, const Steinberg_TUID, void**) -> Steinberg_tresult { return Steinberg_kNoInterface; };
        vtbl.addRef = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.release = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.getParameterId = [](void* self) -> Steinberg_Vst_ParamID {
            auto* q = reinterpret_cast<HostVst3ParamQueue*>(self);
            return q->paramId;
        };
        vtbl.getPointCount = [](void* self) -> Steinberg_int32 {
            auto* q = reinterpret_cast<HostVst3ParamQueue*>(self);
            return q->pointCount;
        };
        vtbl.getPoint = [](void* self, Steinberg_int32 idx, Steinberg_int32* sampleOffset, Steinberg_Vst_ParamValue* val) -> Steinberg_tresult {
            auto* q = reinterpret_cast<HostVst3ParamQueue*>(self);
            if (idx == 0 && q->pointCount > 0) {
                if (sampleOffset) *sampleOffset = 0;
                if (val) *val = q->value;
                return Steinberg_kResultOk;
            }
            return Steinberg_kInvalidArgument;
        };
        vtbl.addPoint = [](void* self, Steinberg_int32 sampleOffset, Steinberg_Vst_ParamValue val, Steinberg_int32* idx) -> Steinberg_tresult {
            auto* q = reinterpret_cast<HostVst3ParamQueue*>(self);
            q->value = val;
            q->pointCount = 1;
            if (idx) *idx = 0;
            return Steinberg_kResultOk;
        };
        iface.lpVtbl = &vtbl;
    }
};

struct HostVst3ParameterChanges {
    Steinberg_Vst_IParameterChanges iface;
    Steinberg_Vst_IParameterChangesVtbl vtbl;
    std::array<HostVst3ParamQueue, MAX_QUEUED_EVENTS> queues;
    int count = 0;

    HostVst3ParameterChanges() {
        vtbl.queryInterface = [](void*, const Steinberg_TUID, void**) -> Steinberg_tresult { return Steinberg_kNoInterface; };
        vtbl.addRef = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.release = [](void*) -> Steinberg_uint32 { return 1; };
        vtbl.getParameterCount = [](void* self) -> Steinberg_int32 {
            auto* p = reinterpret_cast<HostVst3ParameterChanges*>(self);
            return p->count;
        };
        vtbl.getParameterData = [](void* self, Steinberg_int32 idx) -> Steinberg_Vst_IParamValueQueue* {
            auto* p = reinterpret_cast<HostVst3ParameterChanges*>(self);
            if (idx >= 0 && idx < p->count) {
                return &p->queues[idx].iface;
            }
            return nullptr;
        };
        vtbl.addParameterData = [](void* self, const Steinberg_Vst_ParamID* id, Steinberg_int32* idx) -> Steinberg_Vst_IParamValueQueue* {
            auto* p = reinterpret_cast<HostVst3ParameterChanges*>(self);
            if (p->count < MAX_QUEUED_EVENTS && id) {
                int i = p->count++;
                p->queues[i].paramId = *id;
                p->queues[i].pointCount = 0;
                if (idx) *idx = i;
                return &p->queues[i].iface;
            }
            return nullptr;
        };
        iface.lpVtbl = &vtbl;
    }

    void clear() { count = 0; }
    void addChange(Steinberg_Vst_ParamID id, double value) {
        if (count < MAX_QUEUED_EVENTS) {
            queues[count].paramId = id;
            queues[count].value = value;
            queues[count].pointCount = 1;
            count++;
        }
    }
};

class PluginInstance;

struct HostVst3ComponentHandler {
    Steinberg_Vst_IComponentHandler iface;
    Steinberg_Vst_IComponentHandlerVtbl vtbl;
    PluginInstance* parent = nullptr;

    HostVst3ComponentHandler();
};

class PluginInstance {
public:
    PluginInstance() {
        m_dummyInL.fill(0.0f);
        m_dummyInR.fill(0.0f);
        m_tempOutL.fill(0.0f);
        m_tempOutR.fill(0.0f);
        m_componentHandler.parent = this;
    }

    ~PluginInstance() {
        unload();
    }

    bool load(const std::string& path, float sampleRate = 48000.0f, int blockSize = 256) {
        unload();

        m_sampleRate = sampleRate;
        m_blockSize = blockSize;
        m_path = path;

        // Resolve bundle path if a directory or .vst3 folder is provided
        std::string resolvedPath = path;
        if (path.length() >= 5 && path.substr(path.length() - 5) == ".vst3") {
            // Check if standard Linux VST3 bundle directory structure exists
            std::string bundleLib = path + "/Contents/x86_64-linux/";
            size_t slash = path.rfind('/');
            std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
            std::string baseNoExt = (base.length() > 5) ? base.substr(0, base.length() - 5) : base;
            bundleLib += baseNoExt + ".so";
            if (access(bundleLib.c_str(), R_OK) == 0) {
                resolvedPath = bundleLib;
            }
        }

        void* handle = dlopen(resolvedPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::cerr << "[myms PluginHost] dlopen failed for " << resolvedPath << ": " << dlerror() << std::endl;
            return false;
        }

        m_handle = handle;

        // 1. Try loading as CLAP
        const clap_plugin_entry_t* clapEntry = static_cast<const clap_plugin_entry_t*>(dlsym(handle, "clap_entry"));
        if (clapEntry && clapEntry->init && clapEntry->get_factory) {
            if (initClap(clapEntry)) {
                m_format = PLUGIN_FORMAT_CLAP;
                m_isLoaded.store(true, std::memory_order_release);
                std::cout << "[myms PluginHost] Successfully loaded CLAP plugin: " << m_name 
                          << " (" << m_params.size() << " params)" << std::endl;
                return true;
            }
        }

        // 2. Try loading as VST3
        using GetPluginFactoryFunc = Steinberg_IPluginFactory* (*)();
        GetPluginFactoryFunc getFactory = reinterpret_cast<GetPluginFactoryFunc>(dlsym(handle, "GetPluginFactory"));
        if (getFactory) {
            if (initVst3(getFactory)) {
                m_format = PLUGIN_FORMAT_VST3;
                m_isLoaded.store(true, std::memory_order_release);
                std::cout << "[myms PluginHost] Successfully loaded VST3 plugin: " << m_name 
                          << " (" << m_params.size() << " params)" << std::endl;
                return true;
            }
        }

        std::cerr << "[myms PluginHost] File is not a recognized Linux VST3 or CLAP plugin: " << path << std::endl;
        dlclose(m_handle);
        m_handle = nullptr;
        return false;
    }

    void unload() {
        if (!m_isLoaded.load(std::memory_order_acquire)) return;

        m_isLoaded.store(false, std::memory_order_release);
        closeNativeEditor();

        if (m_format == PLUGIN_FORMAT_CLAP) {
            if (m_clapPlugin) {
                m_clapPlugin->stop_processing(m_clapPlugin);
                m_clapPlugin->deactivate(m_clapPlugin);
                m_clapPlugin->destroy(m_clapPlugin);
                m_clapPlugin = nullptr;
            }
            if (m_clapEntry && m_clapEntry->deinit) {
                m_clapEntry->deinit();
                m_clapEntry = nullptr;
            }
        } else if (m_format == PLUGIN_FORMAT_VST3) {
            if (m_vst3Processor) {
                m_vst3Processor->lpVtbl->setProcessing(m_vst3Processor, 0);
                m_vst3Processor->lpVtbl->release(m_vst3Processor);
                m_vst3Processor = nullptr;
            }
            if (m_vst3Controller) {
                m_vst3Controller->lpVtbl->terminate(m_vst3Controller);
                m_vst3Controller->lpVtbl->release(m_vst3Controller);
                m_vst3Controller = nullptr;
            }
            if (m_vst3Component) {
                m_vst3Component->lpVtbl->setActive(m_vst3Component, 0);
                m_vst3Component->lpVtbl->terminate(m_vst3Component);
                m_vst3Component->lpVtbl->release(m_vst3Component);
                m_vst3Component = nullptr;
            }
            if (m_vst3Factory) {
                m_vst3Factory->lpVtbl->release(m_vst3Factory);
                m_vst3Factory = nullptr;
            }
        }

        if (m_handle) {
            dlclose(m_handle);
            m_handle = nullptr;
        }

        m_format = PLUGIN_FORMAT_NONE;
        m_hasEditor = false;
        m_isEnabled.store(true, std::memory_order_release);
        m_name.clear();
        m_vendor.clear();
        m_path.clear();
        m_params.clear();
        m_lastNote = -1;
    }

    bool isLoaded() const {
        return m_isLoaded.load(std::memory_order_acquire);
    }

    bool isEnabled() const {
        return m_isEnabled.load(std::memory_order_relaxed);
    }

    void setEnabled(bool enabled) {
        m_isEnabled.store(enabled, std::memory_order_release);
    }

    PluginFormat getFormat() const { return m_format; }
    std::string getFormatName() const {
        switch (m_format) {
            case PLUGIN_FORMAT_VST3: return "VST3";
            case PLUGIN_FORMAT_CLAP: return "CLAP";
            default: return "NONE";
        }
    }
    const std::string& getName() const { return m_name; }
    const std::string& getVendor() const { return m_vendor; }
    const std::string& getPath() const { return m_path; }

    int getNumParams() const { return static_cast<int>(m_params.size()); }
    const PluginParamInfo* getParamInfo(int index) const {
        if (index >= 0 && index < static_cast<int>(m_params.size())) {
            return &m_params[index];
        }
        return nullptr;
    }

    float getParamValue(int index) const {
        if (index >= 0 && index < static_cast<int>(m_params.size())) {
            return m_params[index].value;
        }
        return 0.0f;
    }

    void setParamValue(int index, float val) {
        if (index < 0 || index >= static_cast<int>(m_params.size())) return;
        val = std::clamp(val, 0.0f, 1.0f);
        m_params[index].value = val;
        int paramId = m_params[index].id;

        if (m_format == PLUGIN_FORMAT_CLAP) {
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::PARAM_CHANGE, 0, 0.0f, paramId, val});
        } else if (m_format == PLUGIN_FORMAT_VST3) {
            if (m_vst3Controller) {
                m_vst3Controller->lpVtbl->setParamNormalized(m_vst3Controller, paramId, val);
            }
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::PARAM_CHANGE, 0, 0.0f, paramId, val});
        }
    }

    void onPluginParamAutomated(int indexOrId, float val) {
        int idx = -1;
        if (indexOrId >= 0 && indexOrId < static_cast<int>(m_params.size()) && m_params[indexOrId].id == indexOrId) {
            idx = indexOrId;
        } else {
            for (size_t i = 0; i < m_params.size(); ++i) {
                if (m_params[i].id == indexOrId) {
                    idx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (idx >= 0 && idx < static_cast<int>(m_params.size())) {
            m_params[idx].value = std::clamp(val, 0.0f, 1.0f);
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::PARAM_CHANGE, 0, 0.0f, m_params[idx].id, m_params[idx].value});
            m_paramsDirty.store(true, std::memory_order_release);
        }
    }

    bool checkAndClearParamsDirty() {
        return m_paramsDirty.exchange(false, std::memory_order_acq_rel);
    }

    // Tracker Monophony: any new note cuts previous note
    void noteOn(int note, float velocity) {
        if (!isLoaded()) return;
        
        // Monophonic cutoff of previous note
        if (m_lastNote >= 0) {
            noteOff(m_lastNote);
        }

        m_lastNote = note;
        queueEvent(QueuedMidiEvent{QueuedMidiEvent::NOTE_ON, note, velocity, 0, 0.0f});
    }

    void noteOff(int note = -1) {
        if (!isLoaded()) return;
        if (note < 0 || note == 254) {
            note = m_lastNote;
        }
        if (note >= 0) {
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::NOTE_OFF, note, 0.0f, 0, 0.0f});
            if (note == m_lastNote) {
                m_lastNote = -1;
            }
        }
    }

    void allNotesOff() {
        if (!isLoaded()) return;
        if (m_lastNote >= 0) {
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::NOTE_OFF, m_lastNote, 0.0f, 0, 0.0f});
            m_lastNote = -1;
        }
        queueEvent(QueuedMidiEvent{QueuedMidiEvent::ALL_OFF, 0, 0.0f, 0, 0.0f});
        m_bufferAvailable = 0;
        m_bufferReadIndex = 0;
        std::fill(m_tempOutL.begin(), m_tempOutL.end(), 0.0f);
        std::fill(m_tempOutR.begin(), m_tempOutR.end(), 0.0f);
    }

    // Instrument Audio Rendering (sample-by-sample for tracker integration)
    inline void processSample(float& outL, float& outR) {
        if (!m_isLoaded.load(std::memory_order_relaxed) || !m_isEnabled.load(std::memory_order_relaxed)) return;

        if (m_bufferReadIndex >= m_bufferAvailable) {
            renderAudioBlock(m_blockSize);
            m_bufferReadIndex = 0;
        }

        if (m_bufferReadIndex < m_bufferAvailable) {
            outL += m_tempOutL[m_bufferReadIndex];
            outR += m_tempOutR[m_bufferReadIndex];
            m_bufferReadIndex++;
        }
    }

    // Insert Audio FX Processing (Takes audio input and modifies in-place)
    void processEffectBlock(float* inL, float* inR, float* outL, float* outR, int numFrames) {
        if (!m_isLoaded.load(std::memory_order_relaxed) || !m_isEnabled.load(std::memory_order_relaxed)) {
            if (inL != outL) {
                std::memcpy(outL, inL, numFrames * sizeof(float));
                std::memcpy(outR, inR, numFrames * sizeof(float));
            }
            return;
        }

        if (m_format == PLUGIN_FORMAT_CLAP && m_clapPlugin && m_clapPlugin->process) {
            ClapEventBuffer evBuf;
            int head = m_eventQueueHead.load(std::memory_order_relaxed);
            int tail = m_eventQueueTail.load(std::memory_order_acquire);
            while (head != tail && evBuf.count < MAX_QUEUED_EVENTS) {
                const QueuedMidiEvent& q = m_eventQueue[head];
                head = (head + 1) % MAX_QUEUED_EVENTS;
                if (q.type == QueuedMidiEvent::PARAM_CHANGE) {
                    clap_event_param_value_t& p = evBuf.params[evBuf.count];
                    p.header.size = sizeof(clap_event_param_value_t);
                    p.header.time = 0;
                    p.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    p.header.type = CLAP_EVENT_PARAM_VALUE;
                    p.header.flags = 0;
                    p.param_id = q.paramId;
                    p.cookie = nullptr;
                    p.note_id = -1;
                    p.port_index = 0;
                    p.channel = 0;
                    p.key = -1;
                    p.value = q.paramValue;
                    evBuf.headers[evBuf.count] = &p.header;
                    evBuf.count++;
                }
            }
            m_eventQueueHead.store(head, std::memory_order_release);

            clap_input_events_t in_events;
            in_events.ctx = &evBuf;
            in_events.size = clapInEventsSize;
            in_events.get = clapInEventsGet;

            clap_output_events_t out_events;
            out_events.ctx = nullptr;
            out_events.try_push = clapOutEventsTryPush;

            float* inChs[2] = { inL, inR };
            float* outChs[2] = { outL, outR };
            clap_audio_buffer_t in_buf;
            in_buf.data32 = inChs; in_buf.data64 = nullptr; in_buf.channel_count = 2; in_buf.latency = 0; in_buf.constant_mask = 0;
            clap_audio_buffer_t out_buf;
            out_buf.data32 = outChs; out_buf.data64 = nullptr; out_buf.channel_count = 2; out_buf.latency = 0; out_buf.constant_mask = 0;
            clap_process_t process;
            std::memset(&process, 0, sizeof(process));
            process.steady_time = -1;
            process.frames_count = numFrames;
            process.audio_inputs = &in_buf;
            process.audio_outputs = &out_buf;
            process.audio_inputs_count = 1;
            process.audio_outputs_count = 1;
            process.in_events = &in_events;
            process.out_events = &out_events;
            m_clapPlugin->process(m_clapPlugin, &process);
        } else if (m_format == PLUGIN_FORMAT_VST3 && m_vst3Processor) {
            HostVst3ParameterChanges paramChanges;
            int head = m_eventQueueHead.load(std::memory_order_relaxed);
            int tail = m_eventQueueTail.load(std::memory_order_acquire);
            while (head != tail) {
                const QueuedMidiEvent& q = m_eventQueue[head];
                head = (head + 1) % MAX_QUEUED_EVENTS;
                if (q.type == QueuedMidiEvent::PARAM_CHANGE) {
                    paramChanges.addChange(q.paramId, q.paramValue);
                    if (m_vst3Controller) {
                        m_vst3Controller->lpVtbl->setParamNormalized(m_vst3Controller, q.paramId, q.paramValue);
                    }
                }
            }
            m_eventQueueHead.store(head, std::memory_order_release);

            float* inChs[2] = { inL, inR };
            float* outChs[2] = { outL, outR };
            Steinberg_Vst_AudioBusBuffers inBus;
            inBus.numChannels = 2; inBus.silenceFlags = 0;
            inBus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = inChs;
            Steinberg_Vst_AudioBusBuffers outBus;
            outBus.numChannels = 2; outBus.silenceFlags = 0;
            outBus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = outChs;
            Steinberg_Vst_ProcessData pData;
            std::memset(&pData, 0, sizeof(pData));
            pData.processMode = 0;
            pData.symbolicSampleSize = 0;
            pData.numSamples = numFrames;
            pData.numInputs = 1;
            pData.numOutputs = 1;
            pData.inputs = &inBus;
            pData.outputs = &outBus;
            pData.inputParameterChanges = reinterpret_cast<Steinberg_Vst_IParameterChanges*>(&paramChanges);
            m_vst3Processor->lpVtbl->process(m_vst3Processor, &pData);
        }
    }

    inline void processEffectSample(float& ioL, float& ioR) {
        if (!m_isLoaded.load(std::memory_order_relaxed) || !m_isEnabled.load(std::memory_order_relaxed)) return;
        float inL[1] = { ioL };
        float inR[1] = { ioR };
        float outL[1] = { 0.0f };
        float outR[1] = { 0.0f };
        processEffectBlock(inL, inR, outL, outR, 1);
        ioL = outL[0];
        ioR = outR[0];
    }

    // Native GUI / Editor
    bool hasNativeEditor() const {
        return m_hasEditor;
    }

    bool openNativeEditor(void* parentWindow) {
        if (!hasNativeEditor() || !parentWindow) return false;

        if (m_format == PLUGIN_FORMAT_CLAP && m_clapPlugin) {
            const clap_plugin_gui_t* gui = static_cast<const clap_plugin_gui_t*>(
                m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_GUI));
            if (gui && gui->is_api_supported && gui->is_api_supported(m_clapPlugin, CLAP_WINDOW_API_X11, false)) {
                if (gui->create(m_clapPlugin, CLAP_WINDOW_API_X11, false)) {
                    clap_window_t win;
                    win.api = CLAP_WINDOW_API_X11;
                    win.x11 = reinterpret_cast<clap_xwnd>(parentWindow);
                    if (gui->set_parent(m_clapPlugin, &win)) {
                        return gui->show(m_clapPlugin);
                    }
                }
            }
        }
        if (m_format == PLUGIN_FORMAT_VST3 && m_vst3Controller) {
            m_vst3View = m_vst3Controller->lpVtbl->createView(m_vst3Controller, "editor");
            if (m_vst3View && m_vst3View->lpVtbl) {
                if (m_vst3View->lpVtbl->isPlatformTypeSupported(m_vst3View, "X11EmbedWindowID") == Steinberg_kResultOk) {
                    if (m_vst3View->lpVtbl->attached(m_vst3View, parentWindow, "X11EmbedWindowID") == Steinberg_kResultOk) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void closeNativeEditor() {
        if (m_format == PLUGIN_FORMAT_CLAP && m_clapPlugin) {
            const clap_plugin_gui_t* gui = static_cast<const clap_plugin_gui_t*>(
                m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_GUI));
            if (gui && gui->destroy) {
                gui->destroy(m_clapPlugin);
            }
        } else if (m_format == PLUGIN_FORMAT_VST3 && m_vst3View) {
            m_vst3View->lpVtbl->removed(m_vst3View);
            m_vst3View->lpVtbl->release(m_vst3View);
            m_vst3View = nullptr;
        }
    }

private:
    void queueEvent(const QueuedMidiEvent& ev) {
        int tail = m_eventQueueTail.load(std::memory_order_relaxed);
        int nextTail = (tail + 1) % MAX_QUEUED_EVENTS;
        if (nextTail != m_eventQueueHead.load(std::memory_order_acquire)) {
            m_eventQueue[tail] = ev;
            m_eventQueueTail.store(nextTail, std::memory_order_release);
        }
    }

    // ==========================================
    // VST3 INITIALIZATION & PROCESSING
    // ==========================================
    bool initVst3(Steinberg_IPluginFactory* (*getFactory)()) {
        Steinberg_IPluginFactory* factory = getFactory();
        if (!factory || !factory->lpVtbl) return false;

        m_vst3Factory = factory;
        factory->lpVtbl->addRef(factory);

        // Optional ModuleEntry
        using ModuleEntryFunc = bool (*)(void*);
        ModuleEntryFunc modEntry = reinterpret_cast<ModuleEntryFunc>(dlsym(m_handle, "ModuleEntry"));
        if (modEntry) modEntry(m_handle);

        int numClasses = factory->lpVtbl->countClasses(factory);
        Steinberg_TUID effectCid;
        bool foundClass = false;

        for (int i = 0; i < numClasses; ++i) {
            Steinberg_PClassInfo info;
            if (factory->lpVtbl->getClassInfo(factory, i, &info) == Steinberg_kResultOk) {
                if (std::strcmp(info.category, "Audio Module") == 0 ||
                    std::strcmp(info.category, "Audio Module Class") == 0 ||
                    i == 0) {
                    std::memcpy(effectCid, info.cid, sizeof(Steinberg_TUID));
                    m_name = info.name;
                    foundClass = true;
                    if (std::strcmp(info.category, "Audio Module") == 0) break;
                }
            }
        }
        if (!foundClass) return false;
        if (m_name.empty()) m_name = "VST3 Plugin";
        m_vendor = "VST3";

        // Create component
        void* compPtr = nullptr;
        if (factory->lpVtbl->createInstance(factory, effectCid, Steinberg_Vst_IComponent_iid, &compPtr) != Steinberg_kResultOk || !compPtr) {
            return false;
        }
        m_vst3Component = static_cast<Steinberg_Vst_IComponent*>(compPtr);
        m_vst3Component->lpVtbl->initialize(m_vst3Component, nullptr);

        // Query audio processor
        void* procPtr = nullptr;
        if (m_vst3Component->lpVtbl->queryInterface(m_vst3Component, Steinberg_Vst_IAudioProcessor_iid, &procPtr) == Steinberg_kResultOk && procPtr) {
            m_vst3Processor = static_cast<Steinberg_Vst_IAudioProcessor*>(procPtr);
            
            Steinberg_Vst_ProcessSetup setup;
            setup.processMode = 0; // kRealtime
            setup.symbolicSampleSize = 0; // kSample32
            setup.maxSamplesPerBlock = m_blockSize;
            setup.sampleRate = m_sampleRate;
            m_vst3Processor->lpVtbl->setupProcessing(m_vst3Processor, &setup);
            m_vst3Component->lpVtbl->setActive(m_vst3Component, 1);
            m_vst3Processor->lpVtbl->setProcessing(m_vst3Processor, 1);
        }

        // Controller
        void* ctrlPtr = nullptr;
        if (m_vst3Component->lpVtbl->queryInterface(m_vst3Component, Steinberg_Vst_IEditController_iid, &ctrlPtr) == Steinberg_kResultOk && ctrlPtr) {
            m_vst3Controller = static_cast<Steinberg_Vst_IEditController*>(ctrlPtr);
        } else {
            Steinberg_TUID ctrlCid;
            if (m_vst3Component->lpVtbl->getControllerClassId(m_vst3Component, ctrlCid) == Steinberg_kResultOk) {
                if (factory->lpVtbl->createInstance(factory, ctrlCid, Steinberg_Vst_IEditController_iid, &ctrlPtr) == Steinberg_kResultOk && ctrlPtr) {
                    m_vst3Controller = static_cast<Steinberg_Vst_IEditController*>(ctrlPtr);
                    m_vst3Controller->lpVtbl->initialize(m_vst3Controller, nullptr);
                }
            }
        }

        // Retrieve parameters
        if (m_vst3Controller) {
            m_componentHandler.parent = this;
            if (m_vst3Controller->lpVtbl->setComponentHandler) {
                m_vst3Controller->lpVtbl->setComponentHandler(m_vst3Controller, reinterpret_cast<Steinberg_Vst_IComponentHandler*>(&m_componentHandler.iface));
            }

            int pCount = m_vst3Controller->lpVtbl->getParameterCount(m_vst3Controller);
            m_params.resize(pCount);
            for (int i = 0; i < pCount; ++i) {
                Steinberg_Vst_ParameterInfo pInfo;
                if (m_vst3Controller->lpVtbl->getParameterInfo(m_vst3Controller, i, &pInfo) == Steinberg_kResultOk) {
                    m_params[i].id = pInfo.id;
                    std::string pTitle;
                    for (int k = 0; k < 128 && pInfo.title[k] != 0; ++k) {
                        pTitle += static_cast<char>(pInfo.title[k]);
                    }
                    m_params[i].name = pTitle.empty() ? ("Param " + std::to_string(i)) : pTitle;
                    m_params[i].defaultValue = static_cast<float>(pInfo.defaultNormalizedValue);
                    m_params[i].value = static_cast<float>(m_vst3Controller->lpVtbl->getParamNormalized(m_vst3Controller, pInfo.id));
                }
            }
        }

        if (m_vst3Controller) {
            Steinberg_IPlugView* testView = m_vst3Controller->lpVtbl->createView(m_vst3Controller, "editor");
            if (testView) {
                m_hasEditor = true;
                testView->lpVtbl->release(testView);
            } else {
                m_hasEditor = false;
            }
        } else {
            m_hasEditor = false;
        }
        return true;
    }

    void renderVst3(int numFrames) {
        if (!m_vst3Processor) return;

        HostVst3EventList evList;
        HostVst3ParameterChanges paramChanges;
        int head = m_eventQueueHead.load(std::memory_order_relaxed);
        int tail = m_eventQueueTail.load(std::memory_order_acquire);
        while (head != tail) {
            const QueuedMidiEvent& q = m_eventQueue[head];
            head = (head + 1) % MAX_QUEUED_EVENTS;
            if (q.type == QueuedMidiEvent::NOTE_ON) {
                evList.addNoteOn(q.note, q.velocity);
            } else if (q.type == QueuedMidiEvent::NOTE_OFF) {
                evList.addNoteOff(q.note);
            } else if (q.type == QueuedMidiEvent::ALL_OFF) {
                for (int p = 0; p < 128; ++p) {
                    evList.addNoteOff(p);
                }
            } else if (q.type == QueuedMidiEvent::PARAM_CHANGE) {
                paramChanges.addChange(q.paramId, q.paramValue);
                if (m_vst3Controller) {
                    m_vst3Controller->lpVtbl->setParamNormalized(m_vst3Controller, q.paramId, q.paramValue);
                }
            }
        }
        m_eventQueueHead.store(head, std::memory_order_release);

        float* inChs[2] = { m_dummyInL.data(), m_dummyInR.data() };
        float* outChs[2] = { m_tempOutL.data(), m_tempOutR.data() };

        Steinberg_Vst_AudioBusBuffers inBus;
        inBus.numChannels = 2;
        inBus.silenceFlags = 3;
        inBus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = inChs;

        Steinberg_Vst_AudioBusBuffers outBus;
        outBus.numChannels = 2;
        outBus.silenceFlags = 0;
        outBus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = outChs;

        Steinberg_Vst_ProcessData pData;
        std::memset(&pData, 0, sizeof(pData));
        pData.processMode = 0;
        pData.symbolicSampleSize = 0;
        pData.numSamples = numFrames;
        pData.numInputs = 1;
        pData.numOutputs = 1;
        pData.inputs = &inBus;
        pData.outputs = &outBus;
        pData.inputEvents = reinterpret_cast<Steinberg_Vst_IEventList*>(&evList);
        pData.inputParameterChanges = reinterpret_cast<Steinberg_Vst_IParameterChanges*>(&paramChanges);

        m_vst3Processor->lpVtbl->process(m_vst3Processor, &pData);
        m_bufferAvailable = numFrames;
    }

    // ==========================================
    // CLAP INITIALIZATION & PROCESSING
    // ==========================================
    static const void* clapHostGetExtension(const clap_host_t* host, const char* extension_id) {
        return nullptr;
    }

    static void clapHostRequestRestart(const clap_host_t* host) {}
    static void clapHostRequestProcess(const clap_host_t* host) {}
    static void clapHostRequestCallback(const clap_host_t* host) {}

    bool initClap(const clap_plugin_entry_t* entry) {
        m_clapEntry = entry;
        if (!m_clapEntry->init(m_path.c_str())) return false;

        const clap_plugin_factory_t* factory = static_cast<const clap_plugin_factory_t*>(
            m_clapEntry->get_factory(CLAP_PLUGIN_FACTORY_ID));
        if (!factory || factory->get_plugin_count(factory) == 0) return false;

        const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, 0);
        if (!desc) return false;

        m_clapHost.clap_version = CLAP_VERSION;
        m_clapHost.host_data = this;
        m_clapHost.name = "myms";
        m_clapHost.vendor = "myms Studios";
        m_clapHost.url = "https://github.com/myavuzuzun/myms";
        m_clapHost.version = "1.0.0";
        m_clapHost.get_extension = clapHostGetExtension;
        m_clapHost.request_restart = clapHostRequestRestart;
        m_clapHost.request_process = clapHostRequestProcess;
        m_clapHost.request_callback = clapHostRequestCallback;

        m_clapPlugin = factory->create_plugin(factory, &m_clapHost, desc->id);
        if (!m_clapPlugin || !m_clapPlugin->init(m_clapPlugin)) return false;

        m_name = desc->name ? desc->name : "CLAP Plugin";
        m_vendor = desc->vendor ? desc->vendor : "Generic";

        if (!m_clapPlugin->activate(m_clapPlugin, m_sampleRate, 1, m_blockSize)) return false;
        m_clapPlugin->start_processing(m_clapPlugin);

        // Parameters extension
        const clap_plugin_params_t* paramsExt = static_cast<const clap_plugin_params_t*>(
            m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_PARAMS));
        if (paramsExt && paramsExt->count) {
            uint32_t count = paramsExt->count(m_clapPlugin);
            m_params.resize(count);
            for (uint32_t i = 0; i < count; ++i) {
                clap_param_info_t info;
                if (paramsExt->get_info(m_clapPlugin, i, &info)) {
                    m_params[i].id = info.id;
                    m_params[i].name = info.name;
                    m_params[i].minValue = static_cast<float>(info.min_value);
                    m_params[i].maxValue = static_cast<float>(info.max_value);
                    m_params[i].defaultValue = static_cast<float>(info.default_value);

                    double val = 0.0;
                    if (paramsExt->get_value(m_clapPlugin, info.id, &val)) {
                        m_params[i].value = static_cast<float>(val);
                    } else {
                        m_params[i].value = m_params[i].defaultValue;
                    }

                    char displayBuf[32] = {0};
                    if (paramsExt->value_to_text && paramsExt->value_to_text(m_clapPlugin, info.id, val, displayBuf, sizeof(displayBuf))) {
                        m_params[i].display = displayBuf;
                    }
                }
            }
        }

        m_hasEditor = (m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_GUI) != nullptr);
        return true;
    }

    struct ClapEventBuffer {
        uint32_t count = 0;
        clap_event_header_t* headers[MAX_QUEUED_EVENTS];
        clap_event_note_t notes[MAX_QUEUED_EVENTS];
        clap_event_param_value_t params[MAX_QUEUED_EVENTS];
    };

    static uint32_t clapInEventsSize(const clap_input_events_t* list) {
        const ClapEventBuffer* b = static_cast<const ClapEventBuffer*>(list->ctx);
        return b->count;
    }

    static const clap_event_header_t* clapInEventsGet(const clap_input_events_t* list, uint32_t index) {
        const ClapEventBuffer* b = static_cast<const ClapEventBuffer*>(list->ctx);
        return (index < b->count) ? b->headers[index] : nullptr;
    }

    static bool clapOutEventsTryPush(const clap_output_events_t* list, const clap_event_header_t* event) {
        return true;
    }

    void renderClap(int numFrames) {
        if (!m_clapPlugin || !m_clapPlugin->process) return;

        ClapEventBuffer evBuf;
        int head = m_eventQueueHead.load(std::memory_order_relaxed);
        int tail = m_eventQueueTail.load(std::memory_order_acquire);

        while (head != tail && evBuf.count < MAX_QUEUED_EVENTS) {
            const QueuedMidiEvent& q = m_eventQueue[head];
            head = (head + 1) % MAX_QUEUED_EVENTS;

            if (q.type == QueuedMidiEvent::NOTE_ON) {
                clap_event_note_t& n = evBuf.notes[evBuf.count];
                n.header.size = sizeof(clap_event_note_t);
                n.header.time = 0;
                n.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                n.header.type = CLAP_EVENT_NOTE_ON;
                n.header.flags = 0;
                n.note_id = -1;
                n.port_index = 0;
                n.channel = 0;
                n.key = q.note;
                n.velocity = q.velocity;
                evBuf.headers[evBuf.count] = &n.header;
                evBuf.count++;
            } else if (q.type == QueuedMidiEvent::NOTE_OFF) {
                clap_event_note_t& n = evBuf.notes[evBuf.count];
                n.header.size = sizeof(clap_event_note_t);
                n.header.time = 0;
                n.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                n.header.type = CLAP_EVENT_NOTE_OFF;
                n.header.flags = 0;
                n.note_id = -1;
                n.port_index = 0;
                n.channel = 0;
                n.key = q.note;
                n.velocity = 0.0;
                evBuf.headers[evBuf.count] = &n.header;
                evBuf.count++;
            } else if (q.type == QueuedMidiEvent::ALL_OFF) {
                for (int k = 0; k < 128 && evBuf.count < MAX_QUEUED_EVENTS; ++k) {
                    clap_event_note_t& n = evBuf.notes[evBuf.count];
                    n.header.size = sizeof(clap_event_note_t);
                    n.header.time = 0;
                    n.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    n.header.type = CLAP_EVENT_NOTE_OFF;
                    n.header.flags = 0;
                    n.note_id = -1;
                    n.port_index = 0;
                    n.channel = 0;
                    n.key = k;
                    n.velocity = 0.0;
                    evBuf.headers[evBuf.count] = &n.header;
                    evBuf.count++;
                }
            } else if (q.type == QueuedMidiEvent::PARAM_CHANGE) {
                clap_event_param_value_t& p = evBuf.params[evBuf.count];
                p.header.size = sizeof(clap_event_param_value_t);
                p.header.time = 0;
                p.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                p.header.type = CLAP_EVENT_PARAM_VALUE;
                p.header.flags = 0;
                p.param_id = q.paramId;
                p.cookie = nullptr;
                p.note_id = -1;
                p.port_index = 0;
                p.channel = 0;
                p.key = -1;
                p.value = q.paramValue;
                evBuf.headers[evBuf.count] = &p.header;
                evBuf.count++;
            }
        }
        m_eventQueueHead.store(head, std::memory_order_release);

        clap_input_events_t in_events;
        in_events.ctx = &evBuf;
        in_events.size = clapInEventsSize;
        in_events.get = clapInEventsGet;

        clap_output_events_t out_events;
        out_events.ctx = nullptr;
        out_events.try_push = clapOutEventsTryPush;

        float* inChs[2] = { m_dummyInL.data(), m_dummyInR.data() };
        float* outChs[2] = { m_tempOutL.data(), m_tempOutR.data() };

        clap_audio_buffer_t in_buf;
        in_buf.data32 = inChs;
        in_buf.data64 = nullptr;
        in_buf.channel_count = 2;
        in_buf.latency = 0;
        in_buf.constant_mask = 0;

        clap_audio_buffer_t out_buf;
        out_buf.data32 = outChs;
        out_buf.data64 = nullptr;
        out_buf.channel_count = 2;
        out_buf.latency = 0;
        out_buf.constant_mask = 0;

        clap_process_t process;
        process.steady_time = -1;
        process.frames_count = numFrames;
        process.transport = nullptr;
        process.audio_inputs = &in_buf;
        process.audio_outputs = &out_buf;
        process.audio_inputs_count = 1;
        process.audio_outputs_count = 1;
        process.in_events = &in_events;
        process.out_events = &out_events;

        m_clapPlugin->process(m_clapPlugin, &process);
        m_bufferAvailable = numFrames;
    }

    void renderAudioBlock(int numFrames) {
        numFrames = std::clamp(numFrames, 1, m_blockSize);
        std::fill(m_tempOutL.begin(), m_tempOutL.begin() + numFrames, 0.0f);
        std::fill(m_tempOutR.begin(), m_tempOutR.begin() + numFrames, 0.0f);

        if (m_format == PLUGIN_FORMAT_CLAP) {
            renderClap(numFrames);
        } else if (m_format == PLUGIN_FORMAT_VST3) {
            renderVst3(numFrames);
        }
    }

    void* m_handle = nullptr;
    std::atomic<bool> m_isLoaded{false};
    bool m_hasEditor = false;
    PluginFormat m_format = PLUGIN_FORMAT_NONE;
    std::string m_path;
    std::string m_name;
    std::string m_vendor;

    float m_sampleRate = 48000.0f;
    int m_blockSize = 256;
    int m_lastNote = -1;

    std::vector<PluginParamInfo> m_params;

    // CLAP State
    const clap_plugin_entry_t* m_clapEntry = nullptr;
    const clap_plugin_t* m_clapPlugin = nullptr;
    clap_host_t m_clapHost;

    // VST3 State
    Steinberg_IPluginFactory* m_vst3Factory = nullptr;
    Steinberg_Vst_IComponent* m_vst3Component = nullptr;
    Steinberg_Vst_IAudioProcessor* m_vst3Processor = nullptr;
    Steinberg_Vst_IEditController* m_vst3Controller = nullptr;
    Steinberg_IPlugView* m_vst3View = nullptr;

    // Pre-allocated ring buffers for zero allocation
    std::array<float, 2048> m_dummyInL;
    std::array<float, 2048> m_dummyInR;
    std::array<float, 2048> m_tempOutL;
    std::array<float, 2048> m_tempOutR;
    int m_bufferReadIndex = 0;
    int m_bufferAvailable = 0;

    // Lock-free event queue
    QueuedMidiEvent m_eventQueue[MAX_QUEUED_EVENTS];
    std::atomic<int> m_eventQueueHead{0};
    std::atomic<int> m_eventQueueTail{0};

    // Parameter dirty flag for bi-directional UI sync
    std::atomic<bool> m_paramsDirty{false};

    // Effect enable / bypass toggle
    std::atomic<bool> m_isEnabled{true};

    // VST3 Component Handler
    HostVst3ComponentHandler m_componentHandler;
};

inline HostVst3ComponentHandler::HostVst3ComponentHandler() {
    vtbl.queryInterface = [](void* self, const Steinberg_TUID iid, void** obj) -> Steinberg_tresult {
        if (!obj) return Steinberg_kInvalidArgument;
        if (std::memcmp(iid, Steinberg_Vst_IComponentHandler_iid, sizeof(Steinberg_TUID)) == 0 ||
            std::memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0) {
            *obj = self;
            return Steinberg_kResultOk;
        }
        *obj = nullptr;
        return Steinberg_kNoInterface;
    };
    vtbl.addRef = [](void*) -> Steinberg_uint32 { return 1; };
    vtbl.release = [](void*) -> Steinberg_uint32 { return 1; };
    vtbl.beginEdit = [](void*, Steinberg_Vst_ParamID) -> Steinberg_tresult { return Steinberg_kResultOk; };
    vtbl.performEdit = [](void* self, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue val) -> Steinberg_tresult {
        auto* h = reinterpret_cast<HostVst3ComponentHandler*>(self);
        if (h && h->parent) {
            h->parent->onPluginParamAutomated(id, static_cast<float>(val));
        }
        return Steinberg_kResultOk;
    };
    vtbl.endEdit = [](void*, Steinberg_Vst_ParamID) -> Steinberg_tresult { return Steinberg_kResultOk; };
    vtbl.restartComponent = [](void*, Steinberg_int32) -> Steinberg_tresult { return Steinberg_kResultOk; };
    iface.lpVtbl = &vtbl;
}

} // namespace myms

#endif // MYMS_PLUGIN_HOST_H
