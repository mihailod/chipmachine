**ChipMachineAS**

<div align="right">
  <img src="https://img.shields.io/github/downloads/mihailod/chipmachine/total?label=Total%20Downloads" alt="Total Downloads">
</div>

**Port of ChipMachine for Apple Silicon**

But this is far more than a simple port!

While ensuring the player runs on modern Apple hardware, my passion for it has expanded its compatibility and scale:

* [60+ plugins](external/musicplayer/src/plugins/README.md) supporting [350+ music formats](data/misc/formats_descriptions.txt)
* [~770,000](data) indexed songs (~100,000 annotaded with screenshots) and counting

**The mission statement: support every single format and index all retro/chip music databases.**

Despite the massive expansion under the hood, the core experience remains untouched: instant, incremental autocomplete search across the entire global library, delivering instant zero-latency (for cached songs) playback from a single, simple, unified interface.

**(Screenshots might show features from dev in progress (not released) code.)**
[![Screenshot](data-notbundled/misc/screenshots/1playback.png)](https://youtu.be/WsNhwxY1c08)
[![Screenshot](data-notbundled/misc/screenshots/2platforms.png)](https://youtu.be/WsNhwxY1c08)
[![Screenshot](data-notbundled/misc/screenshots/3formats.png)](https://youtu.be/WsNhwxY1c08)
[![Screenshot](data-notbundled/misc/screenshots/4databases.png)](https://youtu.be/WsNhwxY1c08)
[![Screenshot](data-notbundled/misc/screenshots/5search.png)](https://youtu.be/WsNhwxY1c08)
[![Screenshot](data-notbundled/misc/screenshots/6plugins.png)](https://youtu.be/WsNhwxY1c08)

## Intro

*A demoscene/retro Jukebox/spotify-like music player*

* **Simply start typing for incremental auto-complete search of the aggregated database**
* **UP/DOWN keys = select a song from search results**
* **ENTER key = play**
* **TAB key = help screen**
* **(Read the scrolling text for more info)**
* **[Things in progress / to come](data-notbundled/misc/TOODOO.txt)**
* **Ultimate goal: every chiptune ever made searchable and instantly playable!** 

## Binaries

Binaries for macOS (tested on Tahoe) are available under [*Releases*](https://github.com/mihailod/chipmachine/releases)

**NOTE although not formally tested, ChipMachineAS should work on pre-Tahoe macOS**

### Running on Mac (Gatekeeper Authorization)

For now, the app is distributed with an ad-hoc code signature and macOS Gatekeeper will block it (official Mac App Store release coming soon).

This is standard behavior for open-source binaries distributed outside the official Mac App Store ecosystem.

To authorize and run the application on your Mac, follow these steps:

1. Download the latest release and unzip it in the Applications folder
2. Double-click `ChipMachineAS.app`.
3. macOS will display a prompt stating the app cannot be opened because the developer cannot be verified.
4. Click **Done** or **Cancel**.
5. Open your Mac's **System Settings**.
6. Navigate to **Privacy & Security** in the left sidebar
7. Scroll down to the **Security** section.
8. Look for the notification stating: `“ChipMachineAS” was blocked from use because it is not from an identified developer.`
9. Click the **Open Anyway** button.
10. Authenticate using your Mac's admin password or Touch ID.
11. Double-click `ChipMachineAS.app` again.
12. The final confirmation prompt will appear.
13. Click **Open**.

*Note: You only need to perform this authorization once per release. Subsequent launches will boot instantly.*

### Opening local music files

Once installed, ChipMachineAS registers with macOS as a player for the hundreds of formats it supports, and you can open a local song three ways:

* **Right-click a file → Open With → ChipMachineAS** (or double-click a file you've made it the default for)
* **Drag and drop a file onto the ChipMachineAS icon** (Dock or Finder)
* **Drag and drop a file straight into the running ChipMachineAS window**

All three play the track immediately.

This is deliberately polite — ChipMachineAS advertises itself as an *alternate* handler and never hijacks files from a player you already use:

* For common audio types (`.mp3`, `.wav`, `.flac`, …) it appears as an option in **Open With** but never becomes the default unless you explicitly choose it via **Get Info → Open with → Change All**.
* For obscure chip/tracker formats that nothing else on your Mac opens, it becomes the de-facto player and shows its own document icon in Finder.

## Prerequisites for development (Tested on macOS 26 / Tahoe only)

* Make sure you have Homebrew installed (Apple Silicon homebrew in /opt/homebrew/ , make sure you are not using Intel legacy /usr/local tools)
* brew install git cmake ninja freetype glew glfw3 lua fftw mpg123 python ffmpeg boost
* (if some packages are reported missing later install then via brew and let me know -- I missed them in the line above!)

## Building for Apple Silicon

All third-party dependencies are **vendored inside the repo** under [external/](external/), so a single clone is all you need. Provenance (each vendored fork's upstream URL and commit) is recorded in [external/VENDORED.md](external/VENDORED.md).

```bash
git clone https://github.com/mihailod/chipmachine.git
mkdir build && cd build
cmake ../chipmachine -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
```

#### Build variants: Plus vs Mac App Store

The default `build/` above produces the full **ChipMachinePlus** variant — everything included (YouTube, self-update), distributed via GitHub with a Developer ID signature. A second **Mac App Store** variant — **ChipMachine** — is built from its own directory by passing `-DCM_VARIANT=mas` (from the workspace root, i.e. the parent of `chipmachine/`):

```bash
cmake -S chipmachine -B build-mas -GNinja -DCMAKE_BUILD_TYPE=Release -DCM_VARIANT=mas
ninja -C build-mas
```

The two are **independent build trees** (each has its own objects and binary) sharing one source — pass `-GNinja` for `build-mas` too, or `ninja` there will have no `build.ninja`. The `mas` variant compiles out everything the App Store disallows or that has no in-sandbox form:

* the **YouTube** plugin and its ~32k catalog rows (yt-dlp is a spawned executable — App Store §2.5.2),
* the bundled **yt-dlp** helper,
* **UADE**, **VICE**, **GoatTracker v2**, **Furnace** (the multi-system DefleMask `.dmf` engine — its SEGA Genesis and Master System modules are reclaimed by the clean-room `dmfcr` plugin, the rest stay hidden), **mdxmini** (the Sharp X68000 `.mdx` player), **SC68**, **ASAP**/PokeyNoise, **AOSDK**, **Highly Theoretical**, **vio2sf**, **USF**, **Ayfly** and **ZXTune**, together with the `data/uade`, `data/c64` and `data/sc68` payloads that go with them — dropping Furnace is also what clears **reSIDfp** from the build, and dropping VICE is what removes Commodore's copyrighted KERNAL/BASIC/chargen ROM images (Compute!'s Sidplayer `.mus`/`.str` survives regardless, via the **ChipMachine Clean Room SIDPlayer** plugin),
* the **GitHub self-update** check (updates come through the App Store).

Why each of those is excluded — the exact terms, what it costs, and whether a replacement exists — is documented per component in [`LEGAL-PLUS`](./LEGAL-PLUS). The build gates themselves are `CM_HAVE_*` in [CMakeLists.txt](CMakeLists.txt).

Dropping those engines costs formats, so the catalog hides what the build cannot play (`songHasNoPlayer()` / `songFormatHasNoPlayer()`) — you never see a song that will not start. Where an extension covers several unrelated formats, only the affected one is hidden, because the gate keys on the *format name* rather than the extension. Which formats each gate costs, and what (if anything) replaces them, is documented in the [per-plugin READMEs](external/musicplayer/src/plugins/README.md).

The C64 formats are **almost** unaffected: the ~6.5k Compute!'s Sidplayer `.mus`/`.str` tunes play via the clean-room [ChipMachine Clean Room SIDPlayer](external/musicplayer/src/plugins/musplugin/README.md), and ~59.9k of the ~62k `.sid` tunes via the permissively-licensed [cSID](external/musicplayer/src/plugins/csidplugin/README.md) engine — whose README lists the measured set of SIDs it cannot voice, and why that list is measured rather than inferred.

DefleMask is being reclaimed the same way, one chip target at a time. [dmfcrplugin](external/musicplayer/src/plugins/dmfcrplugin/README.md) is a clean-room `.dmf` parser and sequencer written from DefleMask's own published format specs and manual — never from Furnace — driving BSD-licensed [ymfm](external/ymfm) for the YM2612 and an SN76489 written against the documented hardware. It covers **SEGA Genesis** and **SEGA Master System** so far, which brings 696 of the 2,071 hidden DefleMask rows back into the MAS index; Game Boy, PC Engine, Neo Geo, NES and C64 remain hidden. It ships in **both** variants but only *claims* files in MAS: Plus registers Furnace first and is byte-for-byte unchanged, which keeps a reference implementation on hand for A/B listening. Which rows survive is decided by the measured list in `data/misc/dmfcr_playable.txt`, because the format column says "Deflemask" without saying which of the eight DefleMask systems a file targets.

It runs **App-Sandboxed**, with a distinct bundle id (`org.mihailod.chipmachine`) and its own cache/database/index, so Plus and MAS never share state. Per-variant product identity (name, bundle id, artifact) is the single source of truth in [variants.conf](variants.conf); the compile-time switch is `CM_VARIANT` / the `CM_MAS` define.

* Running the app (from the build folder): ./chipmachine (-h for all options)
* Running the tests (from the build folder): ./cmtest
* Packaging the app: [package_app.sh](package_app.sh) `--buildapponly` — rebuilds the `chipmachine` target (incremental), bundles the runtime assets, generates the `Info.plist` with file associations, ad-hoc code-signs and zips the `.app`. Run with no args for full usage; add `--applesign` to produce a Developer ID / notarized build (see [Signing & distribution](#signing--distribution-developer-id) below). Defaults to the **Plus** variant; add `--mas` to package the Mac App Store build (`ChipMachine.app` / `.pkg` from `build-mas/` — see [Mac App Store build](#mac-app-store-build-mas-variant)).
* AI tools used to help with the porting: Claude, Gemini, Antigravity, Codex

### macOS file associations (developer notes)

The `.app` advertises the formats it can play as macOS file associations (see the [user-facing note above](#opening-local-music-files)). All the platform-native macOS glue lives under [src/macnative/](src/macnative/):

* **`gen_info_plist.sh`** — the single source of the bundle's `Info.plist`. It builds the file-association document types from three inputs: `extensions.txt` (the playable-extension union — `package_app.sh` dumps the real, variant-specific list from the freshly-built binary into `$BUILD_DIR/extensions.txt` and passes it with `--exts`; the copy checked in under `src/macnative/` is the plus superset, used by `dev_update_doctypes.sh` and as a fallback, and is refreshed by hand), `MacOSSystemTypeExtensions.txt` (formats with an existing system UTI — referenced politely at `LSHandlerRank=Alternate`, never redefined) and `MacOSHandlerDenyList.txt` (non-extension junk / dangerous tokens to drop). Everything else is exported under one umbrella UTI (`org.mihailod.chipmachineplus.chiptune`) with the app's document icon. The last two `.txt` files are hand-editable.
* **`FileOpenHandler.mm`** — patches GLFW's app delegate (`GLFWApplicationDelegate`) at runtime to implement `application:openURLs:`, so double-click / "Open With" / icon drag-and-drop actually play the file (Finder does **not** pass files on `argv`, and this must win the race against GLFW's own `[NSApp run]` inside `glfwInit()`). Dropping a file into the running window instead goes through GLFW's native `glfwSetDropCallback` (wired in the vendored `external/apone/mods/grappix`) — cross-platform, no Apple Event involved. Both paths feed the same play queue.
* **`dev_update_doctypes.sh`** — fast, no-recompile test loop: rewrites the `Info.plist` in an existing bundle, re-signs it and re-registers it with LaunchServices in a couple of seconds. Pass `--with-binary` to also swap in the freshly-built executable and test double-click playback end-to-end. Run with `--help` for full usage.

`package_app.sh` invokes the generator automatically, so a normal release needs no extra steps.

### Signing & distribution (Developer ID)

[package_app.sh](package_app.sh) requires an explicit action flag (run it with no
arguments to see full usage):

| Command | Result |
| --- | --- |
| `./package_app.sh --buildapponly` | Build `.app` + zip, **ad-hoc self-signed** (local/dev; Gatekeeper blocks it on other Macs). |
| `./package_app.sh --applesign --signid="Developer ID Application: Name (TEAMID)"` | Build, then **Developer ID sign** with Hardened Runtime + entitlements. |
| `… --notaryprofile=NAME` | Additionally **notarize with Apple and staple** the ticket. |
| `… --reusebuiltapp` | **Skip the build** and (re)sign the `.app` already on disk — the fast re-sign / re-notarize loop. Requires `--applesign`; can't combine with `--buildapponly`. |
| `… --releaseit` | After packaging, interactively publish the GitHub release. **Requires `--applesign` + `--notaryprofile`** — see below. |

Flags accept a single or double dash and are case-insensitive; value flags use
`--key=value`.

**Why notarization matters:** a Developer ID signature *alone* is not enough.
Since macOS 10.15, a downloaded (quarantined) app must also be notarized by Apple
and have the ticket stapled, or Gatekeeper blocks it with "cannot be checked for
malicious software." Signing + notarizing + stapling are all covered by the same
Apple Developer Program membership and use the free `notarytool` — this is **not**
Mac App Store review. Only the full `--applesign … --notaryprofile=…` build opens
with no warning on other people's Macs.

**One-time setup** (after joining the Apple Developer Program and installing a
*Developer ID Application* certificate in the login keychain):

1. Find your identity string:
   ```bash
   security find-identity -v -p codesigning
   # -> "Developer ID Application: Your Name (ABCDE12345)"
   ```
2. Store a notarization credential profile once (the app-specific password / API
   key lives in the keychain, never on the command line):
   ```bash
   xcrun notarytool store-credentials chipmachine-notary \
       --apple-id you@example.com --team-id ABCDE12345
   ```

**Full distributable build:**

```bash
./package_app.sh --applesign --signid="Developer ID Application: Your Name (ABCDE12345)" --notaryprofile=chipmachine-notary
```

#### Publishing the official release

Every published release is Developer ID signed **and** notarized. `--releaseit`
enforces that: an ad-hoc or un-notarized build is refused before the build even
starts, and there is no override flag. Before uploading, a release gate extracts
the finished zip and re-verifies the copy a user would receive, aborting on any
failure.

```bash
./package_app.sh --applesign --signid="Developer ID Application: Your Name (ABCDE12345)" --notaryprofile=chipmachine-notary --releaseit
```

**Full checklist in [RELEASE_PROCESS.txt](data-notbundled/misc/RELEASE_PROCESS.txt)** —
one-time signing setup, version bump, what the gate checks, post-publish
verification, and how to recall a bad release.

#### In-app update check

[`src/macnative/CheckForUpdate.mm`](src/macnative/CheckForUpdate.mm) polls
`api.github.com/repos/mihailod/chipmachine/releases/latest` at startup and reads
**only `tag_name`**, comparing it against `VERSION_STR`. It never inspects
assets, so renaming or re-signing the download is invisible to it — but it does
constrain how releases are tagged and recalled, which
[RELEASE_PROCESS.txt](data-notbundled/misc/RELEASE_PROCESS.txt) spells out.

The MAS build compiles none of this — `CM_MAS` reduces the entry point to a
no-op, since the App Store delivers its updates and steering users to a GitHub
download would violate App Store guidelines.

**Entitlements** live in [src/macnative/](src/macnative/) and are documented in
[entitlements-README.md](src/macnative/entitlements-README.md): the main
executable and the bundled yt-dlp helper each get
`com.apple.security.cs.disable-library-validation` (the helper is a PyInstaller
Python freeze that fails library validation under Hardened Runtime otherwise).
No JIT entitlements are needed — the app links vanilla Lua, not LuaJIT. The
entitlements plists are intentionally comment-free: macOS's kernel entitlement
parser (AMFI) rejects XML comments.

### Mac App Store build (MAS variant)

The Mac App Store variant is packaged with `--mas`, which builds from `build-mas/`
(configure it once with `-DCM_VARIANT=mas`, see [Build variants](#build-variants-plus-vs-mac-app-store)),
skips the yt-dlp helper, applies the App-Sandbox entitlements, and produces a
signed `.pkg` instead of a zip.

| Command | Result |
| --- | --- |
| `./package_app.sh --mas --buildapponly` | Build `ChipMachine.app`, **ad-hoc-signed but sandboxed** — a local test build. No certificates needed. |
| `./package_app.sh --mas --applesign --distribid="Apple Distribution: Name (TEAMID)" --installerid="3rd Party Mac Developer Installer: Name (TEAMID)" --provision=/path/to/ChipMachine.provisionprofile` | Build, sign for the App Store, embed the provisioning profile, and emit a signed **`.pkg`** for upload. |

The three signing inputs (`--distribid`, `--installerid`, `--provision`) require
the paid **Apple Developer Program** plus an App Store Connect record for the
bundle id `org.mihailod.chipmachine`. Upload the resulting `.pkg` with
**Transporter.app** or `xcrun altool --upload-app`. There is **no zip and no
notarization** for this variant — the App Store handles review and signing.

MAS **entitlements** are in [entitlements-app-mas.plist](src/macnative/entitlements-app-mas.plist):
`com.apple.security.app-sandbox` + `com.apple.security.network.client` (outbound
only — the app runs no listening server; it deliberately does **not** declare
`network.server`). No `disable-library-validation` and no yt-dlp helper, since the
MAS build ships neither.

> Not yet wired for a live submission: the app still needs **file-access sandbox
> entitlements** (`com.apple.security.files.user-selected.read-only` plus
> security-scoped bookmarks) for the double-click / "Open With" path to work under
> the sandbox. `--mas --buildapponly` is fully usable for local testing today.

## Using the application

* Type words separated by spaces for incremental search
* *ENTER* to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F5* = Play/Pause
* *F9* = Advanced search: set/reset search filter by platform (ie. Amiga)
* *F6* = Next Song (or *ENTER* from Player Screen)
* *ESC* = Clear search field
* *SHIFT-ESC* = Quit
* *F7* = Toggle Favorite
* Type _shoutcast_ to see the radio-stations
* [Things in progress / to come](data-notbundled/misc/TOODOO.txt)

## Data Sources

### Chip Music Collections

Metadata from these databases is ingested, normalized, deduped and cross-checked for screenshots.

* Modland - https://ftp.modland.com
* High Voltage SID collection - https://www.hvsc.c64.org
* Gamebase64 - http://www.gb64.com
* AMP (Amiga Music Preservation) - http://amp.dascene.net
* RKO - http://remix.kwed.org
* Atari ST (SNDH) - http://sndh.atari.org
* SNES Music - http://snesmusic.org
* Atari SAP (ASMA) - http://asma.atari.org
* HVTC (High Voltage TED Collection) - http://plus4world.powweb.com
* NSFE (Famicompo mini NSFE archive of 1,228 songs from https://forums.nesdev.org/viewtopic.php?t=21128)
* Mod Archive - https://modarchive.org
* Bitworld - http://janeway.exotica.org.uk
* Exotica - https://www.exotica.org.uk
* CSDb - https://csdb.dk
* ZX Art - https://zxart.ee/eng/music
* CPC-Power - https://www.cpc-power.com
* Zophar's Domain - https://www.zophar.net
* Vampi's MDX Collection (Sharp X68000) - https://mdx.vampi.tech
* Demozoo - https://demozoo.org
* Scene.org - https://scene.org
* OPL Archive - https://opl.wafflenet.com
* Bulba's ZX Spectrum & Amstrad CPC AY/YM Music Archives - https://bulba.untergrund.net/music_e.htm
* VGMRips - https://vgmrips.net
* ZX TUNES - https://zxtunes.com
* SMS POWER! - https://www.smspower.org
* MirSoft - http://www.mirsoft.info
* Chipmusic - https://chipmusic.org
* Battle of the Bits - https://battleofthebits.org
* keygenmusic (Internet Archive) - https://archive.org/details/keygen-music-2020-03-pack

### Remixes / Recordings / Streaming (mp2/mp3/ogg/flac/wav/aif/aiff/opus)

* Pouët.net - https://www.pouet.net
* Amiga Remix (MP3) - http://amigaremix.com
* OverClocked ReMix (MP3) - https://ocremix.org
* Sounds of Scenesat - https://scenesat.com
* Demozoo -- https://demozoo.org

### Youtube Audio
* Pouet - https://www.pouet.net

### Podcasts

Press *F9* and pick **Podcasts** to browse/filter only podcast episodes.
Shows backed by a live RSS feed ship with
a snapshot of their back catalogue and are refreshed from the feed in the
background at startup (throttled to roughly once a day), so newly published
episodes are merged in automatically without a full re-index.

* C64 Take-away — Commodore 64 remixes & original SID (complete, ended 2025) - https://c64takeaway.com
* This Week in Chiptune — chiptune mixes (Dj CUTMAN, 2013–2017 archive) - https://thisweekinchiptune.com
* Pixelated Audio — video game music & interviews - https://pixelatedaudio.com
* GameFuel — video game music (KNGI Network) - https://kngi.org
* Nitro Game Injection — video game music & remixes (KNGI Network) - https://kngi.org
* Demovibes — demoscene music - https://www.demovibes.org
* AmigaVibes — Amiga & demoscene music - http://www.amigavibes.org
* Syntax Error — game & demoscene music (Sol) - http://www.syntaxerror.nu

And one not related to retro music but dear to my heart so here it is:

* Completely Unnecessary Podcast — retro gaming (Pat "The NES Punk" Contri & Ian Ferguson) - https://cupodcast.podbean.com

### Shoutcast Radio Streams

* Scenesat - https://scenesat.com
* SLAY Radio - https://www.slayradio.org
* Nectarine - https://scenestream.net
* VGM Radio - http://vgmradio.com
* NoLife-Radio - https://www.nolife-radio.com
* Rainwave - https://rainwave.cc
* The Sid Station - https://c64radio.com
* Radio PARALAX - https://www.radio-paralax.de
* CVGM Radio - https://radio.cvgm.net
* Kohina - https://kohina.com
* Gyusyabu NEC PC-98/Sharp X68000 - http://gyusyabu.ddo.jp

## What it plays

350+ music formats, decoded by 60+ plugins. Everything is played back natively —
no external players, no command-line helpers.

Every plugin has its own README covering the engine behind it, how shared
extensions are routed, licensing and known gaps. Start at the
[plugin index](external/musicplayer/src/plugins/README.md); the table below is
the short version.

### PC & Amiga trackers

| Plays | Extensions | Plugin |
|---|---|---|
| ProTracker, FastTracker II, Impulse Tracker, ScreamTracker, OctaMED, OpenMPT, Symphonie, Digital Symphony, Graoumf Tracker and ~30 more | `.mod` `.xm` `.it` `.s3m` `.mptm` `.med` `.okt` `.symmod` `.dsym` … | [openmpt](external/musicplayer/src/plugins/openmptplugin/README.md) |
| Amiga "exotic" custom replayers (TFMX, Future Composer, SidMon, Hippel, Sonic Arranger, David Whittaker, Rob Hubbard …) — the original 68k code, ~180 replayers | ~400 extensions, matched as a filename prefix or suffix | [uade](external/musicplayer/src/plugins/uadeplugin/README.md) *(Plus only)* |
| AHX / HivelyTracker | `.ahx` `.hvl` `.thx` | [hively](external/musicplayer/src/plugins/hivelyplugin/README.md) |
| MikMod UNITRK / UNIMOD | `.uni` | [mikmod](external/musicplayer/src/plugins/mikmodplugin/README.md) |
| Old (pre-OctaMED) MED | `.med` | [med](external/musicplayer/src/plugins/medplugin/README.md) |
| MaxTrax | `.mxtx` | [maxtrax](external/musicplayer/src/plugins/maxtraxplugin/README.md) *(Plus only)* |
| JayTrax / Syntrax | `.jxs` | [jxs](external/musicplayer/src/plugins/jxsplugin/README.md) |
| Ixalance | `.ixs` | [ixs](external/musicplayer/src/plugins/ixsplugin/README.md) |
| ProTrekkr / NoiseTrekker | `.ptk` `.ntk` | [ptk](external/musicplayer/src/plugins/ptkplugin/README.md) |
| Quartet | (companion `.set` banks) | [quartet](external/musicplayer/src/plugins/quartetplugin/README.md) |
| Farbrausch V2 Synthesizer System | `.v2` `.v2m` | [v2](external/musicplayer/src/plugins/v2plugin/README.md) |

### Commodore

| Plays | Extensions | Plugin |
|---|---|---|
| C64 SID (6581/8580, incl. 2SID/3SID) | `.sid` `.rsid` | [csid](external/musicplayer/src/plugins/csidplugin/README.md) |
| Compute!'s Sidplayer (clean-room sequencer — what lets the App Store build play these ~6.5k songs) | `.mus` `.str` | [mus](external/musicplayer/src/plugins/musplugin/README.md) |
| Compute!'s Sidplayer via VICE | `.mus` `.str` | [vicebridge](external/musicplayer/src/plugins/vicepluginbridge/README.md) *(Plus only)* |
| GoatTracker | `.sng` | [goattracker](external/musicplayer/src/plugins/goattrackerplugin/README.md) *(Plus only)* |
| Commodore 264 series (16 / 116 / Plus/4) TED | `.prg` | [ted](external/musicplayer/src/plugins/tedplugin/README.md) |
| VIC-20 VIC-TRACKER | `.vt` | [victracker](external/musicplayer/src/plugins/victrackerplugin/README.md) |

### ZX Spectrum, Amstrad CPC, Sam Coupé

| Plays | Extensions | Plugin |
|---|---|---|
| ZX Spectrum AY-3-8912 trackers — 67,305 songs, the largest format family here | `.pt1` `.pt2` `.pt3` `.stc` `.asc` `.psc` `.sqt` `.stp` `.vtx` `.vt2` `.psg` `.ftc` `.psm` `.gtr` `.st11` `.fxm` `.amad` … | [zxay](external/musicplayer/src/plugins/zxayplugin/README.md) |
| TFM Music Maker and Chip Tracker (the two ZX formats that are not AY music) | `.tfe` `.chi` | [zxtune](external/musicplayer/src/plugins/zxtuneplugin/README.md) *(Plus only)* |
| ZX Spectrum 1-bit beeper music (Beepola: SFX, Phaser1, Music Box, Music Studio) | `.bbsong` | [bbsong](external/musicplayer/src/plugins/bbsongplugin/README.md) |
| STarKos / Arkos Tracker (Amstrad CPC) | `.sks` `.aks` | [sks](external/musicplayer/src/plugins/sksplugin/README.md) |
| Sam Coupé SAA1099 music | `.cop` | [cop](external/musicplayer/src/plugins/copplugin/README.md) |

### Atari

| Plays | Extensions | Plugin |
|---|---|---|
| Atari ST/STE SNDH | `.sndh` `.snd` | [sndh](external/musicplayer/src/plugins/sndhplugin/README.md) |
| Atari ST YM2149 register dumps | `.ym` `.mix` | [stsound](external/musicplayer/src/plugins/stsoundplugin/README.md) |
| Atari 16-bit SC68 container | `.sc68` | [sc68](external/musicplayer/src/plugins/sc68plugin/README.md) *(Plus only)* |
| Megatracker (Atari ST) | `.mgt` | [mgt](external/musicplayer/src/plugins/mgtplugin/README.md) |
| Atari XL/XE POKEY (PokeyNoise) | `.pn` | [pokeynoise](external/musicplayer/src/plugins/pokeynoiseplugin/README.md) *(Plus only)* |
| Atari 8-bit SAP (the ASMA corpus) | `.sap` | [gme](external/musicplayer/src/plugins/gmeplugin/README.md) |

### MSX and Japanese computers

| Plays | Extensions | Plugin |
|---|---|---|
| MSX drivers + FAC SoundTracker | `.mgs` `.bgm` `.opx` `.mpk` `.mbm` `.mus` | [kss](external/musicplayer/src/plugins/kssplugin/README.md) |
| SCC-Musixx (Konami SCC) | `.SNG` | [sccmusixx](external/musicplayer/src/plugins/sccmusixxplugin/README.md) |
| NEC PC-98 FMP (incl. OPNA hardware-rhythm drums) | `.opi` `.ovi` `.ozi` | [fmp](external/musicplayer/src/plugins/fmpplugin/README.md) |
| S98 FM register logs (incl. OPNA rhythm drums) | `.s98` | [s98](external/musicplayer/src/plugins/s98plugin/README.md) |
| FM Towns / PC-98 Euphony | `.eup` | [eup](external/musicplayer/src/plugins/eupplugin/README.md) |
| Sharp X68000 MDX | `.mdx` (+ `.pdx` banks) | [mdx](external/musicplayer/src/plugins/mdxplugin/README.md) *(Plus only)* |

### Consoles

| Plays | Extensions | Plugin |
|---|---|---|
| NES/Famicom, Game Boy, SNES, Master System, Genesis/Mega Drive, PC Engine, MSX, ZX, CPC, BBC Micro, POKEY | `.nsf` `.nsfe` `.gbs` `.gbr` `.spc` `.sgc` `.gym` `.hes` `.kss` `.ay` `.vgm` `.vgz` `.sap` `.emul` | [gme](external/musicplayer/src/plugins/gmeplugin/README.md) |
| VGM/VGZ on the libvgm cores (incl. OPL2/OPL3, 32X PWM, Virtual Boy VSU) | `.vgm` `.vgz` | [libvgm](external/musicplayer/src/plugins/libvgmplugin/README.md) |
| FamiTracker (NES + VRC6/VRC7/MMC5/FDS) | `.ftm` | [famitracker](external/musicplayer/src/plugins/famitrackerplugin/README.md) *(Plus only)* |
| NerdTracker II (NES) | `.ned` | [ned](external/musicplayer/src/plugins/nedplugin/README.md) |
| DefleMask / Furnace chiptunes | `.dmf` | [dmf](external/musicplayer/src/plugins/dmfplugin/README.md) *(Plus only)* |
| DefleMask, SEGA Genesis / Master System (clean room) | `.dmf` | [dmfcr](external/musicplayer/src/plugins/dmfcrplugin/README.md) |
| Game Boy Advance | `.gsf` `.minigsf` | [gsf](external/musicplayer/src/plugins/gsfplugin/README.md) |
| Nintendo DS | `.2sf` `.mini2sf` | [nds](external/musicplayer/src/plugins/ndsplugin/README.md) *(Plus only)* |
| Nintendo 64 | `.usf` `.miniusf` | [usf](external/musicplayer/src/plugins/usfplugin/README.md) *(Plus only)* |
| PlayStation 1 & 2, Sega Saturn, Capcom QSound | `.psf` `.minipsf` `.psf2` `.minipsf2` `.ssf` `.qsf` | [ao](external/musicplayer/src/plugins/aoplugin/README.md) *(Plus only)* |
| Dreamcast & Saturn | `.ssf` `.dsf` `.minissf` `.minidsf` | [ht](external/musicplayer/src/plugins/htplugin/README.md) *(Plus only)* |
| Bandai WonderSwan / Color | `.wsr` | [wsr](external/musicplayer/src/plugins/wsrplugin/README.md) |
| RAR-packed SNES sets | `.rsn` `.rps` `.rdc` `.rds` `.rgs` `.r64` | [rsn](external/musicplayer/src/plugins/rsnplugin/README.md) |
| Streamed in-game audio — CRI ADX/HCA, FMOD FSB, XWB/XMA, Nintendo DSP, PS VAG, AT3/AT9 and ~700 more | `.adx` `.hca` `.fsb` `.xwb` `.dsp` `.vag` … | [vgmstream](external/musicplayer/src/plugins/vgmstreamplugin/README.md) |

### Acorn, Apple, Macintosh, MS-DOS

| Plays | Extensions | Plugin |
|---|---|---|
| Acorn Archimedes Tracker (!Tracker, 8 channels) | `.musx` | [musx](external/musicplayer/src/plugins/musxplugin/README.md) |
| Coconizer (Archimedes) | `.coco` | [coco](external/musicplayer/src/plugins/cocoplugin/README.md) |
| Apple IIgs SoundSmith (Ensoniq 5503) | song name + `.W` wavebank | [soundsmith](external/musicplayer/src/plugins/soundsmithplugin/README.md) |
| PlayerPRO (classic Macintosh) | `.mad` | [playerpro](external/musicplayer/src/plugins/playerproplugin/README.md) |
| AdLib / Sound Blaster OPL formats — ~50 of them | `.a2m` `.cmf` `.d00` `.dro` `.hsc` `.imf` `.laa` `.rad` `.rol` `.sci` … | [adplug](external/musicplayer/src/plugins/adplugin/README.md) |
| Funktracker (FunktrackerGOLD / DOS32) | `.fnk` | [fnk](external/musicplayer/src/plugins/fnkplugin/README.md) |
| SBStudio | `.pac` | [sbstudio](external/musicplayer/src/plugins/sbstudioplugin/README.md) |
| MONOTONE (IBM PC speaker) | `.mon` | [monotone](external/musicplayer/src/plugins/monotoneplugin/README.md) |

### Softsynths, modern trackers, streaming audio

| Plays | Extensions | Plugin |
|---|---|---|
| SunVox | `.sunvox` | [sunvox](external/musicplayer/src/plugins/sunvoxplugin/README.md) |
| klystrack | `.kt` | [klystrack](external/musicplayer/src/plugins/klystrackplugin/README.md) |
| PxTone Collage | `.ptcop` `.pttune` | [pxtone](external/musicplayer/src/plugins/pxtoneplugin/README.md) |
| Organya (Cave Story) | `.org` | [org](external/musicplayer/src/plugins/orgplugin/README.md) |
| MP3, AAC, M4A/MP4, Ogg/Vorbis, Opus, FLAC, WAV, AIFF, AC3, MP2, WMA, IFF-8SVX — local files, downloads and radio streams | `.mp3` `.m4a` `.ogg` `.opus` `.flac` `.wav` `.aif` … | [ffmpeg](external/musicplayer/src/plugins/ffmpegplugin/README.md) |
| YouTube links (`youtube.com` / `youtu.be`) — how the Pouët database plays demoscene soundtracks | — | *(Plus only)* |

*(Plus only)* marks a decoder that ships in **ChipMachinePlus** but not in the
Mac App Store build — see
[Build variants](#build-variants-plus-vs-mac-app-store). The catalog hides
anything the running build cannot play, so you never see a song that will not
start.

### Formats we deliberately skip

The collections we index are not curated for us: they carry files that no player
in this stack can turn into sound. If those were indexed they would look like
ordinary songs, download on ENTER, and then dead-end — so the indexer drops them
up front, and the format simply never appears in search. That is why a handful of
directories you can see on modland (or a scene.org compo dir) have no entries
here.

Broadly: formats with no open replayer (Renoise, Psycle, Jeskola Buzz, Sound
Club, BeRoTracker, StoneTracker), files that are not music at all (archives, DAW
projects, ROMs and disk images — we play the ripped chip logs, never the parent
ROM), companion files that belong to a tune rather than being one, and a few
formats where an engine looked like it would work and measurably did not.

The list, with the evidence for each entry, is
[`data/misc/not_supported_extensions.txt`](data/misc/not_supported_extensions.txt).

---

## **Credits**

![Annoying Popup](data-notbundled/misc/argh.jpg)

> ChipMachine is my favorite Mac retro chiptune player and I have been using it forever. However, when opening it on my Mac after a recent macOS update, I saw this annoying popup above. It upset me and I channeled that anger into this project.

This is mostly a preservation effort. I am making this project for myself so I am able to continue enjoying listening to my favorite music in a clean and inspirational way the original Chipmachine was providing to me over many many years.

My work here is mostly based around:

* **Porting**: from Intel to ARM
* **Integration**: with / adding of various new plugins that did not exist in the Intel version
* **Content curation**: updating / adding more songs and fixing their metadata from various databases
* **Administration**: maintenance, releasing, PR merging, support, promoting

**I don't take or imply any credit for the original idea and implementation and actual players development (the hardest part IMO).**

Attribution for the individual emulators, audio players, plugins and core
sub-routines used across this project is maintained as a single source of truth
in the licence notices, **not** here — see [Licensing](#licensing) below.

---

## Licensing

* **ChipMachineAS:** Copyright (c) 2026 Mihailo Despotovic. Licensed under [`PolyForm Noncommercial License 1.0.0.`](./LICENSE)
- 🟢 **Free to use** for personal, educational, research, and non-commercial projects.
- 🔴 **Commercial use prohibited.** If you intend to use my code in any way in any revenue-generating product, contact me for a commercial license.

* **Original ChipMachine Program:** Copyright (c) 2022 Jonas Minnberg. Licensed under the MIT License.
* **Third-party components:** see [`LEGAL`](./LEGAL) — the standalone notice covering everything in the app, and the single source of truth for attribution.
* **ChipMachinePlus extras:** see [`LEGAL-PLUS`](./LEGAL-PLUS) — an addendum to `LEGAL` covering only the copyleft engines the Plus build links and the Mac App Store build does not.

`package_app.sh` assembles the in-app **About** panel (`Credits.rtf`) from those
same two files, so the app, the repo and the release always agree: the Mac App
Store bundle gets `LEGAL`, the Plus bundle gets `LEGAL` + `LEGAL-PLUS`.
