#include <iostream>
#include <cassert>
#include "plugin_host.h"

int main() {
    std::cout << "=== Testing myms PluginHost (VST2 + CLAP) ===" << std::endl;

    myms::PluginInstance host;

    // 1. Test VST2 Loading
    bool okVst2 = host.load("plugins/vst2/demo_synth_vst2.so");
    assert(okVst2 && "VST2 should load successfully");
    assert(host.getFormat() == myms::PLUGIN_FORMAT_VST2);
    assert(host.getName() == "DemoSynth VST2");
    assert(host.getNumParams() == 4);
    std::cout << "✓ Loaded VST2: " << host.getName() << " with " << host.getNumParams() << " params" << std::endl;

    // Note on, render audio
    host.noteOn(60, 0.8f);
    float l = 0.0f, r = 0.0f;
    for (int i = 0; i < 512; ++i) {
        host.processSample(l, r);
    }
    std::cout << "✓ VST2 audio output rendered: L=" << l << ", R=" << r << std::endl;
    assert((l != 0.0f || r != 0.0f) && "VST2 should output non-zero audio on noteOn");

    // Test parameter manipulation
    host.setParamValue(0, 0.9f);
    assert(std::abs(host.getParamValue(0) - 0.9f) < 0.01f);
    std::cout << "✓ VST2 param 0 set to " << host.getParamValue(0) << std::endl;

    host.unload();
    assert(!host.isLoaded());
    std::cout << "✓ VST2 unloaded cleanly" << std::endl;

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

