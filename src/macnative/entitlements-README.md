# Hardened-Runtime entitlements for Developer ID signing

Used by `../../sign_and_notarize.sh`. Two files, deliberately comment-free —
macOS's kernel entitlement parser (AMFI) rejects XML comments with
`AMFIUnserializeXML: syntax error`, so all rationale lives here instead.

## entitlements-app.plist — the main executable (`Contents/MacOS/chipmachine`)

- `com.apple.security.cs.disable-library-validation` — the app `dlopen()`s a
  prebuilt third-party engine at runtime (`Contents/MacOS/sunvox.dylib`, loaded
  by `SunVoxPlugin.cpp`). When every nested Mach-O is re-signed under the *same*
  Developer ID, library validation is already satisfied and this key is not
  strictly required; it is kept as a safety net so a future drop-in of a
  vendor-signed `sunvox.dylib` (different Team ID) does not break launch. Drop
  it if you want the stricter posture.

This is a **Developer ID** (non-App-Store) build: no App Sandbox, no network
entitlement needed (Hardened Runtime does not gate outbound network).

## entitlements-helper.plist — the bundled yt-dlp helper (`Contents/Resources/bin/ytdlp/yt-dlp`)

- `com.apple.security.cs.disable-library-validation` — **required**. yt-dlp is a
  PyInstaller "onedir" freeze whose bootstrap `dlopen()`s an embedded Python
  framework and dozens of CPython `.so` modules. Verified: under
  `--options runtime` the helper fails to launch without this key
  (*"...different Team IDs"* library-validation error). Standard practice for a
  frozen-Python helper inside a notarized Developer ID app.

## No JIT entitlements

The app links **vanilla Lua** (`external/lua`), not LuaJIT, and uses no
`MAP_JIT`/`pthread_jit_write_protect` executable memory, so
`com.apple.security.cs.allow-jit` /
`com.apple.security.cs.allow-unsigned-executable-memory` are intentionally
absent.
