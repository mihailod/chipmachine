# FFmpeg — LGPLv3, decode-only build (App Store compliant)

These are the libav* shared libraries ChipMachinePlus bundles and links **in-process**
(via `FFMPEGPlugin`). They replace the Homebrew FFmpeg, which is a **GPL** build
(`--enable-gpl` + libx264/libx265) and therefore incompatible with Mac App Store terms.

## Provenance

- **Source:** FFmpeg 8.1.1 (https://ffmpeg.org/releases/ffmpeg-8.1.1.tar.xz)
- **License:** **LGPL version 2.1 or later** (`configure` reports
  `License: LGPL version 2.1 or later`; `CONFIG_GPL 0`, `CONFIG_NONFREE 0`).
  We deliberately do NOT pass `--enable-version3`, so the build stays LGPLv2.1 —
  the debate-free choice on Apple platforms (LGPLv3's anti-tivoization clause is
  why VLC-iOS et al. stay on v2.1). License texts: `COPYING.LGPLv2.1` (governing),
  `COPYING.LGPLv3` (kept for reference).
- **Sonames match Homebrew's** (libavcodec.62 / libavformat.62 / libavutil.60 /
  libswresample.6) so they are a drop-in ABI-compatible replacement.
- **Self-contained (no Homebrew ffmpeg needed):** the 8.1.1 headers are vendored in
  `include/` and the build points at both `include/` and `lib/` here (see the
  FFMPEG_LGPL block in `chipmachine/CMakeLists.txt` + the two plugin CMakeLists).
  `libavcodec/`, `libavformat/`, `libavutil/`, `libswresample/` are all present.
- Only non-system dependency is **openssl@3** (Apache-2.0, MAS-fine), used for HTTPS
  (radio / resolved stream URLs). The app already bundles libssl/libcrypto.

## configure line (LGPL, decode-only, no CLI)

```
./configure \
  --enable-shared --disable-static \
  --disable-programs --disable-doc --disable-debug \
  --disable-gpl --disable-nonfree \
  --disable-encoders --disable-muxers \
  --disable-avdevice --disable-avfilter --disable-swscale \
  --disable-libxcb --disable-xlib --disable-sdl2 --disable-vulkan \
  --enable-openssl \
  --enable-audiotoolbox --enable-videotoolbox \
  --enable-neon
```

- `--disable-gpl` (+ no libx264/libx265/nonfree) → LGPL, not GPL.
- `--disable-programs` → no `ffmpeg`/`ffplay`/`ffprobe` CLI at all (the old bundled
  51 MB `Contents/MacOS/ffmpeg` and its spawned-executable 2.5.2 problem are gone).
- `--disable-encoders --disable-muxers` → decode-only; we only demux+decode to PCM.
- `--disable-libxcb --disable-xlib` → no libX11 leak into the bundle (self-contained).

## Install names

Each dylib's id and its sibling libav references are set to `@executable_path/<name>`
so they resolve inside the app bundle. openssl refs are left as `/opt/homebrew/...`;
`package_app.sh`'s `discover_and_patch` bundles those and rewrites them to
`@executable_path` at package time.

## LGPL relinking (§6)

The libraries are dynamically linked and shipped as replaceable dylibs in
`Contents/MacOS/`, so a user can substitute their own compatible build — satisfying
the LGPL relink requirement.

## Rebuilding

See `scripts/build_lgpl_ffmpeg.sh` (or the configure line above). Match the FFmpeg
version to whatever the build headers are (keep sonames aligned).
