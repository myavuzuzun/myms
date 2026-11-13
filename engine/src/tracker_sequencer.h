#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace myms {

constexpr int DEFAULT_NUM_ROWS = 32;
constexpr int DEFAULT_NUM_TRACKS = 1;
constexpr int MAX_TRACKS = 32;
constexpr uint8_t NOTE_OFF = 254;
constexpr uint8_t NO_NOTE = 255;

struct TrackerCell {
    uint8_t note = NO_NOTE;
    uint8_t instrument = 0;
    uint8_t volume = 255;
    uint8_t effectType = 0;
    uint8_t effectParam = 0;

    TrackerCell() = default;
    TrackerCell(uint8_t n, uint8_t inst, uint8_t vol = 255, uint8_t fxT = 0, uint8_t fxP = 0)
        : note(n), instrument(inst), volume(vol), effectType(fxT), effectParam(fxP) {}
};

struct TrackerPattern {
    int numRows = DEFAULT_NUM_ROWS;
    int numTracks = DEFAULT_NUM_TRACKS;
    std::vector<std::vector<TrackerCell>> rows;

    TrackerPattern(int r = DEFAULT_NUM_ROWS, int t = DEFAULT_NUM_TRACKS)
        : numRows(r), numTracks(t) {
        rows.resize(numRows);
        for (int i = 0; i < numRows; ++i) {
            rows[i].resize(numTracks);
        }
    }

    void addTrack() {
        if (numTracks >= MAX_TRACKS) return;
        numTracks++;
        for (int r = 0; r < numRows; ++r) {
            rows[r].emplace_back(TrackerCell());
        }
    }

    void removeTrack(int trackIdx) {
        if (numTracks <= 1 || trackIdx < 0 || trackIdx >= numTracks) return;
        for (int r = 0; r < numRows; ++r) {
            rows[r].erase(rows[r].begin() + trackIdx);
        }
        numTracks--;
    }

    void setRowsCount(int r) {
        r = std::clamp(r, 8, 256);
        if (r == numRows) return;
        rows.resize(r);
        for (int i = numRows; i < r; ++i) {
            rows[i].resize(numTracks);
        }
        numRows = r;
    }
};

class TrackerSequencer {
public:
    void init(float sampleRate) {
        m_sampleRate = sampleRate;
        m_bpm = 120.0f;
        m_lpb = 4;
        m_currentRow = 0;
        m_samplesUntilNextRow = 0;
        m_isPlaying = false;
        m_isLooping = true;
        m_isSongMode = false;
        m_currentPattern = 0;
        m_songOrderIndex = 0;
        m_songOrder = {0};

        m_patterns.clear();
        m_patterns.emplace_back(DEFAULT_NUM_ROWS, DEFAULT_NUM_TRACKS);
    }

    void setBpm(float bpm) {
        if (bpm >= 20.0f && bpm <= 400.0f) {
            m_bpm = bpm;
        }
    }

    float getBpm() const {
        return m_bpm;
    }

    void setLpb(int lpb) {
        if (lpb >= 1 && lpb <= 16) {
            m_lpb = lpb;
        }
    }

    int getLpb() const {
        return m_lpb;
    }

    void setLooping(bool loop) {
        m_isLooping = loop;
    }

    bool isLooping() const {
        return m_isLooping;
    }

    void setSongMode(bool songMode) {
        m_isSongMode = songMode;
        if (songMode && !m_isPlaying && !m_songOrder.empty()) {
            m_songOrderIndex = 0;
            m_currentPattern = std::clamp(m_songOrder[0], 0, static_cast<int>(m_patterns.size()) - 1);
            m_currentRow = 0;
        }
    }

    bool isSongMode() const {
        return m_isSongMode;
    }

    void play() {
        m_samplesUntilNextRow = 0;
        m_isPlaying = true;
    }

    void pause() {
        m_isPlaying = false;
        m_samplesUntilNextRow = 0;
    }

    void stop() {
        m_isPlaying = false;
        m_currentRow = 0;
        if (m_isSongMode) {
            m_songOrderIndex = 0;
            if (!m_songOrder.empty()) {
                m_currentPattern = std::clamp(m_songOrder[0], 0, static_cast<int>(m_patterns.size()) - 1);
            } else {
                m_currentPattern = 0;
            }
        }
        m_samplesUntilNextRow = 0;
    }

    bool isPlaying() const {
        return m_isPlaying;
    }

    int getCurrentRow() const {
        return m_currentRow;
    }

