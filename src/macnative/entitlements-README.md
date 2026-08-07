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
provisioning profile), NOT by the Developer ID path above.

**NEVER put an XML comment in this file.** `plutil -lint` accepts comments, but
the AMFI parser `codesign` uses does not, and it fails mid-signing with
`Failed to parse entitlements: AMFIUnserializeXML: syntax error near line N`
— after it has already re-signed the nested dylibs, so the bundle is left
half-signed. Explanations go here in the README instead. (Learned twice.)

- `com.apple.application-identifier` — **required for App Store upload**, value
  `<TEAMID>.<bundle id>` (`MUYBB8YH5X.org.mihailod.chipmachine`).
- `com.apple.developer.team-identifier` — **required for App Store upload**,
  value `MUYBB8YH5X`.

  These two also appear inside the embedded provisioning profile, which makes
  them look redundant — they are not. Xcode injects them into the signature
  automatically; a manual `codesign --entitlements` run does **not**, and the
  upload is then rejected with:

      the signature for the bundle at "ChipMachine.app" is missing an
      application identifier but has an application identifier in the
      provisioning profile for the bundle                            (90886)

  They must match the profile exactly. If the bundle id or team ever changes,
  change them here too or signing and the profile disagree and the upload fails
  the same way. Verify with:

      codesign -d --entitlements - --xml ChipMachine.app | plutil -p -

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
- `com.apple.security.files.user-selected.read-only` — required so the sandboxed
  app can read files the user opens via Finder "Open With" / double-click / drag
  and drop. LaunchServices/Powerbox issues the per-file grant on open; this
  entitlement is what lets the process accept it. Read-only (not read-write): the
  app only ever plays user files, never writes them back.
- `com.apple.security.files.bookmarks.app-scope` — enables app-scoped
  security-scoped bookmarks. **The key is `app-scope`, NOT `app-scoped`.** This
  file carried the misspelled form until 2026-08-07 and it cost a rejected
  upload, but the validation failure was the *lesser* problem: `codesign`
  accepts unknown entitlement keys silently, so the entitlement was simply
  never granted and every security-scoped bookmark below failed at runtime
  under the sandbox. Nothing surfaced until Apple's validator said:

      Invalid Code Signing Entitlements ... key
      'com.apple.security.files.bookmarks.app-scoped' ... is not supported (90285)

  After changing it, test the behaviour it enables, not just the upload: open a
  local file, quit, relaunch, and confirm the file is still reachable.
  The per-file open grant above dies with the process,
  so a path the user saved to Favorites/a playlist that points at an external
  file would be unreachable next launch. `FileOpenHandler.mm`
  (`rememberOpenedFile` / `restoreSecurityScopedFiles`) persists a bookmark per
  opened file and re-acquires access at startup. That code is **identical in both
  variants** (no `#ifdef CM_MAS`); it self-gates at runtime on
  `APP_SANDBOX_CONTAINER_ID`, so it is a no-op in the non-sandboxed plus build
  and the only file-access divergence between plus and mas is this plist.
- `com.apple.security.cs.disable-library-validation` — **deliberately absent.**
  It is only a droppable safety net for the main executable (see above): every
  nested Mach-O, including `sunvox.dylib`, is re-signed under one Team ID, so
  library validation already passes. Omitting it is the stricter posture MAS
  wants.

**RESOLVED 2026-08-07 — the yt-dlp blocker is gone.** This section used to
record the bundled **yt-dlp** helper as an open blocker: a PyInstaller freeze
that both *required* `disable-library-validation` and spawned an executable,
violating App Store §2.5.2. It is now absent from the mas build in three
independent places, so `entitlements-helper.plist` below is never applied to a
mas bundle:

1. `package_app.sh` skips the helper entirely for `--mas` — `Contents/MacOS/`
   holds only the main executable and there is no `Resources/bin/ytdlp` tree.
2. `main.cpp` gates out both `initYoutube()` and the `cm_execute` Lua binding
   (`fork` + `execl("/bin/sh")`) under `#ifndef CM_MAS`, and the PATH setup that
   named the helper's directory. The literal string `ytdlp` is no longer in the
   binary.
3. `package_app.sh` strips the resolver hook from the bundled `lua/init.lua` and
   **fails the build** if any shipped Lua still matches
   `cm_execute|yt-dlp|os.execute|io.popen`.

The file-access sandbox entitlements are in place — see the two `files.*` keys
above.

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
