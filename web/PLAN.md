# ChipMachine → Web Browser: Porting Plan

Goal (from `PROMPT.md`): compile the existing C/C++ ChipMachine code to a **second
target** — a WebAssembly binary that runs the same multi-format player in the
browser, using **WebAudio** for output — while keeping the native macOS `cm` /
`chipmachine` binaries buildable from the *same* source tree.

This document is the required "detailed plan" deliverable. It is grounded in the
actual code layout (see the "What the code already gives us" section), so the
estimates and blockers are real, not generic.

---

## 1. Executive summary

The good news: **this codebase was originally architected (by the upstream
`apone` framework) with an Emscripten target in mind.** The scaffolding is
partly still there and just bit-rotted. Specifically:

- The audio layer (`AudioPlayer`) is a **pull callback** `void(int16_t*, int)` —
  the exact shape WebAudio wants. `external/apone/mods/audioplayer/CMakeLists.txt`
  already has an `elseif(EMSCRIPTEN)` branch.
- The network layer (`webutils`) already has an `#ifdef EMSCRIPTEN` path using
  `emscripten_async_wget` instead of libcurl.
- The GUI layer (`grappix`) already has an `if(EMSCRIPTEN)` branch and the
  Emscripten toolchain sets `-s USE_GLFW=3` (WebGL).
- There is an `external/apone/cmake/Emscripten.cmake` toolchain file.

So the strategy is **not** a rewrite — it is: (a) resurrect and modernize the
Emscripten build path, (b) tackle the handful of genuine browser blockers
(threads, subprocesses, dlopen, database size), and (c) do it **incrementally**,
starting from a single-plugin proof of concept rather than the full 60-plugin
monster.

**Recommended end-state architecture:** the C++ core (decoder plugins + player
engine) compiles to **one `.wasm` module** exposing a small C API; a **thin
JavaScript/HTML front end** drives it and owns the UI + WebAudio graph. We do
*not* try to port the grappix OpenGL GUI first — the textmode-only `cm`
configuration (`-DTEXTMODE_ONLY`) is the far smaller, cleaner thing to bring up
in WASM, with the browser DOM providing the real UI.

---

## 2. What the code already gives us (survey results)

| Concern | Where | Web-readiness |
|---|---|---|
| Audio output | `apone/mods/audioplayer/audioplayer.{h,cpp}` — pull callback `std::function<void(int16_t*,int)>` | **Excellent.** Callback model = WebAudio. CMake already has EMSCRIPTEN branch (stub). |
| Player engine | `src/MusicPlayerList.cpp`, `src/MusicPlayer.cpp` | Uses `std::thread` for the play loop (`MusicPlayerList.cpp:77`) — needs pthreads or restructuring. |
| Decoder plugins | `external/musicplayer/src/plugins/*` (60+), registered in `src/plugin_register.cpp`; array in root `CMakeLists.txt` (`MUSICPLAYER_PLUGINS`) | Most are pure DSP C/C++ → compile to WASM. A few have blockers (below). |
| File/network fetch | `src/RemoteLoader.cpp` → `apone/mods/webutils` | **Good.** `webgetter.cpp` already has `#ifdef EMSCRIPTEN` (`emscripten_async_wget`). |
| Song database | `src/MusicDatabase.cpp` — SQLite `music.db` + song-list files + Lua config (`~770K` songs) | **Biggest product problem.** Cannot ship locally; needs a hosted query service or a slimmed/remote index. |
| GUI | `src/ChipMachine*.cpp` + `apone/mods/grappix` (OpenGL/GLFW/FreeType) | Portable via `-s USE_GLFW=3` but heavy. **Defer** — use DOM UI instead. |
| Textmode UI | `src/textmode.cpp`, `TelnetInterface.cpp` (`cm` binary, `-DTEXTMODE_ONLY`) | The lean path; core-only, no GL. Good bring-up target. |
| Config/scripting | `external/lua` + `sol2` | Compiles to WASM fine (pure C). |

### Genuine browser blockers found

