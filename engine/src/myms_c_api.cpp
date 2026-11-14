#include "myms_c_api.h"
#include "audio_engine.h"
#include <cstring>
#include <iostream>

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

void myms_set_track_type(int track, int type) {
    myms::AudioEngine::instance().setTrackType(track, type);
}

int myms_get_track_type(int track) {
    return myms::AudioEngine::instance().getTrackType(track);
}

// Track Mixer Controls
void myms_set_track_volume(int track, float vol) {
    myms::AudioEngine::instance().setTrackVolume(track, vol);
}

float myms_get_track_volume(int track) {
    return myms::AudioEngine::instance().getTrackVolume(track);
}

void myms_set_track_pan(int track, float pan) {
    myms::AudioEngine::instance().setTrackPan(track, pan);
}

float myms_get_track_pan(int track) {
    return myms::AudioEngine::instance().getTrackPan(track);
}

void myms_set_track_mute(int track, bool mute) {
    myms::AudioEngine::instance().setTrackMute(track, mute);
}

bool myms_get_track_mute(int track) {
    return myms::AudioEngine::instance().getTrackMute(track);
}

void myms_set_track_solo(int track, bool solo) {
    myms::AudioEngine::instance().setTrackSolo(track, solo);
}

bool myms_get_track_solo(int track) {
    return myms::AudioEngine::instance().getTrackSolo(track);
}

void myms_set_track_sf2_preset(int track, int preset) {
    myms::AudioEngine::instance().setTrackSf2Preset(track, preset);
}

int myms_get_track_sf2_preset(int track) {
    return myms::AudioEngine::instance().getTrackSf2Preset(track);
}

void myms_get_track_meters(int track, float* out_peak_l, float* out_peak_r) {
    myms::AudioEngine::instance().getTrackMeters(track, out_peak_l, out_peak_r);
}

void myms_set_inst_name(int inst, const char* name) {
    if (name) {
        myms::AudioEngine::instance().setInstrumentName(inst, std::string(name));
    }
}

