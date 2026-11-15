import os
import unittest
import sys

os.environ["QT_QPA_PLATFORM"] = "offscreen"
from PyQt6.QtWidgets import QApplication
from PyQt6.QtQuickWidgets import QQuickWidget
from PyQt6.QtCore import QUrl, QObject
from myms_bridge import MymsEngine
from main import AppWindow

app = QApplication.instance() or QApplication(sys.argv)

class TestPhase4ChunkSelection(unittest.TestCase):
    def setUp(self):
        self.engine = MymsEngine()
        self.engine.newProject()

    def tearDown(self):
        self.engine.stop()

    def test_01_copy_and_paste_single_cell(self):
        while self.engine.numTracks < 2:
            self.engine.addTrack()

        # Set a cell at Track 0, Row 4: Note 60 (C-4), Inst 1, Vol 200, FX 0x01, 0x20
        self.engine.setCell(0, 4, 60, 1, 200, 1, 32)
        cell = self.engine.getCell(0, 4)
        self.assertEqual(cell["note"], 60)
        self.assertEqual(cell["volume"], 200)

        # Copy single cell range
        self.engine.copyRange(0, 4, 0, 4)
        self.assertTrue(self.engine.hasClipboardData)
        clip = self.engine._cell_clipboard
        self.assertEqual(clip["width"], 1)
        self.assertEqual(clip["height"], 1)
        self.assertEqual(len(clip["rows"]), 1)
        self.assertEqual(clip["rows"][0][0]["note"], 60)

        # Paste onto Track 1, Row 10 in overwrite mode
        self.engine.pasteClipboard(1, 10)
        pasted = self.engine.getCell(1, 10)
        self.assertEqual(pasted["note"], 60)
        self.assertEqual(pasted["instrument"], 1)
        self.assertEqual(pasted["volume"], 200)
        self.assertEqual(pasted["effectType"], 1)
        self.assertEqual(pasted["effectParam"], 32)

    def test_02_copy_and_paste_multitrack_chunk(self):
        # Ensure we have at least 4 tracks
        while self.engine.numTracks < 4:
            self.engine.addTrack()

        # Populate a 2x3 block: Tracks 0..1, Rows 0..2
        # (T0, R0): 60, (T1, R0): 64
        # (T0, R1): 62, (T1, R1): 65
        # (T0, R2): 64, (T1, R2): 67
        self.engine.setCell(0, 0, 60, 1, 180, 0, 0)
        self.engine.setCell(1, 0, 64, 2, 190, 0, 0)
        self.engine.setCell(0, 1, 62, 1, 185, 0, 0)
        self.engine.setCell(1, 1, 65, 2, 195, 0, 0)
        self.engine.setCell(0, 2, 64, 1, 190, 0, 0)
        self.engine.setCell(1, 2, 67, 2, 200, 0, 0)

        # Copy the chunk
        self.engine.copyRange(0, 0, 1, 2)
        self.assertTrue(self.engine.hasClipboardData)
        clip = self.engine._cell_clipboard
        self.assertEqual(clip["width"], 2)
        self.assertEqual(clip["height"], 3)

        # Paste onto Tracks 2..3, Rows 8..10
        self.engine.pasteClipboard(2, 8)

        # Verify target cells
        self.assertEqual(self.engine.getCell(2, 8)["note"], 60)
        self.assertEqual(self.engine.getCell(3, 8)["note"], 64)
        self.assertEqual(self.engine.getCell(2, 9)["note"], 62)
        self.assertEqual(self.engine.getCell(3, 9)["note"], 65)
        self.assertEqual(self.engine.getCell(2, 10)["note"], 64)
        self.assertEqual(self.engine.getCell(3, 10)["note"], 67)

        # Verify neighboring cells untouched
        self.assertEqual(self.engine.getCell(2, 7)["note"], 255)
        self.assertEqual(self.engine.getCell(2, 11)["note"], 255)

    def test_03_cut_and_delete_chunk(self):
        self.engine.setCell(0, 5, 72, 1, 220, 0, 0)
        self.engine.setCell(0, 6, 74, 1, 220, 0, 0)

        # Cut Range (0, 5) to (0, 6)
        self.engine.cutRange(0, 5, 0, 6)
        self.assertTrue(self.engine.hasClipboardData)

        # Verify cells cut to empty
        self.assertEqual(self.engine.getCell(0, 5)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 6)["note"], 255)

        # Paste elsewhere
        self.engine.pasteClipboard(0, 12)
        self.assertEqual(self.engine.getCell(0, 12)["note"], 72)
        self.assertEqual(self.engine.getCell(0, 13)["note"], 74)

        # Test deleteRange directly
        self.engine.deleteRange(0, 12, 0, 13)
        self.assertEqual(self.engine.getCell(0, 12)["note"], 255)
        self.assertEqual(self.engine.getCell(0, 13)["note"], 255)

    def test_04_paste_clipping_at_bounds(self):
        # Create a 3x3 chunk
        while self.engine.numTracks < 3:
            self.engine.addTrack()
        for t in range(3):
            for r in range(3):
                self.engine.setCell(t, r, 50 + t * 3 + r, t + 1, 200, 0, 0)

        self.engine.copyRange(0, 0, 2, 2)

        # Paste near bottom and right border: Track = numTracks - 1, Row = numRows - 1
        last_t = self.engine.numTracks - 1
        last_r = self.engine.numRows - 1
        self.engine.pasteClipboard(last_t, last_r)

        # Should safely paste the top-left cell into (last_t, last_r) and clip the rest without error
        cell = self.engine.getCell(last_t, last_r)
        self.assertEqual(cell["note"], 50)

    def test_05_qml_trackerview_selection_logic(self):
        win = AppWindow(self.engine)
        root = win.quick_widget.rootObject()
        self.assertIsNotNone(root)
        tracker = root.findChild(QObject, "trackerView")
        self.assertIsNotNone(tracker)

        # Initially no selection
        self.assertFalse(tracker.property("hasSelection"))

        # Select a range from (T0, R2) to (T2, R8)
        tracker.setProperty("selStartTrack", 0)
        tracker.setProperty("selStartRow", 2)
        tracker.setProperty("selEndTrack", 2)
        tracker.setProperty("selEndRow", 8)
        tracker.setProperty("hasSelection", True)

        self.assertTrue(tracker.property("hasSelection"))
        # Test helper functions
        self.assertTrue(tracker.isCellSelected(0, 2))
        self.assertTrue(tracker.isCellSelected(1, 5))
        self.assertTrue(tracker.isCellSelected(2, 8))
        self.assertFalse(tracker.isCellSelected(0, 1))
        self.assertFalse(tracker.isCellSelected(3, 5))
        self.assertFalse(tracker.isCellSelected(1, 9))

        # Test selectAll
        tracker.selectAll()
        self.assertTrue(tracker.property("hasSelection"))
        self.assertEqual(tracker.property("selStartTrack"), 0)
        self.assertEqual(tracker.property("selStartRow"), 0)
        self.assertEqual(tracker.property("selEndTrack"), self.engine.numTracks - 1)
        self.assertEqual(tracker.property("selEndRow"), self.engine.numRows - 1)

        # Test clearSelection
        tracker.clearSelection()
        self.assertFalse(tracker.property("hasSelection"))

        win.close()

    def test_06_escape_clears_selection_without_stopping(self):
        win = AppWindow(self.engine)
        root = win.quick_widget.rootObject()
        tracker = root.findChild(QObject, "trackerView")

        # Start playback
        self.engine.play()
        self.assertTrue(self.engine.isPlaying)

        # Set active chunk selection
        tracker.setProperty("hasSelection", True)
        tracker.setProperty("selStartTrack", 0)
        tracker.setProperty("selStartRow", 0)
        tracker.setProperty("selEndTrack", 1)
        tracker.setProperty("selEndRow", 4)

        # Trigger Escape
        win._on_escape_or_stop()

        # Selection should be cleared, but playback MUST keep playing!
        self.assertFalse(tracker.property("hasSelection"))
        self.assertTrue(self.engine.isPlaying)

        # Trigger Escape again: now with no selection, playback stops
        win._on_escape_or_stop()
        self.assertFalse(self.engine.isPlaying)

        win.close()

    def test_07_menu_clipboard_actions(self):
        win = AppWindow(self.engine)
        root = win.quick_widget.rootObject()
        tracker = root.findChild(QObject, "trackerView")

        while self.engine.numTracks < 3:
            self.engine.addTrack()

        # Set cell at (0, 0)
        self.engine.setCell(0, 0, 65, 1, 210, 0, 0)

        # Select (0, 0) via tracker selection
        tracker.setProperty("hasSelection", True)
        tracker.setProperty("selStartTrack", 0)
        tracker.setProperty("selStartRow", 0)
        tracker.setProperty("selEndTrack", 0)
        tracker.setProperty("selEndRow", 0)

        # Trigger Copy menu action
        win._on_copy()
        self.assertTrue(self.engine.hasClipboardData)

        # Move tracker selection to (1, 5)
        tracker.setProperty("hasSelection", False)
        tracker.setProperty("selectedTrack", 1)
        tracker.setProperty("selectedRow", 5)

        # Trigger Paste menu action
        win._on_paste()
        self.assertEqual(self.engine.getCell(1, 5)["note"], 65)

        # Select (1, 5) and trigger Cut menu action
        tracker.setProperty("hasSelection", True)
        tracker.setProperty("selStartTrack", 1)
        tracker.setProperty("selStartRow", 5)
        tracker.setProperty("selEndTrack", 1)
        tracker.setProperty("selEndRow", 5)

        win._on_cut()
        self.assertEqual(self.engine.getCell(1, 5)["note"], 255)
        self.assertFalse(tracker.property("hasSelection"))

        # Test Select All menu action
        win._on_select_all()
        self.assertTrue(tracker.property("hasSelection"))
        self.assertEqual(tracker.property("selEndTrack"), self.engine.numTracks - 1)
        self.assertEqual(tracker.property("selEndRow"), self.engine.numRows - 1)

        win.close()

if __name__ == "__main__":
    unittest.main()
