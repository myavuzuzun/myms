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

struct ERect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};

class DemoFilterFx {
public:
    DemoFilterFx(audioMasterCallback audioMaster) : m_audioMaster(audioMaster) {
        std::memset(&m_effect, 0, sizeof(AEffect));
        m_effect.magic = kEffectMagic;
        m_effect.dispatcher = dispatcherCallback;
        m_effect.process = nullptr;
        m_effect.processReplacing = processReplacingCallback;
        m_effect.setParameter = setParameterCallback;
        m_effect.getParameter = getParameterCallback;
        m_effect.numPrograms = 1;
        m_effect.numParams = 4;
        m_effect.numInputs = 2;  // Stereo Input
        m_effect.numOutputs = 2; // Stereo Output
        m_effect.flags = effFlagsCanReplacing | effFlagsHasEditor;
        m_effect.uniqueID = CCONST('F', 'L', 'T', '1');
        m_effect.user = this;

        m_params[0] = 0.70f; // Cutoff (70%)
        m_params[1] = 0.50f; // Resonance (50%)
        m_params[2] = 0.25f; // Drive / Warmth
        m_params[3] = 1.00f; // Dry/Wet (100% Wet)

        m_rect.top = 0;
        m_rect.left = 0;
        m_rect.bottom = 280;
        m_rect.right = 460;
    }

    ~DemoFilterFx() {
        closeGui();
    }

    AEffect* getAEffect() { return &m_effect; }