1. **Threads** — `MusicPlayerList` runs a dedicated `playerThread`. Options:
   pthreads-in-WASM (needs `SharedArrayBuffer` → COOP/COEP headers +
   cross-origin isolation), or restructure so decoding happens synchronously
   inside the WebAudio pull callback (cleaner for the browser). See §5.
2. **Subprocesses** — `FFMPEGPlugin.cpp` and `src/youtube.cpp` shell out to a
   bundled `ffmpeg`/`yt-dlp` binary (`bin/ffmpeg`). **No `fork/exec` in the
   browser.** Streaming radio, YouTube, and generic AAC/OGG via subprocess must
   be dropped or replaced (browser can decode mp3/aac/ogg natively via
   `<audio>`/WebCodecs, or link libav* directly into WASM — heavy).
3. **`dlopen`** — `SunVoxPlugin.cpp` `dlopen()`s a prebuilt proprietary
   `.dylib`/`.so` blob at runtime. WASM has no dlopen for native code. SunVox
   must be **dropped** from the web build (no source).
4. **Database size** — 770K-song index + SQLite screenshot DB can't live in the
   browser. Needs a server-side search API or a heavily reduced static index.
5. **Binary size** — all 60+ plugins (VICE, UADE, vgmstream, zxtune, …) in one
   `.wasm` would be tens of MB. Mitigate with an incremental plugin allow-list
   and/or lazy-loaded plugin modules (§6).

---

## 3. Strategy decision: port-the-C++ vs. glue-existing-JS

`PROMPT.md` asks us to weigh compiling our own code vs. reusing chiptune2.js /
WebSID. Recommendation:

- **Primary: compile our own C++ to WASM.** It is the only path that preserves
  the project's whole value — *one engine, 350+ formats, unified search*.
  chiptune2.js only covers `.mod/.it/.s3m/.xm` (we already have OpenMPT), and
  WebSID only covers SID (we already have VICE). Bolting on foreign JS players
  would fork the format-routing logic and defeat the "same capable player" goal.
- **Use the JS libs only as a schedule de-risker / fallback.** A 1–2 day spike
  wiring chiptune2.js to our DOM shell is worthwhile *early* to validate the
  WebAudio + UI + fetch plumbing against a known-good decoder while the WASM
  build is still being brought up. It becomes throwaway scaffolding once
  OpenMPT-in-WASM works. Do **not** invest in it as the product.

---

## 4. Target architecture (recommended)

```
 ┌───────────────────────── Browser tab ─────────────────────────┐
 │  index.html + app.js  (UI: search box, list, transport)       │
 │        │                         ▲                             │
 │        │ calls (cm_play, …)      │ callbacks (onMeta, results) │
 │        ▼                         │                             │
 │  ┌──────────────── chipmachine.wasm ─────────────────┐        │
 │  │  C API shim (extern "C")                           │        │
 │  │   ├─ MusicPlayerList / MusicPlayer  (engine)       │        │
 │  │   ├─ plugin_register → OpenMPT, GME, SID(VICE),…   │        │
 │  │   ├─ RemoteLoader → emscripten fetch               │        │
 │  │   └─ render(int16_t*, N)  ← pulled by audio thread │        │
 │  └───────────────────────────────────────────────────┘        │
 │        │ PCM frames                                            │
 │        ▼                                                       │
 │  AudioWorkletNode  ──►  WebAudio destination (speakers)        │
 │                                                               │
 │  Search/metadata  ──► (fetch) ──►  hosted DB/query API         │
 │  Song files       ──► (fetch) ──►  existing modland/HVSC/… CDNs│
 └───────────────────────────────────────────────────────────────┘
```

Key points:
- **Audio**: implement a new `player_web` backend (or reuse `player_sdl` via
  `-s USE_SDL`) whose "device" is an **AudioWorklet** that calls back into the
  same `int16_t*` fill function. Prefer AudioWorklet over the deprecated
  ScriptProcessorNode.
- **Front end owns the UI.** Search results, now-playing, seek, subsong,
  favorites — all DOM. The `ChipInterface` facade (`src/ChipInterface.h`) is
  already the clean seam: expose *its* methods (`createQuery`, `play`,
  `nextSong`, `setTune`, `pause`, `onMeta`, `seconds`) over the C API.
