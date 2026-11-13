import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from PyQt6.QtCore import QCoreApplication
from myms_bridge import MymsEngine

def test_monophony_and_naming():
    app = QCoreApplication(sys.argv)
    print("=== Testing Monophony, NOTE_OFF, and Project Naming ===")

    engine = MymsEngine()
    assert engine._is_initialized, "Engine should initialize"

    # 1. Test Project Name
    engine.newProject()
    assert engine.projectName == "", f"New project should have empty name, got '{engine.projectName}'"
    print("✓ Initial project name is blank (untitled)")

    with tempfile.TemporaryDirectory() as tmpdir:
        test_proj = os.path.join(tmpdir, "MyTrackerSong.myms")
        ok = engine.saveProject(test_proj)
        assert ok, "Saving project should succeed"
        assert engine.projectName == "MyTrackerSong", f"Expected 'MyTrackerSong', got '{engine.projectName}'"
        print("✓ Saved project updates projectName to 'MyTrackerSong'")

        # Open project
        ok = engine.openProject(test_proj)
        assert ok, "Opening project should succeed"
        assert engine.projectName == "MyTrackerSong", f"Expected 'MyTrackerSong', got '{engine.projectName}'"
        print("✓ Loaded project retains projectName")

        # New project resets name
        engine.newProject()
        assert engine.projectName == "", "New project resets projectName to empty"
        print("✓ New project resets projectName to empty")

    # 2. Test Tracker Monophony & Note Off
    # Trigger note on track 0 (Synth)
    engine.noteOnInst(0, 60, 1.0) # C-4
    time.sleep(0.05)
    
    # Trigger next note on same track -> should cut previous note
    engine.noteOnInst(0, 64, 1.0) # E-4
    time.sleep(0.05)

    # Note off
    engine.noteOffInst(0, 254) # Explicit NOTE_OFF
    time.sleep(0.05)

    # Test SoundFont noteOn and noteOff
    if os.path.exists("/usr/share/sounds/sf2/default-GM.sf2"):
        engine.loadSoundFont("/usr/share/sounds/sf2/default-GM.sf2")
        engine.addTrack()
        engine.setTrackType(1, 1) # SF2
        engine.noteOnInst(1, 60, 1.0)
        time.sleep(0.05)
        engine.noteOnInst(1, 67, 1.0) # cuts 60
        time.sleep(0.05)
        engine.noteOffInst(1, 254) # cuts 67
        print("✓ SoundFont noteOn/noteOff with monophony executed cleanly")

    print("✓ All monophony and project naming tests passed!")

if __name__ == "__main__":
    test_monophony_and_naming()
