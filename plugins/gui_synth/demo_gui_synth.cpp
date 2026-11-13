#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vst2/aeffectx.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

inline float note_to_freq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

struct ERect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};

class DemoGuiSynth {
public:
    DemoGuiSynth(audioMasterCallback audioMaster) : m_audioMaster(audioMaster) {
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
        m_effect.flags = effFlagsCanReplacing | effFlagsIsSynth | effFlagsHasEditor;
        m_effect.uniqueID = CCONST('G', 'S', 'Y', '1');
        m_effect.user = this;

        m_params[0] = 0.65f; // Cutoff
        m_params[1] = 0.40f; // Resonance
        m_params[2] = 0.35f; // Decay
        m_params[3] = 0.80f; // Volume

        m_rect.top = 0;
        m_rect.left = 0;
        m_rect.bottom = 320;
        m_rect.right = 480;
    }

    ~DemoGuiSynth() {
        closeGui();
    }

    AEffect* getAEffect() { return &m_effect; }

    static intptr_t VST_CALL_CONV dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        auto* self = static_cast<DemoGuiSynth*>(effect->user);
        if (!self) return 0;
        return self->dispatcher(opcode, index, value, ptr, opt);
    }

    static void VST_CALL_CONV processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int sampleFrames) {
        auto* self = static_cast<DemoGuiSynth*>(effect->user);
        if (self) self->processReplacing(inputs, outputs, sampleFrames);
    }

    static void VST_CALL_CONV setParameterCallback(AEffect* effect, int32_t index, float value) {
        auto* self = static_cast<DemoGuiSynth*>(effect->user);
        if (self && index >= 0 && index < 4) {
            self->m_params[index] = std::clamp(value, 0.0f, 1.0f);
            if (self->m_window && self->m_display) {
                self->redrawGui();
            }
        }
    }

    static float VST_CALL_CONV getParameterCallback(AEffect* effect, int32_t index) {
        auto* self = static_cast<DemoGuiSynth*>(effect->user);
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
                if (ptr) std::strncpy(static_cast<char*>(ptr), "Demo GUI Synth", 32);
                return 1;
            case effGetVendorString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms Studios", 32);
                return 1;
            case effGetProductString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms GUI Synth", 32);
                return 1;
            case effGetParamName:
                if (ptr) {
                    if (index == 0) std::strncpy(static_cast<char*>(ptr), "Cutoff", 16);
                    else if (index == 1) std::strncpy(static_cast<char*>(ptr), "Resonance", 16);
                    else if (index == 2) std::strncpy(static_cast<char*>(ptr), "Decay", 16);
                    else if (index == 3) std::strncpy(static_cast<char*>(ptr), "Volume", 16);
                }
                return 1;
            case effGetParamLabel:
                if (ptr) {
                    if (index == 0) std::strncpy(static_cast<char*>(ptr), "Hz", 8);
                    else if (index == 1) std::strncpy(static_cast<char*>(ptr), "%", 8);
                    else if (index == 2) std::strncpy(static_cast<char*>(ptr), "s", 8);
                    else if (index == 3) std::strncpy(static_cast<char*>(ptr), "%", 8);
                }
                return 1;
            case effGetParamDisplay:
                if (ptr) {
                    if (index == 0) {
                        float hz = 200.0f + m_params[0] * 8000.0f;
                        std::snprintf(static_cast<char*>(ptr), 16, "%.0f", hz);
                    } else if (index == 1) {
                        std::snprintf(static_cast<char*>(ptr), 16, "%.0f", m_params[1] * 100.0f);
                    } else if (index == 2) {
                        std::snprintf(static_cast<char*>(ptr), 16, "%.2f", 0.05f + m_params[2] * 2.0f);
                    } else if (index == 3) {
                        std::snprintf(static_cast<char*>(ptr), 16, "%.0f", m_params[3] * 100.0f);
                    }
                }
                return 1;
            case effEditGetRect:
                if (ptr) {
                    *static_cast<ERect**>(ptr) = &m_rect;
                    return 1;
                }
                return 0;
            case effEditOpen:
                return openGui(ptr) ? 1 : 0;
            case effEditClose:
                closeGui();
                return 1;
            case effProcessEvents: {
                auto* events = static_cast<VstEvents*>(ptr);
                if (events) handleVstEvents(events);
                return 1;
            }
            default:
                return 0;
        }
    }

    void handleVstEvents(VstEvents* events) {
        if (!events) return;
        for (int i = 0; i < events->numEvents; ++i) {
            auto* midi = reinterpret_cast<VstMidiEvent*>(events->events[i]);
            if (midi && midi->type == kVstMidiType) {
                uint8_t status = static_cast<uint8_t>(midi->midiData[0]) & 0xF0;
                int note = static_cast<uint8_t>(midi->midiData[1]);
                int vel = static_cast<uint8_t>(midi->midiData[2]);

                if (status == 0x90 && vel > 0) {
                    m_note = note;
                    m_freq = note_to_freq(note);
                    m_env = 1.0f;
                    m_velocity = vel / 127.0f;
                } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
                    if (m_note == note) {
                        m_env = 0.0f;
                    }
                } else if (status == 0xB0 && note == 123) {
                    m_env = 0.0f;
                }
            }
        }
    }

    void processReplacing(float** inputs, float** outputs, int sampleFrames) {
        (void)inputs;
        float* outL = outputs[0];
        float* outR = outputs[1];

        float cutoffNorm = m_params[0];
        float resNorm = m_params[1];
        float decayNorm = m_params[2];
        float masterVol = m_params[3];

        float decayRate = 1.0f - (1.0f / (m_sampleRate * (0.05f + decayNorm * 2.0f)));
        float filterCutoffHz = 200.0f + cutoffNorm * 8000.0f;
        float f = 2.0f * std::sin(PI * filterCutoffHz / m_sampleRate);
        float q = 1.0f - resNorm * 0.95f;

        for (int i = 0; i < sampleFrames; ++i) {
            float sample = 0.0f;
            if (m_env > 0.0001f) {
                // Dual detuned saw waves
                m_phase1 += m_freq / m_sampleRate;
                if (m_phase1 >= 1.0f) m_phase1 -= 1.0f;
                float saw1 = 2.0f * m_phase1 - 1.0f;

                m_phase2 += (m_freq * 1.005f) / m_sampleRate;
                if (m_phase2 >= 1.0f) m_phase2 -= 1.0f;
                float saw2 = 2.0f * m_phase2 - 1.0f;

                float raw = 0.5f * (saw1 + saw2) * m_env * m_velocity;

                // 24dB Resonant Lowpass Filter
                m_fltLow += f * m_fltBand;
                m_fltHigh = raw - m_fltLow - q * m_fltBand;
                m_fltBand += f * m_fltHigh;

                sample = m_fltLow * 0.7f * masterVol;
                m_env *= decayRate;
            }

            outL[i] = sample;
            outR[i] = sample;

            // Store sample for oscilloscope visualization
            m_scopeBuf[m_scopeWriteIdx] = sample;
            m_scopeWriteIdx = (m_scopeWriteIdx + 1) % SCOPE_SIZE;
        }
    }

    // ==========================================
    // Native X11 Graphical User Interface
    // ==========================================
    bool openGui(void* parentWindow) {
        closeGui();
        if (!parentWindow) return false;

        Display* dpy = XOpenDisplay(nullptr);
        if (!dpy) return false;

        m_display = dpy;
        m_parentWindow = reinterpret_cast<Window>(parentWindow);
        int screen = DefaultScreen(dpy);

        XSetWindowAttributes swa;
        swa.background_pixel = 0x232629; // Breeze dark background
        swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;

        m_window = XCreateWindow(dpy, m_parentWindow, 0, 0, 480, 320, 0,
                                 CopyFromParent, InputOutput, CopyFromParent,
                                 CWBackPixel | CWEventMask, &swa);

        m_gc = XCreateGC(dpy, m_window, 0, nullptr);
        XMapWindow(dpy, m_window);
        XFlush(dpy);

        m_guiRunning.store(true);
        m_guiThread = std::thread(&DemoGuiSynth::guiEventLoop, this);
        return true;
    }

    void closeGui() {
        if (!m_guiRunning.load()) return;
        m_guiRunning.store(false);

        if (m_guiThread.joinable()) {
            m_guiThread.join();
        }

        if (m_display) {
            if (m_gc) {
                XFreeGC(m_display, m_gc);
                m_gc = nullptr;
            }
            if (m_window) {
                XDestroyWindow(m_display, m_window);
                m_window = 0;
            }
            XCloseDisplay(m_display);
            m_display = nullptr;
        }
    }