- **Two build outputs, one tree**: add an `EMSCRIPTEN`/`WEB` branch to the root
  `CMakeLists.txt` that (a) selects the web audio backend, (b) excludes GUI +
  blocker plugins, (c) builds a `chipmachine_web` target instead of the
  `chipmachine`/`cm` executables. Native builds are untouched.

---

## 5. Threading decision

Two viable models — pick based on how much engine surgery we want:

- **Option A — pthreads + cross-origin isolation (least code change).** Build
  with `-pthread -s USE_PTHREADS`, serve with `COOP: same-origin` +
  `COEP: require-corp` headers so `SharedArrayBuffer` is available. The existing
  `playerThread` model then works largely as-is. Cost: hosting constraints
  (cross-origin isolation breaks some third-party embeds/CDN fetches; song CDNs
  must send CORP/CORS headers or be proxied).
- **Option B — single-threaded pull (most robust in the browser).** Remove the
  dedicated play thread for the web build; decode synchronously inside the
  AudioWorklet's render callback and marshal control/metadata over messages.
  More faithful to how browser audio wants to work, avoids SAB header
  requirements, but requires refactoring `MusicPlayerList` so its state pump is
  callable without owning a thread.

**Recommendation: start with Option A** to get sound out fast, then migrate hot
paths to Option B if cross-origin-isolation hosting proves painful. Decouple
this decision behind the `ChipInterface`/`AudioPlayer` seam so the front end
doesn't care.

---

## 6. Plugin roadmap (incremental — do NOT build all 60 first)

Bring plugins up in waves; each wave is a shippable increment.

- **Wave 0 (proof of life):** `openmptplugin` only (`.mod/.xm/.it/.s3m`). Pure
  C++, no deps, validates the whole WASM+WebAudio+fetch+UI pipeline. Cross-check
  against chiptune2.js output.
- **Wave 1 (chip staples):** `gmeplugin` (NES/SPC/GB/…), `sidplugin` **or**
  `vicepluginbridge` (C64 SID), `hivelyplugin` (AHX/HVL), `sc68plugin`,
  `stsoundplugin`, `adplugin`. Mostly self-contained emulators.
- **Wave 2 (PSF/console family):** `htplugin`, `usfplugin`, `ndsplugin`,
  `gsfplugin`, `aoplugin` (which now owns PS1/PS2 `.psf*` too, since `heplugin`
  was deleted along with the Sony BIOS it needed) + the `psf` support lib.
- **Wave 3 (large/complex):** `uadeplugin` (verify the in-process m68k path has
  no fork/exec — `uade/src/frontends/*` reference `posix_spawn`; may need the
  library-mode entry points only), `zxtuneplugin`, `vgmstreamplugin`,
  `famitrackerplugin`, and the many small trackers.
- **Dropped on web (initially):** `sunvoxplugin` (dlopen blob),
  `ffmpegplugin` + `youtube` (subprocess), `mp3plugin`/streaming radio via
  subprocess. Reintroduce mp3/aac/ogg later either via linked-in libav* or by
  handing those URLs to a native browser `<audio>` element outside the WASM.
- **Size control:** consider Emscripten `dylink`/side-modules so waves 2–3 load
  lazily on first use of a matching extension, keeping the initial `.wasm` small.

Note the existing content-based format routing (`SongFileIdentifier.cpp`,
`canHandle`) must remain the arbiter — the browser build just registers a
subset of plugins; routing logic is unchanged.

---

## 7. The database problem (needs a product decision — see questions)

The native app indexes ~770K songs into a local SQLite `music.db` and reads
song-list files (`MusicDatabase.cpp`), fetching actual song files on demand from
the original collection hosts (modland, HVSC, …) via `RemoteLoader`. In the
browser:

- **Song files**: fine to keep fetching from the same remote hosts *if* those
  hosts send CORS headers; otherwise route through a small proxy. This is
  already how native works (RemoteLoader + webutils), so minimal change.
