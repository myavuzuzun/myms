#ifndef MYMS_C_API_H
#define MYMS_C_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Engine lifecycle
bool myms_init(int sample_rate, int buffer_size);
void myms_shutdown();

// Playback & Clock
void myms_play();
void myms_pause();
void myms_stop();
bool myms_is_playing();
void myms_set_bpm(float bpm);
float myms_get_bpm();
int myms_get_current_row();
void myms_set_current_row(int row);
int myms_get_num_rows();
void myms_set_num_rows(int rows);
void myms_set_looping(bool loop);
bool myms_is_looping();

// Dynamic Tracks & Types (0 = Synth, 1 = SoundFont, 2 = Drum Sampler)
int myms_get_num_tracks();
void myms_add_track();
void myms_remove_track(int track_idx);
void myms_set_track_type(int track, int type);
int myms_get_track_type(int track);

// Custom Instrument Naming
void myms_set_inst_name(int inst, const char* name);
void myms_get_inst_name(int inst, char* out_name, int max_len);

// Live Preview per Instrument
void myms_note_on_inst(int inst, int note, float velocity);
void myms_note_off_inst(int inst, int note);

// Multi-Instrument Parameters (inst: 0..31)
void myms_set_inst_osc1(int inst, int wave);
void myms_set_inst_osc2(int inst, int wave);
void myms_set_inst_osc_mix(int inst, float mix);
void myms_set_inst_detune(int inst, float semi, float cents);
void myms_set_inst_filter(int inst, float cutoff, float resonance, float env_amount);
void myms_set_inst_amp_env(int inst, float a, float d, float s, float r);
void myms_set_inst_filter_env(int inst, float a, float d, float s, float r);

void myms_get_inst_params(
    int inst,
    int* out_osc1, int* out_osc2, float* out_mix,
    float* out_detune_semi, float* out_detune_cents,
    float* out_cutoff, float* out_res, float* out_env_amt,
    float* out_amp_a, float* out_amp_d, float* out_amp_s, float* out_amp_r
);

// SoundFont Player (FontBox)
bool myms_sf2_load(const char* filepath);
bool myms_sf2_is_loaded();
int myms_sf2_get_preset_count();
void myms_sf2_get_preset_name(int index, char* out_name, int max_len);
void myms_sf2_set_preset(int index);
int myms_sf2_get_preset();
void myms_sf2_set_volume(float vol);
float myms_sf2_get_volume();
void myms_sf2_get_path(char* out_path, int max_len);

// Drum Sampler (BeatPad)
bool myms_drum_load_sample(int pad, const char* filepath);
void myms_drum_trigger(int pad, float velocity);
void myms_drum_set_pitch(int pad, float semi);
float myms_drum_get_pitch(int pad);
void myms_drum_set_decay(int pad, float sec);
float myms_drum_get_decay(int pad);
void myms_drum_set_volume(int pad, float vol);
float myms_drum_get_volume(int pad);
void myms_drum_set_pan(int pad, float pan);
float myms_drum_get_pan(int pad);
void myms_drum_set_reverse(int pad, bool rev);
bool myms_drum_get_reverse(int pad);
void myms_drum_get_name(int pad, char* out_name, int max_len);
void myms_drum_get_filepath(int pad, char* out_path, int max_len);

// Pattern & Song Sequencer
int myms_get_num_patterns();
int myms_get_current_pattern();
void myms_set_current_pattern(int pat);
int myms_add_pattern();
int myms_duplicate_pattern(int source_pat);
bool myms_remove_pattern(int pat);
void myms_set_song_mode(bool song_mode);
bool myms_get_song_mode();

void myms_set_pattern_cell(int pat, int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effect_type, uint8_t effect_param);
void myms_get_pattern_cell(int pat, int track, int row, uint8_t* out_note, uint8_t* out_instrument, uint8_t* out_volume, uint8_t* out_effect_type, uint8_t* out_effect_param);

// Smart Macro Strip
void myms_set_macro_punch(float punch);
void myms_set_macro_warmth(float warmth);
void myms_set_macro_tone(float tone);
void myms_set_macro_space(float space);
void myms_set_master_volume(float volume);

// Real-Time Metering
void myms_get_meters(float* out_peak_l, float* out_peak_r, float* out_rms);

// Tracker Pattern Editing (Current Pattern)
void myms_set_cell(int track, int row, uint8_t note, uint8_t instrument, uint8_t volume, uint8_t effect_type, uint8_t effect_param);
void myms_get_cell(int track, int row, uint8_t* out_note, uint8_t* out_instrument, uint8_t* out_volume, uint8_t* out_effect_type, uint8_t* out_effect_param);
void myms_clear_track(int track);
void myms_clear_all_tracks();
void myms_transpose_track(int track, int semitones);
void myms_load_demo();

#ifdef __cplusplus
}
#endif

#endif // MYMS_C_API_H
