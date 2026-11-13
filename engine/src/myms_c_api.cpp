#include "myms_c_api.h"
#include "audio_engine.h"
#include <cstring>

extern "C" {

bool myms_init(int sample_rate, int buffer_size) {
    return myms::AudioEngine::instance().init(sample_rate, buffer_size);
}

void myms_shutdown() {
    myms::AudioEngine::instance().shutdown();
}

void myms_play() {
    myms::AudioEngine::instance().play();
}

void myms_pause() {
    myms::AudioEngine::instance().pause();
}

void myms_stop() {
    myms::AudioEngine::instance().stop();
}

bool myms_is_playing() {
    return myms::AudioEngine::instance().isPlaying();
}

void myms_set_bpm(float bpm) {
    myms::AudioEngine::instance().setBpm(bpm);
}

float myms_get_bpm() {
    return myms::AudioEngine::instance().getBpm();
}

int myms_get_current_row() {
    return myms::AudioEngine::instance().getCurrentRow();
}

void myms_set_current_row(int row) {
    myms::AudioEngine::instance().setCurrentRow(row);
}

int myms_get_num_rows() {
    return myms::AudioEngine::instance().getNumRows();
}

void myms_set_num_rows(int rows) {
    myms::AudioEngine::instance().setNumRows(rows);
}

void myms_set_looping(bool loop) {
    myms::AudioEngine::instance().setLooping(loop);
}

bool myms_is_looping() {
    return myms::AudioEngine::instance().isLooping();
}

int myms_get_num_tracks() {
    return myms::AudioEngine::instance().getNumTracks();
}

void myms_add_track() {
    myms::AudioEngine::instance().addTrack();
}

void myms_remove_track(int track_idx) {
    myms::AudioEngine::instance().removeTrack(track_idx);
}

void myms_set_inst_name(int inst, const char* name) {
    if (name) {
        myms::AudioEngine::instance().setInstrumentName(inst, std::string(name));
    }
}

void myms_get_inst_name(int inst, char* out_name, int max_len) {
    if (out_name && max_len > 0) {
        std::string n = myms::AudioEngine::instance().getInstrumentName(inst);
        std::strncpy(out_name, n.c_str(), max_len - 1);
        out_name[max_len - 1] = '\0';
    }
}

void myms_note_on_inst(int inst, int note, float velocity) {
    myms::AudioEngine::instance().noteOn(inst, note, velocity);
}

void myms_note_off_inst(int inst, int note) {
    myms::AudioEngine::instance().noteOff(inst, note);
}

void myms_set_inst_osc1(int inst, int wave) {
    myms::AudioEngine::instance().setInstrumentOsc1(inst, wave);
}

void myms_set_inst_osc2(int inst, int wave) {
    myms::AudioEngine::instance().setInstrumentOsc2(inst, wave);
}

void myms_set_inst_osc_mix(int inst, float mix) {
    myms::AudioEngine::instance().setInstrumentOscMix(inst, mix);
}

void myms_set_inst_detune(int inst, float semi, float cents) {
    myms::AudioEngine::instance().setInstrumentDetune(inst, semi, cents);
}

void myms_set_inst_filter(int inst, float cutoff, float resonance, float env_amount) {
    myms::AudioEngine::instance().setInstrumentFilter(inst, cutoff, resonance, env_amount);
}

void myms_set_inst_amp_env(int inst, float a, float d, float s, float r) {
    myms::AudioEngine::instance().setInstrumentAmpEnv(inst, a, d, s, r);
}

void myms_set_inst_filter_env(int inst, float a, float d, float s, float r) {
    myms::AudioEngine::instance().setInstrumentFilterEnv(inst, a, d, s, r);
}

void myms_get_inst_params(
    int inst,
    int* out_osc1, int* out_osc2, float* out_mix,
    float* out_detune_semi, float* out_detune_cents,
    float* out_cutoff, float* out_res, float* out_env_amt,
    float* out_amp_a, float* out_amp_d, float* out_amp_s, float* out_amp_r
) {
    myms::SynthParams p = myms::AudioEngine::instance().getInstrumentParams(inst);
    if (out_osc1) *out_osc1 = static_cast<int>(p.osc1Wave);
    if (out_osc2) *out_osc2 = static_cast<int>(p.osc2Wave);
    if (out_mix) *out_mix = p.oscMix;
    if (out_detune_semi) *out_detune_semi = p.osc2DetuneSemi;
    if (out_detune_cents) *out_detune_cents = p.osc2FineCents;
    if (out_cutoff) *out_cutoff = p.filterCutoff;
    if (out_res) *out_res = p.filterResonance;
    if (out_env_amt) *out_env_amt = p.filterEnvAmount;
    if (out_amp_a) *out_amp_a = p.ampAttack;
    if (out_amp_d) *out_amp_d = p.ampDecay;
    if (out_amp_s) *out_amp_s = p.ampSustain;
    if (out_amp_r) *out_amp_r = p.ampRelease;
}

void myms_set_macro_punch(float punch) {
    myms::AudioEngine::instance().setMacroPunch(punch);
}

void myms_set_macro_warmth(float warmth) {
    myms::AudioEngine::instance().setMacroWarmth(warmth);
}

void myms_set_macro_tone(float tone) {
    myms::AudioEngine::instance().setMacroTone(tone);
}

void myms_set_macro_space(float space) {
    myms::AudioEngine::instance().setMacroSpace(space);
}

void myms_set_master_volume(float volume) {
    myms::AudioEngine::instance().setMasterVolume(volume);
}

void myms_get_meters(float* out_peak_l, float* out_peak_r, float* out_rms) {
    if (out_peak_l) *out_peak_l = myms::AudioEngine::instance().getMasterPeakL();
    if (out_peak_r) *out_peak_r = myms::AudioEngine::instance().getMasterPeakR();
    if (out_rms) *out_rms = myms::AudioEngine::instance().getMasterRms();
}

void myms_set_cell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effect_type, uint8_t effectParam) {
    myms::AudioEngine::instance().setTrackerCell(track, row, note, instrument, volume, effect_type, effectParam);
}

void myms_get_cell(int track, int row, uint8_t* out_note, uint8_t* out_instrument, uint8_t* out_volume, uint8_t* out_effect_type, uint8_t* out_effect_param) {
    myms::TrackerCell c = myms::AudioEngine::instance().getTrackerCell(track, row);
    if (out_note) *out_note = c.note;
    if (out_instrument) *out_instrument = c.instrument;
    if (out_volume) *out_volume = c.volume;
    if (out_effect_type) *out_effect_type = c.effectType;
    if (out_effect_param) *out_effect_param = c.effectParam;
}

void myms_clear_track(int track) {
    myms::AudioEngine::instance().clearTrack(track);
}

void myms_clear_all_tracks() {
    myms::AudioEngine::instance().clearAllTracks();
}

void myms_transpose_track(int track, int semitones) {
    myms::AudioEngine::instance().transposeTrack(track, semitones);
}

void myms_load_demo() {
    myms::AudioEngine::instance().loadDemoGroove();
}

} // extern "C"