    void setCurrentRow(int row) {
        int curPat = getCurrentPattern();
        if (curPat >= 0 && curPat < static_cast<int>(m_patterns.size())) {
            m_currentRow = row % m_patterns[curPat].numRows;
            m_samplesUntilNextRow = 0;
        }
    }

    int getNumRows() const {
        int curPat = getCurrentPattern();
        if (curPat >= 0 && curPat < static_cast<int>(m_patterns.size())) {
            return m_patterns[curPat].numRows;
        }
        return DEFAULT_NUM_ROWS;
    }

    void setNumRows(int rows) {
        int curPat = getCurrentPattern();
        if (curPat >= 0 && curPat < static_cast<int>(m_patterns.size())) {
            m_patterns[curPat].setRowsCount(rows);
            if (m_currentRow >= m_patterns[curPat].numRows) {
                m_currentRow = 0;
            }
        }
    }

    int getNumTracks() const {
        if (m_patterns.empty()) return DEFAULT_NUM_TRACKS;
        return m_patterns[0].numTracks;
    }

    void addTrack() {
        for (auto& pat : m_patterns) {
            pat.addTrack();
        }
    }

    void removeTrack(int trackIdx) {
        for (auto& pat : m_patterns) {
            pat.removeTrack(trackIdx);
        }
    }

    // Pattern Management
    int getNumPatterns() const {
        return static_cast<int>(m_patterns.size());
    }

    int getCurrentPattern() const {
        return std::clamp(m_currentPattern, 0, std::max(0, static_cast<int>(m_patterns.size()) - 1));
    }

    void setCurrentPattern(int pat) {
        if (pat >= 0 && pat < static_cast<int>(m_patterns.size())) {
            m_currentPattern = pat;
            if (m_currentRow >= m_patterns[m_currentPattern].numRows) {
                m_currentRow = 0;
            }
        }
    }

    int addPattern() {
        int rows = getNumRows();
        int tracks = getNumTracks();
        m_patterns.emplace_back(rows, tracks);
        int newIdx = static_cast<int>(m_patterns.size()) - 1;
        m_songOrder.push_back(newIdx);
        m_currentPattern = newIdx;
        return newIdx;
    }

    int duplicatePattern(int sourceIdx) {
        if (sourceIdx < 0 || sourceIdx >= static_cast<int>(m_patterns.size())) {
            return addPattern();
        }
        TrackerPattern clone = m_patterns[sourceIdx];
        m_patterns.push_back(clone);
        int newIdx = static_cast<int>(m_patterns.size()) - 1;
        m_songOrder.push_back(newIdx);
        m_currentPattern = newIdx;
        return newIdx;
    }

    bool removePattern(int patIdx) {
        if (m_patterns.size() <= 1 || patIdx < 0 || patIdx >= static_cast<int>(m_patterns.size())) {
            return false;
        }
        m_patterns.erase(m_patterns.begin() + patIdx);
        if (m_currentPattern >= static_cast<int>(m_patterns.size())) {
            m_currentPattern = static_cast<int>(m_patterns.size()) - 1;
        }
        // Clean song order
        std::vector<int> newOrder;
        for (int p : m_songOrder) {
            if (p == patIdx) continue;
            newOrder.push_back(p > patIdx ? p - 1 : p);
        }
        if (newOrder.empty()) newOrder.push_back(0);
        m_songOrder = newOrder;
        m_songOrderIndex = 0;
        return true;
    }

    std::vector<int> getSongOrder() const {
        return m_songOrder;
    }

    void setSongOrder(const std::vector<int>& order) {
        if (!order.empty()) {
            m_songOrder = order;
            m_songOrderIndex = 0;
        }
    }

    int getSongOrderLength() const {
        return static_cast<int>(m_songOrder.size());
    }

    int getSongOrderStep(int step) const {
        if (step >= 0 && step < static_cast<int>(m_songOrder.size())) {
            return m_songOrder[step];
        }
        return 0;
    }

    void setSongOrderStep(int step, int pat) {
        if (step >= 0 && step < static_cast<int>(m_songOrder.size())) {
            m_songOrder[step] = std::clamp(pat, 0, std::max(0, static_cast<int>(m_patterns.size()) - 1));
        }
    }

    void addSongOrderStep(int pat) {
        m_songOrder.push_back(std::clamp(pat, 0, std::max(0, static_cast<int>(m_patterns.size()) - 1)));
    }