- **Search index**: the 770K-entry index + incremental autocomplete
  (`SearchIndex.cpp`) is too big to ship to every tab. Options, in order of
  recommendation:
  1. **Server-side search API** — host the index; the browser sends query
     prefixes, gets back result pages. Preserves full library + instant search.
  2. **Reduced static index** — ship a curated subset (e.g. one collection) as a
     downloadable index file for a fully static, backend-free demo.
  3. **Full client index** — only viable for a small curated corpus; not the
     770K library.

---

## 8. Step-by-step execution plan

**Phase 0 — Tooling & CI (0.5–1 wk)**
- Install/pin the Emscripten SDK; modernize `apone/cmake/Emscripten.cmake`
  (it parses a legacy `~/.emscripten` file — replace with `emcmake`/the current
  `Emscripten.cmake` toolchain).
- Add a `web/` build script (`emcmake cmake … && emmake ninja`) and a static
  dev server that sends COOP/COEP headers.

**Phase 1 — Bootstrap PoC with existing JS (2–3 days, throwaway)**
- Static `web/index.html` + `app.js`: search box → list → play, wired to
  chiptune2.js + WebAudio. Proves UI/UX and audio plumbing independent of our
  WASM build. De-risks everything downstream.

**Phase 2 — WASM audio spine (1–2 wk)**
- New `player_web` audio backend (AudioWorklet) behind `AudioPlayer`; wire the
  EMSCRIPTEN CMake branch to select it.
- Add a `web` build configuration to the root `CMakeLists.txt`: build a
  `chipmachine_web` target = MAIN_FILES (no GUI_FILES) + textmode core +
  **only `openmptplugin`**, with a small `extern "C"` shim + `--bind`/embind or
  raw exported functions.
- Get a single `.mod` decoding through our engine → AudioWorklet → speakers.
  **This is the milestone that proves the PROMPT's core feasibility.**

**Phase 3 — Loading & metadata (1 wk)**
- Wire `RemoteLoader`/`webutils` emscripten fetch path; fetch a real song file
  from a remote host (or bundled via `--preload-file` for the demo).
- Surface `ChipInterface` metadata callbacks (`onMeta`, `seconds`, subsong) to
  JS; build the real transport UI (play/pause/seek/next/subsong).

**Phase 4 — Search (1–2 wk, depends on §7 decision)**
- Stand up the chosen search backend (hosted API vs. reduced static index) and
  wire incremental autocomplete in the DOM to it.

**Phase 5 — Plugin waves (ongoing)**
- Add Wave 1 → 2 → 3 plugins per §6, verifying size and correctness each wave.
  Adopt side-module lazy loading once binary size warrants it.

**Phase 6 — Hardening & polish**
- Resolve threading model (§5), COOP/COEP hosting, mobile Safari audio-unlock
  (user-gesture requirement), error/`Skip` handling for declined formats,
  favorites/persistence via `localStorage`/IndexedDB.

---

## 9. Risks / open questions

- **Binary size** vs. format coverage — the central tension; mitigated by
  incremental waves + side modules, but the full 350-format build may never be a
  single small download.
- **Cross-origin isolation** (if Option A threads) restricts hosting and
  third-party song-CDN fetches (may force a proxy).
- **Song-host CORS** — some collection mirrors may not send CORS headers.
- **Subprocess-dependent features** (YouTube, streaming radio, generic
  ffmpeg-decoded formats) are out of scope for v1.
- **UADE / any plugin with `fork`/`posix_spawn`** in its non-library frontends
  must be built in strict library mode or deferred.
- **Licensing** — the combined work is GPLv3 (VICE/SC68/UADE); a hosted web
  build must continue to honor that. The one NonCommercial component
  (`webixs`/Ixalance) should be excluded from any commercial hosting.

---

## 10. Immediate next actions

1. Confirm the two product decisions in §7/§9 (search backend; which features
   are acceptable to drop for v1).
2. Stand up the Emscripten SDK + modernized toolchain (Phase 0).
3. Execute Phase 2 Wave-0 PoC (OpenMPT-only `.mod` → WebAudio) — the smallest
   end-to-end proof that the existing C++ compiles and plays in the browser.
