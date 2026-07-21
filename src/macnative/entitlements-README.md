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

## entitlements-app-mas.plist — the Mac App Store variant of the main executable

Used only when signing for MAS submission (App Store distribution cert +
provisioning profile), NOT by the Developer ID path above. Deliberately
comment-free for the same AMFI reason.

- `com.apple.security.app-sandbox` — required for MAS. This is the master switch;
  everything below only takes effect under it.
- `com.apple.security.network.client` — required. The app makes only *outbound*
  connections (song/screenshot downloads via libcurl, HTTP/FTP streaming, ffmpeg
  progressive streaming). Hardened Runtime alone did not gate these, but the
  sandbox does, so the client entitlement must be present.
- `com.apple.security.network.server` — **deliberately absent.** The app opens no
  listening socket. The telnet server (its only real listener) was removed and
  dead-stripped from the binary; a full `otool -Iv` audit confirms the only
  socket primitive actually called is `socketpair(AF_UNIX,...)` for UADE's local
  player IPC, which needs no server entitlement. Never add this key.
- `com.apple.security.cs.disable-library-validation` — **deliberately absent.**
  It is only a droppable safety net for the main executable (see above): every
  nested Mach-O, including `sunvox.dylib`, is re-signed under one Team ID, so
  library validation already passes. Omitting it is the stricter posture MAS
  wants.

Still-open, NON-network MAS work (out of scope of the network pass, tracked
separately): (1) file-access sandbox entitlements for user-opened files
(`com.apple.security.files.user-selected.read-only` and security-scoped bookmarks
for the double-click "Open With" path); (2) the bundled **yt-dlp** helper — a
PyInstaller freeze that both *requires* `disable-library-validation` (see below)
and spawns an executable, violating App Store §2.5.2. yt-dlp has no MAS-legal
form as-is; it must be replaced or removed before submission. That blocker is
unrelated to networking.

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
