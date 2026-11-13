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

// Dynamic Tracks
int myms_get_num_tracks();
void myms_add_track();
void myms_remove_track(int track_idx);

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

// Smart Macro Strip
void myms_set_macro_punch(float punch);
void myms_set_macro_warmth(float warmth);
void myms_set_macro_tone(float tone);
void myms_set_macro_space(float space);
void myms_set_master_volume(float volume);

// Real-Time Metering
void myms_get_meters(float* out_peak_l, float* out_peak_r, float* out_rms);

// Tracker Pattern Editing
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
