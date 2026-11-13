#ifndef MYMS_TRACKER_SEQUENCER_H
#define MYMS_TRACKER_SEQUENCER_H

#include <vector>
#include <cstdint>
#include <atomic>
#include <algorithm>

namespace myms {

constexpr uint8_t NOTE_EMPTY = 255;
constexpr uint8_t NOTE_OFF   = 254;

constexpr int DEFAULT_NUM_ROWS = 32;
constexpr int DEFAULT_NUM_TRACKS = 1; // Start with exactly 1 empty track
constexpr int MAX_TRACKS = 32;

struct TrackerCell {
    uint8_t note = NOTE_EMPTY;
    uint8_t instrument = 0;
    uint8_t volume = 255;
    uint8_t effectType = 0;
    uint8_t effectParam = 0;
};

struct TrackerPattern {
    int numRows = DEFAULT_NUM_ROWS;
    int numTracks = DEFAULT_NUM_TRACKS;
    std::vector<std::vector<TrackerCell>> rows;

    TrackerPattern(int r = DEFAULT_NUM_ROWS, int t = DEFAULT_NUM_TRACKS) 
        : numRows(r), numTracks(t) 
    {
        resize(r, t);
    }

    void resize(int r, int t) {
        numRows = std::clamp(r, 8, 256);
        numTracks = std::clamp(t, 1, MAX_TRACKS);
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
        m_samplesUntilNextRow = 0;
    }

    bool isPlaying() const {
        return m_isPlaying;
    }

    int getCurrentRow() const {
        return m_currentRow;
    }

    void setCurrentRow(int row) {
        if (!m_patterns.empty()) {
            m_currentRow = row % m_patterns[0].numRows;
            m_samplesUntilNextRow = 0;
        }
    }

    int getNumRows() const {
        if (m_patterns.empty()) return DEFAULT_NUM_ROWS;
        return m_patterns[0].numRows;
    }

    void setNumRows(int rows) {
        if (!m_patterns.empty()) {
            m_patterns[0].setRowsCount(rows);
            if (m_currentRow >= m_patterns[0].numRows) {
                m_currentRow = 0;
            }
        }
    }

    int getNumTracks() const {
        if (m_patterns.empty()) return DEFAULT_NUM_TRACKS;
        return m_patterns[0].numTracks;
    }

    void addTrack() {
        if (!m_patterns.empty()) {
            m_patterns[0].addTrack();
        }
    }

    void removeTrack(int trackIdx) {
        if (!m_patterns.empty()) {
            m_patterns[0].removeTrack(trackIdx);
        }
    }

    void setCell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effectType, uint8_t effectParam) {
        if (m_patterns.empty()) return;
        if (row >= 0 && row < m_patterns[0].numRows && track >= 0 && track < m_patterns[0].numTracks) {
            TrackerCell& c = m_patterns[0].rows[row][track];
            c.note = note;
            c.instrument = instrument;
            c.volume = volume;
            c.effectType = effectType;
            c.effectParam = effectParam;
        }
    }

    TrackerCell getCell(int track, int row) const {
        if (!m_patterns.empty() && row >= 0 && row < m_patterns[0].numRows && track >= 0 && track < m_patterns[0].numTracks) {
            return m_patterns[0].rows[row][track];
        }
        return TrackerCell();
    }

    inline int getSamplesPerRow() const {
        return static_cast<int>((m_sampleRate * 60.0f) / (m_bpm * static_cast<float>(m_lpb)));
    }

    inline bool advanceSample(int& outRow) {
        if (!m_isPlaying || m_patterns.empty()) {
            return false;
        }

        if (m_samplesUntilNextRow <= 0) {
            outRow = m_currentRow;
            m_samplesUntilNextRow = getSamplesPerRow();
            
            int nextRow = m_currentRow + 1;
            if (nextRow >= m_patterns[0].numRows) {
                if (m_isLooping) {
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
    int m_samplesUntilNextRow = 0;
    bool m_isPlaying = false;
    bool m_isLooping = true;

    std::vector<TrackerPattern> m_patterns;
};

} // namespace myms

#endif // MYMS_TRACKER_SEQUENCER_H
