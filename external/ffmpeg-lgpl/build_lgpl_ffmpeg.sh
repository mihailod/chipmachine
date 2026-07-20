#!/bin/bash
# Reproducible LGPLv3, decode-only, no-CLI FFmpeg build for ChipMachineAS (arm64 macOS).
# Produces libavcodec.62 / libavformat.62 / libavutil.60 / libswresample.6 dylibs that
# drop into this directory's lib/. See README.md for rationale (App Store / GPL).
#
# Usage: ./build_lgpl_ffmpeg.sh [workdir]   (default workdir: ./_build)
set -euo pipefail

FFVER="8.1.1"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="${1:-$HERE/_build}"
PREFIX="$WORK/install"
SRC="$WORK/ffmpeg-$FFVER"

export PKG_CONFIG_PATH="/opt/homebrew/opt/openssl@3/lib/pkgconfig"
mkdir -p "$WORK"; cd "$WORK"

[ -f "ffmpeg-$FFVER.tar.xz" ] || curl -L --fail -o "ffmpeg-$FFVER.tar.xz" \
  "https://ffmpeg.org/releases/ffmpeg-$FFVER.tar.xz"
[ -d "$SRC" ] || tar xf "ffmpeg-$FFVER.tar.xz"
cd "$SRC"

./configure \
  --prefix="$PREFIX" \
  --enable-shared --disable-static \
  --disable-programs --disable-doc --disable-debug \
  --disable-gpl --disable-nonfree \
  --disable-encoders --disable-muxers \
  --disable-avdevice --disable-avfilter --disable-swscale \
  --disable-libxcb --disable-xlib --disable-sdl2 --disable-vulkan \
  --enable-openssl \
  --enable-audiotoolbox --enable-videotoolbox \
  --enable-neon \
  --extra-cflags="-I/opt/homebrew/opt/openssl@3/include" \
  --extra-ldflags="-L/opt/homebrew/opt/openssl@3/lib"

grep -q "license='LGPL" ffbuild/config.log || { echo "NOT LGPL — aborting"; exit 1; }
make clean >/dev/null 2>&1 || true
make -j"$(sysctl -n hw.ncpu)"
make install

# Vendor headers (self-contained build; no Homebrew ffmpeg needed).
rm -rf "$HERE/include"; mkdir -p "$HERE/include"
cp -R "$PREFIX/include/." "$HERE/include/"

# Vendor: copy deref'd major-soname dylibs into ../lib and rewrite ids/sibling refs
# to @executable_path (openssl refs left for package_app.sh to bundle).
DEST="$HERE/lib"; mkdir -p "$DEST"
for m in libavcodec.62 libavformat.62 libavutil.60 libswresample.6; do
  cp -L "$PREFIX/lib/$m.dylib" "$DEST/$m.dylib"; chmod +w "$DEST/$m.dylib"
done
for f in "$DEST"/*.dylib; do
  install_name_tool -id "@executable_path/$(basename "$f")" "$f"
  otool -L "$f" | awk '{print $1}' | grep "$PREFIX/lib/" | while read -r r; do
    install_name_tool -change "$r" "@executable_path/$(basename "$r")" "$f"
  done
done
# Unversioned symlinks so the linker's -lavcodec/... resolve here.
( cd "$DEST"
  ln -sf libavcodec.62.dylib libavcodec.dylib
  ln -sf libavformat.62.dylib libavformat.dylib
  ln -sf libavutil.60.dylib libavutil.dylib
  ln -sf libswresample.6.dylib libswresample.dylib )
# Re-sign ad-hoc: install_name_tool invalidated the linker's signature, and arm64
# dyld refuses to load an unsigned/altered dylib when copied next to a dev binary.
codesign -f -s - "$DEST"/libavcodec.62.dylib "$DEST"/libavformat.62.dylib \
                 "$DEST"/libavutil.60.dylib "$DEST"/libswresample.6.dylib
echo "Vendored LGPL headers -> $HERE/include ; dylibs -> $DEST"
