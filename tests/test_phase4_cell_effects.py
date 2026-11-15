import os
import unittest
import sys
import tempfile
import json
import numpy as np

os.environ["QT_QPA_PLATFORM"] = "offscreen"
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import QUrl, QEventLoop, QTimer, QObject
from myms_bridge import MymsEngine
from main import AppWindow

app = QApplication.instance() or QApplication(sys.argv)

class TestPhase4CellEffects(unittest.TestCase):
    def setUp(self):
        self.engine = MymsEngine()
        self.engine.newProject()

    def tearDown(self):
        self.engine.stop()

    def _run_export_sync(self, params, timeout_sec=10.0):
        loop = QEventLoop()
        result = {"success": False, "msg": ""}

        def on_finished(success, msg):
            result["success"] = success
            result["msg"] = msg
            loop.quit()

        self.engine.exportFinished.connect(on_finished)
        timer = QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)
        timer.start(int(timeout_sec * 1000))

        self.engine.startExport(params)
        loop.exec()

        try:
            self.engine.exportFinished.disconnect(on_finished)
        except Exception:
            pass
        return result

    def test_01_filter_cutoff_effect_audible(self):
        # Set Track 0 Row 0: Note 60 (C-4), Inst 1, Vol 255, Effect 0x10 (Filter Cutoff), Param 0x10 (16, deep muffled)
        self.engine.setCell(0, 0, 60, 1, 255, 0x10, 16)
        c0 = self.engine.getCell(0, 0)
        self.assertEqual(c0["effectType"], 16)
        self.assertEqual(c0["effectParam"], 16)

        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            muffled_wav = tf.name
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            open_wav = tf.name

        try:
            # Render muffled
            res = self._run_export_sync({
                "filepath": muffled_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res["success"], f"Muffled export failed: {res['msg']}")

            # Now set Track 0 Row 0: wide open cutoff (0x10, Param 255)
            self.engine.setCell(0, 0, 60, 1, 255, 0x10, 255)
            res = self._run_export_sync({
                "filepath": open_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res["success"], f"Open export failed: {res['msg']}")

            # Read PCM samples and verify frequency content differs
            import wave
            with wave.open(muffled_wav, "rb") as wf:
                raw_muffled = wf.readframes(wf.getnframes())
            with wave.open(open_wav, "rb") as wf:
                raw_open = wf.readframes(wf.getnframes())

            samples_muffled = np.frombuffer(raw_muffled, dtype=np.int16).astype(np.float32)
            samples_open = np.frombuffer(raw_open, dtype=np.int16).astype(np.float32)

            # High frequencies in the open filter will have higher high-frequency variance/energy
            diff_muffled = np.diff(samples_muffled)
            diff_open = np.diff(samples_open)
            hf_energy_muffled = np.mean(diff_muffled ** 2)
            hf_energy_open = np.mean(diff_open ** 2)

            self.assertGreater(hf_energy_open, hf_energy_muffled, "Open filter must have higher high-frequency content than muffled filter")
        finally:
            if os.path.exists(muffled_wav):
                os.remove(muffled_wav)
            if os.path.exists(open_wav):
                os.remove(open_wav)

    def test_02_delay_echo_effect_produces_tail(self):
        # Set Track 0 Row 0: Note 60, Inst 1, Vol 255, Effect 0x0E (Delay/Echo), Param 200 (~80% wet)
        self.engine.setCell(0, 0, 60, 1, 255, 0x0E, 200)

        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            delay_wav = tf.name

        try:
            res = self._run_export_sync({
                "filepath": delay_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 1.0,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res["success"], f"Delay export failed: {res['msg']}")

            import wave
            with wave.open(delay_wav, "rb") as wf:
                raw = wf.readframes(wf.getnframes())
            samples = np.frombuffer(raw, dtype=np.int16).astype(np.float32)

            # Energy after row 0 (echo taps) must be present
            echo_region = samples[12000:24000]
            echo_energy = np.max(np.abs(echo_region))
            self.assertGreater(echo_energy, 50.0, "Delay effect must produce audible echo taps")
        finally:
            if os.path.exists(delay_wav):
                os.remove(delay_wav)

    def test_03_note_cut_effect_silences_step(self):
        # Row 0: Note 60 with Note Cut 0x0C, Param 2 (cuts note very fast)
        self.engine.setCell(0, 0, 60, 1, 255, 0x0C, 2)
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            cut_wav = tf.name

        # Row 0: Note 60 without cut on pattern 1
        self.engine.addPattern()
        self.engine.setCurrentPattern(1)
        self.engine.setCell(0, 0, 60, 1, 255, 0, 0)
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            normal_wav = tf.name

        try:
            res_cut = self._run_export_sync({
                "filepath": cut_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.0,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res_cut["success"])

            res_norm = self._run_export_sync({
                "filepath": normal_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 1,
                "tail_seconds": 0.0,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res_norm["success"])

            import wave
            with wave.open(cut_wav, "rb") as wf:
                samples_cut = np.frombuffer(wf.readframes(wf.getnframes()), dtype=np.int16).astype(np.float32)
            with wave.open(normal_wav, "rb") as wf:
                samples_norm = np.frombuffer(wf.readframes(wf.getnframes()), dtype=np.int16).astype(np.float32)

            energy_cut = np.sum(samples_cut ** 2)
            energy_norm = np.sum(samples_norm ** 2)
            self.assertGreater(energy_norm, energy_cut * 1.5, "Uncut note must have substantially more energy than cut note")
        finally:
            if os.path.exists(cut_wav):
                os.remove(cut_wav)
            if os.path.exists(normal_wav):
                os.remove(normal_wav)

    def test_04_qml_ctrl_selection_and_no_move(self):
        window = AppWindow(self.engine)
        root = window.quick_widget.rootObject()
        tracker_view = root.findChild(QObject, "trackerView")
        self.assertIsNotNone(tracker_view)

        # Initially no selection
        self.assertFalse(tracker_view.property("hasSelection"))

        # Verify helper properties exist and function
        tracker_view.setProperty("selectedTrack", 0)
        tracker_view.setProperty("selectedRow", 2)
        self.assertEqual(tracker_view.property("selectedTrack"), 0)
        self.assertEqual(tracker_view.property("selectedRow"), 2)

        window.close()

    def test_05_cell_effects_per_note_isolation(self):
        # Verify that an effect on note 1 does not leak into note 2
        # Row 0: Note 60, heavily muffled filter cutoff 0x10, param 20
        self.engine.setCell(0, 0, 60, 1, 255, 0x10, 20)
        # Row 4: Note 60, no effect (0, 0)
        self.engine.setCell(0, 4, 60, 1, 255, 0, 0)

        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            iso_wav = tf.name

        try:
            res = self._run_export_sync({
                "filepath": iso_wav,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.0,
                "sample_rate": 48000,
                "wav_bit_depth": 16,
                "pattern_loops": 1
            })
            self.assertTrue(res["success"])

            import wave
            with wave.open(iso_wav, "rb") as wf:
                raw = wf.readframes(wf.getnframes())
            samples = np.frombuffer(raw, dtype=np.int16).astype(np.float32)

            # Row 0 starts at index 0. At 90bpm (4 lpb), row = (60 / 90 / 4) * 48000 = 8000 samples.
            # Row 0: samples 0 to 8000
            # Row 4: samples 32000 to 40000
            # Note on Row 4 must have restored high frequencies (diff squared)
            r0_samples = samples[500:7500]
            r4_samples = samples[32500:39500]
            hf_r0 = np.mean(np.diff(r0_samples) ** 2)
            hf_r4 = np.mean(np.diff(r4_samples) ** 2)

            self.assertGreater(hf_r4, hf_r0 * 2.0, "Note 2 on row 4 must NOT inherit Note 1's low-pass filter")
        finally:
            if os.path.exists(iso_wav):
                os.remove(iso_wav)

    def test_06_default_bpm_is_90(self):
        # Default engine BPM must be 90.0
        self.assertAlmostEqual(self.engine.bpm, 90.0, places=1)
        self.engine.setBpm(135.0)
        self.assertAlmostEqual(self.engine.bpm, 135.0, places=1)
        self.engine.newProject()
        self.assertAlmostEqual(self.engine.bpm, 90.0, places=1)

    def test_07_in_window_menu_bar_and_toggle(self):
        from PyQt6.QtCore import QSettings
        QSettings("myms", "myms").setValue("use_global_menu", False)
        window = AppWindow(self.engine)
        window.show()
        self.assertTrue(hasattr(window, "mb"))
        self.assertFalse(window.mb.isNativeMenuBar())

        titles = [action.text().replace("&", "") for action in window.mb.actions()]
        self.assertIn("File", titles)
        self.assertIn("Edit", titles)
        self.assertIn("Playback", titles)
        self.assertIn("View", titles)
        self.assertIn("Tracks", titles)
        self.assertIn("Settings", titles)

        # Test toggle global menu action
        self.assertTrue(hasattr(window, "act_global_menu"))
        self.assertTrue(window.act_global_menu.isCheckable())
        self.assertFalse(window.act_global_menu.isChecked())

        # Toggle to global menu
        window.act_global_menu.setChecked(True)
        from PyQt6.QtCore import QSettings
        s = QSettings("myms", "myms")
        self.assertTrue(s.value("use_global_menu", False, type=bool))

        # Toggle back to in-window menu
        window.act_global_menu.setChecked(False)
        self.assertFalse(s.value("use_global_menu", True, type=bool))
        self.assertFalse(window.mb.isNativeMenuBar())

        window.close()

if __name__ == "__main__":
    unittest.main()
