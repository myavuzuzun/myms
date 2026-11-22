#!/usr/bin/env bash
set -euo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

PKG_NAME="myms"
PKG_VERSION="1.0.0"
ARCH="$(dpkg --print-architecture)"
OUT_DIR="$DIR/dist"
DEB_FILE="$OUT_DIR/${PKG_NAME}_${PKG_VERSION}_${ARCH}.deb"

echo "[myms] Compiling C++ audio engine (libmyms_audio.so)..."
g++ -O3 -shared -fPIC -Iengine/include -Iengine/src \
    engine/src/audio_engine.cpp engine/src/myms_c_api.cpp \
    -o libmyms_audio.so -lpthread -ldl -lm

STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

# Stage application files
install -d "$STAGE_DIR/DEBIAN"
install -d "$STAGE_DIR/usr/lib/myms/qml"
install -d "$STAGE_DIR/usr/bin"
install -d "$STAGE_DIR/usr/share/applications"
install -d "$STAGE_DIR/usr/share/doc/myms"

install -m 0644 main.py myms_bridge.py "$STAGE_DIR/usr/lib/myms/"
install -m 0755 libmyms_audio.so "$STAGE_DIR/usr/lib/myms/"
install -m 0644 qml/*.qml "$STAGE_DIR/usr/lib/myms/qml/"
install -m 0644 myms.desktop "$STAGE_DIR/usr/share/applications/myms.desktop"
install -m 0644 README.md "$STAGE_DIR/usr/share/doc/myms/README.md"

# Launcher script
cat > "$STAGE_DIR/usr/bin/myms" <<'EOF'
#!/usr/bin/env bash
if [ -z "$QT_QPA_PLATFORM" ] && [ -n "$DISPLAY" ]; then
    export QT_QPA_PLATFORM=xcb
fi
exec /usr/bin/python3 /usr/lib/myms/main.py "$@"
EOF
chmod 0755 "$STAGE_DIR/usr/bin/myms"

# Debian control metadata
cat > "$STAGE_DIR/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}
Section: sound
Priority: optional
Architecture: ${ARCH}
Maintainer: myavuzuzun <myavuzuzun@myms.local>
Depends: python3, python3-pyqt6, python3-pyqt6.qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-window
Recommends: ffmpeg, fluid-soundfont-gm | timgm6mb-soundfont
Description: Lightweight Linux tracker and music creation studio
 myms is a real-time Linux tracker and DAW combining keyboard-driven
 pattern sequencing, a polyphonic subtractive synthesizer, SoundFont2
 playback, an 8-slot drum sampler, VST3/CLAP plugin hosting, and
 multi-format audio export.
EOF

mkdir -p "$OUT_DIR"
if dpkg-deb --help | grep -q -- '--root-owner-group'; then
    dpkg-deb --build --root-owner-group "$STAGE_DIR" "$DEB_FILE"
else
    dpkg-deb --build "$STAGE_DIR" "$DEB_FILE"
fi

echo "[myms] Built Debian package: $DEB_FILE"

