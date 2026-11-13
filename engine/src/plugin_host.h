#ifndef MYMS_PLUGIN_HOST_H
#define MYMS_PLUGIN_HOST_H

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>

#include "vst2/aeffectx.h"
#include "clap/clap.h"

namespace myms {

enum PluginFormat {
    PLUGIN_FORMAT_NONE = 0,
    PLUGIN_FORMAT_VST2 = 1,
    PLUGIN_FORMAT_CLAP = 2
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

class PluginInstance {
public:
    PluginInstance() {
        m_dummyInL.fill(0.0f);
        m_dummyInR.fill(0.0f);
        m_tempOutL.fill(0.0f);
        m_tempOutR.fill(0.0f);
    }

    ~PluginInstance() {
        unload();
    }

    bool load(const std::string& path, float sampleRate = 48000.0f, int blockSize = 256) {
        unload();

        m_sampleRate = sampleRate;
        m_blockSize = blockSize;
        m_path = path;

        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::cerr << "[myms PluginHost] dlopen failed for " << path << ": " << dlerror() << std::endl;
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

        // 2. Try loading as VST 2.4
        using VstMainFunc = AEffect* (*)(audioMasterCallback);
        VstMainFunc vstMain = reinterpret_cast<VstMainFunc>(dlsym(handle, "VSTPluginMain"));
        if (!vstMain) {
            vstMain = reinterpret_cast<VstMainFunc>(dlsym(handle, "main_plugin"));
        }
        if (!vstMain) {
            vstMain = reinterpret_cast<VstMainFunc>(dlsym(handle, "main"));
        }

        if (vstMain) {
            if (initVst2(vstMain)) {
                m_format = PLUGIN_FORMAT_VST2;
                m_isLoaded.store(true, std::memory_order_release);
                std::cout << "[myms PluginHost] Successfully loaded VST2 plugin: " << m_name 
                          << " (" << m_params.size() << " params)" << std::endl;
                return true;
            }
        }

        std::cerr << "[myms PluginHost] File is neither a recognized Linux VST2 nor CLAP plugin: " << path << std::endl;
        dlclose(m_handle);
        m_handle = nullptr;
        return false;
    }

    void unload() {
        if (!m_isLoaded.load(std::memory_order_acquire)) return;

        m_isLoaded.store(false, std::memory_order_release);

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
        } else if (m_format == PLUGIN_FORMAT_VST2) {
            if (m_vstEffect) {
                m_vstEffect->dispatcher(m_vstEffect, effMainsChanged, 0, 0, nullptr, 0.0f);
                m_vstEffect->dispatcher(m_vstEffect, effClose, 0, 0, nullptr, 0.0f);
                m_vstEffect = nullptr;
            }
        }

        if (m_handle) {
            dlclose(m_handle);
            m_handle = nullptr;
        }

        m_format = PLUGIN_FORMAT_NONE;
        m_name.clear();
        m_vendor.clear();
        m_path.clear();
        m_params.clear();
        m_lastNote = -1;
    }

    bool isLoaded() const {
        return m_isLoaded.load(std::memory_order_acquire);
    }

    PluginFormat getFormat() const { return m_format; }
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

        if (m_format == PLUGIN_FORMAT_VST2 && m_vstEffect && m_vstEffect->setParameter) {
            m_vstEffect->setParameter(m_vstEffect, index, val);
        } else if (m_format == PLUGIN_FORMAT_CLAP) {
            queueEvent(QueuedMidiEvent{QueuedMidiEvent::PARAM_CHANGE, 0, 0.0f, index, val});
        }
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
        queueEvent(QueuedMidiEvent{QueuedMidiEvent::ALL_OFF, 0, 0.0f, 0, 0.0f});
        m_lastNote = -1;
    }

    // Real-Time Audio Processing (sample-by-sample for tracker integration)
    inline void processSample(float& outL, float& outR) {
        if (!m_isLoaded.load(std::memory_order_relaxed)) return;

        // If local buffer empty, render a block
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

    // Block processing (frames frames)
    void processBlock(float* outL, float* outR, int numFrames) {
        if (!m_isLoaded.load(std::memory_order_relaxed)) return;

        int framesRemaining = numFrames;
        int dstOffset = 0;

        while (framesRemaining > 0) {
            int available = m_bufferAvailable - m_bufferReadIndex;
            if (available <= 0) {
                int toRender = std::min(framesRemaining, m_blockSize);
                renderAudioBlock(toRender);
                m_bufferReadIndex = 0;
                available = m_bufferAvailable;
            }

            int toCopy = std::min(framesRemaining, available);
            for (int i = 0; i < toCopy; ++i) {
                outL[dstOffset + i] += m_tempOutL[m_bufferReadIndex + i];
                outR[dstOffset + i] += m_tempOutR[m_bufferReadIndex + i];
            }

            m_bufferReadIndex += toCopy;
            dstOffset += toCopy;
            framesRemaining -= toCopy;
        }
    }

    // Native GUI / Editor
    bool hasNativeEditor() const {
        if (m_format == PLUGIN_FORMAT_VST2 && m_vstEffect) {
            return (m_vstEffect->flags & effFlagsHasEditor) != 0;
        }
        if (m_format == PLUGIN_FORMAT_CLAP && m_clapPlugin) {
            return (m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_GUI) != nullptr);
        }
        return false;
    }