private:
    void guiEventLoop() {
        while (m_guiRunning.load()) {
            if (!m_display || !m_window) break;

            while (XPending(m_display)) {
                XEvent ev;
                XNextEvent(m_display, &ev);
                if (ev.type == Expose) {
                    redrawGui();
                } else if (ev.type == ButtonPress) {
                    handleMouse(ev.xbutton.x, ev.xbutton.y, true);
                } else if (ev.type == MotionNotify) {
                    if (ev.xmotion.state & Button1Mask) {
                        handleMouse(ev.xmotion.x, ev.xmotion.y, false);
                    }
                } else if (ev.type == ButtonRelease) {
                    m_draggedSlider = -1;
                }
            }

            redrawGui();
            usleep(30000); // ~33 FPS refresh
        }
    }

    void handleMouse(int x, int y, bool isPress) {
        int sliderStartY = 80;
        int sliderHeight = 36;
        int sliderSpacing = 48;

        for (int i = 0; i < 4; ++i) {
            int sy = sliderStartY + i * sliderSpacing;
            if (y >= sy && y <= sy + sliderHeight) {
                m_draggedSlider = i;
                break;
            }
        }

        if (m_draggedSlider >= 0 && m_draggedSlider < 4) {
            int trackLeft = 140;
            int trackRight = 440;
            float val = static_cast<float>(x - trackLeft) / static_cast<float>(trackRight - trackLeft);
            m_params[m_draggedSlider] = std::clamp(val, 0.0f, 1.0f);
        }
    }

    void redrawGui() {
        if (!m_display || !m_window || !m_gc) return;

        // 1. Background
        XSetForeground(m_display, m_gc, 0x232629);
        XFillRectangle(m_display, m_window, m_gc, 0, 0, 480, 320);

        // 2. Header Bar
        XSetForeground(m_display, m_gc, 0x31363b);
        XFillRectangle(m_display, m_window, m_gc, 0, 0, 480, 44);
        XSetForeground(m_display, m_gc, 0x3daee9); // Breeze sky blue accent line
        XDrawLine(m_display, m_window, m_gc, 0, 44, 480, 44);

        // 3. Title text
        XSetForeground(m_display, m_gc, 0xffffff);
        const char* title = "myms Reference Synth GUI";
        XDrawString(m_display, m_window, m_gc, 16, 28, title, std::strlen(title));

        const char* sub = "[X11 Native Editor]";
        XSetForeground(m_display, m_gc, 0x3daee9);
        XDrawString(m_display, m_window, m_gc, 340, 28, sub, std::strlen(sub));

        // 4. Oscilloscope Box
        XSetForeground(m_display, m_gc, 0x1b1e20);
        XFillRectangle(m_display, m_window, m_gc, 20, 52, 440, 24);
        XSetForeground(m_display, m_gc, 0x3daee9);
        XDrawRectangle(m_display, m_window, m_gc, 20, 52, 440, 24);

        // Draw waveform points
        XSetForeground(m_display, m_gc, 0x2ecc71); // Green waveform
        int prevX = 20, prevY = 64;
        for (int px = 0; px < 440; px += 2) {
            int bufIdx = (m_scopeWriteIdx - 440 + px + SCOPE_SIZE) % SCOPE_SIZE;
            float s = m_scopeBuf[bufIdx];
            int py = 64 - static_cast<int>(s * 10.0f);
            py = std::clamp(py, 54, 74);
            if (px > 0) {
                XDrawLine(m_display, m_window, m_gc, prevX, prevY, 20 + px, py);
            }
            prevX = 20 + px;
            prevY = py;
        }

        // 5. Draw 4 Interactive Breeze Sliders
        const char* paramLabels[4] = { "CUTOFF", "RESONANCE", "DECAY", "VOLUME" };
        int sliderStartY = 88;
        int sliderSpacing = 48;
        int trackLeft = 140;
        int trackWidth = 300;

        for (int i = 0; i < 4; ++i) {
            int sy = sliderStartY + i * sliderSpacing;

            // Parameter Label
            XSetForeground(m_display, m_gc, 0xbdc3c7);
            XDrawString(m_display, m_window, m_gc, 24, sy + 18, paramLabels[i], std::strlen(paramLabels[i]));

            // Track Background
            XSetForeground(m_display, m_gc, 0x31363b);
            XFillRectangle(m_display, m_window, m_gc, trackLeft, sy + 6, trackWidth, 18);
            XSetForeground(m_display, m_gc, 0x474747);
            XDrawRectangle(m_display, m_window, m_gc, trackLeft, sy + 6, trackWidth, 18);

            // Active Fill
            int fillW = static_cast<int>(m_params[i] * trackWidth);
            XSetForeground(m_display, m_gc, 0x3daee9);
            if (fillW > 0) {
                XFillRectangle(m_display, m_window, m_gc, trackLeft + 1, sy + 7, fillW - 1, 16);
            }

            // Slider Handle Thumb
            int thumbX = trackLeft + fillW - 4;
            thumbX = std::clamp(thumbX, trackLeft, trackLeft + trackWidth - 8);
            XSetForeground(m_display, m_gc, 0xffffff);
            XFillRectangle(m_display, m_window, m_gc, thumbX, sy + 3, 8, 24);

            // Value text
            char valStr[16];
            std::snprintf(valStr, sizeof(valStr), "%.0f%%", m_params[i] * 100.0f);
            XSetForeground(m_display, m_gc, 0xf0f2f5);
            XDrawString(m_display, m_window, m_gc, trackLeft + trackWidth + 12, sy + 18, valStr, std::strlen(valStr));
        }

        // 6. Footer hint
        XSetForeground(m_display, m_gc, 0x7f8c8d);
        const char* hint = "Click & drag sliders to sculpt sound in real-time";
        XDrawString(m_display, m_window, m_gc, 90, 300, hint, std::strlen(hint));

        XFlush(m_display);
    }

    audioMasterCallback m_audioMaster = nullptr;
    AEffect m_effect;
    ERect m_rect;
    float m_params[4];
    float m_sampleRate = 48000.0f;
    bool m_active = true;

    // DSP Voice State
    int m_note = -1;
    float m_freq = 440.0f;
    float m_phase1 = 0.0f;
    float m_phase2 = 0.0f;
    float m_env = 0.0f;
    float m_velocity = 0.0f;
    float m_fltLow = 0.0f;
    float m_fltBand = 0.0f;
    float m_fltHigh = 0.0f;

    // Oscilloscope Ring Buffer
    static constexpr int SCOPE_SIZE = 512;
    float m_scopeBuf[SCOPE_SIZE] = {0.0f};
    int m_scopeWriteIdx = 0;

    // X11 GUI State
    std::atomic<bool> m_guiRunning{false};
    std::thread m_guiThread;
    Display* m_display = nullptr;
    Window m_parentWindow = 0;
    Window m_window = 0;
    GC m_gc = nullptr;
    int m_draggedSlider = -1;
};

} // namespace

extern "C" {

#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
  #define VST_EXPORT __attribute__ ((visibility ("default")))
#else
  #define VST_EXPORT
#endif

VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    if (!audioMaster) return nullptr;
    auto* synth = new DemoGuiSynth(audioMaster);
    return synth->getAEffect();
}

VST_EXPORT AEffect* main_plugin(audioMasterCallback audioMaster) {
    return VSTPluginMain(audioMaster);
}

}
