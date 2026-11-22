#include <iostream>
#include <cassert>
#include "plugin_host.h"

int main() {
    std::cout << "=== Testing myms PluginHost (VST3 + CLAP) ===" << std::endl;

    myms::PluginInstance host;

    // 1. Test VST3 Loading
    bool okVst3 = host.load("plugins/vst3/demo_synth_vst3.so");
    assert(okVst3 && "VST3 should load successfully");
    assert(host.getFormat() == myms::PLUGIN_FORMAT_VST3);
    assert(host.getName() == "Demo VST3 Synth");
    assert(host.getNumParams() == 3);
    std::cout << "✓ Loaded VST3: " << host.getName() << " with " << host.getNumParams() << " params" << std::endl;

    // Note on, render audio
    host.noteOn(60, 0.8f);
    float l = 0.0f, r = 0.0f;
    for (int i = 0; i < 512; ++i) {
        host.processSample(l, r);
    }
    std::cout << "✓ VST3 audio output rendered: L=" << l << ", R=" << r << std::endl;
    assert((l != 0.0f || r != 0.0f) && "VST3 should output non-zero audio on noteOn");

    // Test parameter manipulation
    host.setParamValue(0, 0.9f);
    assert(std::abs(host.getParamValue(0) - 0.9f) < 0.01f);
    std::cout << "✓ VST3 param 0 set to " << host.getParamValue(0) << std::endl;

    host.unload();
    assert(!host.isLoaded());
    std::cout << "✓ VST3 unloaded cleanly" << std::endl;

    // 2. Test CLAP Loading
    bool okClap = host.load("plugins/clap/demo_synth_clap.clap");
    assert(okClap && "CLAP should load successfully");
    assert(host.getFormat() == myms::PLUGIN_FORMAT_CLAP);
    assert(host.getName() == "DemoSynth CLAP");
    assert(host.getNumParams() == 4);
    std::cout << "✓ Loaded CLAP: " << host.getName() << " with " << host.getNumParams() << " params" << std::endl;

    // Note on, render audio
    host.noteOn(64, 0.9f);
    l = 0.0f; r = 0.0f;
    for (int i = 0; i < 512; ++i) {
        host.processSample(l, r);
    }
    std::cout << "✓ CLAP audio output rendered: L=" << l << ", R=" << r << std::endl;
    assert((l != 0.0f || r != 0.0f) && "CLAP should output non-zero audio on noteOn");

    host.setParamValue(1, 0.75f);
    assert(std::abs(host.getParamValue(1) - 0.75f) < 0.01f);
    std::cout << "✓ CLAP param 1 set to " << host.getParamValue(1) << std::endl;

    host.unload();
    assert(!host.isLoaded());
    std::cout << "✓ CLAP unloaded cleanly" << std::endl;

    std::cout << "\nALL PLUGIN HOST C++ UNIT TESTS PASSED PERFECTLY!\n";
    return 0;
}

