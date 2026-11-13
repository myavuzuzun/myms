#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "tsf.h"

namespace myms {

struct SoundFontPreset {
    int index;
    std::string name;
};

class SoundFontPlayer {
public:
    SoundFontPlayer() 
        : m_tsf(nullptr), m_currentPreset(0), m_volume(0.85f), m_sampleRate(48000.0f) {}

    ~SoundFontPlayer() {
        close();
    }

    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        
        // Auto-load available system default SoundFont
        const char* systemFonts[] = {
            "/usr/share/sounds/sf2/default-GM.sf2",
            "/usr/share/sounds/sf2/TimGM6mb.sf2",
            "/usr/share/soundfonts/default.sf2"
        };
        for (const char* path : systemFonts) {
            if (loadSoundFont(path)) {
                std::cout << "[myms] Auto-loaded system SoundFont: " << path << std::endl;
                break;
            }
        }
    }

    bool loadSoundFont(const std::string& filepath) {
        tsf* newTsf = tsf_load_filename(filepath.c_str());
        if (!newTsf) {
            std::cerr << "[myms] Failed to load SoundFont: " << filepath << std::endl;
            return false;
        }

        tsf_set_output(newTsf, TSF_STEREO_INTERLEAVED, static_cast<int>(m_sampleRate), 0.0f);

        std::vector<SoundFontPreset> presets;
        int count = tsf_get_presetcount(newTsf);
        presets.reserve(count);
        for (int i = 0; i < count; ++i) {
            const char* name = tsf_get_presetname(newTsf, i);
            SoundFontPreset p;
            p.index = i;
            p.name = name ? name : ("Preset " + std::to_string(i));
            presets.push_back(p);
        }

        tsf* oldTsf = m_tsf.exchange(newTsf, std::memory_order_acq_rel);
        {
            std::lock_guard<std::mutex> lock(m_metaMutex);
            m_loadedPath = filepath;
            m_presets = std::move(presets);
            m_currentPreset.store(0, std::memory_order_relaxed);
        }

        if (oldTsf) {
            tsf_close(oldTsf);
        }

        return true;
    }

    void close() {
        tsf* oldTsf = m_tsf.exchange(nullptr, std::memory_order_acq_rel);
        if (oldTsf) {
            tsf_close(oldTsf);
        }
    }

    bool isLoaded() const {
        return m_tsf.load(std::memory_order_relaxed) != nullptr;
    }

    std::string getLoadedPath() {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        return m_loadedPath;
    }

    int getPresetCount() {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        return static_cast<int>(m_presets.size());
    }

    std::string getPresetName(int index) {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        if (index >= 0 && index < static_cast<int>(m_presets.size())) {
            return m_presets[index].name;
        }
        return "";
    }

    void setPreset(int index) {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        if (!m_presets.empty()) {
            m_currentPreset.store(std::clamp(index, 0, static_cast<int>(m_presets.size()) - 1), std::memory_order_relaxed);
        }
    }

    int getPreset() const {
        return m_currentPreset.load(std::memory_order_relaxed);
    }

    void setVolume(float vol) {
        m_volume.store(std::clamp(vol, 0.0f, 2.0f), std::memory_order_relaxed);
    }

    float getVolume() const {
        return m_volume.load(std::memory_order_relaxed);
    }

    inline void noteOn(int note, float velocity) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        int preset = m_currentPreset.load(std::memory_order_relaxed);
        tsf_note_on(f, preset, note, velocity);
    }

    inline void noteOff(int note) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        int preset = m_currentPreset.load(std::memory_order_relaxed);
        tsf_note_off(f, preset, note);
    }

    inline void allNotesOff() {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        tsf_note_off_all(f);
    }

    // Process a block of samples (mixing into sL and sR)
    inline void processSample(float& outL, float& outR) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        float buf[2] = {0.0f, 0.0f};
        tsf_render_float(f, buf, 1, 0);
        float vol = m_volume.load(std::memory_order_relaxed);
        outL += buf[0] * vol;
        outR += buf[1] * vol;
    }

    // Process multi-frame block
    void processBlock(float* outStereoInterleaved, int frames) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        tsf_render_float(f, outStereoInterleaved, frames, 1 /* mixing */);
    }

private:
    std::atomic<tsf*> m_tsf{nullptr};
    std::atomic<int> m_currentPreset{0};
    std::atomic<float> m_volume{0.85f};
    float m_sampleRate{48000.0f};

    std::mutex m_metaMutex;
    std::string m_loadedPath;
    std::vector<SoundFontPreset> m_presets;
};

} // namespace myms
