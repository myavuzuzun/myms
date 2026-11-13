import os
import sys
import ctypes
from PyQt6.QtCore import QObject, pyqtSignal, pyqtProperty, pyqtSlot, QTimer

class MymsEngine(QObject):
    playingChanged = pyqtSignal(bool)
    bpmChanged = pyqtSignal(float)
    currentRowChanged = pyqtSignal(int)
    numTracksChanged = pyqtSignal(int)
    numRowsChanged = pyqtSignal(int)
    loopingChanged = pyqtSignal(bool)
    editModeChanged = pyqtSignal(bool)
    baseOctaveChanged = pyqtSignal(int)
    metersUpdated = pyqtSignal(float, float, float)
    gridCellChanged = pyqtSignal(int, int)
    instrumentChanged = pyqtSignal(int)
    instrumentNameChanged = pyqtSignal(int, str)
    keyboardLayoutChanged = pyqtSignal(str)
    pausedChanged = pyqtSignal(bool)
    projectLoaded = pyqtSignal()
    projectSaved = pyqtSignal(str)
    currentProjectPathChanged = pyqtSignal(str)
    macrosChanged = pyqtSignal()
    requestOpenProject = pyqtSignal()
    requestSaveProject = pyqtSignal()
    requestSaveProjectAs = pyqtSignal()
    requestNewProject = pyqtSignal()

    # Phase 2 Signals
    trackTypeChanged = pyqtSignal(int, int)
    trackMixerChanged = pyqtSignal(int)
    sf2Changed = pyqtSignal()
    drumPadsChanged = pyqtSignal()
    numPatternsChanged = pyqtSignal(int)
    currentPatternChanged = pyqtSignal(int)
    songModeChanged = pyqtSignal(bool)
    songOrderChanged = pyqtSignal()
    songOrderIndexChanged = pyqtSignal(int)

    # Phase 3 Signals (Unified Plugin Host & Track Insert FX)
    trackPluginChanged = pyqtSignal(int)
    trackPluginParamsChanged = pyqtSignal(int)
    requestOpenPluginDialog = pyqtSignal(int)
    requestOpenPluginGui = pyqtSignal(int)
    trackFxChanged = pyqtSignal(int, int)
    requestOpenTrackFxDialog = pyqtSignal(int, int)
    requestOpenTrackFxGui = pyqtSignal(int, int)

    def _detect_system_keyboard_layout(self):
        try:
            if os.path.exists("/etc/default/keyboard"):
                with open("/etc/default/keyboard", "r", encoding="utf-8") as f:
                    content = f.read().lower()
                    if 'xkblayout="tr"' in content or "xkblayout='tr'" in content or "xkblayout=tr" in content:
                        return "tr"
        except Exception:
            pass

        for var in ("LANG", "LC_ALL", "LC_MESSAGES"):
            val = os.environ.get(var, "").lower()
            if "tr_" in val or "turkish" in val:
                return "tr"

        try:
            import subprocess
            out = subprocess.check_output(["setxkbmap", "-query"], stderr=subprocess.DEVNULL, text=True)
            if "layout:     tr" in out or "layout:\ttr" in out or " tr" in out:
                return "tr"
        except Exception:
            pass

        return "tr"

    def __init__(self, parent=None):
        super().__init__(parent)
        
        lib_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "libmyms_audio.so")
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Cannot find audio library at {lib_path}")
            
        self.lib = ctypes.CDLL(lib_path)
        self._setup_c_signatures()

        self._is_initialized = self.lib.myms_init(48000, 256)
        if not self._is_initialized:
            print("[myms] Warning: Could not initialize audio output hardware.")

        self._is_playing = False
        self._bpm = 120.0
        self._current_row = 0
        self._current_pattern = 0
        self._num_patterns = 1
        self._is_song_mode = False
        self._song_order_index = 0
        self._num_rows = self.lib.myms_get_num_rows()
        self._num_tracks = self.lib.myms_get_num_tracks()
        self._is_looping = self.lib.myms_is_looping()
        self._is_edit_mode = True
        self._base_octave = 4
        self._selected_instrument = 0
        self._keyboard_layout = self._detect_system_keyboard_layout()
        self._is_paused = False
        self._current_project_path = ""

        self._macro_punch = 0.2
        self._macro_warmth = 0.25
        self._macro_tone = 0.5
        self._macro_space = 0.15
        self._master_volume = 0.85

        self._timer = QTimer(self)
        self._timer.setInterval(16)
        self._timer.timeout.connect(self._on_ui_tick)
        self._timer.start()

    def _setup_c_signatures(self):
        self.lib.myms_init.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_init.restype = ctypes.c_bool
        self.lib.myms_shutdown.restype = None

        self.lib.myms_play.restype = None
        self.lib.myms_pause.restype = None
        self.lib.myms_stop.restype = None
        self.lib.myms_is_playing.restype = ctypes.c_bool
        self.lib.myms_set_bpm.argtypes = [ctypes.c_float]
        self.lib.myms_get_bpm.restype = ctypes.c_float
        self.lib.myms_get_current_row.restype = ctypes.c_int
        self.lib.myms_set_current_row.argtypes = [ctypes.c_int]
        self.lib.myms_get_num_rows.restype = ctypes.c_int
        self.lib.myms_set_num_rows.argtypes = [ctypes.c_int]
        self.lib.myms_set_looping.argtypes = [ctypes.c_bool]
        self.lib.myms_is_looping.restype = ctypes.c_bool

        self.lib.myms_get_num_tracks.restype = ctypes.c_int
        self.lib.myms_add_track.restype = None
        self.lib.myms_remove_track.argtypes = [ctypes.c_int]
        self.lib.myms_set_track_type.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_get_track_type.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_type.restype = ctypes.c_int

        # Mixer Controls
        self.lib.myms_set_track_volume.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_get_track_volume.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_volume.restype = ctypes.c_float
        self.lib.myms_set_track_pan.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_get_track_pan.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_pan.restype = ctypes.c_float
        self.lib.myms_set_track_mute.argtypes = [ctypes.c_int, ctypes.c_bool]
        self.lib.myms_get_track_mute.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_mute.restype = ctypes.c_bool
        self.lib.myms_set_track_solo.argtypes = [ctypes.c_int, ctypes.c_bool]
        self.lib.myms_get_track_solo.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_solo.restype = ctypes.c_bool
        self.lib.myms_set_track_sf2_preset.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_get_track_sf2_preset.argtypes = [ctypes.c_int]
        self.lib.myms_get_track_sf2_preset.restype = ctypes.c_int
        self.lib.myms_get_track_meters.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]

        self.lib.myms_set_inst_name.argtypes = [ctypes.c_int, ctypes.c_char_p]
        self.lib.myms_get_inst_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]

        self.lib.myms_note_on_inst.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float]
        self.lib.myms_note_off_inst.argtypes = [ctypes.c_int, ctypes.c_int]

        self.lib.myms_set_inst_osc1.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_set_inst_osc2.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_set_inst_osc_mix.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_set_inst_detune.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float]
        self.lib.myms_set_inst_filter.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.myms_set_inst_amp_env.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.myms_set_inst_filter_env.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]

        self.lib.myms_get_inst_params.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)
        ]

        # SoundFont (FontBox)
        self.lib.myms_sf2_load.argtypes = [ctypes.c_char_p]
        self.lib.myms_sf2_load.restype = ctypes.c_bool
        self.lib.myms_sf2_is_loaded.restype = ctypes.c_bool
        self.lib.myms_sf2_get_preset_count.restype = ctypes.c_int
        self.lib.myms_sf2_get_preset_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_sf2_set_preset.argtypes = [ctypes.c_int]
        self.lib.myms_sf2_get_preset.restype = ctypes.c_int
        self.lib.myms_sf2_set_volume.argtypes = [ctypes.c_float]
        self.lib.myms_sf2_get_volume.restype = ctypes.c_float
        self.lib.myms_sf2_get_path.argtypes = [ctypes.c_char_p, ctypes.c_int]

        # Drum Sampler (BeatPad)
        self.lib.myms_drum_load_sample.argtypes = [ctypes.c_int, ctypes.c_char_p]
        self.lib.myms_drum_load_sample.restype = ctypes.c_bool
        self.lib.myms_drum_trigger.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_drum_set_pitch.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_drum_get_pitch.argtypes = [ctypes.c_int]
        self.lib.myms_drum_get_pitch.restype = ctypes.c_float
        self.lib.myms_drum_set_decay.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_drum_get_decay.argtypes = [ctypes.c_int]
        self.lib.myms_drum_get_decay.restype = ctypes.c_float
        self.lib.myms_drum_set_volume.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_drum_get_volume.argtypes = [ctypes.c_int]
        self.lib.myms_drum_get_volume.restype = ctypes.c_float
        self.lib.myms_drum_set_pan.argtypes = [ctypes.c_int, ctypes.c_float]
        self.lib.myms_drum_get_pan.argtypes = [ctypes.c_int]
        self.lib.myms_drum_get_pan.restype = ctypes.c_float
        self.lib.myms_drum_set_reverse.argtypes = [ctypes.c_int, ctypes.c_bool]
        self.lib.myms_drum_get_reverse.argtypes = [ctypes.c_int]
        self.lib.myms_drum_get_reverse.restype = ctypes.c_bool
        self.lib.myms_drum_get_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_drum_get_filepath.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]

        # Plugin Host (VST2 & CLAP)
        self.lib.myms_plugin_load.argtypes = [ctypes.c_int, ctypes.c_char_p]
        self.lib.myms_plugin_load.restype = ctypes.c_bool
        self.lib.myms_plugin_unload.argtypes = [ctypes.c_int]
        self.lib.myms_plugin_is_loaded.argtypes = [ctypes.c_int]
        self.lib.myms_plugin_is_loaded.restype = ctypes.c_bool
        self.lib.myms_plugin_get_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_vendor.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_path.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_format.argtypes = [ctypes.c_int]
        self.lib.myms_plugin_get_format.restype = ctypes.c_int
        self.lib.myms_plugin_get_format_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_num_params.argtypes = [ctypes.c_int]
        self.lib.myms_plugin_get_num_params.restype = ctypes.c_int
        self.lib.myms_plugin_get_param_name.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_param_display.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_param_label.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_plugin_get_param_value.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_plugin_get_param_value.restype = ctypes.c_float
        self.lib.myms_plugin_set_param_value.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float]
        self.lib.myms_plugin_has_native_editor.argtypes = [ctypes.c_int]
        self.lib.myms_plugin_has_native_editor.restype = ctypes.c_bool
        self.lib.myms_plugin_open_native_editor.argtypes = [ctypes.c_int, ctypes.c_void_p]
        self.lib.myms_plugin_open_native_editor.restype = ctypes.c_bool
        self.lib.myms_plugin_close_native_editor.argtypes = [ctypes.c_int]

        # Track FX Host (VST3, CLAP, VST2)
        self.lib.myms_track_fx_load.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
        self.lib.myms_track_fx_load.restype = ctypes.c_bool
        self.lib.myms_track_fx_unload.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_is_loaded.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_is_loaded.restype = ctypes.c_bool
        self.lib.myms_track_fx_get_name.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_track_fx_get_vendor.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_track_fx_get_path.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_track_fx_get_format.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_get_format.restype = ctypes.c_int
        self.lib.myms_track_fx_get_format_name.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_track_fx_get_num_params.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_get_num_params.restype = ctypes.c_int
        self.lib.myms_track_fx_get_param_name.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.myms_track_fx_get_param_value.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_get_param_value.restype = ctypes.c_float
        self.lib.myms_track_fx_set_param_value.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_float]
        self.lib.myms_track_fx_has_native_editor.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_track_fx_has_native_editor.restype = ctypes.c_bool
        self.lib.myms_track_fx_open_native_editor.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
        self.lib.myms_track_fx_open_native_editor.restype = ctypes.c_bool
        self.lib.myms_track_fx_close_native_editor.argtypes = [ctypes.c_int, ctypes.c_int]

        # Pattern & Song Sequencer
        self.lib.myms_get_num_patterns.restype = ctypes.c_int
        self.lib.myms_get_current_pattern.restype = ctypes.c_int
        self.lib.myms_set_current_pattern.argtypes = [ctypes.c_int]
        self.lib.myms_add_pattern.restype = ctypes.c_int
        self.lib.myms_duplicate_pattern.argtypes = [ctypes.c_int]
        self.lib.myms_duplicate_pattern.restype = ctypes.c_int
        self.lib.myms_remove_pattern.argtypes = [ctypes.c_int]
        self.lib.myms_remove_pattern.restype = ctypes.c_bool
        self.lib.myms_set_song_mode.argtypes = [ctypes.c_bool]
        self.lib.myms_get_song_mode.restype = ctypes.c_bool

        # Song Order Sequencer
        self.lib.myms_get_song_order_length.restype = ctypes.c_int
        self.lib.myms_get_song_order_step.argtypes = [ctypes.c_int]
        self.lib.myms_get_song_order_step.restype = ctypes.c_int
        self.lib.myms_set_song_order_step.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_add_song_order_step.argtypes = [ctypes.c_int]
        self.lib.myms_remove_song_order_step.argtypes = [ctypes.c_int]
        self.lib.myms_get_song_order_index.restype = ctypes.c_int
        self.lib.myms_set_song_order_index.argtypes = [ctypes.c_int]
        self.lib.myms_set_pattern_cell.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8]
        self.lib.myms_get_pattern_cell.argtypes = [
            ctypes.c_int, ctypes.c_int, ctypes.c_int,
            ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8)
        ]

        self.lib.myms_set_macro_punch.argtypes = [ctypes.c_float]
        self.lib.myms_set_macro_warmth.argtypes = [ctypes.c_float]
        self.lib.myms_set_macro_tone.argtypes = [ctypes.c_float]
        self.lib.myms_set_macro_space.argtypes = [ctypes.c_float]
        self.lib.myms_set_master_volume.argtypes = [ctypes.c_float]

        self.lib.myms_get_meters.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]

        self.lib.myms_set_cell.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8]
        self.lib.myms_get_cell.argtypes = [
            ctypes.c_int, ctypes.c_int,
            ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8)
        ]

        self.lib.myms_clear_track.argtypes = [ctypes.c_int]
        self.lib.myms_clear_all_tracks.restype = None
        self.lib.myms_transpose_track.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.myms_load_demo.restype = None

    def _on_ui_tick(self):
        playing = self.lib.myms_is_playing()
        if playing != self._is_playing:
            self._is_playing = playing
            self.playingChanged.emit(self._is_playing)

        row = self.lib.myms_get_current_row()
        if row != self._current_row:
            self._current_row = row
            self.currentRowChanged.emit(self._current_row)

        pat = self.lib.myms_get_current_pattern()
        if pat != self._current_pattern:
            self._current_pattern = pat
            self.currentPatternChanged.emit(self._current_pattern)

        so_idx = self.lib.myms_get_song_order_index()
        if so_idx != self._song_order_index:
            self._song_order_index = so_idx
            self.songOrderIndexChanged.emit(self._song_order_index)

        tracks = self.lib.myms_get_num_tracks()
        if tracks != self._num_tracks:
            self._num_tracks = tracks
            self.numTracksChanged.emit(self._num_tracks)

        pL = ctypes.c_float(0)
        pR = ctypes.c_float(0)
        rms = ctypes.c_float(0)
        self.lib.myms_get_meters(ctypes.byref(pL), ctypes.byref(pR), ctypes.byref(rms))
        self.metersUpdated.emit(pL.value, pR.value, rms.value)

    @pyqtProperty(bool, notify=playingChanged)
    def isPlaying(self):
        return self._is_playing

    @pyqtProperty(float, notify=bpmChanged)
    def bpm(self):
        return self.lib.myms_get_bpm()

    @pyqtProperty(int, notify=currentRowChanged)
    def currentRow(self):
        return self._current_row

    @pyqtProperty(int, notify=numRowsChanged)
    def numRows(self):
        return self._num_rows

    @pyqtProperty(int, notify=numTracksChanged)
    def numTracks(self):
        return self._num_tracks

    @pyqtProperty(bool, notify=loopingChanged)
    def isLooping(self):
        return self._is_looping

    @pyqtProperty(bool, notify=pausedChanged)
    def isPaused(self):
        return self._is_paused

    @pyqtProperty(str, notify=currentProjectPathChanged)
    def currentProjectPath(self):
        return self._current_project_path

    @pyqtProperty(str, notify=currentProjectPathChanged)
    def projectName(self):
        if self._current_project_path:
            return os.path.splitext(os.path.basename(self._current_project_path))[0]
        return ""

    @pyqtProperty(float, notify=macrosChanged)
    def macroPunch(self):
        return self._macro_punch

    @pyqtProperty(float, notify=macrosChanged)
    def macroWarmth(self):
        return self._macro_warmth

    @pyqtProperty(float, notify=macrosChanged)
    def macroTone(self):
        return self._macro_tone

    @pyqtProperty(float, notify=macrosChanged)
    def macroSpace(self):
        return self._macro_space

    @pyqtProperty(float, notify=macrosChanged)
    def masterVolume(self):
        return self._master_volume

    @pyqtProperty(bool, notify=editModeChanged)
    def isEditMode(self):
        return self._is_edit_mode

    @pyqtProperty(int, notify=baseOctaveChanged)
    def baseOctave(self):
        return self._base_octave

    @pyqtProperty(str, notify=keyboardLayoutChanged)
    def keyboardLayout(self):
        return self._keyboard_layout

    # Phase 2 Properties
    @pyqtProperty(bool, notify=sf2Changed)
    def sf2Loaded(self):
        return self.lib.myms_sf2_is_loaded()

    @pyqtProperty(str, notify=sf2Changed)
    def sf2Path(self):
        buf = ctypes.create_string_buffer(512)
        self.lib.myms_sf2_get_path(buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtProperty(int, notify=sf2Changed)
    def sf2PresetCount(self):
        return self.lib.myms_sf2_get_preset_count()

    @pyqtProperty(int, notify=sf2Changed)
    def sf2CurrentPreset(self):
        return self.lib.myms_sf2_get_preset()

    @pyqtProperty(float, notify=sf2Changed)
    def sf2Volume(self):
        return self.lib.myms_sf2_get_volume()

    @pyqtProperty(int, notify=numPatternsChanged)
    def numPatterns(self):
        return self.lib.myms_get_num_patterns()

    @pyqtProperty(int, notify=currentPatternChanged)
    def currentPattern(self):
        return self.lib.myms_get_current_pattern()

    @pyqtProperty(bool, notify=songModeChanged)
    def isSongMode(self):
        return self.lib.myms_get_song_mode()

    @pyqtProperty(int, notify=songOrderIndexChanged)
    def songOrderIndex(self):
        return self._song_order_index

    @pyqtProperty(int, notify=songOrderChanged)
    def songOrderLength(self):
        return self.lib.myms_get_song_order_length()

    @pyqtSlot(str)
    def setKeyboardLayout(self, layout):
        if layout in ("tr", "us") and layout != self._keyboard_layout:
            self._keyboard_layout = layout
            self.keyboardLayoutChanged.emit(self._keyboard_layout)

    @pyqtSlot()
    def toggleKeyboardLayout(self):
        new_layout = "us" if self._keyboard_layout == "tr" else "tr"
        self.setKeyboardLayout(new_layout)

    @pyqtProperty(int, notify=instrumentChanged)
    def selectedInstrument(self):
        return self._selected_instrument

    @pyqtSlot(int)
    def setSelectedInstrument(self, inst):
        if inst >= 0 and inst < self.numTracks and inst != self._selected_instrument:
            self._selected_instrument = inst
            self.instrumentChanged.emit(self._selected_instrument)

    # Track Engine Type Slot (0 = Synth, 1 = SoundFont, 2 = Drum Sampler)
    @pyqtSlot(int, result=int)
    def getTrackType(self, track):
        return self.lib.myms_get_track_type(track)

    @pyqtSlot(int, int)
    def setTrackType(self, track, type_idx):
        self.lib.myms_set_track_type(track, type_idx)
        self.trackTypeChanged.emit(track, type_idx)

    # Per-Track Mixer Slots (MuseScore-style Channel Strip)
    @pyqtSlot(int, result=float)
    def getTrackVolume(self, track):
        return self.lib.myms_get_track_volume(track)

    @pyqtSlot(int, float)
    def setTrackVolume(self, track, vol):
        self.lib.myms_set_track_volume(track, vol)
        self.trackMixerChanged.emit(track)

    @pyqtSlot(int, result=float)
    def getTrackPan(self, track):
        return self.lib.myms_get_track_pan(track)

    @pyqtSlot(int, float)
    def setTrackPan(self, track, pan):
        self.lib.myms_set_track_pan(track, pan)
        self.trackMixerChanged.emit(track)

    @pyqtSlot(int, result=bool)
    def getTrackMute(self, track):
        return self.lib.myms_get_track_mute(track)

    @pyqtSlot(int, bool)
    def setTrackMute(self, track, mute):
        self.lib.myms_set_track_mute(track, mute)
        self.trackMixerChanged.emit(track)

    @pyqtSlot(int, result=bool)
    def getTrackSolo(self, track):
        return self.lib.myms_get_track_solo(track)

    @pyqtSlot(int, bool)
    def setTrackSolo(self, track, solo):
        self.lib.myms_set_track_solo(track, solo)
        self.trackMixerChanged.emit(track)

    @pyqtSlot(int, result=int)
    def getTrackSf2Preset(self, track):
        return self.lib.myms_get_track_sf2_preset(track)

    @pyqtSlot(int, int)
    def setTrackSf2Preset(self, track, preset):
        self.lib.myms_set_track_sf2_preset(track, preset)
        self.trackMixerChanged.emit(track)

    @pyqtSlot(int, result=list)
    def getTrackMeters(self, track):
        pL = ctypes.c_float(0)
        pR = ctypes.c_float(0)
        self.lib.myms_get_track_meters(track, ctypes.byref(pL), ctypes.byref(pR))
        return [pL.value, pR.value]

    # SoundFont Player Slots (FontBox)
    @pyqtSlot(str, result=bool)
    def loadSoundFont(self, filepath):
        ok = self.lib.myms_sf2_load(filepath.encode('utf-8'))
        if ok:
            self.sf2Changed.emit()
        return ok

    @pyqtSlot(int)
    def setSoundFontPreset(self, index):
        self.lib.myms_sf2_set_preset(index)
        self.sf2Changed.emit()

    @pyqtSlot(int, result=str)
    def getSoundFontPresetName(self, index):
        buf = ctypes.create_string_buffer(256)
        self.lib.myms_sf2_get_preset_name(index, buf, 256)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(float)
    def setSoundFontVolume(self, vol):
        self.lib.myms_sf2_set_volume(vol)
        self.sf2Changed.emit()

    # Drum Sampler Slots (BeatPad)
    @pyqtSlot(int, float)
    def triggerDrumPad(self, pad, velocity=1.0):
        self.lib.myms_drum_trigger(pad, ctypes.c_float(velocity))

    @pyqtSlot(int, str, result=bool)
    def loadDrumSample(self, pad, filepath):
        ok = self.lib.myms_drum_load_sample(pad, filepath.encode('utf-8'))
        if ok:
            self.drumPadsChanged.emit()
        return ok

    @pyqtSlot(int, result=str)
    def getDrumPadName(self, pad):
        buf = ctypes.create_string_buffer(256)
        self.lib.myms_drum_get_name(pad, buf, 256)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, result=str)
    def getDrumPadFilePath(self, pad):
        buf = ctypes.create_string_buffer(512)
        self.lib.myms_drum_get_filepath(pad, buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, result=float)
    def getDrumPadPitch(self, pad):
        return self.lib.myms_drum_get_pitch(pad)

    @pyqtSlot(int, float)
    def setDrumPadPitch(self, pad, semi):
        self.lib.myms_drum_set_pitch(pad, ctypes.c_float(semi))
        self.drumPadsChanged.emit()

    @pyqtSlot(int, result=float)
    def getDrumPadDecay(self, pad):
        return self.lib.myms_drum_get_decay(pad)

    @pyqtSlot(int, float)
    def setDrumPadDecay(self, pad, sec):
        self.lib.myms_drum_set_decay(pad, ctypes.c_float(sec))
        self.drumPadsChanged.emit()

    @pyqtSlot(int, result=float)
    def getDrumPadVolume(self, pad):
        return self.lib.myms_drum_get_volume(pad)

    @pyqtSlot(int, float)
    def setDrumPadVolume(self, pad, vol):
        self.lib.myms_drum_set_volume(pad, ctypes.c_float(vol))
        self.drumPadsChanged.emit()

    @pyqtSlot(int, result=float)
    def getDrumPadPan(self, pad):
        return self.lib.myms_drum_get_pan(pad)

    @pyqtSlot(int, float)
    def setDrumPadPan(self, pad, pan):
        self.lib.myms_drum_set_pan(pad, ctypes.c_float(pan))
        self.drumPadsChanged.emit()

    @pyqtSlot(int, result=bool)
    def getDrumPadReverse(self, pad):
        return self.lib.myms_drum_get_reverse(pad)

    @pyqtSlot(int, bool)
    def setDrumPadReverse(self, pad, rev):
        self.lib.myms_drum_set_reverse(pad, ctypes.c_bool(rev))
        self.drumPadsChanged.emit()

    # Pattern & Song Sequencer Slots
    @pyqtSlot(int)
    def setCurrentPattern(self, pat):
        self.lib.myms_set_current_pattern(pat)
        self._current_pattern = self.lib.myms_get_current_pattern()
        self.currentPatternChanged.emit(self._current_pattern)
        self.gridCellChanged.emit(0, 0)

    @pyqtSlot(result=int)
    def addPattern(self):
        idx = self.lib.myms_add_pattern()
        self._num_patterns = self.lib.myms_get_num_patterns()
        self._current_pattern = idx
        self.numPatternsChanged.emit(self._num_patterns)
        self.currentPatternChanged.emit(self._current_pattern)
        self.songOrderChanged.emit()
        so_idx = self.lib.myms_get_song_order_index()
        self._song_order_index = so_idx
        self.songOrderIndexChanged.emit(so_idx)
        self.gridCellChanged.emit(0, 0)
        return idx

    @pyqtSlot(int, result=int)
    def duplicatePattern(self, source_pat):
        idx = self.lib.myms_duplicate_pattern(source_pat)
        self._num_patterns = self.lib.myms_get_num_patterns()
        self._current_pattern = idx
        self.numPatternsChanged.emit(self._num_patterns)
        self.currentPatternChanged.emit(self._current_pattern)
        self.songOrderChanged.emit()
        so_idx = self.lib.myms_get_song_order_index()
        self._song_order_index = so_idx
        self.songOrderIndexChanged.emit(so_idx)
        self.gridCellChanged.emit(0, 0)
        return idx

    @pyqtSlot(int, result=bool)
    def removePattern(self, pat):
        ok = self.lib.myms_remove_pattern(pat)
        if ok:
            self._num_patterns = self.lib.myms_get_num_patterns()
            self._current_pattern = self.lib.myms_get_current_pattern()
            self.numPatternsChanged.emit(self._num_patterns)
            self.currentPatternChanged.emit(self._current_pattern)
            self.songOrderChanged.emit()
            so_idx = self.lib.myms_get_song_order_index()
            self._song_order_index = so_idx
            self.songOrderIndexChanged.emit(so_idx)
            self.gridCellChanged.emit(0, 0)
        return ok

    @pyqtSlot(bool)
    def setSongMode(self, song_mode):
        self.lib.myms_set_song_mode(song_mode)
        self._is_song_mode = song_mode
        self.songModeChanged.emit(self._is_song_mode)
        cur_pat = self.lib.myms_get_current_pattern()
        self._current_pattern = cur_pat
        self.currentPatternChanged.emit(cur_pat)
        so_idx = self.lib.myms_get_song_order_index()
        self._song_order_index = so_idx
        self.songOrderIndexChanged.emit(so_idx)
        self._current_row = self.lib.myms_get_current_row()
        self.currentRowChanged.emit(self._current_row)
        self.gridCellChanged.emit(0, 0)

    @pyqtSlot()
    def toggleSongMode(self):
        self.setSongMode(not self.isSongMode)

    # Song Order Sequencer Slots
    @pyqtSlot(result=list)
    def getSongOrderList(self):
        length = self.lib.myms_get_song_order_length()
        return [self.lib.myms_get_song_order_step(i) for i in range(length)]

    @pyqtSlot(int, result=int)
    def getSongOrderStep(self, step):
        return self.lib.myms_get_song_order_step(step)

    @pyqtSlot(int, int)
    def setSongOrderStep(self, step, pat):
        self.lib.myms_set_song_order_step(step, pat)
        self.songOrderChanged.emit()

    @pyqtSlot(int)
    def addSongOrderStep(self, pat):
        self.lib.myms_add_song_order_step(pat)
        self.songOrderChanged.emit()

    @pyqtSlot(int)
    def removeSongOrderStep(self, step):
        self.lib.myms_remove_song_order_step(step)
        self.songOrderChanged.emit()
        so_idx = self.lib.myms_get_song_order_index()
        self._song_order_index = so_idx
        self.songOrderIndexChanged.emit(so_idx)
        cur_pat = self.lib.myms_get_current_pattern()
        self._current_pattern = cur_pat
        self.currentPatternChanged.emit(cur_pat)
        self.gridCellChanged.emit(0, 0)

    # ==========================================
    # Phase 3: Linux VST2 & CLAP Plugin Host API
    # ==========================================
    @pyqtSlot(int, str, result=bool)
    def loadTrackPlugin(self, track, filepath):
        if not filepath or not os.path.exists(filepath):
            return False
        ok = self.lib.myms_plugin_load(track, filepath.encode('utf-8'))
        if ok:
            new_type = self.lib.myms_get_track_type(track)
            self.trackTypeChanged.emit(track, new_type)
            self.trackPluginChanged.emit(track)
            self.trackPluginParamsChanged.emit(track)
            print(f"[myms] Loaded plugin on track {track}: {self.getTrackPluginName(track)} ({self.getTrackPluginFormatName(track)})")
        return ok

    @pyqtSlot(int)
    def unloadTrackPlugin(self, track):
        self.lib.myms_plugin_unload(track)
        self.trackPluginChanged.emit(track)
        self.trackPluginParamsChanged.emit(track)

    @pyqtSlot(int, result=bool)
    def isTrackPluginLoaded(self, track):
        return bool(self.lib.myms_plugin_is_loaded(track))

    @pyqtSlot(int, result=str)
    def getTrackPluginName(self, track):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_plugin_get_name(track, buf, 64)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, result=str)
    def getTrackPluginVendor(self, track):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_plugin_get_vendor(track, buf, 64)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, result=str)
    def getTrackPluginPath(self, track):
        buf = ctypes.create_string_buffer(512)
        self.lib.myms_plugin_get_path(track, buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, result=int)
    def getTrackPluginFormat(self, track):
        return self.lib.myms_plugin_get_format(track)

    @pyqtSlot(int, result=str)
    def getTrackPluginFormatName(self, track):
        fmt = self.lib.myms_plugin_get_format(track)
        if fmt == 1:
            return "VST2"
        elif fmt == 2:
            return "CLAP"
        elif fmt == 3:
            return "VST3"
        return "None"

    @pyqtSlot(int, result=int)
    def getTrackPluginNumParams(self, track):
        return self.lib.myms_plugin_get_num_params(track)

    @pyqtSlot(int, result='QVariantList')
    def getTrackPluginParams(self, track):
        n = self.lib.myms_plugin_get_num_params(track)
        res = []
        name_buf = ctypes.create_string_buffer(32)
        disp_buf = ctypes.create_string_buffer(32)
        lbl_buf = ctypes.create_string_buffer(32)
        for i in range(n):
            self.lib.myms_plugin_get_param_name(track, i, name_buf, 32)
            self.lib.myms_plugin_get_param_display(track, i, disp_buf, 32)
            self.lib.myms_plugin_get_param_label(track, i, lbl_buf, 32)
            val = self.lib.myms_plugin_get_param_value(track, i)
            p_name = name_buf.value.decode('utf-8', errors='ignore')
            p_disp = disp_buf.value.decode('utf-8', errors='ignore')
            p_lbl = lbl_buf.value.decode('utf-8', errors='ignore')
            res.append({
                "id": i,
                "name": p_name if p_name else f"Param {i+1}",
                "display": p_disp,
                "label": p_lbl,
                "value": float(val)
            })
        return res

    @pyqtSlot(int, int, float)
    def setTrackPluginParamValue(self, track, param_idx, val):
        self.lib.myms_plugin_set_param_value(track, param_idx, ctypes.c_float(val))

    @pyqtSlot(int, result=bool)
    def hasTrackPluginNativeEditor(self, track):
        return bool(self.lib.myms_plugin_has_native_editor(track))

    @pyqtSlot(int, int, result=bool)
    def openTrackPluginNativeEditor(self, track, win_id):
        return bool(self.lib.myms_plugin_open_native_editor(track, ctypes.c_void_p(win_id)))

    @pyqtSlot(int)
    def closeTrackPluginNativeEditor(self, track):
        self.lib.myms_plugin_close_native_editor(track)

    @pyqtSlot(int)
    def promptOpenPlugin(self, track):
        self.requestOpenPluginDialog.emit(track)

    @pyqtSlot(int)
    def promptOpenPluginGui(self, track):
        self.requestOpenPluginGui.emit(track)

    # ==========================================
    # Track Insert FX API (VST3, CLAP, VST2)
    # ==========================================
    @pyqtSlot(int, int, str, result=bool)
    def loadTrackFx(self, track, slot, filepath):
        if not filepath or not os.path.exists(filepath):
            return False
        ok = self.lib.myms_track_fx_load(track, slot, filepath.encode('utf-8'))
        if ok:
            self.trackFxChanged.emit(track, slot)
            print(f"[myms] Loaded FX on track {track} slot {slot}: {self.getTrackFxName(track, slot)} ({self.getTrackFxFormatName(track, slot)})")
        return ok

    @pyqtSlot(int, int)
    def unloadTrackFx(self, track, slot):
        self.lib.myms_track_fx_unload(track, slot)
        self.trackFxChanged.emit(track, slot)

    @pyqtSlot(int, int, result=bool)
    def isTrackFxLoaded(self, track, slot):
        return bool(self.lib.myms_track_fx_is_loaded(track, slot))

    @pyqtSlot(int, int, result=str)
    def getTrackFxName(self, track, slot):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_track_fx_get_name(track, slot, buf, 64)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, int, result=str)
    def getTrackFxVendor(self, track, slot):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_track_fx_get_vendor(track, slot, buf, 64)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, int, result=str)
    def getTrackFxPath(self, track, slot):
        buf = ctypes.create_string_buffer(512)
        self.lib.myms_track_fx_get_path(track, slot, buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    @pyqtSlot(int, int, result=int)
    def getTrackFxFormat(self, track, slot):
        return self.lib.myms_track_fx_get_format(track, slot)

    @pyqtSlot(int, int, result=str)
    def getTrackFxFormatName(self, track, slot):
        buf = ctypes.create_string_buffer(16)
        self.lib.myms_track_fx_get_format_name(track, slot, buf, 16)
        res = buf.value.decode('utf-8', errors='ignore')
        return res if res else "None"

    @pyqtSlot(int, int, result=int)
    def getTrackFxNumParams(self, track, slot):
        return self.lib.myms_track_fx_get_num_params(track, slot)

    @pyqtSlot(int, int, result='QVariantList')
    def getTrackFxParams(self, track, slot):
        n = self.lib.myms_track_fx_get_num_params(track, slot)
        res = []
        name_buf = ctypes.create_string_buffer(32)
        for i in range(n):
            self.lib.myms_track_fx_get_param_name(track, slot, i, name_buf, 32)
            val = self.lib.myms_track_fx_get_param_value(track, slot, i)
            p_name = name_buf.value.decode('utf-8', errors='ignore')
            res.append({
                "id": i,
                "name": p_name if p_name else f"Param {i+1}",
                "value": float(val)
            })
        return res

    @pyqtSlot(int, int, int, float)
    def setTrackFxParamValue(self, track, slot, param_idx, val):
        self.lib.myms_track_fx_set_param_value(track, slot, param_idx, ctypes.c_float(val))

    @pyqtSlot(int, int, result=bool)
    def hasTrackFxNativeEditor(self, track, slot):
        return bool(self.lib.myms_track_fx_has_native_editor(track, slot))

    @pyqtSlot(int, int, int, result=bool)
    def openTrackFxNativeEditor(self, track, slot, win_id):
        return bool(self.lib.myms_track_fx_open_native_editor(track, slot, ctypes.c_void_p(win_id)))

    @pyqtSlot(int, int)
    def closeTrackFxNativeEditor(self, track, slot):
        self.lib.myms_track_fx_close_native_editor(track, slot)

    @pyqtSlot(int, int)
    def promptOpenTrackFxGui(self, track, slot):
        self.requestOpenTrackFxGui.emit(track, slot)

    @pyqtSlot(int, int)
    def promptOpenTrackFxDialog(self, track, slot):
        self.requestOpenTrackFxDialog.emit(track, slot)

    def getTrackFxChainData(self, track):
        res = []
        for s in range(2):
            if self.isTrackFxLoaded(track, s):
                res.append({
                    "slot": s,
                    "path": self.getTrackFxPath(track, s),
                    "name": self.getTrackFxName(track, s),
                    "format": self.getTrackFxFormatName(track, s),
                    "params": [p["value"] for p in self.getTrackFxParams(track, s)]
                })
        return res

    @pyqtSlot(int)
    def setSongOrderIndex(self, step):
        self.lib.myms_set_song_order_index(step)
        self._song_order_index = step
        self.songOrderIndexChanged.emit(self._song_order_index)
        self._current_pattern = self.lib.myms_get_current_pattern()
        self.currentPatternChanged.emit(self._current_pattern)
        self.gridCellChanged.emit(0, 0)

    @pyqtSlot()
    def promptOpenProject(self):
        self.requestOpenProject.emit()

    @pyqtSlot()
    def promptSaveProject(self):
        self.requestSaveProject.emit()

    @pyqtSlot()
    def promptSaveProjectAs(self):
        self.requestSaveProjectAs.emit()

    @pyqtSlot()
    def promptNewProject(self):
        self.requestNewProject.emit()

    @pyqtSlot(int, result=str)
    def getInstrumentName(self, inst):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_get_inst_name(inst, buf, 64)
        name = buf.value.decode('utf-8', errors='ignore')
        return name if name else f"Synth {inst + 1}"

    @pyqtSlot(int, str)
    def setInstrumentName(self, inst, name):
        if not name:
            name = f"Synth {inst + 1}"
        self.lib.myms_set_inst_name(inst, name.encode('utf-8'))
        self.instrumentNameChanged.emit(inst, name)

    @pyqtSlot()
    def play(self):
        self._is_paused = False
        self.pausedChanged.emit(False)
        self.lib.myms_play()

    @pyqtSlot()
    def pause(self):
        self.lib.myms_pause()
        self._is_paused = True
        self._current_row = self.lib.myms_get_current_row()
        self.pausedChanged.emit(True)
        self.currentRowChanged.emit(self._current_row)

    @pyqtSlot()
    def togglePlay(self):
        if self._is_playing:
            self.pause()
        else:
            self.play()

    @pyqtSlot()
    def stop(self):
        self.lib.myms_stop()
        self._is_paused = False
        self._current_row = 0
        self.pausedChanged.emit(False)
        self.currentRowChanged.emit(0)
        self._current_pattern = self.lib.myms_get_current_pattern()
        self.currentPatternChanged.emit(self._current_pattern)
        self._song_order_index = self.lib.myms_get_song_order_index()
        self.songOrderIndexChanged.emit(self._song_order_index)
        self.gridCellChanged.emit(0, 0)

    @pyqtSlot(float)
    def setBpm(self, bpm):
        self.lib.myms_set_bpm(bpm)
        self._bpm = bpm
        self.bpmChanged.emit(bpm)

    @pyqtSlot(int)
    def setCurrentRow(self, row):
        nRows = self.numRows
        target_row = max(0, min(row, nRows - 1)) if nRows > 0 else 0
        self.lib.myms_set_current_row(target_row)
        self._current_row = target_row
        self.currentRowChanged.emit(self._current_row)

    @pyqtSlot(int)
    def setNumRows(self, rows):
        self.lib.myms_set_num_rows(rows)
        self._num_rows = self.lib.myms_get_num_rows()
        self.numRowsChanged.emit(self._num_rows)
        if self._current_row >= self._num_rows:
            self.setCurrentRow(0)

    @pyqtSlot(bool)
    def setLooping(self, loop):
        self.lib.myms_set_looping(loop)
        self._is_looping = loop
        self.loopingChanged.emit(loop)

    @pyqtSlot(bool)
    def setEditMode(self, edit_mode):
        self._is_edit_mode = edit_mode
        self.editModeChanged.emit(edit_mode)

    @pyqtSlot(int)
    def setBaseOctave(self, oct_val):
        oct_val = max(1, min(7, oct_val))
        if oct_val != self._base_octave:
            self._base_octave = oct_val
            self.baseOctaveChanged.emit(oct_val)

    @pyqtSlot()
    def addTrack(self):
        self.lib.myms_add_track()
        self._num_tracks = self.lib.myms_get_num_tracks()
        self.numTracksChanged.emit(self._num_tracks)

    @pyqtSlot(int)
    def removeTrack(self, track_idx):
        self.lib.myms_remove_track(track_idx)
        self._num_tracks = self.lib.myms_get_num_tracks()
        self.numTracksChanged.emit(self._num_tracks)
        if self._selected_instrument >= self._num_tracks:
            self.setSelectedInstrument(max(0, self._num_tracks - 1))

    @pyqtSlot(int, int, float)
    def noteOnInst(self, inst, note, vel=1.0):
        self.lib.myms_note_on_inst(inst, note, vel)

    @pyqtSlot(int, int)
    def noteOffInst(self, inst, note):
        self.lib.myms_note_off_inst(inst, note)

    @pyqtSlot(int, int)
    def setInstOsc1(self, inst, wave):
        self.lib.myms_set_inst_osc1(inst, wave)

    @pyqtSlot(int, int)
    def setInstOsc2(self, inst, wave):
        self.lib.myms_set_inst_osc2(inst, wave)

    @pyqtSlot(int, float)
    def setInstOscMix(self, inst, mix):
        self.lib.myms_set_inst_osc_mix(inst, mix)

    @pyqtSlot(int, float, float)
    def setInstDetune(self, inst, semi, cents):
        self.lib.myms_set_inst_detune(inst, semi, cents)

    @pyqtSlot(int, float, float, float)
    def setInstFilter(self, inst, cutoff, res, env_amt):
        self.lib.myms_set_inst_filter(inst, cutoff, res, env_amt)

    @pyqtSlot(int, float, float, float, float)
    def setInstAmpEnv(self, inst, a, d, s, r):
        self.lib.myms_set_inst_amp_env(inst, a, d, s, r)

    @pyqtSlot(int, float, float, float, float)
    def setInstFilterEnv(self, inst, a, d, s, r):
        self.lib.myms_set_inst_filter_env(inst, a, d, s, r)

    @pyqtSlot(int, result="QVariantMap")
    def getInstParams(self, inst):
        o1 = ctypes.c_int(0)
        o2 = ctypes.c_int(0)
        mix = ctypes.c_float(0.0)
        semi = ctypes.c_float(0.0)
        cents = ctypes.c_float(0.0)
        cutoff = ctypes.c_float(0.0)
        res = ctypes.c_float(0.0)
        env_amt = ctypes.c_float(0.0)
        aa = ctypes.c_float(0.0)
        ad = ctypes.c_float(0.0)
        as_ = ctypes.c_float(0.0)
        ar = ctypes.c_float(0.0)

        self.lib.myms_get_inst_params(
            inst,
            ctypes.byref(o1), ctypes.byref(o2), ctypes.byref(mix),
            ctypes.byref(semi), ctypes.byref(cents),
            ctypes.byref(cutoff), ctypes.byref(res), ctypes.byref(env_amt),
            ctypes.byref(aa), ctypes.byref(ad), ctypes.byref(as_), ctypes.byref(ar)
        )

        return {
            "osc1": o1.value,
            "osc2": o2.value,
            "mix": mix.value,
            "detuneSemi": semi.value,
            "detuneCents": cents.value,
            "cutoff": cutoff.value,
            "resonance": res.value,
            "envAmount": env_amt.value,
            "ampA": aa.value,
            "ampD": ad.value,
            "ampS": as_.value,
            "ampR": ar.value
        }

    @pyqtSlot(float)
    def setMacroPunch(self, val):
        self._macro_punch = val
        self.lib.myms_set_macro_punch(ctypes.c_float(val))
        self.macrosChanged.emit()

    @pyqtSlot(float)
    def setMacroWarmth(self, val):
        self._macro_warmth = val
        self.lib.myms_set_macro_warmth(ctypes.c_float(val))
        self.macrosChanged.emit()

    @pyqtSlot(float)
    def setMacroTone(self, val):
        self._macro_tone = val
        self.lib.myms_set_macro_tone(ctypes.c_float(val))
        self.macrosChanged.emit()

    @pyqtSlot(float)
    def setMacroSpace(self, val):
        self._macro_space = val
        self.lib.myms_set_macro_space(ctypes.c_float(val))
        self.macrosChanged.emit()

    @pyqtSlot(float)
    def setMasterVolume(self, val):
        self._master_volume = val
        self.lib.myms_set_master_volume(ctypes.c_float(val))
        self.macrosChanged.emit()

    @pyqtSlot(int, int, int, int, int, int, int)
    def setCell(self, track, row, note, instrument, volume, effectType, effectParam):
        self.lib.myms_set_cell(track, row, note, instrument, volume, effectType, effectParam)
        self.gridCellChanged.emit(track, row)

    @pyqtSlot(int, int, result="QVariantMap")
    def getCell(self, track, row):
        n = ctypes.c_uint8(0)
        inst = ctypes.c_uint8(0)
        vol = ctypes.c_uint8(0)
        fxT = ctypes.c_uint8(0)
        fxP = ctypes.c_uint8(0)
        self.lib.myms_get_cell(
            track, row,
            ctypes.byref(n), ctypes.byref(inst),
            ctypes.byref(vol), ctypes.byref(fxT), ctypes.byref(fxP)
        )
        return {
            "note": n.value,
            "instrument": inst.value,
            "volume": vol.value,
            "effectType": fxT.value,
            "effectParam": fxP.value
        }

    @pyqtSlot(int)
    def clearTrack(self, track):
        self.lib.myms_clear_track(track)
        self.gridCellChanged.emit(track, 0)

    @pyqtSlot()
    def clearAllTracks(self):
        self.lib.myms_clear_all_tracks()
        self.gridCellChanged.emit(0, 0)

    @pyqtSlot(int, int)
    def transposeTrack(self, track, semitones):
        self.lib.myms_transpose_track(track, semitones)
        self.gridCellChanged.emit(track, 0)

    @pyqtSlot(str, result=bool)
    def saveProject(self, filepath=""):
        target_path = filepath if filepath else self._current_project_path
        if not target_path:
            return False

        try:
            import json
            data = {
                "format_version": 3,
                "app": "myms",
                "bpm": self.bpm,
                "num_rows": self.numRows,
                "num_tracks": self.numTracks,
                "num_patterns": self.numPatterns,
                "is_looping": self.isLooping,
                "is_song_mode": self.isSongMode,
                "base_octave": self.baseOctave,
                "keyboard_layout": self.keyboardLayout,
                "soundfont": {
                    "path": self.sf2Path,
                    "preset": self.sf2CurrentPreset,
                    "volume": self.sf2Volume
                },
                "drum_pads": [],
                "macros": {
                    "punch": self._macro_punch,
                    "warmth": self._macro_warmth,
                    "tone": self._macro_tone,
                    "space": self._macro_space,
                    "master_volume": self._master_volume
                },
                "tracks": [],
                "patterns": []
            }

            for p in range(8):
                data["drum_pads"].append({
                    "name": self.getDrumPadName(p),
                    "file_path": self.getDrumPadFilePath(p),
                    "pitch": self.getDrumPadPitch(p),
                    "decay": self.getDrumPadDecay(p),
                    "volume": self.getDrumPadVolume(p),
                    "pan": self.getDrumPadPan(p),
                    "reverse": self.getDrumPadReverse(p)
                })

            for t in range(self.numTracks):
                p = self.getInstParams(t)
                name = self.getInstrumentName(t)
                trk_type = self.getTrackType(t)
                data["tracks"].append({
                    "name": name,
                    "type": trk_type,
                    "volume": self.getTrackVolume(t),
                    "pan": self.getTrackPan(t),
                    "mute": self.getTrackMute(t),
                    "solo": self.getTrackSolo(t),
                    "sf2_preset": self.getTrackSf2Preset(t),
                    "plugin_path": self.getTrackPluginPath(t) if self.isTrackPluginLoaded(t) else "",
                    "plugin_format": self.getTrackPluginFormatName(t) if self.isTrackPluginLoaded(t) else "",
                    "plugin_params": [param["value"] for param in self.getTrackPluginParams(t)] if self.isTrackPluginLoaded(t) else [],
                    "fx_chain": self.getTrackFxChainData(t),
                    "synth": {
                        "osc1_wave": p.get("osc1", 0),
                        "osc2_wave": p.get("osc2", 1),
                        "osc_mix": p.get("mix", 0.45),
                        "detune_semi": p.get("detuneSemi", 0.0),
                        "detune_cents": p.get("detuneCents", 8.0),
                        "filter_cutoff": p.get("cutoff", 3500.0),
                        "filter_resonance": p.get("resonance", 0.35),
                        "filter_env_amount": p.get("envAmount", 0.40),
                        "amp_attack": p.get("ampA", 0.005),
                        "amp_decay": p.get("ampD", 0.25),
                        "amp_sustain": p.get("ampS", 0.65),
                        "amp_release": p.get("ampR", 0.25)
                    }
                })

            data["song_order"] = self.getSongOrderList()

            # Save all patterns
            cur_pat = self.currentPattern
            for pat_idx in range(self.numPatterns):
                pat_cells = []
                for t in range(self.numTracks):
                    for r in range(self.numRows):
                        n = ctypes.c_uint8(0)
                        inst = ctypes.c_uint8(0)
                        vol = ctypes.c_uint8(0)
                        fxT = ctypes.c_uint8(0)
                        fxP = ctypes.c_uint8(0)
                        self.lib.myms_get_pattern_cell(
                            pat_idx, t, r,
                            ctypes.byref(n), ctypes.byref(inst),
                            ctypes.byref(vol), ctypes.byref(fxT), ctypes.byref(fxP)
                        )
                        if n.value != 255:
                            pat_cells.append({
                                "track": t,
                                "row": r,
                                "note": n.value,
                                "instrument": inst.value,
                                "volume": vol.value,
                                "effect_type": fxT.value,
                                "effect_param": fxP.value
                            })
                data["patterns"].append(pat_cells)

            with open(target_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)

            self._current_project_path = target_path
            self.currentProjectPathChanged.emit(target_path)
            self.projectSaved.emit(target_path)
            print(f"[myms] Project saved successfully to: {target_path}")
            return True
        except Exception as e:
            print(f"[myms] Failed to save project to {target_path}: {e}")
            return False

    @pyqtSlot(str, result=bool)
    def openProject(self, filepath):
        if not filepath or not os.path.exists(filepath):
            return False

        try:
            import json
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.stop()

            # 1. Global metadata
            if "bpm" in data:
                self.setBpm(float(data["bpm"]))
            if "num_rows" in data:
                self.setNumRows(int(data["num_rows"]))
            if "is_looping" in data:
                self.setLooping(bool(data["is_looping"]))
            if "is_song_mode" in data:
                self.setSongMode(bool(data["is_song_mode"]))
            if "base_octave" in data:
                self.setBaseOctave(int(data["base_octave"]))
            if "keyboard_layout" in data:
                self.setKeyboardLayout(str(data["keyboard_layout"]))

            # 2. Macros
            macros = data.get("macros", {})
            if "punch" in macros: self.setMacroPunch(float(macros["punch"]))
            if "warmth" in macros: self.setMacroWarmth(float(macros["warmth"]))
            if "tone" in macros: self.setMacroTone(float(macros["tone"]))
            if "space" in macros: self.setMacroSpace(float(macros["space"]))
            if "master_volume" in macros: self.setMasterVolume(float(macros["master_volume"]))

            # 3. SoundFont
            sf2 = data.get("soundfont", {})
            if "path" in sf2 and sf2["path"] and os.path.exists(sf2["path"]):
                self.loadSoundFont(sf2["path"])
                if "preset" in sf2: self.setSoundFontPreset(int(sf2["preset"]))
                if "volume" in sf2: self.setSoundFontVolume(float(sf2["volume"]))

            # 4. Drum Pads
            drum_pads = data.get("drum_pads", [])
            for p_idx, pad in enumerate(drum_pads):
                if p_idx < 8:
                    if "pitch" in pad: self.setDrumPadPitch(p_idx, float(pad["pitch"]))
                    if "decay" in pad: self.setDrumPadDecay(p_idx, float(pad["decay"]))
                    if "volume" in pad: self.setDrumPadVolume(p_idx, float(pad["volume"]))
                    if "pan" in pad: self.setDrumPadPan(p_idx, float(pad["pan"]))
                    if "reverse" in pad: self.setDrumPadReverse(p_idx, bool(pad["reverse"]))
                    if "file_path" in pad and pad["file_path"] and os.path.exists(pad["file_path"]):
                        self.loadDrumSample(p_idx, pad["file_path"])

            # 5. Synchronize Tracks count
            tracks = data.get("tracks", [])
            target_tracks = max(1, len(tracks))
            while self.numTracks < target_tracks:
                self.addTrack()
            while self.numTracks > target_tracks:
                self.removeTrack(self.numTracks - 1)

            # 6. Load Instruments & Synths
            for t, trk in enumerate(tracks):
                if t >= self.numTracks:
                    break
                if "name" in trk:
                    self.setInstrumentName(t, trk["name"])
                if "type" in trk:
                    self.setTrackType(t, int(trk["type"]))
                if "volume" in trk:
                    self.setTrackVolume(t, float(trk["volume"]))
                if "pan" in trk:
                    self.setTrackPan(t, float(trk["pan"]))
                if "mute" in trk:
                    self.setTrackMute(t, bool(trk["mute"]))
                if "solo" in trk:
                    self.setTrackSolo(t, bool(trk["solo"]))
                if "sf2_preset" in trk:
                    self.setTrackSf2Preset(t, int(trk["sf2_preset"]))
                if "plugin_path" in trk and trk["plugin_path"] and os.path.exists(trk["plugin_path"]):
                    self.loadTrackPlugin(t, trk["plugin_path"])
                    p_vals = trk.get("plugin_params", [])
                    for p_idx, val in enumerate(p_vals):
                        self.setTrackPluginParamValue(t, p_idx, float(val))

                fx_chain = trk.get("fx_chain", [])
                for fx in fx_chain:
                    slot = fx.get("slot", 0)
                    p_path = fx.get("path", "")
                    if p_path and os.path.exists(p_path):
                        self.loadTrackFx(t, slot, p_path)
                        fx_vals = fx.get("params", [])
                        for p_idx, val in enumerate(fx_vals):
                            self.setTrackFxParamValue(t, slot, p_idx, float(val))

                synth = trk.get("synth", {})
                if "osc1_wave" in synth: self.setInstOsc1(t, synth["osc1_wave"])
                if "osc2_wave" in synth: self.setInstOsc2(t, synth["osc2_wave"])
                if "osc_mix" in synth: self.setInstOscMix(t, synth["osc_mix"])
                if "detune_semi" in synth and "detune_cents" in synth:
                    self.setInstDetune(t, synth["detune_semi"], synth["detune_cents"])
                if "filter_cutoff" in synth:
                    self.setInstFilter(t, synth["filter_cutoff"], synth.get("filter_resonance", 0.35), synth.get("filter_env_amount", 0.40))
                if "amp_attack" in synth:
                    self.setInstAmpEnv(t, synth["amp_attack"], synth.get("amp_decay", 0.25), synth.get("amp_sustain", 0.65), synth.get("amp_release", 0.25))

            # 7. Patterns & Cells
            self.clearAllTracks()
            patterns_data = data.get("patterns", [])
            if patterns_data:
                # Synchronize patterns count
                while self.numPatterns < len(patterns_data):
                    self.addPattern()
                while self.numPatterns > len(patterns_data):
                    self.removePattern(self.numPatterns - 1)

                for p_idx, p_cells in enumerate(patterns_data):
                    for cell in p_cells:
                        t = cell.get("track", 0)
                        r = cell.get("row", 0)
                        self.lib.myms_set_pattern_cell(
                            p_idx, t, r,
                            cell.get("note", 255),
                            cell.get("instrument", t + 1),
                            cell.get("volume", 255),
                            cell.get("effect_type", 0),
                            cell.get("effect_param", 0)
                        )
            elif "cells" in data: # v1 backward compatibility
                for cell in data.get("cells", []):
                    t = cell.get("track", 0)
                    r = cell.get("row", 0)
                    if t < self.numTracks and r < self.numRows:
                        self.setCell(
                            t, r,
                            cell.get("note", 255),
                            cell.get("instrument", t + 1),
                            cell.get("volume", 255),
                            cell.get("effect_type", 0),
                            cell.get("effect_param", 0)
                        )

            # 8. Restore song order
            if "song_order" in data and isinstance(data["song_order"], list) and len(data["song_order"]) > 0:
                self.setSongOrderStep(0, int(data["song_order"][0]))
                while self.songOrderLength > 1:
                    self.removeSongOrderStep(self.songOrderLength - 1)
                for step_pat in data["song_order"][1:]:
                    self.addSongOrderStep(int(step_pat))

            self.setCurrentPattern(0)
            self.setSongOrderIndex(0)
            self.setSelectedInstrument(0)
            self._current_project_path = filepath
            self.currentProjectPathChanged.emit(filepath)
            self.projectLoaded.emit()
            print(f"[myms] Project opened successfully from: {filepath}")
            return True
        except Exception as e:
            print(f"[myms] Failed to open project from {filepath}: {e}")
            return False

    @pyqtSlot()
    def newProject(self):
        self.stop()
        self.clearAllTracks()
        for t in range(self.numTracks):
            self.unloadTrackPlugin(t)
            for s in range(2):
                self.unloadTrackFx(t, s)
        while self.numTracks > 1:
            self.removeTrack(self.numTracks - 1)
        while self.numPatterns > 1:
            self.removePattern(self.numPatterns - 1)
        self.setTrackType(0, 0) # Synth
        self.setInstrumentName(0, "Synth 1")
        self.setTrackVolume(0, 1.0)
        self.setTrackPan(0, 0.5)
        self.setTrackMute(0, False)
        self.setTrackSolo(0, False)
        self.setTrackSf2Preset(0, 0)
        self.setSongOrderStep(0, 0)
        while self.songOrderLength > 1:
            self.removeSongOrderStep(self.songOrderLength - 1)
        self.setBpm(120.0)
        self.setNumRows(32)
        self.setLooping(False)
        self.setSongMode(False)
        self.setMacroPunch(0.20)
        self.setMacroWarmth(0.25)
        self.setMacroTone(0.50)
        self.setMacroSpace(0.15)
        self.setMasterVolume(0.85)
        self.setCurrentPattern(0)
        self.setSongOrderIndex(0)
        self.setSelectedInstrument(0)
        self._current_project_path = ""
        self.currentProjectPathChanged.emit("")
        self.projectLoaded.emit()
        print("[myms] New project created.")

    @pyqtSlot()
    def loadDemoGroove(self):
        self.lib.myms_load_demo()
        self._num_tracks = self.lib.myms_get_num_tracks()
        self.numTracksChanged.emit(self._num_tracks)
        self.projectLoaded.emit()

    def close(self):
        self._timer.stop()
        self.lib.myms_shutdown()