    bool openNativeEditor(void* parentWindow) {
        if (!hasNativeEditor() || !parentWindow) return false;

        if (m_format == PLUGIN_FORMAT_VST2 && m_vstEffect) {
            return m_vstEffect->dispatcher(m_vstEffect, effEditOpen, 0, 0, parentWindow, 0.0f) != 0;
        }
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
        return false;
    }

    void closeNativeEditor() {
        if (m_format == PLUGIN_FORMAT_VST2 && m_vstEffect) {
            m_vstEffect->dispatcher(m_vstEffect, effEditClose, 0, 0, nullptr, 0.0f);
        } else if (m_format == PLUGIN_FORMAT_CLAP && m_clapPlugin) {
            const clap_plugin_gui_t* gui = static_cast<const clap_plugin_gui_t*>(
                m_clapPlugin->get_extension(m_clapPlugin, CLAP_EXT_GUI));
            if (gui && gui->destroy) {
                gui->destroy(m_clapPlugin);
            }
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
    // VST 2.4 INITIALIZATION & PROCESSING
    // ==========================================
    static intptr_t VST_CALL_CONV staticAudioMaster(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        switch (opcode) {
            case audioMasterVersion:
                return 2400;
            case audioMasterGetSampleRate:
                return 48000;
            case audioMasterGetBlockSize:
                return 256;
            case audioMasterGetVendorString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms", 64);
                return 1;
            case audioMasterGetProductString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms Tracker Studio", 64);
                return 1;
            case audioMasterGetVendorVersion:
                return 1000;
            case audioMasterCanDo:
                if (ptr) {
                    const char* s = static_cast<const char*>(ptr);
                    if (std::strcmp(s, "sendVstEvents") == 0) return 1;
                    if (std::strcmp(s, "sendVstMidiEvent") == 0) return 1;
                    if (std::strcmp(s, "receiveVstEvents") == 0) return 1;
                    if (std::strcmp(s, "receiveVstMidiEvent") == 0) return 1;
                    if (std::strcmp(s, "sizeWindow") == 0) return 1;
                }
                return 0;
            default:
                return 0;
        }
    }

    bool initVst2(AEffect* (*vstMain)(audioMasterCallback)) {
        m_vstEffect = vstMain(staticAudioMaster);
        if (!m_vstEffect || m_vstEffect->magic != kEffectMagic) {
            std::cerr << "[myms PluginHost] Invalid VST2 magic" << std::endl;
            return false;
        }

        m_vstEffect->dispatcher(m_vstEffect, effOpen, 0, 0, nullptr, 0.0f);
        m_vstEffect->dispatcher(m_vstEffect, effSetSampleRate, 0, 0, nullptr, m_sampleRate);
        m_vstEffect->dispatcher(m_vstEffect, effSetBlockSize, 0, m_blockSize, nullptr, 0.0f);
        m_vstEffect->dispatcher(m_vstEffect, effMainsChanged, 0, 1 /* activate */, nullptr, 0.0f);

        // Retrieve plugin metadata
        char nameBuf[64] = {0};
        if (m_vstEffect->dispatcher(m_vstEffect, effGetEffectName, 0, 0, nameBuf, 0.0f) && nameBuf[0]) {
            m_name = nameBuf;
        } else {
            m_name = "VST2 Plugin";
        }

        char vendorBuf[64] = {0};
        if (m_vstEffect->dispatcher(m_vstEffect, effGetVendorString, 0, 0, vendorBuf, 0.0f) && vendorBuf[0]) {
            m_vendor = vendorBuf;
        } else {
            m_vendor = "Generic";
        }

        // Retrieve parameter definitions
        int numParams = m_vstEffect->numParams;
        m_params.resize(numParams);
        for (int i = 0; i < numParams; ++i) {
            m_params[i].id = i;
            m_params[i].value = m_vstEffect->getParameter ? m_vstEffect->getParameter(m_vstEffect, i) : 0.0f;

            char pName[32] = {0};
            if (m_vstEffect->dispatcher(m_vstEffect, effGetParamName, i, 0, pName, 0.0f) && pName[0]) {
                m_params[i].name = pName;
            } else {
                m_params[i].name = "Param " + std::to_string(i + 1);
            }

            char pDisplay[32] = {0};
            if (m_vstEffect->dispatcher(m_vstEffect, effGetParamDisplay, i, 0, pDisplay, 0.0f) && pDisplay[0]) {
                m_params[i].display = pDisplay;
            }

            char pLabel[32] = {0};
            if (m_vstEffect->dispatcher(m_vstEffect, effGetParamLabel, i, 0, pLabel, 0.0f) && pLabel[0]) {
                m_params[i].label = pLabel;
            }
        }

        return true;
    }

    void renderVst2(int numFrames) {
        if (!m_vstEffect || !m_vstEffect->processReplacing) return;

        // Drain event queue into VstEvents
        struct {
            int32_t numEvents;
            void* reserved;
            VstEvent* events[MAX_QUEUED_EVENTS];
        } vstEvents;

        VstMidiEvent midiEvents[MAX_QUEUED_EVENTS];
        int evCount = 0;

        int head = m_eventQueueHead.load(std::memory_order_relaxed);
        int tail = m_eventQueueTail.load(std::memory_order_acquire);

        while (head != tail && evCount < MAX_QUEUED_EVENTS) {
            const QueuedMidiEvent& q = m_eventQueue[head];
            head = (head + 1) % MAX_QUEUED_EVENTS;

            if (q.type == QueuedMidiEvent::NOTE_ON) {
                VstMidiEvent& m = midiEvents[evCount];
                std::memset(&m, 0, sizeof(VstMidiEvent));
                m.type = kVstMidiType;
                m.byteSize = sizeof(VstMidiEvent);
                m.midiData[0] = static_cast<char>(0x90);
                m.midiData[1] = static_cast<char>(q.note & 0x7F);
                m.midiData[2] = static_cast<char>(std::clamp(static_cast<int>(q.velocity * 127.0f), 1, 127));
                vstEvents.events[evCount] = reinterpret_cast<VstEvent*>(&m);
                evCount++;
            } else if (q.type == QueuedMidiEvent::NOTE_OFF) {
                VstMidiEvent& m = midiEvents[evCount];
                std::memset(&m, 0, sizeof(VstMidiEvent));
                m.type = kVstMidiType;
                m.byteSize = sizeof(VstMidiEvent);
                m.midiData[0] = static_cast<char>(0x80);
                m.midiData[1] = static_cast<char>(q.note & 0x7F);
                m.midiData[2] = 0;
                vstEvents.events[evCount] = reinterpret_cast<VstEvent*>(&m);
                evCount++;
            } else if (q.type == QueuedMidiEvent::ALL_OFF) {
                for (int ch = 0; ch < 16 && evCount < MAX_QUEUED_EVENTS; ++ch) {
                    VstMidiEvent& m = midiEvents[evCount];
                    std::memset(&m, 0, sizeof(VstMidiEvent));
                    m.type = kVstMidiType;
                    m.byteSize = sizeof(VstMidiEvent);
                    m.midiData[0] = static_cast<char>(0xB0 | ch);
                    m.midiData[1] = 123; // All Notes Off
                    m.midiData[2] = 0;
                    vstEvents.events[evCount] = reinterpret_cast<VstEvent*>(&m);
                    evCount++;
                }
            } else if (q.type == QueuedMidiEvent::PARAM_CHANGE) {
                if (m_vstEffect->setParameter) {
                    m_vstEffect->setParameter(m_vstEffect, q.paramId, q.paramValue);
                }
            }
        }
        m_eventQueueHead.store(head, std::memory_order_release);

        if (evCount > 0) {
            vstEvents.numEvents = evCount;
            vstEvents.reserved = nullptr;
            m_vstEffect->dispatcher(m_vstEffect, effProcessEvents, 0, 0, &vstEvents, 0.0f);
        }

        // Prepare zero-allocation buffers
        float* inPtrs[2] = { m_dummyInL.data(), m_dummyInR.data() };
        float* outPtrs[2] = { m_tempOutL.data(), m_tempOutR.data() };

        m_vstEffect->processReplacing(m_vstEffect, inPtrs, outPtrs, numFrames);
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

        // Audio buffers
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

        if (m_format == PLUGIN_FORMAT_VST2) {
            renderVst2(numFrames);
        } else if (m_format == PLUGIN_FORMAT_CLAP) {
            renderClap(numFrames);
        }
    }

    void* m_handle = nullptr;
    std::atomic<bool> m_isLoaded{false};
    PluginFormat m_format = PLUGIN_FORMAT_NONE;
    std::string m_path;
    std::string m_name;
    std::string m_vendor;

    float m_sampleRate = 48000.0f;
    int m_blockSize = 256;
    int m_lastNote = -1;

    std::vector<PluginParamInfo> m_params;

    // VST2 State
    AEffect* m_vstEffect = nullptr;

    // CLAP State
    const clap_plugin_entry_t* m_clapEntry = nullptr;
    const clap_plugin_t* m_clapPlugin = nullptr;
    clap_host_t m_clapHost;

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
};

} // namespace myms

#endif // MYMS_PLUGIN_HOST_H