void myms_get_inst_name(int inst, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getInstrumentName(inst);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_note_on_inst(int inst, int note, float velocity) {
    myms::AudioEngine::instance().noteOn(inst, note, velocity);
}

void myms_note_off_inst(int inst, int note) {
    myms::AudioEngine::instance().noteOff(inst, note);
}

void myms_all_notes_off() {
    myms::AudioEngine::instance().allNotesOff();
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

// SoundFont Player Controls (FontBox)
bool myms_sf2_load(const char* filepath) {
    if (!filepath) return false;
    return myms::AudioEngine::instance().loadSoundFont(filepath);
}

bool myms_sf2_is_loaded() {
    return myms::AudioEngine::instance().isSoundFontLoaded();
}

int myms_sf2_get_preset_count() {
    return myms::AudioEngine::instance().getSoundFontPresetCount();
}

void myms_sf2_get_preset_name(int index, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getSoundFontPresetName(index);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_sf2_set_preset(int index) {
    myms::AudioEngine::instance().setSoundFontPreset(index);
}

int myms_sf2_get_preset() {
    return myms::AudioEngine::instance().getSoundFontPreset();
}

void myms_sf2_set_volume(float vol) {
    myms::AudioEngine::instance().setSoundFontVolume(vol);
}

float myms_sf2_get_volume() {
    return myms::AudioEngine::instance().getSoundFontVolume();
}

void myms_sf2_get_path(char* out_path, int max_len) {
    if (!out_path || max_len <= 0) return;
    std::string path = myms::AudioEngine::instance().getSoundFontPath();
    std::strncpy(out_path, path.c_str(), max_len - 1);
    out_path[max_len - 1] = '\0';
}

// Drum Sampler Controls (BeatPad)
bool myms_drum_load_sample(int pad, const char* filepath) {
    if (!filepath) return false;
    return myms::AudioEngine::instance().loadDrumSample(pad, filepath);
}

void myms_drum_trigger(int pad, float velocity) {
    myms::AudioEngine::instance().triggerDrumPad(pad, velocity);
}

void myms_drum_set_pitch(int pad, float semi) {
    myms::AudioEngine::instance().setDrumPadPitch(pad, semi);
}

float myms_drum_get_pitch(int pad) {
    return myms::AudioEngine::instance().getDrumPadPitch(pad);
}

void myms_drum_set_decay(int pad, float sec) {
    myms::AudioEngine::instance().setDrumPadDecay(pad, sec);
}

float myms_drum_get_decay(int pad) {
    return myms::AudioEngine::instance().getDrumPadDecay(pad);
}

void myms_drum_set_volume(int pad, float vol) {
    myms::AudioEngine::instance().setDrumPadVolume(pad, vol);
}

float myms_drum_get_volume(int pad) {
    return myms::AudioEngine::instance().getDrumPadVolume(pad);
}

void myms_drum_set_pan(int pad, float pan) {
    myms::AudioEngine::instance().setDrumPadPan(pad, pan);
}

float myms_drum_get_pan(int pad) {
    return myms::AudioEngine::instance().getDrumPadPan(pad);
}

void myms_drum_set_reverse(int pad, bool rev) {
    myms::AudioEngine::instance().setDrumPadReverse(pad, rev);
}

bool myms_drum_get_reverse(int pad) {
    return myms::AudioEngine::instance().getDrumPadReverse(pad);
}

void myms_drum_get_name(int pad, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getDrumPadName(pad);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_drum_get_filepath(int pad, char* out_path, int max_len) {
    if (!out_path || max_len <= 0) return;
    std::string path = myms::AudioEngine::instance().getDrumPadFilePath(pad);
    std::strncpy(out_path, path.c_str(), max_len - 1);
    out_path[max_len - 1] = '\0';
}

// Pattern & Song Sequencer
int myms_get_num_patterns() {
    return myms::AudioEngine::instance().getNumPatterns();
}

int myms_get_current_pattern() {
    return myms::AudioEngine::instance().getCurrentPattern();
}

void myms_set_current_pattern(int pat) {
    myms::AudioEngine::instance().setCurrentPattern(pat);
}

int myms_add_pattern() {
    return myms::AudioEngine::instance().addPattern();
}

int myms_duplicate_pattern(int source_pat) {
    return myms::AudioEngine::instance().duplicatePattern(source_pat);
}

bool myms_remove_pattern(int pat) {
    return myms::AudioEngine::instance().removePattern(pat);
}

void myms_set_song_mode(bool song_mode) {
    myms::AudioEngine::instance().setSongMode(song_mode);
}

bool myms_get_song_mode() {
    return myms::AudioEngine::instance().isSongMode();
}

int myms_get_song_order_length() {
    return myms::AudioEngine::instance().getSongOrderLength();
}

int myms_get_song_order_step(int step) {
    return myms::AudioEngine::instance().getSongOrderStep(step);
}

void myms_set_song_order_step(int step, int pat) {
    myms::AudioEngine::instance().setSongOrderStep(step, pat);
}

void myms_add_song_order_step(int pat) {
    myms::AudioEngine::instance().addSongOrderStep(pat);
}

void myms_remove_song_order_step(int step) {
    myms::AudioEngine::instance().removeSongOrderStep(step);
}

int myms_get_song_order_index() {
    return myms::AudioEngine::instance().getSongOrderIndex();
}

void myms_set_song_order_index(int step) {
    myms::AudioEngine::instance().setSongOrderIndex(step);
}

void myms_set_pattern_cell(int pat, int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effect_type, uint8_t effect_param) {
    myms::AudioEngine::instance().setPatternCell(pat, track, row, note, instrument, volume, effect_type, effect_param);
}

void myms_get_pattern_cell(int pat, int track, int row, uint8_t* out_note, uint8_t* out_instrument, uint8_t* out_volume, uint8_t* out_effect_type, uint8_t* out_effect_param) {
    myms::TrackerCell c = myms::AudioEngine::instance().getPatternCell(pat, track, row);
    if (out_note) *out_note = c.note;
    if (out_instrument) *out_instrument = c.instrument;
    if (out_volume) *out_volume = c.volume;
    if (out_effect_type) *out_effect_type = c.effectType;
    if (out_effect_param) *out_effect_param = c.effectParam;
}

// Smart Macro Strip
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

// Tracker Pattern Editing (Current Pattern)
void myms_set_cell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effect_type, uint8_t effect_param) {
    myms::AudioEngine::instance().setTrackerCell(track, row, note, instrument, volume, effect_type, effect_param);
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

// Plugin Host (VST2 & CLAP)
bool myms_plugin_load(int track, const char* filepath) {
    if (!filepath) return false;
    return myms::AudioEngine::instance().loadTrackPlugin(track, filepath);
}

void myms_plugin_unload(int track) {
    myms::AudioEngine::instance().unloadTrackPlugin(track);
}

bool myms_plugin_is_loaded(int track) {
    return myms::AudioEngine::instance().isTrackPluginLoaded(track);
}

void myms_plugin_get_name(int track, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getTrackPluginName(track);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_plugin_get_vendor(int track, char* out_vendor, int max_len) {
    if (!out_vendor || max_len <= 0) return;
    std::string vendor = myms::AudioEngine::instance().getTrackPluginVendor(track);
    std::strncpy(out_vendor, vendor.c_str(), max_len - 1);
    out_vendor[max_len - 1] = '\0';
}

void myms_plugin_get_path(int track, char* out_path, int max_len) {
    if (!out_path || max_len <= 0) return;
    std::string path = myms::AudioEngine::instance().getTrackPluginPath(track);
    std::strncpy(out_path, path.c_str(), max_len - 1);
    out_path[max_len - 1] = '\0';
}

int myms_plugin_get_format(int track) {
    return myms::AudioEngine::instance().getTrackPluginFormat(track);
}

void myms_plugin_get_format_name(int track, char* out_format, int max_len) {
    if (!out_format || max_len <= 0) return;
    std::string fmt = myms::AudioEngine::instance().getTrackPluginFormatName(track);
    std::strncpy(out_format, fmt.c_str(), max_len - 1);
    out_format[max_len - 1] = '\0';
}

int myms_plugin_get_num_params(int track) {
    return myms::AudioEngine::instance().getTrackPluginNumParams(track);
}

void myms_plugin_get_param_name(int track, int param_idx, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getTrackPluginParamName(track, param_idx);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_plugin_get_param_display(int track, int param_idx, char* out_display, int max_len) {
    if (!out_display || max_len <= 0) return;
    std::string display = myms::AudioEngine::instance().getTrackPluginParamDisplay(track, param_idx);
    std::strncpy(out_display, display.c_str(), max_len - 1);
    out_display[max_len - 1] = '\0';
}

void myms_plugin_get_param_label(int track, int param_idx, char* out_label, int max_len) {
    if (!out_label || max_len <= 0) return;
    std::string label = myms::AudioEngine::instance().getTrackPluginParamLabel(track, param_idx);
    std::strncpy(out_label, label.c_str(), max_len - 1);
    out_label[max_len - 1] = '\0';
}

float myms_plugin_get_param_value(int track, int param_idx) {
    return myms::AudioEngine::instance().getTrackPluginParamValue(track, param_idx);
}

void myms_plugin_set_param_value(int track, int param_idx, float val) {
    myms::AudioEngine::instance().setTrackPluginParamValue(track, param_idx, val);
}

bool myms_plugin_is_params_dirty(int track) {
    return myms::AudioEngine::instance().isTrackPluginParamsDirty(track);
}

bool myms_plugin_has_native_editor(int track) {
    return myms::AudioEngine::instance().hasTrackPluginNativeEditor(track);
}

bool myms_plugin_open_native_editor(int track, void* parent_window) {
    return myms::AudioEngine::instance().openTrackPluginNativeEditor(track, parent_window);
}

void myms_plugin_close_native_editor(int track) {
    myms::AudioEngine::instance().closeTrackPluginNativeEditor(track);
}

// ==========================================
// Track Insert FX Controls (VST3, CLAP, VST2)
// ==========================================
bool myms_track_fx_load(int track, int slot, const char* filepath) {
    if (!filepath) return false;
    return myms::AudioEngine::instance().loadTrackFx(track, slot, filepath);
}

void myms_track_fx_unload(int track, int slot) {
    myms::AudioEngine::instance().unloadTrackFx(track, slot);
}

bool myms_track_fx_is_loaded(int track, int slot) {
    return myms::AudioEngine::instance().isTrackFxLoaded(track, slot);
}

int myms_track_fx_get_count(int track) {
    return myms::AudioEngine::instance().getTrackFxCount(track);
}

int myms_get_max_track_fx() {
    return myms::MAX_TRACK_FX;
}

void myms_track_fx_get_name(int track, int slot, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getTrackFxName(track, slot);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_track_fx_get_vendor(int track, int slot, char* out_vendor, int max_len) {
    if (!out_vendor || max_len <= 0) return;
    std::string vendor = myms::AudioEngine::instance().getTrackFxVendor(track, slot);
    std::strncpy(out_vendor, vendor.c_str(), max_len - 1);
    out_vendor[max_len - 1] = '\0';
}

void myms_track_fx_get_path(int track, int slot, char* out_path, int max_len) {
    if (!out_path || max_len <= 0) return;
    std::string path = myms::AudioEngine::instance().getTrackFxPath(track, slot);
    std::strncpy(out_path, path.c_str(), max_len - 1);
    out_path[max_len - 1] = '\0';
}

int myms_track_fx_get_format(int track, int slot) {
    return myms::AudioEngine::instance().getTrackFxFormat(track, slot);
}

void myms_track_fx_get_format_name(int track, int slot, char* out_format, int max_len) {
    if (!out_format || max_len <= 0) return;
    std::string fmt = myms::AudioEngine::instance().getTrackFxFormatName(track, slot);
    std::strncpy(out_format, fmt.c_str(), max_len - 1);
    out_format[max_len - 1] = '\0';
}

int myms_track_fx_get_num_params(int track, int slot) {
    return myms::AudioEngine::instance().getTrackFxNumParams(track, slot);
}

void myms_track_fx_get_param_name(int track, int slot, int param_idx, char* out_name, int max_len) {
    if (!out_name || max_len <= 0) return;
    std::string name = myms::AudioEngine::instance().getTrackFxParamName(track, slot, param_idx);
    std::strncpy(out_name, name.c_str(), max_len - 1);
    out_name[max_len - 1] = '\0';
}

void myms_track_fx_get_param_display(int track, int slot, int param_idx, char* out_display, int max_len) {
    if (!out_display || max_len <= 0) return;
    std::string disp = myms::AudioEngine::instance().getTrackFxParamDisplay(track, slot, param_idx);
    std::strncpy(out_display, disp.c_str(), max_len - 1);
    out_display[max_len - 1] = '\0';
}

float myms_track_fx_get_param_value(int track, int slot, int param_idx) {
    return myms::AudioEngine::instance().getTrackFxParamValue(track, slot, param_idx);
}

void myms_track_fx_set_param_value(int track, int slot, int param_idx, float val) {
    myms::AudioEngine::instance().setTrackFxParamValue(track, slot, param_idx, val);
}

bool myms_track_fx_is_params_dirty(int track, int slot) {
    return myms::AudioEngine::instance().isTrackFxParamsDirty(track, slot);
}

bool myms_track_fx_has_native_editor(int track, int slot) {
    return myms::AudioEngine::instance().hasTrackFxEditor(track, slot);
}

bool myms_track_fx_open_native_editor(int track, int slot, void* parent_window) {
    return myms::AudioEngine::instance().openTrackFxEditor(track, slot, parent_window);
}

void myms_track_fx_close_native_editor(int track, int slot) {
    myms::AudioEngine::instance().closeTrackFxEditor(track, slot);
}
