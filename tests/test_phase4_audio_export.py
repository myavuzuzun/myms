#!/usr/bin/env python3
"""
Unit and Integration Tests for Phase 4:
- High-fidelity offline audio export
- Multiple container & codec support: WAV (16/24/32-bit), MP3, OGG Vorbis, FLAC
- Song Mode vs Pattern Mode export
- Tail duration handling (natural reverb/delay decays)
- Background worker thread with progress reporting and cancellation
"""

import os
import sys
import tempfile
import time
import subprocess
import unittest

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from PyQt6.QtCore import QCoreApplication, QEventLoop, QTimer
from myms_bridge import MymsEngine

class TestPhase4AudioExport(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Ensure QCoreApplication exists for QThread and signals
        cls.app = QCoreApplication.instance()
        if not cls.app:
            cls.app = QCoreApplication(sys.argv)
        cls.engine = MymsEngine()

    @classmethod
    def tearDownClass(cls):
        cls.engine.close()

    def setUp(self):
        self.engine.newProject()
        self.engine.loadDemoGroove()

    def _run_export_sync(self, params, timeout_sec=10.0):
        """Helper to run startExport asynchronously and wait via Qt event loop."""
        loop = QEventLoop()
        result = {"success": False, "msg": "", "progress_reports": []}

        def on_progress(p, msg):
            result["progress_reports"].append((p, msg))

        def on_finished(success, msg):
            result["success"] = success
            result["msg"] = msg
            loop.quit()

        self.engine.exportProgressChanged.connect(on_progress)
        self.engine.exportFinished.connect(on_finished)

        timer = QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)
        timer.start(int(timeout_sec * 1000))

        self.engine.startExport(params)
        loop.exec()

        try:
            self.engine.exportProgressChanged.disconnect(on_progress)
            self.engine.exportFinished.disconnect(on_finished)
        except Exception:
            pass

        return result

    def _probe_audio(self, filepath):
        """Uses ffprobe to extract stream information."""
        cmd = [
            "ffprobe", "-v", "error",
            "-show_entries", "stream=codec_name,channels,sample_rate,bits_per_raw_sample,duration",
            "-show_entries", "format=format_name,duration,bit_rate",
            "-of", "default=noprint_wrappers=1",
            filepath
        ]
        out = subprocess.check_output(cmd, text=True)
        info = {}
        for line in out.strip().split("\n"):
            if "=" in line:
                k, v = line.split("=", 1)
                info[k.strip()] = v.strip()
        return info

    def test_01_export_wav_24bit_48k(self):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "wav_bit_depth": 24
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 10000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "pcm_s24le")
            self.assertEqual(probe.get("sample_rate"), "48000")
            self.assertEqual(probe.get("channels"), "2")
            self.assertGreater(len(res["progress_reports"]), 2)
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_02_export_wav_16bit_44k(self):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.0,
                "sample_rate": 44100,
                "wav_bit_depth": 16
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 10000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "pcm_s16le")
            self.assertEqual(probe.get("sample_rate"), "44100")
            self.assertEqual(probe.get("channels"), "2")
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_03_export_wav_32bit_float(self):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "wav_bit_depth": 32
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 10000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "pcm_f32le")
            self.assertEqual(probe.get("sample_rate"), "48000")
            self.assertEqual(probe.get("channels"), "2")
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_04_export_mp3_320k(self):
        with tempfile.NamedTemporaryFile(suffix=".mp3", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "mp3",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "mp3_bitrate": 320
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 5000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "mp3")
            self.assertEqual(probe.get("sample_rate"), "48000")
            self.assertEqual(probe.get("channels"), "2")
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_05_export_ogg_vorbis(self):
        with tempfile.NamedTemporaryFile(suffix=".ogg", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "ogg",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "ogg_quality": 8
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 5000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "vorbis")
            self.assertEqual(probe.get("sample_rate"), "48000")
            self.assertEqual(probe.get("channels"), "2")
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_06_export_flac_24bit(self):
        with tempfile.NamedTemporaryFile(suffix=".flac", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "flac",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "sample_rate": 48000,
                "flac_bit_depth": 24,
                "flac_compression": 8
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            self.assertGreater(os.path.getsize(target), 5000)

            probe = self._probe_audio(target)
            self.assertEqual(probe.get("codec_name"), "flac")
            self.assertEqual(probe.get("sample_rate"), "48000")
            self.assertEqual(probe.get("channels"), "2")
            self.assertEqual(probe.get("bits_per_raw_sample"), "24")
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_07_export_song_mode_multi_pattern(self):
        # Create second pattern and add to song order
        self.engine.addPattern()
        self.engine.addSongOrderStep(1)
        self.assertEqual(self.engine.songOrderLength, 2)

        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": True,
                "tail_seconds": 1.0,
                "sample_rate": 48000,
                "wav_bit_depth": 16
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Song mode export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            probe = self._probe_audio(target)
            # Duration should be at least (32 rows * 2 patterns * samplesPerRow / 48000) + 1.0s tail
            # At 120bpm, 1 row = 0.125s -> 64 rows = 8.0s + 1.0s tail = 9.0s
            duration = float(probe.get("duration", 0.0))
            self.assertGreaterEqual(duration, 8.5)
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_08_export_cancellation(self):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            loop = QEventLoop()
            finished_result = {"success": None, "msg": ""}

            def on_finished(success, msg):
                finished_result["success"] = success
                finished_result["msg"] = msg
                loop.quit()

            self.engine.exportFinished.connect(on_finished)

            # Initiate export of a long song
            self.engine.addPattern()
            for _ in range(8):
                self.engine.addSongOrderStep(0)

            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": True,
                "tail_seconds": 4.0,
                "sample_rate": 48000,
                "wav_bit_depth": 24
            }
            self.engine.startExport(params)
            self.assertTrue(self.engine.isExporting)

            # Abort after 50ms
            QTimer.singleShot(50, self.engine.cancelExport)
            loop.exec()

            self.engine.exportFinished.disconnect(on_finished)
            self.assertFalse(self.engine.isExporting)
            self.assertFalse(finished_result["success"])
            self.assertIn("cancel", finished_result["msg"].lower())
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_09_default_export_path_generation(self):
        self.engine.newProject()
        wav_path = self.engine.getDefaultExportPath("wav")
        self.assertTrue(wav_path.endswith("untitled.wav"))
        flac_path = self.engine.getDefaultExportPath("flac")
        self.assertTrue(flac_path.endswith("untitled.flac"))
        mp3_path = self.engine.getDefaultExportPath("mp3")
        self.assertTrue(mp3_path.endswith("untitled.mp3"))
        ogg_path = self.engine.getDefaultExportPath("ogg")
        self.assertTrue(ogg_path.endswith("untitled.ogg"))

    def test_10_song_order_rearranging(self):
        self.engine.newProject()
        self.assertEqual(self.engine.songOrderLength, 1)
        self.assertEqual(self.engine.getSongOrderStep(0), 0)

        # Add patterns and steps
        self.engine.addPattern()
        self.engine.addPattern()
        self.engine.addSongOrderStep(1)
        self.engine.addSongOrderStep(2)
        self.assertEqual(self.engine.songOrderLength, 3)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1, 2])

        # Move step 2 left to index 0: [2, 0, 1]
        ok = self.engine.moveSongOrderStep(2, 0)
        self.assertTrue(ok)
        self.assertEqual(self.engine.getSongOrderList(), [2, 0, 1])

        # Move step 0 right to index 1: [0, 2, 1]
        ok = self.engine.moveSongOrderStep(0, 1)
        self.assertTrue(ok)
        self.assertEqual(self.engine.getSongOrderList(), [0, 2, 1])

        # Swap steps 1 and 2: [0, 1, 2]
        ok = self.engine.swapSongOrderSteps(1, 2)
        self.assertTrue(ok)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1, 2])

        # Invalid bounds checks
        self.assertFalse(self.engine.moveSongOrderStep(-1, 0))
        self.assertFalse(self.engine.moveSongOrderStep(0, 99))
        self.assertFalse(self.engine.swapSongOrderSteps(0, 0))

    def test_11_pattern_export_looping(self):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
            target = tf.name

        try:
            # 32 rows at 120bpm = 4.0s. 2 loops = 8.0s + 0.5s tail = 8.5s
            params = {
                "filepath": target,
                "format": "wav",
                "song_mode": False,
                "pattern_idx": 0,
                "tail_seconds": 0.5,
                "pattern_loops": 2,
                "sample_rate": 48000,
                "wav_bit_depth": 16
            }
            res = self._run_export_sync(params)
            self.assertTrue(res["success"], f"Pattern loop export failed: {res['msg']}")
            self.assertTrue(os.path.exists(target))
            probe = self._probe_audio(target)
            duration = float(probe.get("duration", 0.0))
            self.assertGreaterEqual(duration, 8.0)
        finally:
            if os.path.exists(target):
                os.unlink(target)

    def test_12_remove_selected_track(self):
        self.engine.newProject()
        self.engine.addTrack()
        self.engine.addTrack()
        self.assertEqual(self.engine.numTracks, 3)
        self.engine.setSelectedInstrument(1)

        self.engine.removeSelectedTrack()
        self.assertEqual(self.engine.numTracks, 2)

if __name__ == "__main__":
    unittest.main()

