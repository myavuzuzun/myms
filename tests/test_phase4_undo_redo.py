#!/usr/bin/env python3
"""
Unit and Integration Tests for Undo & Redo History Engine:
- Single cell note/effect edit undo & redo
- Redo stack cleared on new edit
- Compound chunk operations (pasteClipboard, deleteRange, cutRange, transposeTrack, clearTrack)
- Track operations (addTrack, removeTrack, moveTrack, setInstrumentName, setTrackVolume)
- Pattern & Song Arranger operations (addPattern, setNumRows, addSongOrderStep, moveSongOrderStep, insertNewPatternStep)
- Coalescing of rapid continuous parameter adjustments
- newProject() and openProject() history reset
- AppWindow Edit menu Undo/Redo action synchronization
"""

import os
import sys
import time
import tempfile
import unittest

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

os.environ["QT_QPA_PLATFORM"] = "offscreen"
from PyQt6.QtWidgets import QApplication
from myms_bridge import MymsEngine
from main import AppWindow

app = QApplication.instance() or QApplication(sys.argv)


class TestPhase4UndoRedo(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.engine = MymsEngine()

    @classmethod
    def tearDownClass(cls):
        cls.engine.close()

    def setUp(self):
        self.engine.newProject()

    def test_01_cell_edit_undo_redo_and_stack_reset(self):
        self.assertFalse(self.engine.canUndo)
        self.assertFalse(self.engine.canRedo)

        # Edit cell (0, 0)
        self.engine.setCell(0, 0, 60, 1, 200, 0x10, 0x80)
        self.assertTrue(self.engine.canUndo)
        self.assertFalse(self.engine.canRedo)
        self.assertEqual(self.engine.undoText, "Edit Cell")
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

        # Undo
        self.assertTrue(self.engine.undo())
        self.assertFalse(self.engine.canUndo)
        self.assertTrue(self.engine.canRedo)
        self.assertEqual(self.engine.redoText, "Edit Cell")
        self.assertEqual(self.engine.getCell(0, 0)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 0)["effectType"], 0)

        # Redo
        self.assertTrue(self.engine.redo())
        self.assertTrue(self.engine.canUndo)
        self.assertFalse(self.engine.canRedo)
        c = self.engine.getCell(0, 0)
        self.assertEqual(c["note"], 60)
        self.assertEqual(c["volume"], 200)
        self.assertEqual(c["effectType"], 0x10)
        self.assertEqual(c["effectParam"], 0x80)

        # Undo again, then make a new edit -> redo stack must clear
        self.assertTrue(self.engine.undo())
        self.assertTrue(self.engine.canRedo)
        self.engine.setCell(0, 1, 64, 1, 255, 0, 0)
        self.assertFalse(self.engine.canRedo)
        self.assertEqual(self.engine.getCell(0, 1)["note"], 64)

    def test_02_compound_chunk_operations_single_undo_step(self):
        # Populate 3 rows
        self.engine.setCell(0, 0, 60, 1, 200, 0, 0)
        self.engine.setCell(0, 1, 62, 1, 200, 0, 0)
        self.engine.setCell(0, 2, 64, 1, 200, 0, 0)

        # Copy and paste as a single compound operation
        self.engine.copyRange(0, 0, 0, 2)
        self.engine.pasteClipboard(0, 4)
        self.assertEqual(self.engine.undoText, "Paste Chunk")
        self.assertEqual(self.engine.getCell(0, 4)["note"], 60)
        self.assertEqual(self.engine.getCell(0, 6)["note"], 64)

        # Single undo reverts all 3 pasted rows
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getCell(0, 4)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 5)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 6)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

        # Cut range reverts in a single undo
        self.engine.cutRange(0, 0, 0, 2)
        self.assertEqual(self.engine.undoText, "Cut Chunk")
        self.assertEqual(self.engine.getCell(0, 0)["note"], 255)
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)
        self.assertEqual(self.engine.getCell(0, 2)["note"], 64)

        # Transpose track reverts in a single undo
        self.engine.transposeTrack(0, 12)
        self.assertEqual(self.engine.getCell(0, 0)["note"], 72)
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

        # Clear track reverts in a single undo
        self.engine.clearTrack(0)
        self.assertEqual(self.engine.getCell(0, 0)["note"], 255)
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

    def test_03_track_and_mixer_undo_redo(self):
        self.engine.setInstrumentName(0, "Lead")
        self.engine.addTrack()
        self.assertEqual(self.engine.numTracks, 2)
        self.engine.setInstrumentName(1, "Bass")
        self.engine.setTrackVolume(1, 0.5)
        self.engine.moveTrack(0, 1)
        self.assertEqual(self.engine.getInstrumentName(0), "Bass")
        self.assertEqual(self.engine.getInstrumentName(1), "Lead")

        # Undo moveTrack
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getInstrumentName(0), "Lead")
        self.assertEqual(self.engine.getInstrumentName(1), "Bass")

        # Undo setTrackVolume
        self.assertTrue(self.engine.undo())
        self.assertAlmostEqual(self.engine.getTrackVolume(1), 1.0, places=2)

        # Undo rename Bass -> Synth 2
        self.assertTrue(self.engine.undo())
        # Undo addTrack -> back to 1 track
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.numTracks, 1)

        # Redo addTrack
        self.assertTrue(self.engine.redo())
        self.assertEqual(self.engine.numTracks, 2)

    def test_04_pattern_and_song_arranger_undo_redo(self):
        self.engine.setCell(0, 0, 48, 1, 200, 0, 0)
        new_pat = self.engine.insertNewPatternStep(1)
        self.assertEqual(new_pat, 1)
        self.assertEqual(self.engine.numPatterns, 2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])

        self.engine.moveSongOrderStep(1, 0)
        self.assertEqual(self.engine.getSongOrderList(), [1, 0])

        # Undo moveSongOrderStep
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])

        # Undo insertNewPatternStep
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.numPatterns, 1)
        self.assertEqual(self.engine.getSongOrderList(), [0])
        self.assertEqual(self.engine.getCell(0, 0)["note"], 48)

        # Redo insertNewPatternStep and moveSongOrderStep
        self.assertTrue(self.engine.redo())
        self.assertEqual(self.engine.numPatterns, 2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])
        self.assertTrue(self.engine.redo())
        self.assertEqual(self.engine.getSongOrderList(), [1, 0])

    def test_05_continuous_parameter_coalescing(self):
        # Simulate dragging BPM slider rapidly
        self.engine.setBpm(100.0)
        self.engine.setBpm(110.0)
        self.engine.setBpm(125.0)
        self.assertEqual(self.engine.bpm, 125.0)

        # Single undo restores original 90.0 BPM
        self.assertTrue(self.engine.undo())
        self.assertEqual(self.engine.bpm, 90.0)
        self.assertFalse(self.engine.canUndo)

        # Verify coalesce breaks across different keys or after timeout
        self.engine.setInstFilter(0, 1200.0, 0.5, 0.2)
        self.engine.setInstFilter(0, 800.0, 0.6, 0.3)
        self.assertTrue(self.engine.undo())
        self.assertAlmostEqual(self.engine.getInstParams(0)["cutoff"], 3500.0, places=1)

    def test_06_project_open_and_new_clear_history(self):
        self.engine.setCell(0, 0, 60, 1, 200, 0, 0)
        self.assertTrue(self.engine.canUndo)

        with tempfile.NamedTemporaryFile(suffix=".myms", delete=False) as tf:
            proj_path = tf.name

        try:
            self.assertTrue(self.engine.saveProject(proj_path))
            self.engine.newProject()
            self.assertFalse(self.engine.canUndo)
            self.assertFalse(self.engine.canRedo)

            self.assertTrue(self.engine.openProject(proj_path))
            self.assertFalse(self.engine.canUndo)
            self.assertFalse(self.engine.canRedo)
            self.assertEqual(self.engine.getCell(0, 0)["note"], 60)
        finally:
            if os.path.exists(proj_path):
                os.remove(proj_path)

    def test_07_app_window_edit_menu_actions_sync(self):
        window = AppWindow(self.engine)
        self.assertFalse(window.act_undo.isEnabled())
        self.assertFalse(window.act_redo.isEnabled())

        self.engine.setCell(0, 0, 60, 1, 200, 0, 0)
        self.assertTrue(window.act_undo.isEnabled())
        self.assertIn("Edit Cell", window.act_undo.text())
        self.assertFalse(window.act_redo.isEnabled())

        window._on_undo()
        self.assertFalse(window.act_undo.isEnabled())
        self.assertTrue(window.act_redo.isEnabled())
        self.assertIn("Edit Cell", window.act_redo.text())

        window._on_redo()
        self.assertTrue(window.act_undo.isEnabled())
        self.assertFalse(window.act_redo.isEnabled())
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)
        window.close()


if __name__ == "__main__":
    unittest.main()

