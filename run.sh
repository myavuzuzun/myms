#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Compile C++ Audio Engine if not built or if source files are newer
if [ ! -f "libmyms_audio.so" ] || [ "engine/src/audio_engine.cpp" -nt "libmyms_audio.so" ] || [ "engine/src/myms_c_api.cpp" -nt "libmyms_audio.so" ] || [ "engine/src/plugin_host.h" -nt "libmyms_audio.so" ] || [ "engine/src/audio_engine.h" -nt "libmyms_audio.so" ]; then
    echo "[myms] Compiling high-performance C++ audio engine..."
    g++ -O3 -shared -fPIC -Iengine/include -Iengine/src \
        engine/src/audio_engine.cpp engine/src/myms_c_api.cpp \
        -o libmyms_audio.so -lpthread -ldl -lm
    echo "[myms] Compilation complete: libmyms_audio.so"
fi

# Compile demo plugins if not built
if [ ! -f "plugins/vst2/demo_synth_vst2.so" ]; then
    mkdir -p plugins/vst2
    g++ -O3 -shared -fPIC -Iengine/include plugins/vst2/demo_synth_vst2.cpp -o plugins/vst2/demo_synth_vst2.so -lm
fi
if [ ! -f "plugins/clap/demo_synth_clap.clap" ]; then
    mkdir -p plugins/clap
    g++ -O3 -shared -fPIC -Iengine/include plugins/clap/demo_synth_clap.cpp -o plugins/clap/demo_synth_clap.clap -lm
fi
if [ ! -f "plugins/vst3/demo_synth_vst3.so" ]; then
    mkdir -p plugins/vst3
    g++ -O3 -shared -fPIC -Iengine/include plugins/vst3/demo_synth_vst3.cpp -o plugins/vst3/demo_synth_vst3.so -lm
fi
if [ ! -f "plugins/gui_synth/demo_gui_synth.so" ]; then
    mkdir -p plugins/gui_synth
    g++ -O3 -shared -fPIC -Iengine/include \
        -I/home/yavuz/.local/share/mamba/envs/ros_env/include \
        -L/home/yavuz/.local/share/mamba/envs/ros_env/lib \
        plugins/gui_synth/demo_gui_synth.cpp \
        -o plugins/gui_synth/demo_gui_synth.so -lX11 -lpthread -lm
fi
if [ ! -f "plugins/effects/demo_filter_fx.so" ]; then
    mkdir -p plugins/effects
    g++ -O3 -shared -fPIC -Iengine/include \
        -I/home/yavuz/.local/share/mamba/envs/ros_env/include \
        -L/home/yavuz/.local/share/mamba/envs/ros_env/lib \
        plugins/effects/demo_filter_fx.cpp \
        -o plugins/effects/demo_filter_fx.so -lX11 -lpthread -lm
fi

# Ensure xcb platform for native X11 plugin GUI window support
if [ -z "$QT_QPA_PLATFORM" ] && [ -n "$DISPLAY" ]; then
    export QT_QPA_PLATFORM=xcb
fi

# Run myms with system Python 3 (PyQt6 + KDE Plasma integration)
exec /usr/bin/python3 main.py "$@"

