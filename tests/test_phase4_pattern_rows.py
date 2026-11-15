import os
import unittest
import sys
import tempfile
import json

os.environ["QT_QPA_PLATFORM"] = "offscreen"
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import QUrl
from myms_bridge import MymsEngine
from main import AppWindow

app = QApplication.instance() or QApplication(sys.argv)

class TestPhase4PatternRows(unittest.TestCase):
    def setUp(self):
        self.engine = MymsEngine()
        self.engine.newProject()

    def tearDown(self):
        self.engine.stop()

    def test_01_pattern_row_count_switching(self):
        # By default, pattern 0 should have 32 rows
        self.assertEqual(self.engine.numRows, 32)
        self.assertEqual(self.engine.getPatternNumRows(0), 32)

        # Set pattern 0 to 16 rows
        rows_signals = []
        self.engine.numRowsChanged.connect(lambda r: rows_signals.append(r))
        self.engine.setNumRows(16)
        self.assertEqual(self.engine.numRows, 16)
        self.assertEqual(self.engine.getPatternNumRows(0), 16)
        self.assertIn(16, rows_signals)

        # Add pattern 1
        new_pat_idx = self.engine.addPattern()
        self.assertEqual(new_pat_idx, 1)
        self.assertEqual(self.engine.currentPattern, 1)

        # Set pattern 1 to 48 rows
        self.engine.setNumRows(48)
        self.assertEqual(self.engine.numRows, 48)
        self.assertEqual(self.engine.getPatternNumRows(1), 48)

        # Add pattern 2 and set to 64 rows
        p2_idx = self.engine.addPattern()
        self.assertEqual(p2_idx, 2)
        self.assertEqual(self.engine.currentPattern, 2)
        self.engine.setNumRows(64)
        self.assertEqual(self.engine.numRows, 64)
        self.assertEqual(self.engine.getPatternNumRows(2), 64)

        # Clear signals and switch back to pattern 0
        rows_signals.clear()
        self.engine.setCurrentPattern(0)
        self.assertEqual(self.engine.currentPattern, 0)
        self.assertEqual(self.engine.numRows, 16)
        self.assertEqual(rows_signals[-1], 16)

        # Switch to pattern 1
        self.engine.setCurrentPattern(1)
        self.assertEqual(self.engine.currentPattern, 1)
        self.assertEqual(self.engine.numRows, 48)
        self.assertEqual(rows_signals[-1], 48)

        # Switch to pattern 2
        self.engine.setCurrentPattern(2)
        self.assertEqual(self.engine.currentPattern, 2)
        self.assertEqual(self.engine.numRows, 64)
        self.assertEqual(rows_signals[-1], 64)

    def test_02_per_pattern_cells_and_serialization(self):
        # Pattern 0: 16 rows. Set cell at track 0, row 15
        self.engine.setNumRows(16)
        self.engine.setCell(0, 15, 60, 1, 200, 0, 0)

        # Pattern 1: 48 rows. Set cell at track 0, row 40
        self.engine.addPattern()
        self.engine.setNumRows(48)
        self.engine.setCell(0, 40, 64, 1, 180, 0, 0)

        # Save project
        with tempfile.NamedTemporaryFile(suffix=".myms", delete=False) as tf:
            save_path = tf.name

        try:
            self.engine.saveProject(save_path)

            # Inspect saved JSON to ensure num_rows is stored per pattern
            with open(save_path, "r", encoding="utf-8") as f:
                saved_data = json.load(f)

            self.assertIn("patterns", saved_data)
            self.assertEqual(len(saved_data["patterns"]), 2)
            self.assertEqual(saved_data["patterns"][0]["num_rows"], 16)
            self.assertEqual(saved_data["patterns"][1]["num_rows"], 48)

            # Create clean project and reload
            self.engine.newProject()
            self.assertEqual(self.engine.numPatterns, 1)
            self.assertEqual(self.engine.numRows, 32)

            self.engine.openProject(save_path)
            self.assertEqual(self.engine.numPatterns, 2)

            # Check pattern 0
            self.engine.setCurrentPattern(0)
            self.assertEqual(self.engine.numRows, 16)
            cell_p0 = self.engine.getCell(0, 15)
            self.assertEqual(cell_p0["note"], 60)

            # Check pattern 1
            self.engine.setCurrentPattern(1)
            self.assertEqual(self.engine.numRows, 48)
            cell_p1 = self.engine.getCell(0, 40)
            self.assertEqual(cell_p1["note"], 64)
        finally:
            if os.path.exists(save_path):
                os.remove(save_path)

    def test_03_legacy_project_backward_compatibility(self):
        # Create a mock legacy .myms where "patterns" is a list of cell lists (no dict)
        with tempfile.NamedTemporaryFile(suffix=".myms", delete=False, mode="w", encoding="utf-8") as tf:
            legacy_data = {
                "version": 1,
                "bpm": 120.0,
                "tracks": [{"name": "Track 1", "volume": 0.8, "pan": 0.0, "color": "#3daee9", "effects": []}],
                "patterns": [
                    [{"track": 0, "row": 5, "note": 48, "instrument": 1, "volume": 200, "effect_type": 0, "effect_param": 0}],
                    [{"track": 0, "row": 10, "note": 55, "instrument": 1, "volume": 200, "effect_type": 0, "effect_param": 0}]
                ],
                "song_order": [0, 1]
            }
            json.dump(legacy_data, tf)
            legacy_path = tf.name

        try:
            self.engine.newProject()
            self.engine.openProject(legacy_path)
            self.assertEqual(self.engine.numPatterns, 2)
            self.engine.setCurrentPattern(0)
            cell = self.engine.getCell(0, 5)
            self.assertEqual(cell["note"], 48)
            self.engine.setCurrentPattern(1)
            cell2 = self.engine.getCell(0, 10)
            self.assertEqual(cell2["note"], 55)
        finally:
            if os.path.exists(legacy_path):
                os.remove(legacy_path)

    def test_04_qml_rows_input_synchronization(self):
        # Instantiate AppWindow and test QML UI bindings
        window = AppWindow(self.engine)

        # Pattern 0 set to 16 rows
        self.engine.setNumRows(16)
        # Add pattern 1 and set to 48 rows
        self.engine.addPattern()
        self.engine.setNumRows(48)

        # Switch back to pattern 0
        self.engine.setCurrentPattern(0)
        self.assertEqual(self.engine.numRows, 16)

        # Find rowsInput in QML hierarchy
        rows_input = window.findChild(object, "rowsInput")
        if rows_input is not None:
            self.assertEqual(rows_input.property("text"), "16")

            # Switch to pattern 1
            self.engine.setCurrentPattern(1)
            self.assertEqual(rows_input.property("text"), "48")

            # Switch back to pattern 0
            self.engine.setCurrentPattern(0)
            self.assertEqual(rows_input.property("text"), "16")

        window.close()

if __name__ == "__main__":
    unittest.main()
