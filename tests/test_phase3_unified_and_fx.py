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

if __name__ == "__main__":
    unittest.main()
