#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Compile C++ Audio Engine if not built or if source files are newer
if [ ! -f "libmyms_audio.so" ] || [ "engine/src/audio_engine.cpp" -nt "libmyms_audio.so" ] || [ "engine/src/myms_c_api.cpp" -nt "libmyms_audio.so" ]; then
    echo "[myms] Compiling high-performance C++ audio engine..."
    g++ -O3 -shared -fPIC -Iengine/include -Iengine/src \
        engine/src/audio_engine.cpp engine/src/myms_c_api.cpp \
        -o libmyms_audio.so -lpthread -ldl -lm
    echo "[myms] Compilation complete: libmyms_audio.so"
fi

# Run myms with system Python 3 (PyQt6 + KDE Plasma integration)
exec /usr/bin/python3 main.py "$@"

