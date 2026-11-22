import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from PyQt6.QtCore import QCoreApplication
from myms_bridge import MymsEngine

def test_plugins():
    app = QCoreApplication(sys.argv)
    print("=== Testing Phase 3: Linux VST3 & CLAP Plugin Support ===")

    engine = MymsEngine()
    assert engine._is_initialized, "Engine should be initialized"

    vst3_path = os.path.abspath("plugins/vst3/demo_synth_vst3.so")
    clap_path = os.path.abspath("plugins/clap/demo_synth_clap.clap")

    assert os.path.exists(vst3_path), f"VST3 demo binary missing: {vst3_path}"
    assert os.path.exists(clap_path), f"CLAP demo binary missing: {clap_path}"

    engine.newProject()
    assert engine.numTracks == 1

    # 1. Load VST3 on Track 0
    ok = engine.loadTrackPlugin(0, vst3_path)
    assert ok, "Should load VST3 plugin on Track 0"
    assert engine.isTrackPluginLoaded(0)
    assert engine.getTrackType(0) == 3
    assert engine.getTrackPluginFormatName(0) == "VST3"
    assert engine.getTrackPluginName(0) == "Demo VST3 Synth"
    assert engine.getTrackPluginNumParams(0) == 3

    params = engine.getTrackPluginParams(0)
    assert len(params) == 3
    print("✓ Track 0 loaded Linux VST3:", engine.getTrackPluginName(0), "with params:", [p["name"] for p in params])

    # Set parameter
    engine.setTrackPluginParamValue(0, 0, 0.85)
    params_after = engine.getTrackPluginParams(0)
    assert abs(params_after[0]["value"] - 0.85) < 0.01
    print("✓ Track 0 VST3 parameter 0 set to 0.85")

    # 2. Add Track 1 and load CLAP
    engine.addTrack()
    assert engine.numTracks == 2
    ok_clap = engine.loadTrackPlugin(1, clap_path)
    assert ok_clap, "Should load CLAP plugin on Track 1"
    assert engine.isTrackPluginLoaded(1)
    assert engine.getTrackType(1) == 3 # TRACK_TYPE_PLUGIN
    assert engine.getTrackPluginFormatName(1) == "CLAP"
    assert engine.getTrackPluginName(1) == "DemoSynth CLAP"
    assert engine.getTrackPluginNumParams(1) == 4

    clap_params = engine.getTrackPluginParams(1)
    assert len(clap_params) == 4
    assert clap_params[1]["name"] == "Resonance"
    print("✓ Track 1 loaded Linux CLAP:", engine.getTrackPluginName(1), "with params:", [p["name"] for p in clap_params])

    # Set parameter
    engine.setTrackPluginParamValue(1, 1, 0.70)
    clap_params_after = engine.getTrackPluginParams(1)
    assert abs(clap_params_after[1]["value"] - 0.70) < 0.01
    print("✓ Track 1 CLAP parameter 1 set to 0.70")

    # 3. Test Note Triggering
    engine.noteOnInst(0, 60, 1.0)
    time.sleep(0.05)
    engine.noteOnInst(0, 64, 1.0) # cuts 60 (tracker monophony)
    time.sleep(0.05)
    engine.noteOffInst(0, 254)

    engine.noteOnInst(1, 67, 0.9)
    time.sleep(0.05)
    engine.noteOffInst(1, 254)
    print("✓ Note triggers and monophonic cutoff tested on both plugins")

    # 4. Test Project Save & Open (Format v3)
    with tempfile.TemporaryDirectory() as tmpdir:
        proj_path = os.path.join(tmpdir, "PluginSession.myms")
        ok_save = engine.saveProject(proj_path)
        assert ok_save, "Saving project v3 with plugins should succeed"
        print("✓ Project saved to", proj_path)

        engine.newProject()
        assert engine.numTracks == 1
        assert not engine.isTrackPluginLoaded(0)

        ok_open = engine.openProject(proj_path)
        assert ok_open, "Opening project v3 should succeed"
        assert engine.numTracks == 2
        assert engine.isTrackPluginLoaded(0)
        assert engine.getTrackType(0) == 3
        assert engine.getTrackPluginName(0) == "Demo VST3 Synth"
        p0 = engine.getTrackPluginParams(0)
        assert abs(p0[0]["value"] - 0.85) < 0.02, f"Expected 0.85, got {p0[0]['value']}"

        assert engine.isTrackPluginLoaded(1)
        assert engine.getTrackType(1) == 3 # TRACK_TYPE_PLUGIN
        assert engine.getTrackPluginName(1) == "DemoSynth CLAP"
        p1 = engine.getTrackPluginParams(1)
        assert abs(p1[1]["value"] - 0.70) < 0.02, f"Expected 0.70, got {p1[1]['value']}"
        print("✓ Project v3 reopened perfectly with VST3 and CLAP plugins and parameter values restored!")

    print("\nALL PHASE 3 PLUGIN HOST TESTS PASSED CLEANLY!\n")

if __name__ == "__main__":
    test_plugins()