    void removeSongOrderStep(int step) {
        if (m_songOrder.size() > 1 && step >= 0 && step < static_cast<int>(m_songOrder.size())) {
            m_songOrder.erase(m_songOrder.begin() + step);
            if (m_songOrderIndex >= static_cast<int>(m_songOrder.size())) {
                m_songOrderIndex = 0;
            }
        }
    }

    int getSongOrderIndex() const {
        return m_songOrderIndex;
    }

    void setSongOrderIndex(int step) {
        if (step >= 0 && step < static_cast<int>(m_songOrder.size())) {
            m_songOrderIndex = step;
            m_currentPattern = std::clamp(m_songOrder[m_songOrderIndex], 0, static_cast<int>(m_patterns.size()) - 1);
            m_currentRow = 0;
        }
    }

    void setPatternCell(int pat, int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam) {
        if (pat >= 0 && pat < static_cast<int>(m_patterns.size())) {
            TrackerPattern& p = m_patterns[pat];
            if (row >= 0 && row < p.numRows && track >= 0 && track < p.numTracks) {
                TrackerCell& c = p.rows[row][track];
                c.note = note;
                c.instrument = instrument;
                c.volume = volume;
                c.effectType = effectType;
                c.effectParam = effectParam;
            }
        }
    }

    TrackerCell getPatternCell(int pat, int track, int row) const {
        if (pat >= 0 && pat < static_cast<int>(m_patterns.size())) {
            const TrackerPattern& p = m_patterns[pat];
            if (row >= 0 && row < p.numRows && track >= 0 && track < p.numTracks) {
                return p.rows[row][track];
            }
        }
        return TrackerCell();
    }

    void setCell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam) {
        setPatternCell(getCurrentPattern(), track, row, note, instrument, volume, effectType, effectParam);
    }

    TrackerCell getCell(int track, int row) const {
        return getPatternCell(getCurrentPattern(), track, row);
    }

    inline int getSamplesPerRow() const {
        return static_cast<int>((m_sampleRate * 60.0f) / (m_bpm * static_cast<float>(m_lpb)));
    }

    inline bool advanceSample(int& outRow, int& outPattern, int& outSongOrderIndex) {
        if (!m_isPlaying || m_patterns.empty()) {
            return false;
        }

        if (m_samplesUntilNextRow <= 0) {
            outRow = m_currentRow;
            outPattern = getCurrentPattern();
            outSongOrderIndex = m_songOrderIndex;
            m_samplesUntilNextRow = getSamplesPerRow();
            
            int curRows = (outPattern < static_cast<int>(m_patterns.size())) ? m_patterns[outPattern].numRows : DEFAULT_NUM_ROWS;
            int nextRow = m_currentRow + 1;
            if (nextRow >= curRows) {
                if (m_isSongMode && !m_songOrder.empty()) {
                    int nextSongIdx = m_songOrderIndex + 1;
                    if (nextSongIdx >= static_cast<int>(m_songOrder.size())) {
                        if (m_isLooping) {
                            m_songOrderIndex = 0;
                            m_currentPattern = std::clamp(m_songOrder[0], 0, static_cast<int>(m_patterns.size()) - 1);
                            m_currentRow = 0;
                        } else {
                            m_isPlaying = false;
                            m_songOrderIndex = 0;
                            m_currentPattern = std::clamp(m_songOrder[0], 0, static_cast<int>(m_patterns.size()) - 1);
                            m_currentRow = 0;
                            return true;
                        }
                    } else {
                        m_songOrderIndex = nextSongIdx;
                        m_currentPattern = std::clamp(m_songOrder[m_songOrderIndex], 0, static_cast<int>(m_patterns.size()) - 1);
                        m_currentRow = 0;
                    }
                } else if (m_isLooping) {
                    m_currentRow = 0;
                } else {
                    m_isPlaying = false;
                    m_currentRow = 0;
                    return true;
                }
            } else {
                m_currentRow = nextRow;
            }
            return true;
        }

        m_samplesUntilNextRow--;
        return false;
    }

private:
    float m_sampleRate = 48000.0f;
    float m_bpm = 120.0f;
    int m_lpb = 4;
    int m_currentRow = 0;
    int m_currentPattern = 0;
    int m_samplesUntilNextRow = 0;
    bool m_isPlaying = false;
    bool m_isLooping = true;
    bool m_isSongMode = false;

    std::vector<int> m_songOrder = {0};
    int m_songOrderIndex = 0;

    std::vector<TrackerPattern> m_patterns;
};

} // namespace myms