    static intptr_t VST_CALL_CONV dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        auto* self = static_cast<DemoFilterFx*>(effect->user);
        if (!self) return 0;
        return self->dispatcher(opcode, index, value, ptr, opt);
    }

    static void VST_CALL_CONV processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int sampleFrames) {
        auto* self = static_cast<DemoFilterFx*>(effect->user);
        if (self) self->processReplacing(inputs, outputs, sampleFrames);
    }

    static void VST_CALL_CONV setParameterCallback(AEffect* effect, int32_t index, float value) {
        auto* self = static_cast<DemoFilterFx*>(effect->user);
        if (self && index >= 0 && index < 4) {
            self->m_params[index] = std::clamp(value, 0.0f, 1.0f);
        }
    }

    static float VST_CALL_CONV getParameterCallback(AEffect* effect, int32_t index) {
        auto* self = static_cast<DemoFilterFx*>(effect->user);
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
                if (ptr) std::strncpy(static_cast<char*>(ptr), "Demo Filter FX", 32);
                return 1;
            case effGetVendorString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms Studios", 32);
                return 1;
            case effGetProductString:
                if (ptr) std::strncpy(static_cast<char*>(ptr), "myms Track Filter FX", 32);
                return 1;
            case effGetParamName:
                if (ptr) {
                    if (index == 0) std::strncpy(static_cast<char*>(ptr), "Cutoff", 16);
                    else if (index == 1) std::strncpy(static_cast<char*>(ptr), "Resonance", 16);
                    else if (index == 2) std::strncpy(static_cast<char*>(ptr), "Drive", 16);
                    else if (index == 3) std::strncpy(static_cast<char*>(ptr), "Dry/Wet", 16);
                }
                return 1;
            case effGetParamLabel:
                if (ptr) {
                    if (index == 0) std::strncpy(static_cast<char*>(ptr), "Hz", 8);
                    else if (index == 1) std::strncpy(static_cast<char*>(ptr), "%", 8);
                    else if (index == 2) std::strncpy(static_cast<char*>(ptr), "x", 8);
                    else if (index == 3) std::strncpy(static_cast<char*>(ptr), "%", 8);
                }
                return 1;
            case effGetParamDisplay:
                if (ptr) {
                    if (index == 0) {
                        float hz = 40.0f + m_params[0] * 12000.0f;
                        std::snprintf(static_cast<char*>(ptr), 16, "%.0f", hz);
                    } else if (index == 1) {
                        std::snprintf(static_cast<char*>(ptr), 16, "%.0f", m_params[1] * 100.0f);
                    } else if (index == 2) {
                        std::snprintf(static_cast<char*>(ptr), 16, "%.1f", 1.0f + m_params[2] * 4.0f);
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
            default:
                return 0;
        }
    }

    void processReplacing(float** inputs, float** outputs, int sampleFrames) {
        float* inL = inputs ? inputs[0] : nullptr;
        float* inR = inputs ? inputs[1] : nullptr;
        float* outL = outputs[0];
        float* outR = outputs[1];

        float cutoffNorm = m_params[0];
        float resNorm = m_params[1];
        float driveNorm = m_params[2];
        float wet = m_params[3];
        float dry = 1.0f - wet;

        float drive = 1.0f + driveNorm * 4.0f;
        float cutoffHz = 40.0f + cutoffNorm * 12000.0f;
        float f = 2.0f * std::sin(PI * cutoffHz / m_sampleRate);
        f = std::clamp(f, 0.001f, 0.999f);
        float q = 1.0f - resNorm * 0.92f;

        for (int i = 0; i < sampleFrames; ++i) {
            float xL = inL ? inL[i] : 0.0f;
            float xR = inR ? inR[i] : 0.0f;

            // Drive / soft saturation
            float satL = std::tanh(xL * drive);
            float satR = std::tanh(xR * drive);

            // Left Channel Resonant SVF Filter
            m_lowL += f * m_bandL;
            m_highL = satL - m_lowL - q * m_bandL;
            m_bandL += f * m_highL;
            float filtL = m_lowL;

            // Right Channel Resonant SVF Filter
            m_lowR += f * m_bandR;
            m_highR = satR - m_lowR - q * m_bandR;
            m_bandR += f * m_highR;
            float filtR = m_lowR;

            outL[i] = xL * dry + filtL * wet;
            outR[i] = xR * dry + filtR * wet;
        }
    }

    // ==========================================
    // Native X11 GUI
    // ==========================================
    bool openGui(void* parentWindow) {
        closeGui();
        if (!parentWindow) return false;

        Display* dpy = XOpenDisplay(nullptr);
        if (!dpy) return false;

        m_display = dpy;
        m_parentWindow = reinterpret_cast<Window>(parentWindow);

        XSetWindowAttributes swa;
        swa.background_pixel = 0x232629;
        swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;

        m_window = XCreateWindow(dpy, m_parentWindow, 0, 0, 460, 280, 0,
                                 CopyFromParent, InputOutput, CopyFromParent,
                                 CWBackPixel | CWEventMask, &swa);

        m_gc = XCreateGC(dpy, m_window, 0, nullptr);
        XMapWindow(dpy, m_window);
        XFlush(dpy);

        m_guiRunning.store(true);
        m_guiThread = std::thread(&DemoFilterFx::guiEventLoop, this);
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
                    handleMouse(ev.xbutton.x, ev.xbutton.y);
                } else if (ev.type == MotionNotify) {
                    if (ev.xmotion.state & Button1Mask) {
                        handleMouse(ev.xmotion.x, ev.xmotion.y);
                    }
                } else if (ev.type == ButtonRelease) {
                    m_draggedSlider = -1;
                }
            }

            redrawGui();
            usleep(30000);
        }
    }

    void handleMouse(int x, int y) {
        int startY = 70;
        int spacing = 44;
        for (int i = 0; i < 4; ++i) {
            int sy = startY + i * spacing;
            if (y >= sy && y <= sy + 32) {
                m_draggedSlider = i;
                break;
            }
        }

        if (m_draggedSlider >= 0 && m_draggedSlider < 4) {
            int trackLeft = 130;
            int trackRight = 410;
            float val = static_cast<float>(x - trackLeft) / static_cast<float>(trackRight - trackLeft);
            m_params[m_draggedSlider] = std::clamp(val, 0.0f, 1.0f);
            if (m_audioMaster) {
                m_audioMaster(&m_effect, 0 /* audioMasterAutomate */, m_draggedSlider, 0, nullptr, m_params[m_draggedSlider]);
            }
        }
    }

    void redrawGui() {
        if (!m_display || !m_window || !m_gc) return;

        // Background
        XSetForeground(m_display, m_gc, 0x232629);
        XFillRectangle(m_display, m_window, m_gc, 0, 0, 460, 280);

        // Header
        XSetForeground(m_display, m_gc, 0x31363b);
        XFillRectangle(m_display, m_window, m_gc, 0, 0, 460, 44);
        XSetForeground(m_display, m_gc, 0xe67e22); // Warm orange insert FX line
        XDrawLine(m_display, m_window, m_gc, 0, 44, 460, 44);

        XSetForeground(m_display, m_gc, 0xffffff);
        const char* title = "myms Resonant Filter FX";
        XDrawString(m_display, m_window, m_gc, 16, 28, title, std::strlen(title));

        const char* tag = "[Track Insert FX]";
        XSetForeground(m_display, m_gc, 0xe67e22);
        XDrawString(m_display, m_window, m_gc, 330, 28, tag, std::strlen(tag));

        // Sliders
        const char* labels[4] = { "CUTOFF", "RESONANCE", "DRIVE", "DRY/WET" };
        int startY = 70;
        int spacing = 44;
        int trackLeft = 130;
        int trackWidth = 280;

        for (int i = 0; i < 4; ++i) {
            int sy = startY + i * spacing;

            XSetForeground(m_display, m_gc, 0xbdc3c7);
            XDrawString(m_display, m_window, m_gc, 20, sy + 18, labels[i], std::strlen(labels[i]));

            XSetForeground(m_display, m_gc, 0x31363b);
            XFillRectangle(m_display, m_window, m_gc, trackLeft, sy + 6, trackWidth, 18);
            XSetForeground(m_display, m_gc, 0x474747);
            XDrawRectangle(m_display, m_window, m_gc, trackLeft, sy + 6, trackWidth, 18);

            int fillW = static_cast<int>(m_params[i] * trackWidth);
            XSetForeground(m_display, m_gc, (i == 2) ? 0xe67e22 : 0x3daee9);
            if (fillW > 0) {
                XFillRectangle(m_display, m_window, m_gc, trackLeft + 1, sy + 7, fillW - 1, 16);
            }

            int thumbX = trackLeft + fillW - 4;
            thumbX = std::clamp(thumbX, trackLeft, trackLeft + trackWidth - 8);
            XSetForeground(m_display, m_gc, 0xffffff);
            XFillRectangle(m_display, m_window, m_gc, thumbX, sy + 3, 8, 24);
        }

        XFlush(m_display);
    }

    audioMasterCallback m_audioMaster = nullptr;
    AEffect m_effect;
    ERect m_rect;
    float m_params[4];
    float m_sampleRate = 48000.0f;
    bool m_active = true;

    // Filter DSP State
    float m_lowL = 0.0f, m_bandL = 0.0f, m_highL = 0.0f;
    float m_lowR = 0.0f, m_bandR = 0.0f, m_highR = 0.0f;

    // GUI State
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
    auto* fx = new DemoFilterFx(audioMaster);
    return fx->getAEffect();
}

VST_EXPORT AEffect* main_plugin(audioMasterCallback audioMaster) {
    return VSTPluginMain(audioMaster);
}

}
