#!/usr/bin/env python3
"""
Unit and Integration Tests for Phase 3:
- Unified Plugin Instrument (VST3 / CLAP / VST2)
- Native GUI Window hosting hooks
- Interactive reference plugins (demo_gui_synth, demo_filter_fx)
- Per-Track Plugin Insert Effects Chain (MAX_TRACK_FX = 2)
- Project format v3 persistence and serialization
"""

import os
import sys
import json
import tempfile
import unittest

# Ensure project root is in Python module search path
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from myms_bridge import MymsEngine

class TestPhase3UnifiedAndFx(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.engine = MymsEngine()

    @classmethod
    def tearDownClass(cls):
        cls.engine.close()

    def setUp(self):
        self.engine.newProject()

    def test_01_unified_track_type_enum(self):
        # 0 = Synth, 1 = SoundFont, 2 = Drums, 3 = Plugin
        self.assertEqual(self.engine.getTrackType(0), 0)
        self.engine.setTrackType(0, 3)
        self.assertEqual(self.engine.getTrackType(0), 3)

    def test_02_load_unified_gui_synth(self):
        gui_synth_path = os.path.join(PROJECT_ROOT, "plugins", "gui_synth", "demo_gui_synth.so")
        self.assertTrue(os.path.exists(gui_synth_path), f"File not found: {gui_synth_path}")

        ok = self.engine.loadTrackPlugin(0, gui_synth_path)
        self.assertTrue(ok)
        self.assertTrue(self.engine.isTrackPluginLoaded(0))
        self.assertEqual(self.engine.getTrackType(0), 3)
        self.assertEqual(self.engine.getTrackPluginName(0), "Demo GUI Synth")
        self.assertEqual(self.engine.getTrackPluginFormatName(0), "VST2")
        self.assertTrue(self.engine.hasTrackPluginNativeEditor(0))

        # Check parameters
        num_params = self.engine.getTrackPluginNumParams(0)
        self.assertEqual(num_params, 4)
        params = self.engine.getTrackPluginParams(0)
        self.assertEqual(len(params), 4)
        param_names = [p["name"] for p in params]
        self.assertIn("Cutoff", param_names)
        self.assertIn("Resonance", param_names)
        self.assertIn("Decay", param_names)
        self.assertIn("Volume", param_names)

        # Set parameter
        self.engine.setTrackPluginParamValue(0, 0, 0.42)
        val = self.engine.lib.myms_plugin_get_param_value(0, 0)
        self.assertAlmostEqual(val, 0.42, places=2)

    def test_03_track_insert_fx_chain(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertTrue(os.path.exists(filter_fx_path), f"File not found: {filter_fx_path}")

        # Slot 0 initial state
        self.assertFalse(self.engine.isTrackFxLoaded(0, 0))

        # Load FX on Track 0, Slot 0
        ok = self.engine.loadTrackFx(0, 0, filter_fx_path)
        self.assertTrue(ok)
        self.assertTrue(self.engine.isTrackFxLoaded(0, 0))
        self.assertEqual(self.engine.getTrackFxName(0, 0), "Demo Filter FX")
        self.assertEqual(self.engine.getTrackFxFormatName(0, 0), "VST2")
        self.assertTrue(self.engine.hasTrackFxNativeEditor(0, 0))

        # Check FX parameters
        num_params = self.engine.getTrackFxNumParams(0, 0)
        self.assertEqual(num_params, 4)
        fx_params = self.engine.getTrackFxParams(0, 0)
        self.assertEqual(len(fx_params), 4)
        fx_names = [p["name"] for p in fx_params]
        self.assertIn("Cutoff", fx_names)
        self.assertIn("Resonance", fx_names)
        self.assertIn("Drive", fx_names)
        self.assertIn("Dry/Wet", fx_names)

        # Set FX parameter
        self.engine.setTrackFxParamValue(0, 0, 0, 0.55)
        val = self.engine.lib.myms_track_fx_get_param_value(0, 0, 0)
        self.assertAlmostEqual(val, 0.55, places=2)

        # Unload FX
        self.engine.unloadTrackFx(0, 0)
        self.assertFalse(self.engine.isTrackFxLoaded(0, 0))

    def test_04_project_format_v3_persistence(self):
        gui_synth_path = os.path.join(PROJECT_ROOT, "plugins", "gui_synth", "demo_gui_synth.so")
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")

        # Load synth and FX
        self.engine.loadTrackPlugin(0, gui_synth_path)
        self.engine.setTrackPluginParamValue(0, 0, 0.77)
        self.engine.loadTrackFx(0, 0, filter_fx_path)
        self.engine.setTrackFxParamValue(0, 0, 2, 0.66)

        with tempfile.NamedTemporaryFile(suffix=".myms", delete=False) as tmp:
            tmp_path = tmp.name

        try:
            self.engine.saveProject(tmp_path)
            self.assertTrue(os.path.exists(tmp_path))

            # Inspect JSON file
            with open(tmp_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.assertEqual(data.get("format_version"), 3)
            tracks = data.get("tracks", [])
            self.assertGreater(len(tracks), 0)
            t0 = tracks[0]
            self.assertEqual(t0.get("type"), 3)
            self.assertEqual(t0.get("plugin_path"), gui_synth_path)
            self.assertIn("fx_chain", t0)
            self.assertEqual(len(t0["fx_chain"]), 1)
            self.assertEqual(t0["fx_chain"][0].get("slot"), 0)
            self.assertEqual(t0["fx_chain"][0].get("path"), filter_fx_path)

            # Reset project and load back
            self.engine.newProject()
            self.assertFalse(self.engine.isTrackPluginLoaded(0))
            self.assertFalse(self.engine.isTrackFxLoaded(0, 0))

            self.engine.openProject(tmp_path)
            self.assertEqual(self.engine.getTrackType(0), 3)
            self.assertTrue(self.engine.isTrackPluginLoaded(0))
            self.assertEqual(self.engine.getTrackPluginName(0), "Demo GUI Synth")
            val_p0 = self.engine.lib.myms_plugin_get_param_value(0, 0)
            self.assertAlmostEqual(val_p0, 0.77, places=2)

            self.assertTrue(self.engine.isTrackFxLoaded(0, 0))
            self.assertEqual(self.engine.getTrackFxName(0, 0), "Demo Filter FX")
            val_fx2 = self.engine.lib.myms_track_fx_get_param_value(0, 0, 2)
            self.assertAlmostEqual(val_fx2, 0.66, places=2)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

    def test_05_load_vst3_plugin(self):
        vst3_path = os.path.join(PROJECT_ROOT, "plugins", "vst3", "demo_synth_vst3.so")
        self.assertTrue(os.path.exists(vst3_path), f"File not found: {vst3_path}")

        ok = self.engine.loadTrackPlugin(0, vst3_path)
        self.assertTrue(ok)
        self.assertTrue(self.engine.isTrackPluginLoaded(0))
        self.assertEqual(self.engine.getTrackPluginFormatName(0), "VST3")
        self.assertEqual(self.engine.getTrackPluginName(0), "Demo VST3 Synth")
        self.assertEqual(self.engine.getTrackPluginNumParams(0), 3)

        params = self.engine.getTrackPluginParams(0)
        self.assertEqual(len(params), 3)
        p_names = [p["name"] for p in params]
        self.assertIn("Cutoff", p_names)
        self.assertIn("Resonance", p_names)
        self.assertIn("Volume", p_names)

        # Note playback with VST3
        self.engine.noteOnInst(0, 60, 0.8)
        self.engine.noteOffInst(0, 60)

    def test_06_track_fx_slots_and_count(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertEqual(self.engine.getTrackFxCount(0), 0)
        self.assertEqual(self.engine.getMaxTrackFx(), 32)

        # Load slot 0 and slot 2
        self.engine.loadTrackFx(0, 0, filter_fx_path)
        self.engine.loadTrackFx(0, 2, filter_fx_path)
        self.assertEqual(self.engine.getTrackFxCount(0), 2)

        slots = self.engine.getTrackFxSlots(0)
        self.assertEqual(len(slots), 32)
        self.assertTrue(slots[0]["loaded"])
        self.assertFalse(slots[1]["loaded"])
        self.assertTrue(slots[2]["loaded"])
        self.assertFalse(slots[3]["loaded"])

        self.engine.unloadTrackFx(0, 0)
        self.engine.unloadTrackFx(0, 2)
        self.assertEqual(self.engine.getTrackFxCount(0), 0)

    def test_06b_unlimited_fx_chain_add_remove(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertEqual(self.engine.getTrackFxCount(0), 0)

        # Add 6 effects dynamically (exceeding old 4 FX limit)
        added_slots = []
        for i in range(6):
            slot = self.engine.addTrackFx(0, filter_fx_path)
            self.assertGreaterEqual(slot, 0)
            added_slots.append(slot)

        self.assertEqual(self.engine.getTrackFxCount(0), 6)
        fx_list = self.engine.getTrackFxList(0)
        self.assertEqual(len(fx_list), 6)
        for i, item in enumerate(fx_list):
            self.assertEqual(item["slot"], added_slots[i])
            self.assertEqual(item["name"], "Demo Filter FX")

        # Remove the second added effect
        removed_slot = added_slots[1]
        self.engine.removeTrackFx(0, removed_slot)
        self.assertEqual(self.engine.getTrackFxCount(0), 5)
        fx_list_after = self.engine.getTrackFxList(0)
        self.assertEqual(len(fx_list_after), 5)
        remaining_slots = [item["slot"] for item in fx_list_after]
        self.assertNotIn(removed_slot, remaining_slots)

        # Test FX parameter display
        disp = self.engine.getTrackFxParamDisplay(0, added_slots[0], 0)
        self.assertTrue(len(disp) > 0)

        # Clean up
        for slot in remaining_slots:
            self.engine.removeTrackFx(0, slot)
        self.assertEqual(self.engine.getTrackFxCount(0), 0)

    def test_07_vst_daw_parameter_controls_and_display(self):
        vst2_path = os.path.join(PROJECT_ROOT, "plugins", "vst2", "demo_synth_vst2.so")
        self.assertTrue(os.path.exists(vst2_path))
        self.engine.loadTrackPlugin(0, vst2_path)

        # Modulate parameter 0 (Cutoff) via DAW controls
        self.engine.setTrackPluginParamValue(0, 0, 0.75)
        val = self.engine.lib.myms_plugin_get_param_value(0, 0)
        self.assertAlmostEqual(val, 0.75, places=2)

        # Parameter display formatting
        disp = self.engine.getTrackPluginParamDisplay(0, 0)
        self.assertTrue(len(disp) > 0, "Parameter display string should not be empty")

        # Modulate VST3 parameter
        vst3_path = os.path.join(PROJECT_ROOT, "plugins", "vst3", "demo_synth_vst3.so")
        self.engine.loadTrackPlugin(0, vst3_path)
        self.engine.setTrackPluginParamValue(0, 1, 0.82)
        val_vst3 = self.engine.lib.myms_plugin_get_param_value(0, 1)
        self.assertAlmostEqual(val_vst3, 0.82, places=2)

    def test_08_soundfont_player_insert_fx_routing(self):
        # Configure Track 0 as SoundFont
        self.engine.setTrackType(0, 1) # TRACK_TYPE_SOUNDFONT
        self.assertEqual(self.engine.getTrackType(0), 1)

        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertTrue(os.path.exists(filter_fx_path))

        # Load FX on SoundFont track slot 0
        ok = self.engine.loadTrackFx(0, 0, filter_fx_path)
        self.assertTrue(ok)
        self.assertTrue(self.engine.isTrackFxLoaded(0, 0))
        self.assertEqual(self.engine.getTrackFxName(0, 0), "Demo Filter FX")

        # Modulate FX parameters
        self.engine.setTrackFxParamValue(0, 0, 0, 0.35)
        fx_val = self.engine.lib.myms_track_fx_get_param_value(0, 0, 0)
        self.assertAlmostEqual(fx_val, 0.35, places=2)

    def test_09_stop_notes_and_playback_silencing(self):
        # Trigger note while playback is stopped
        self.assertFalse(self.engine.isPlaying)
        self.engine.noteOnInst(0, 60, 0.9)

        # Silencing notes when playback is stopped / allNotesOff
        self.engine.allNotesOff()

        # Starting and stopping playback cleanly silences notes
        self.engine.play()
        self.assertTrue(self.engine.isPlaying)
        self.engine.stop()
        self.assertFalse(self.engine.isPlaying)

    def test_10_universal_track_renaming(self):
        # Test custom track naming on Track 0
        signal_received = []
        self.engine.instrumentNameChanged.connect(lambda inst, name: signal_received.append((inst, name)))

        self.engine.setInstrumentName(0, "Sub Heavy 808")
        self.assertEqual(self.engine.getInstrumentName(0), "Sub Heavy 808")
        self.assertIn((0, "Sub Heavy 808"), signal_received)

        # Test track renaming with SoundFont track type fallback
        self.engine.setTrackType(0, 1)
        self.engine.setInstrumentName(0, "") # empty name triggers smart default
        self.assertEqual(self.engine.getInstrumentName(0), "SoundFont 1")

        # Test track renaming with Drum track type fallback
        self.engine.setTrackType(0, 2)
        self.engine.setInstrumentName(0, "   ")
        self.assertEqual(self.engine.getInstrumentName(0), "BeatPad 1")

    def test_11_pre_instrument_fx_chain(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertTrue(os.path.exists(filter_fx_path))

        # Initially no pre-fx
        self.assertEqual(self.engine.getTrackPreFxCount(0), 0)
        self.assertFalse(self.engine.isTrackPreFxLoaded(0, 0))

        # Add pre-fx
        slot = self.engine.addTrackPreFx(0, filter_fx_path)
        self.assertEqual(slot, 0)
        self.assertTrue(self.engine.isTrackPreFxLoaded(0, 0))
        self.assertEqual(self.engine.getTrackPreFxCount(0), 1)
        self.assertEqual(self.engine.getTrackPreFxName(0, 0), "Demo Filter FX")
        self.assertEqual(self.engine.getTrackPreFxFormatName(0, 0), "VST2")

        # Check parameters
        num_params = self.engine.getTrackPreFxNumParams(0, 0)
        self.assertEqual(num_params, 4)
        params = self.engine.getTrackPreFxParams(0, 0)
        self.assertEqual(len(params), 4)

        # Modulate parameter
        self.engine.setTrackPreFxParamValue(0, 0, 0, 0.45)
        self.assertAlmostEqual(self.engine.getTrackPreFxParamValue(0, 0, 0), 0.45, places=2)

        # Remove pre-fx
        self.engine.removeTrackPreFx(0, 0)
        self.assertFalse(self.engine.isTrackPreFxLoaded(0, 0))
        self.assertEqual(self.engine.getTrackPreFxCount(0), 0)

    def test_12_master_track_fx_chain(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertTrue(os.path.exists(filter_fx_path))

        # Initially no master fx
        self.assertEqual(self.engine.getTrackFxCount(-1), 0)
        self.assertFalse(self.engine.isTrackFxLoaded(-1, 0))

        # Load master fx in slot 0
        ok = self.engine.loadTrackFx(-1, 0, filter_fx_path)
        self.assertTrue(ok)
        self.assertTrue(self.engine.isTrackFxLoaded(-1, 0))
        self.assertEqual(self.engine.getTrackFxCount(-1), 1)
        self.assertEqual(self.engine.getTrackFxName(-1, 0), "Demo Filter FX")

        # Modulate master fx parameter
        self.engine.setTrackFxParamValue(-1, 0, 1, 0.88)
        self.assertAlmostEqual(self.engine.getTrackFxParamValue(-1, 0, 1), 0.88, places=2)

        # Master fx list
        master_list = self.engine.getTrackFxList(-1)
        self.assertEqual(len(master_list), 1)
        self.assertEqual(master_list[0]["slot"], 0)
        self.assertEqual(master_list[0]["name"], "Demo Filter FX")

        # Remove master fx
        self.engine.removeTrackFx(-1, 0)
        self.assertFalse(self.engine.isTrackFxLoaded(-1, 0))
        self.assertEqual(self.engine.getTrackFxCount(-1), 0)

    def test_13_project_persistence_pre_fx_and_master_fx(self):
        filter_fx_path = os.path.join(PROJECT_ROOT, "plugins", "effects", "demo_filter_fx.so")
        self.assertTrue(os.path.exists(filter_fx_path))

        # Setup track 0 pre-fx and post-fx
        self.engine.loadTrackPreFx(0, 0, filter_fx_path)
        self.engine.setTrackPreFxParamValue(0, 0, 0, 0.33)

        self.engine.loadTrackFx(0, 0, filter_fx_path)
        self.engine.setTrackFxParamValue(0, 0, 0, 0.66)

        # Setup master fx
        self.engine.loadTrackFx(-1, 0, filter_fx_path)
        self.engine.setTrackFxParamValue(-1, 0, 0, 0.77)

        # Save project
        with tempfile.NamedTemporaryFile(suffix=".myms", delete=False) as f:
            tmp_path = f.name

        try:
            saved = self.engine.saveProject(tmp_path)
            self.assertTrue(saved)

            # Reset project
            self.engine.newProject()
            self.assertFalse(self.engine.isTrackPreFxLoaded(0, 0))
            self.assertFalse(self.engine.isTrackFxLoaded(0, 0))
            self.assertFalse(self.engine.isTrackFxLoaded(-1, 0))

            # Open project
            opened = self.engine.openProject(tmp_path)
            self.assertTrue(opened)

            # Verify track 0 pre-fx
            self.assertTrue(self.engine.isTrackPreFxLoaded(0, 0))
            self.assertEqual(self.engine.getTrackPreFxName(0, 0), "Demo Filter FX")
            self.assertAlmostEqual(self.engine.getTrackPreFxParamValue(0, 0, 0), 0.33, places=2)

            # Verify track 0 post-fx
            self.assertTrue(self.engine.isTrackFxLoaded(0, 0))
            self.assertEqual(self.engine.getTrackFxName(0, 0), "Demo Filter FX")
            self.assertAlmostEqual(self.engine.getTrackFxParamValue(0, 0, 0), 0.66, places=2)

            # Verify master fx
            self.assertTrue(self.engine.isTrackFxLoaded(-1, 0))
            self.assertEqual(self.engine.getTrackFxName(-1, 0), "Demo Filter FX")
            self.assertAlmostEqual(self.engine.getTrackFxParamValue(-1, 0, 0), 0.77, places=2)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

if __name__ == "__main__":
    unittest.main()
