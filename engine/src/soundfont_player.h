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

constexpr int MAX_SF2_CHANNELS = 32;

struct SoundFontPreset {
    int index;
    std::string name;
};

class SoundFontPlayer {
public:
    SoundFontPlayer() 
        : m_tsf(nullptr), m_masterVolume(0.85f), m_sampleRate(48000.0f) {
        for (int i = 0; i < MAX_SF2_CHANNELS; ++i) {
            m_channelPresets[i] = 0;
            m_channelVolumes[i] = 1.0f;
            m_channelPans[i] = 0.5f;
            m_channelMutes[i] = false;
        }
    }

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

        // Apply channel presets & parameters to new tsf
        for (int ch = 0; ch < MAX_SF2_CHANNELS; ++ch) {
            if (m_channelPresets[ch] < count) {
                tsf_channel_set_presetindex(newTsf, ch, m_channelPresets[ch]);
            }
            tsf_channel_set_volume(newTsf, ch, m_channelVolumes[ch]);
            tsf_channel_set_pan(newTsf, ch, m_channelPans[ch]);
        }

        tsf* oldTsf = m_tsf.exchange(newTsf, std::memory_order_acq_rel);
        {
            std::lock_guard<std::mutex> lock(m_metaMutex);
            m_loadedPath = filepath;
            m_presets = std::move(presets);
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

    // Global / Default Channel 0 Helpers
    void setPreset(int index) {
        setChannelPreset(0, index);
    }

    int getPreset() const {
        return getChannelPreset(0);
    }

    void setVolume(float vol) {
        setMasterVolume(vol);
    }

    float getVolume() const {
        return getMasterVolume();
    }

    inline void noteOn(int note, float velocity) {
        noteOn(0, note, velocity);
    }

    inline void noteOff(int note) {
        noteOff(0, note);
    }

    // Per-Channel (Per-Track) SoundFont Preset Control
    void setChannelPreset(int channel, int presetIndex) {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return;
        m_channelPresets[channel] = std::max(0, presetIndex);

        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (f) {
            tsf_channel_set_presetindex(f, channel, m_channelPresets[channel]);
        }
    }

    int getChannelPreset(int channel) const {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return 0;
        return m_channelPresets[channel];
    }

    void setChannelVolume(int channel, float vol) {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return;
        m_channelVolumes[channel] = std::clamp(vol, 0.0f, 2.0f);
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (f) {
            float effVol = m_channelMutes[channel] ? 0.0f : m_channelVolumes[channel];
            tsf_channel_set_volume(f, channel, effVol);
        }
    }

    float getChannelVolume(int channel) const {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return 1.0f;
        return m_channelVolumes[channel];
    }

    void setChannelPan(int channel, float pan) {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return;
        m_channelPans[channel] = std::clamp(pan, 0.0f, 1.0f);
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (f) {
            tsf_channel_set_pan(f, channel, m_channelPans[channel]);
        }
    }

    void setChannelMute(int channel, bool mute) {
        if (channel < 0 || channel >= MAX_SF2_CHANNELS) return;
        m_channelMutes[channel] = mute;
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (f) {
            float effVol = mute ? 0.0f : m_channelVolumes[channel];
            tsf_channel_set_volume(f, channel, effVol);
        }
    }

    void setMasterVolume(float vol) {
        m_masterVolume.store(std::clamp(vol, 0.0f, 2.0f), std::memory_order_relaxed);
    }

    float getMasterVolume() const {
        return m_masterVolume.load(std::memory_order_relaxed);
    }

    // Per-Channel Note Triggering
    inline void noteOn(int channel, int note, float velocity) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        int ch = std::clamp(channel, 0, MAX_SF2_CHANNELS - 1);
        tsf_channel_set_presetindex(f, ch, m_channelPresets[ch]);
        tsf_channel_note_on(f, ch, note, velocity);
    }

    inline void noteOff(int channel, int note) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        int ch = std::clamp(channel, 0, MAX_SF2_CHANNELS - 1);
        tsf_channel_note_off(f, ch, note);
    }

    inline void allNotesOff(int channel = -1) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        if (channel >= 0 && channel < MAX_SF2_CHANNELS) {
            tsf_channel_sounds_off_all(f, channel);
        } else {
            tsf_note_off_all(f);
        }
    }

    // Render all active SoundFont channels into interleaved stereo buffer
    inline void processSample(float& outL, float& outR) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        float buf[2] = {0.0f, 0.0f};
        tsf_render_float(f, buf, 1, 0);
        float master = m_masterVolume.load(std::memory_order_relaxed);
        outL += buf[0] * master;
        outR += buf[1] * master;
    }

    void processBlock(float* outStereoInterleaved, int frames) {
        tsf* f = m_tsf.load(std::memory_order_acquire);
        if (!f) return;
        tsf_render_float(f, outStereoInterleaved, frames, 1 /* mixing */);
    }

private:
    std::atomic<tsf*> m_tsf{nullptr};
    std::atomic<float> m_masterVolume{0.85f};
    float m_sampleRate{48000.0f};

    int m_channelPresets[MAX_SF2_CHANNELS];
    float m_channelVolumes[MAX_SF2_CHANNELS];
    float m_channelPans[MAX_SF2_CHANNELS];
    bool m_channelMutes[MAX_SF2_CHANNELS];

    std::mutex m_metaMutex;
    std::string m_loadedPath;
    std::vector<SoundFontPreset> m_presets;
};

} // namespace myms
