import os
import sys
import tempfile
import time

# Ensure current directory is in sys.path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from PyQt6.QtCore import QCoreApplication
from myms_bridge import MymsEngine

def run_tests():
    app = QCoreApplication(sys.argv)
    print("=== Testing MymsEngine Phase 2 Refinements ===")
    
    engine = MymsEngine()
    assert engine._is_initialized, "Audio engine should be initialized"
    print("✓ Audio engine initialized successfully")

    # 1. Test Track addition and types
    engine.newProject()
    assert engine.numTracks == 1, f"Expected 1 track at start, got {engine.numTracks}"
    assert engine.getTrackType(0) == 0, "Track 0 should be Synth"

    # Add second track and set to SF2
    engine.addTrack()
    assert engine.numTracks == 2, f"Expected 2 tracks, got {engine.numTracks}"
    engine.setTrackType(1, 1) # SF2
    assert engine.getTrackType(1) == 1, "Track 1 should be SF2"

    # Add third track and set to Drums
    engine.addTrack()
    assert engine.numTracks == 3, f"Expected 3 tracks, got {engine.numTracks}"
    engine.setTrackType(2, 2) # Drums
    assert engine.getTrackType(2) == 2, "Track 2 should be Drums"
    print("✓ Track adding and engine typing (Synth, SF2, Drums) passed")

    # 2. Test Independent SF2 presets per track
    if os.path.exists("/usr/share/sounds/sf2/default-GM.sf2"):
        ok = engine.loadSoundFont("/usr/share/sounds/sf2/default-GM.sf2")
        assert ok, "Failed to load default SoundFont"
        print(f"✓ SoundFont loaded with {engine.sf2PresetCount} presets")
        
        # Track 0 as SF2 with preset 0 (Piano)
        engine.setTrackType(0, 1)
        engine.setTrackSf2Preset(0, 0)

        # Track 1 as SF2 with preset 19 (Church Organ)
        engine.setTrackSf2Preset(1, 19)

        assert engine.getTrackSf2Preset(0) == 0, "Track 0 should have preset 0"
        assert engine.getTrackSf2Preset(1) == 19, "Track 1 should have preset 19"
        print("✓ Independent SoundFont presets per track confirmed (Track 0 != Track 1)")

    # 3. Test Per-Track Mixer Controls (Volume, Pan, Mute, Solo)
    engine.setTrackVolume(0, 0.75)
    engine.setTrackPan(0, 0.25)
    engine.setTrackMute(0, True)
    engine.setTrackSolo(1, True)

    assert abs(engine.getTrackVolume(0) - 0.75) < 0.001, "Track 0 volume mismatch"
    assert abs(engine.getTrackPan(0) - 0.25) < 0.001, "Track 0 pan mismatch"
    assert engine.getTrackMute(0) is True, "Track 0 should be muted"
    assert engine.getTrackSolo(1) is True, "Track 1 should be soloed"
    print("✓ Mixer controls (Volume, Pan, Mute, Solo) passed")

    # 4. Test Multi-Pattern & Song Order
    engine.setCurrentPattern(0)
    engine.setCell(0, 0, 60, 1, 200, 0, 0) # Pattern 0, Track 0, Row 0 = C-4
    cell_p0 = engine.getCell(0, 0)
    assert cell_p0["note"] == 60, "Pattern 0 cell mismatch"

    # Add Pattern 1
    new_pat = engine.addPattern()
    assert new_pat == 1
    assert engine.numPatterns == 2
    assert engine.currentPattern == 1

    # Pattern 1 cell should be empty NO_NOTE (255)
    cell_p1 = engine.getCell(0, 0)
    assert cell_p1["note"] == 255, f"Pattern 1 cell should be empty, got {cell_p1['note']}"

    # Set note in Pattern 1
    engine.setCell(0, 0, 72, 1, 240, 0, 0) # Pattern 1, Track 0, Row 0 = C-5
    cell_p1_mod = engine.getCell(0, 0)
    assert cell_p1_mod["note"] == 72, "Pattern 1 modified cell mismatch"

    # Switch back to Pattern 0 and check note is still C-4
    engine.setCurrentPattern(0)
    assert engine.currentPattern == 0
    cell_p0_back = engine.getCell(0, 0)
    assert cell_p0_back["note"] == 60, f"Pattern 0 cell should still be 60, got {cell_p0_back['note']}"
    print("✓ Multi-pattern note isolation and switching passed")

    # 5. Test Song Order Arranger
    order = engine.getSongOrderList()
    print("Initial song order:", order)
    engine.addSongOrderStep(0)
    engine.addSongOrderStep(1)
    engine.addSongOrderStep(0)
    order_after = engine.getSongOrderList()
    print("Song order after adding steps:", order_after)
    assert len(order_after) >= 4, f"Expected at least 4 song steps, got {len(order_after)}"
    
    engine.setSongOrderIndex(1)
    assert engine.songOrderIndex == 1
    assert engine.currentPattern == order_after[1]
    print("✓ Song Order Arranger sequencing passed")

    # 6. Test Project Save & Open
    with tempfile.NamedTemporaryFile(suffix=".myms", delete=False) as tmp:
        tmp_path = tmp.name

    try:
        ok_save = engine.saveProject(tmp_path)
        assert ok_save, "saveProject failed"
        print(f"✓ Project saved to {tmp_path}")

        # Reset to new empty project
        engine.newProject()
        assert engine.numTracks == 1
        assert engine.numPatterns == 1

        # Open saved project
        ok_open = engine.openProject(tmp_path)
        assert ok_open, "openProject failed"
        assert engine.numTracks == 3, f"Expected 3 tracks after load, got {engine.numTracks}"
        assert abs(engine.getTrackVolume(0) - 0.75) < 0.001, "Loaded track 0 volume mismatch"
        assert abs(engine.getTrackPan(0) - 0.25) < 0.001, "Loaded track 0 pan mismatch"
        assert engine.getTrackMute(0) is True, "Loaded track 0 mute mismatch"
        assert engine.getTrackSolo(1) is True, "Loaded track 1 solo mismatch"
        if os.path.exists("/usr/share/sounds/sf2/default-GM.sf2"):
            assert engine.getTrackSf2Preset(1) == 19, "Loaded track 1 SF2 preset mismatch"
        
        # Check patterns after load
        assert engine.numPatterns == 2, f"Expected 2 patterns after load, got {engine.numPatterns}"
        engine.setCurrentPattern(0)
        assert engine.getCell(0, 0)["note"] == 60, "Loaded pattern 0 cell mismatch"
        engine.setCurrentPattern(1)
        assert engine.getCell(0, 0)["note"] == 72, "Loaded pattern 1 cell mismatch"
        print("✓ Full project save/open with mixer states and patterns verified!")
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)

    engine.close()
    print("\nALL PHASE 2 REFINEMENT TESTS PASSED PERFECTLY!")

if __name__ == "__main__":
    run_tests()
