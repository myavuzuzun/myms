import unittest
from myms_bridge import MymsEngine

class TestPhase4SongArranger(unittest.TestCase):
    def setUp(self):
        self.engine = MymsEngine()
        self.engine.newProject()

    def tearDown(self):
        self.engine.stop()

    def test_01_default_song_order(self):
        # Default project should have 1 step pointing to pattern 0
        self.assertEqual(self.engine.songOrderLength, 1)
        self.assertEqual(self.engine.getSongOrderStep(0), 0)
        self.assertEqual(self.engine.getSongOrderList(), [0])
        self.assertFalse(self.engine.isSongMode)

    def test_02_mode_toggle(self):
        # Toggle song mode
        self.engine.toggleSongMode()
        self.assertTrue(self.engine.isSongMode)
        self.engine.toggleSongMode()
        self.assertFalse(self.engine.isSongMode)

    def test_03_add_and_insert_step(self):
        # Add pattern 1
        p1 = self.engine.addPattern()
        self.assertEqual(p1, 1)

        # Add step pointing to pattern 1 -> [0, 1]
        self.engine.addSongOrderStep(1)
        self.assertEqual(self.engine.songOrderLength, 2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])

        # Insert pattern 0 at step 1 -> [0, 0, 1]
        self.engine.insertSongOrderStep(1, 0)
        self.assertEqual(self.engine.songOrderLength, 3)
        self.assertEqual(self.engine.getSongOrderList(), [0, 0, 1])

        # Insert pattern 1 at step 0 -> [1, 0, 0, 1]
        self.engine.insertSongOrderStep(0, 1)
        self.assertEqual(self.engine.songOrderLength, 4)
        self.assertEqual(self.engine.getSongOrderList(), [1, 0, 0, 1])

    def test_04_insert_new_pattern_step(self):
        # Insert new pattern at step 1
        new_pat = self.engine.insertNewPatternStep(1)
        self.assertEqual(new_pat, 1)
        self.assertEqual(self.engine.numPatterns, 2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])
        self.assertEqual(self.engine.currentPattern, 1)

    def test_05_duplicate_pattern_step(self):
        # Set a note in pattern 0
        self.engine.setCell(0, 0, 60, 1, 200, 0, 0)
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

        # Duplicate step 0
        dup_pat = self.engine.duplicatePatternStep(0)
        self.assertEqual(dup_pat, 1)
        self.assertEqual(self.engine.numPatterns, 2)
        self.assertEqual(self.engine.songOrderLength, 2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1])
        # Pattern 1 should contain the duplicated note
        self.assertEqual(self.engine.getCell(0, 0)["note"], 60)

    def test_06_move_and_swap_steps(self):
        # Setup song order: [0, 1, 2]
        self.engine.addPattern() # 1
        self.engine.addPattern() # 2
        self.engine.addSongOrderStep(1)
        self.engine.addSongOrderStep(2)
        self.assertEqual(self.engine.getSongOrderList(), [0, 1, 2])

        # Move step 2 to step 0 -> [2, 0, 1]
        ok = self.engine.moveSongOrderStep(2, 0)
        self.assertTrue(ok)
        self.assertEqual(self.engine.getSongOrderList(), [2, 0, 1])

        # Swap step 1 and 2 -> [2, 1, 0]
        ok = self.engine.swapSongOrderSteps(1, 2)
        self.assertTrue(ok)
        self.assertEqual(self.engine.getSongOrderList(), [2, 1, 0])

    def test_07_remove_step(self):
        self.engine.addSongOrderStep(0)
        self.assertEqual(self.engine.songOrderLength, 2)
        self.engine.removeSongOrderStep(1)
        self.assertEqual(self.engine.songOrderLength, 1)
        # Cannot remove last step
        self.engine.removeSongOrderStep(0)
        self.assertEqual(self.engine.songOrderLength, 1)

if __name__ == "__main__":
    unittest.main()
