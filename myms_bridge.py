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

    def _detect_system_keyboard_layout(self):
        # 1. Check system XKB configuration (/etc/default/keyboard)
        try:
            if os.path.exists("/etc/default/keyboard"):
                with open("/etc/default/keyboard", "r", encoding="utf-8") as f:
                    content = f.read().lower()
                    if 'xkblayout="tr"' in content or "xkblayout='tr'" in content or "xkblayout=tr" in content:
                        return "tr"
        except Exception:
            pass

        # 2. Check environment locale
        for var in ("LANG", "LC_ALL", "LC_MESSAGES"):
            val = os.environ.get(var, "").lower()
            if "tr_" in val or "turkish" in val:
                return "tr"

        # 3. Check setxkbmap if available
        try:
            import subprocess
            out = subprocess.check_output(["setxkbmap", "-query"], stderr=subprocess.DEVNULL, text=True)
            if "layout:     tr" in out or "layout:\ttr" in out or " tr" in out:
                return "tr"
        except Exception:
            pass

        # Default to Turkish Q as requested
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
        self._num_rows = self.lib.myms_get_num_rows()
        self._num_tracks = self.lib.myms_get_num_tracks()
        self._is_looping = self.lib.myms_is_looping()
        self._is_edit_mode = True # True = Edit/Stamp, False = Jam mode
        self._base_octave = 4     # LMMS default octave
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
        return "Untitled"

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

    @pyqtProperty(bool, notify=editModeChanged)
    def isEditMode(self):
        return self._is_edit_mode

    @pyqtProperty(int, notify=baseOctaveChanged)
    def baseOctave(self):
        return self._base_octave

    @pyqtProperty(str, notify=keyboardLayoutChanged)
    def keyboardLayout(self):
        return self._keyboard_layout

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
        if 0 <= inst < 32:
            self._selected_instrument = inst
            self.instrumentChanged.emit(inst)

    @pyqtSlot(int, str)
    def setInstrumentName(self, inst, name):
        if 0 <= inst < 32 and name:
            self.lib.myms_set_inst_name(inst, name.encode('utf-8'))
            self.instrumentNameChanged.emit(inst, name)

    @pyqtSlot(int, result=str)
    def getInstrumentName(self, inst):
        buf = ctypes.create_string_buffer(64)
        self.lib.myms_get_inst_name(inst, buf, 64)
        val = buf.value.decode('utf-8', errors='ignore')
        return val if val else f"Synth {inst + 1}"

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

    @pyqtSlot(int)
    def setNumRows(self, rows):
        if rows in (16, 32, 48, 64, 128):
            self.lib.myms_set_num_rows(rows)
            self._num_rows = rows
            self.numRowsChanged.emit(rows)

    @pyqtSlot(bool)
    def setLooping(self, loop):
        self._is_looping = loop
        self.lib.myms_set_looping(loop)
        self.loopingChanged.emit(loop)

    @pyqtSlot(bool)
    def setEditMode(self, mode):
        self._is_edit_mode = mode
        self.editModeChanged.emit(mode)

    @pyqtSlot(int)
    def setBaseOctave(self, oct_val):
        oct_val = max(1, min(7, oct_val))
        self._base_octave = oct_val
        self.baseOctaveChanged.emit(oct_val)

    @pyqtSlot()
    def togglePlay(self):
        if self._is_playing:
            self.pause()
        else:
            self.play()

    @pyqtSlot()
    def play(self):
        self.lib.myms_play()
        self._is_playing = True
        self._is_paused = False
        self.playingChanged.emit(True)
        self.pausedChanged.emit(False)

    @pyqtSlot()
    def pause(self):
        self.lib.myms_pause()
        self._is_playing = False
        self._is_paused = True
        self.playingChanged.emit(False)
        self.pausedChanged.emit(True)

    @pyqtSlot()
    def stop(self):
        self.lib.myms_stop()
        self._is_playing = False
        self._is_paused = False
        self._current_row = 0
        self.playingChanged.emit(False)
        self.pausedChanged.emit(False)
        self.currentRowChanged.emit(0)

    @pyqtSlot()
    def playFromStart(self):
        self.setCurrentRow(0)
        self.play()

    @pyqtSlot(float)
    def setBpm(self, bpm):
        self.lib.myms_set_bpm(ctypes.c_float(bpm))
        self.bpmChanged.emit(bpm)

    @pyqtSlot(int)
    def setCurrentRow(self, row):
        self.lib.myms_set_current_row(row)
        self._current_row = row
        self._is_paused = False
        self.pausedChanged.emit(False)
        self.currentRowChanged.emit(row)

    @pyqtSlot(int, int, float)
    def noteOnInst(self, inst, note, velocity=1.0):
        self.lib.myms_note_on_inst(inst, note, ctypes.c_float(velocity))

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
        self.lib.myms_set_inst_osc_mix(inst, ctypes.c_float(mix))

    @pyqtSlot(int, float, float)
    def setInstDetune(self, inst, semi, cents):
        self.lib.myms_set_inst_detune(inst, ctypes.c_float(semi), ctypes.c_float(cents))

    @pyqtSlot(int, float, float, float)
    def setInstFilter(self, inst, cutoff, resonance, env_amount):
        self.lib.myms_set_inst_filter(inst, ctypes.c_float(cutoff), ctypes.c_float(resonance), ctypes.c_float(env_amount))

    @pyqtSlot(int, float, float, float, float)
    def setInstAmpEnv(self, inst, a, d, s, r):
        self.lib.myms_set_inst_amp_env(inst, ctypes.c_float(a), ctypes.c_float(d), ctypes.c_float(s), ctypes.c_float(r))

    @pyqtSlot(int, result="QVariantMap")
    def getInstParams(self, inst):
        o1 = ctypes.c_int(0)
        o2 = ctypes.c_int(0)
        mix = ctypes.c_float(0)
        semi = ctypes.c_float(0)
        cents = ctypes.c_float(0)
        cutoff = ctypes.c_float(0)
        res = ctypes.c_float(0)
        env_amt = ctypes.c_float(0)
        aa = ctypes.c_float(0)
        ad = ctypes.c_float(0)
        as_ = ctypes.c_float(0)
        ar = ctypes.c_float(0)

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
        note = ctypes.c_uint8(0)
        inst = ctypes.c_uint8(0)
        vol = ctypes.c_uint8(0)
        fx_t = ctypes.c_uint8(0)
        fx_p = ctypes.c_uint8(0)

        self.lib.myms_get_cell(
            track, row,
            ctypes.byref(note), ctypes.byref(inst),
            ctypes.byref(vol), ctypes.byref(fx_t),
            ctypes.byref(fx_p)
        )
        return {
            "note": note.value,
            "instrument": inst.value,
            "volume": vol.value,
            "effectType": fx_t.value,
            "effectParam": fx_p.value
        }

    @pyqtSlot(int)
    def clearTrack(self, track):
        self.lib.myms_clear_track(track)

    @pyqtSlot()
    def clearAllTracks(self):
        self.lib.myms_clear_all_tracks()

    @pyqtSlot(int, int)
    def transposeTrack(self, track, semitones):
        self.lib.myms_transpose_track(track, semitones)

    @pyqtSlot(str, result=bool)
    def saveProject(self, filepath=""):
        target_path = filepath if filepath else self._current_project_path
        if not target_path:
            return False

        try:
            import json
            data = {
                "format_version": 1,
                "app": "myms",
                "bpm": self.bpm,
                "num_rows": self.numRows,
                "num_tracks": self.numTracks,
                "is_looping": self.isLooping,
                "base_octave": self.baseOctave,
                "keyboard_layout": self.keyboardLayout,
                "macros": {
                    "punch": self._macro_punch,
                    "warmth": self._macro_warmth,
                    "tone": self._macro_tone,
                    "space": self._macro_space,
                    "master_volume": self._master_volume
                },
                "tracks": [],
                "cells": []
            }

            for t in range(self.numTracks):
                p = self.getInstParams(t)
                name = self.getInstrumentName(t)
                data["tracks"].append({
                    "name": name,
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

            for r in range(self.numRows):
                for t in range(self.numTracks):
                    c = self.getCell(t, r)
                    if c and (c.get("note", 255) != 255 or c.get("effectType", 0) != 0):
                        data["cells"].append({
                            "track": t,
                            "row": r,
                            "note": c.get("note", 255),
                            "instrument": c.get("instrument", t + 1),
                            "volume": c.get("volume", 255),
                            "effect_type": c.get("effectType", 0),
                            "effect_param": c.get("effectParam", 0)
                        })

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

            if data.get("app") != "myms":
                print(f"[myms] Warning: Not a standard myms project file: {filepath}")

            self.stop()

            # 1. Transport settings
            if "bpm" in data:
                self.setBpm(float(data["bpm"]))
            if "num_rows" in data:
                self.setNumRows(int(data["num_rows"]))
            if "is_looping" in data:
                self.setLooping(bool(data["is_looping"]))
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

            # 3. Synchronize Tracks count
            tracks = data.get("tracks", [])
            target_tracks = max(1, len(tracks))
            while self.numTracks < target_tracks:
                self.addTrack()
            while self.numTracks > target_tracks:
                self.removeTrack(self.numTracks - 1)

            # 4. Load Instruments & Synths
            for t, trk in enumerate(tracks):
                if t >= self.numTracks:
                    break
                if "name" in trk:
                    self.setInstrumentName(t, trk["name"])
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

            # 5. Clear pattern cells and populate loaded cells
            self.clearAllTracks()
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
        while self.numTracks > 1:
            self.removeTrack(self.numTracks - 1)
        self.setInstrumentName(0, "Synth 1")
        self.setBpm(120.0)
        self.setNumRows(32)
        self.setLooping(False)
        self.setMacroPunch(0.20)
        self.setMacroWarmth(0.25)
        self.setMacroTone(0.50)
        self.setMacroSpace(0.15)
        self.setMasterVolume(0.85)
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
