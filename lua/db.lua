
-- source : Base path or url from where to load files
-- song_list : List of all songs to add. May not be needed if db is local and supports file scanning
-- local_dir : If exists, will be checked first for files before downloading.
-- If song_list or or source can not be found, database will not be added

-- the intel chipmachine stopped at 23 (November 2017)
-- after 8+ years, May 2026 24 is a major update
-- to almost all databases bringing ~25% songs
-- 26: Euphony (.eup) indexed standalone; .fmb/.pmb/.pvi banks excluded as
--     songs (parseModland secondary set) and fetched via getSecondaryFiles
-- 27: forces a clean reindex after the per-song index/screenshot LOGD spam was
--     downgraded to LOGV (a full reindex was unusably slow printing it)
-- 28: secondary-extension exclusion now case-insensitive, so FMP's UPPERCASE
--     .PVI bank files stop being indexed as ~1074 bogus standalone songs
-- 29: Modland ".info" metadata siblings are no longer indexed as bogus songs,
--     and PokeyNoise "pn.<song>" prefix files type/title correctly (PokeyNoise)
-- 30: SoundSmith (Apple IIgs) support -- the ".W" wavebank is excluded as a song
--     (parseModland secondary set) and fetched via getSecondaryFiles next to the
--     bare-named song; "SoundSmith" maps to the Apple IIgs platform/format
-- 31: added Kohina radio station, removed .snd files
-- 32: ext field added to SongInfo and DB; parseStandard now recognizes "ext" keyword
-- 34: skip KrisHatlelid "songplay" driver files (companion, not a standalone
--     song; it sorted before the .kh and became the MULTI: primary -> silent)
-- 35: The Sid Station radio: switched from the :2199 HTTPS .pls redirector
--     (which hangs the in-app fetch -> "BUFFERING..." forever) to the direct
--     :8144 HTTP Shoutcast MP3 stream, so it plays without a playlist fetch
-- 36: Fixed 762 malformed UnExoticA paths where a per-version subdir inside the
--     product .lha (Custom_Version, Direct_from_Composer, Bonus, AGA_Version, ...)
--     was encoded as "<Game>/<Version>.lha/<member>" -> FTP 550. The real archive
--     is "<Game>.lha" with the version as an inner dir, so they are now
--     "<Game>.lha/<Version>/<member>" (e.g. Advanced Ski Simulator Custom).
-- 49: zxart.ee MUSIC collection (~29k ZX Spectrum / Sam Coupe tunes). Built by
--     chipmachine/scripts/build_zxart.py from the zxart API; routes by chip type (AY ->
--     "ZX Spectrum 128", beeper -> "ZX Spectrum 16/48", COP/SAA -> new "Sam
--     Coupe" category). Originals play natively when the format is supported;
--     otherwise the per-tune ogg fallback (music.zxart.ee) streams via ffmpeg.
--     New SAMCOUPE format byte shifts later enum values, so a full reindex is
--     mandatory (this bump forces it).
-- 50: zxart.txt rebuilt -- native-vs-ogg and the stored ext now come from the
--     REAL file extension, not zxart's `type`. `type` mislabels some files
--     (e.g. an ETracker .etc tune tagged type=COP), and renaming to the type's
--     extension fed them to the wrong native player (silent / could not play).
--     Such mismatches now stream as ogg; only genuinely-supported extensions
--     play natively. Category (filter) still groups by type/compo.
-- 51: zxart multi-part game soundtracks ("<game> - <part> (<chip>) <N>") are
--     folded into one row per chip class instead of N redundant rows. zxart lists
--     each internal subsong of a rip as a separate tune-id but they are all the
--     SAME multi-subsong file (verified by identical md5), so we emit a single URL
--     -- the file's own subsongs drive next/prev as an instant mp.seek(), not a
--     reload. Mixed beeper+AY games are separate files -> "<game> (Beeper)" /
--     "<game> (AY)". The (chip) title label drives the category, so .ay beeper
--     rips classify as ZX 16/48 beeper rather than defaulting to AY.
-- 52: (above corrected) -- v51 first shipped a MULTI: umbrella of N identical-file
--     URLs, which reloaded the same file each step and replayed subsong 0 ("always
--     1st tune", slow); switched to the single-file form so subsong nav is instant.
-- 55: CPC-Power YM audiotheque (small Wayback subset). Amstrad CPC game .ym rips
--     (AY-3-8912), LHA-wrapped; stsoundplugin depacks them natively. Full set has
--     no browsable index (/YM/ is 403), so only the ~51 Wayback-known files are
--     indexed, with URLs pointing at the live /YM/ files (fetched on demand).
--     Built by chipmachine/scripts/build_cpcpower.py; format "Amstrad CPC" -> AMSTRAD filter.
-- 56: Zophar's Domain pilot -- Sega Genesis VGM gamerips (the genuinely net-new
--     slice; modland already has SNES/GB/PS/N64/Saturn/NES/Dreamcast but NO VGM
--     and NO arcade). Each row's path is a "<game> (EMU).zophar.zip" of per-track
--     .vgm files; MusicPlayerList detects the ZIP by magic, extracts it, and plays
--     the tracks as local subsongs. Built by chipmachine/scripts/build_zophar.py (dedup vs the
--     265 modland Megadrive GYM games). format "Sega Genesis" -> MEGADRIVE filter.
-- 57: zophar.txt deduped properly. v56 wrongly deduped Genesis only vs "Megadrive
--     GYM" (265) and missed modland's huge "Video Game Music/Sega Megadrive" tree
--     (10961 files / 621 games), so 480 of the 815 were dups. Now dedups vs the
--     Video Game Music Sega dirs -> 335 genuinely-new games (215 MD + 119 Sega CD
--     + 1 32X). Also fixed the libcurl HTTP/2 prune crash (web.cpp FORBID_REUSE)
--     and the ZipFile extractor (archive.cpp) that this collection first exposed.
-- 58: Vampi's MDX Database (mdx.vampi.tech) -- Sharp X68000 MDX. Vampi has 9321
--     MDX vs modland's 7449, so we onboard ONLY the net-new ones: best-effort
--     dedup by (basename,size) vs modland MDX -> 6872 kept, 2449 dups skipped.
--     Built by chipmachine/scripts/build_vampi.py; path = data/<filename> (verified .mdx);
--     format "MDX" -> JPFM (X68000/FM Towns filter); plays via mdxplugin.
-- 64: four more music podcasts (type=podcast, live remote_list + shipped
--     snapshot, per-episode itunes:image art): This Week in Chiptune (Dj
--     CUTMAN), Pixelated Audio, GameFuel + Nitro Game Injection (KNGI). All
--     show under the TAB Podcasts category alongside C64 Take-away / CU Podcast.
-- 65: HVTC (Commodore 16/116/+4 TED .prg) pivoted from the flaky online
--     plus4world/Wayback mirror to a shipped local store (music/hvtc), like
--     nsfe -> music/Console. Forces a reindex so the new local_dir is stored.
-- 66: Demozoo (demozoo.org). Built by chipmachine/scripts/build_demozoo.py from the daily
--     Postgres dump (data.demozoo.org/demozoo-export.sql.gz). Onboards only
--     net-new demoscene *music* (supertype=music): a production is dropped if
--     any of its download links is one we already mirror -- a ModlandFile, or a
--     BaseUrl on a host we ship as its own DB (HVSC/sndh/asma/AMP/csdb/zxart/
--     cpc-power). Kept buckets: media.demozoo.org + files.zxdemo.org (BaseUrl),
--     scene.org via archive.scene.org direct HTTPS (validated lone-module compo
--     zips, host-extracted by magic; NOT files.scene.org/get which 302s to http),
--     and the Fujiology Atari/Amiga archive. Chip tunes route into existing
--     filters (Commodore 64 / Amiga / Atari ST / Atari 8Bit / ZX Spectrum) by
--     demozoo platform, or by file extension when untagged; the cross-platform
--     / streamed remainder lands in a new "Demoscene" category. Per-tune
--     screenshots (the production's own media.demozoo.org screens) load from
--     data/demozoo_screenshots.txt keyed by the song URL (like zxart). Full
--     URLs in the path column -> source empty.
-- 67: demozoo scene.org URLs repointed from files.scene.org/get/ (302 -> plain
--     http:// mirror, which the in-app curl can't follow: RemoteLoader CODE -1 /
--     hang) to archive.scene.org/pub/ (direct HTTPS 200). The song URLs are
--     stored in the indexed DB, so this needs a reindex -> the bump forces it.
-- 68: scene.org tracker modules (.mod/.xm/.it/.s3m). These four formats are the
--     most-mirrored content we ship (modland ~165k + modarchive ~155k of exactly
--     these), so onboarding all ~9k of scene.org would be almost pure dup. Scoped
--     (user choice) to the slice modland/modarchive cover WORST: the Spanish
--     demoscene mirror (mirrors/scenesp.org/) + the party archive (parties/).
--     Built by chipmachine/scripts/build_sceneorg.py from the shipped TSV dumps in
--     data/misc/scene.org/. No md5 exists anywhere (TSV / files.scene.org view
--     page / API), so dedup is by lowercased basename vs allmods (modland),
--     modarchive.txt and our own demozoo.txt -> 3278 net-new kept of 6838 in
--     scope. composer = the party/artist dir the file sits in (provenance);
--     format MOD/XM/IT/S3M routes into the existing per-format filters. URLs are
--     archive.scene.org/pub/<path> (direct HTTPS 200, same lesson as v67); a few
--     are .mod.zip etc. (ext "zip", host-extracted by magic). source empty. The
--     bump forces a reindex so the new rows land.
-- 69: FAC SoundTracker .sm1/.sm2 excluded as songs (parseModland secondary set):
--     they are the drumkit SAMPLE BANKS a .mus song loads, not standalone tunes,
--     and showed in the GUI as ~666 bogus entries that download then can't play.
--     Fetched at play time via KSSPlugin::getSecondaryFiles (<DRUMKIT>.SM1/.SM2)
--     now that FAC .mus plays. All 666 .sm1/.sm2 on Modland are FAC drumkits, so
--     the exclusion is global. Bump forces a reindex so the rows drop.
--     Also skips two .mus orphans no open replayer decodes, parked so they stop
--     showing as broken GUI entries: "Ad Lib/MUS/" (1 file, Nick Jones/first
--     samurai.mus -- AdPlug's MUS loader rejects it, OpenMPT/UADE/Vice all fail)
--     and "MVS Tracker/" (2 files, magic MVSM1 -- nothing in our stack handles it).
-- 70: Fix partial Demozoo index. The SongInfo ctor treated "path;<suffix>" as a
--     subtune selector and called stoi() on the suffix; 9 scene.org URLs in
--     demozoo.txt carried a stray TRAILING ';' (empty suffix), so stoi("") threw
--     "no conversion", an uncaught exception that aborted Demozoo indexing at the
--     FIRST such row -- only ~3.2k of ~41.9k songs landed (e.g. the 67 Atari
--     2600/VCS tunes were all past the cutoff). SongInfo now only parses an
--     all-digit suffix; the 9 stray ';' were stripped from demozoo.txt (and
--     build_demozoo.py rstrips them going forward). Bump forces a full reindex so
--     the rest of Demozoo lands.
-- 71: Dropped 140 Fujiology (ftp.untergrund.net) .zip rows that contain NO
--     playable member -- native-platform disk-images/ROMs/executables (e.g.
--     BOING.ZIP->BOING.ATR Atari 8-bit disk image, BATTLE.BIN Atari 2600 ROM,
--     HAWKEYE.XEX). These showed as dead "No playable tracks" entries, mostly in
--     the Atari/console TAB filters. chipmachine/scripts/filter_demozoo_archives.py downloads the
--     417 Fujiology zip rows and keeps only the 277 with a decodable member
--     (module/mp3/etc., matching MusicPlayerList songExt+audioExt); demozoo.txt
--     41929->41789. Also fixed nested-member zip extraction (audio in a subfolder
--     now extracts; ZipFile::extract makedirs the parent + skips dir entries).
-- 72: Extended the archive filter to scene.org too. chipmachine/scripts/filter_demozoo_archives.py
--     --all-hosts --native-only inspects the ~2005 non-Demoscene/non-Amiga .zip rows
--     (native/console platforms, where program-only zips concentrate) and drops the
--     601 with NO playable member (2600 ROMs/.xex exes/disk-images like snakepit.bin,
--     staxx_goldrunner.xex, kk_tia_filler.bin). Demoscene/Amiga zips (module/mp3
--     compo entries) left untouched. demozoo.txt 41789->41188. Bump forces reindex.
-- 73: Manual database patch (data/manualDatabasePatch.txt) -- a hand-maintained
--     list of extra items indexed alongside every other collection. Each row is
--     "name<TAB>platform<TAB>author<TAB>youtube_link<TAB>screenshot_link". Plays
--     the YouTube link like the Pouet/Youtube collection; the screenshot is a
--     full URL stored verbatim per song (new `screenshot` song_template keyword
--     in parseStandard -> song.artwork column -> getSongScreenshots fast path).
--     Bump forces a reindex so the rows land.
-- 74: Manual patch rows gained a 6th column, a free-text description that scrolls
--     while the tune plays (like a module's embedded message). New `info`
--     song_template keyword maps it to metaIndex -> song.metadata[INFO], which
--     ChipMachine's scroller already renders. Bump forces a reindex.
-- 75: AMP (Amiga Music Preservation, amp.dascene.net) rebuilt from the full
--     178k catalogue scrape (data/misc/amp/MODULES.csv) into a real, indexed
--     collection -- the old entry was `index = "no"` (unsearchable) and only
--     half-played (MOD/STK routed via OpenMPT's prefix special-case; IT/XM/
--     S3M/MED/... silently failed). chipmachine/scripts/build_amp.py dedups vs modland +
--     modarchive + demozoo + scene.org + unexotica on normalised (composer,
--     title) -> 58432 net-new of 178032. Each path is a bare module id; the
--     source appends it to "downmod.php?index=" (302 -> gzip module, no
--     Content-Disposition). MusicPlayerList now inflates the gzip body by
--     magic (1F 8B), then the `ext` column (mapped from AMP's short FORMAT
--     code by build_amp.py) routes it to OpenMPT/UADE/hively -- the same
--     id-URL pattern as modarchive. New AMP-specific short format codes
--     (stk/fst/oss/bp/gmc/ml/... -> Amiga, tcb -> Atari) added to initFormats
--     so classifyFormat lands them correctly despite the extension-less path.
--     The legacy `parseAmp` parser (dispatched by type->id "amp", expected the
--     old "L/Composer/FMT.title.gz" paths) was removed so AMP now parses via
--     parseStandard like every other .txt collection. Bump forces a reindex.
-- 76: The OPL Archive (opl.wafflenet.com) -- ~1341 non-game OPL2/OPL3 chiptunes
--     (demoscene + covers) by 215 artists. Every file is a VGM log gzipped to
--     .vgz; a random archive sample is 100% YM3812 (OPL2) / YMF262 (OPL3), the
--     AdLib / Sound Blaster PC chips. GME's Vgm_Emu has no OPL cores (renders
--     them silent, aborts on some OPL2), so a new libvgmplugin (ValleyBell
--     libvgm, vendored at zxtune/3rdparty/vgm) plays them and GME declines any
--     OPL-carrying VGM -- the ~14k non-OPL console VGZ stay on GME. Built by
--     chipmachine/scripts/build_oplarchive.py from data/misc/opl/files.csv; path = a full
--     URI-encoded wafflenet URL (source empty), kept as .vgz (libvgm reads gzip
--     directly). format "OPL Archive" -> ADPLUG -> the "IBM PC (AdLib/OPL)"
--     filter. Bump forces a reindex.
-- 77: Project AY / AY-EMUL (Sergey Bulba, bulba.untergrund.net) -- 613 raw Z80
--     machine-code music rips in the ZXAYEMUL (.ay) container: Ironfist's ZX
--     Spectrum game rips (210) + Bulba's own (30) -> "Spectrum AY"/ZXAY, and
--     SoLO/CORPSE's Amstrad CPC demo rips (373) -> "Amstrad CPC"/AMSTRAD.
--     Shipped LOCAL (music/projectay, like nsfe->music/Console): Bulba only
--     serves big archives and Ironfist's site is dead. Built by
--     chipmachine/scripts/build_projectay.py from embedded AY metadata (PAuthor=composer,
--     PMisc=game). .ay is now owned by gmeplugin (Ay_Emul lineage plays ZX AND
--     CPC; Ayfly, which renders CPC rips silent, no longer claims .ay and keeps
--     pt3/stc/vtx/...). Not deduped: distinct "definitive" raw rips vs zxart's
--     register-dumps (modland has zero .ay). Screenshots: ZX rips carry the real
--     game name, so 198/240 match a World of Spectrum loading screen via ZXDB
--     (scripts/update_projectay_screenshots.py -> data/projectay_screenshots.txt,
--     keyed by the local song path); CPC demo rips get none. Bump forces a
--     reindex.
-- 78: VGMRips (vgmrips.net) game rips, from the Internet Archive
--     "vgmrips-all-of-them" item (one outer zip served member-by-member). Each
--     path is a full archive.org member URL of a game's inner .zip; the
--     ZIP-by-magic handler extracts the .vgz tracks and plays them as subsongs
--     (like Zophar). 3574 games kept after title-deduping the MegaDrive subset
--     vs modland Sega + Zophar (modland's "Video Game Music" is ~99% Sega, so
--     the arcade / PC-98 / PC-88 / X68000 / FM Towns / TurboGrafx / Neo Geo /
--     NES / GameBoy / WonderSwan / ... platforms are all net-new). Built by
--     chipmachine/scripts/build_vgmrips.py from data/misc/vgmrips/files.csv. ROUTING: VGM is a
--     multi-chip container and GME's Vgm_Emu only decodes the Sega/AY logs
--     (SN76489/YM2413/YM2612/AY8910); the chip gate in vgm_opl_detect.h
--     (vgmNeedsLibVGM, superseding vgmHasOPL) now routes every other chip
--     (OPL, YM2151, the OPN family, HuC6280, NES APU, GameBoy, C140, QSound,
--     K053260/K054539, SegaPCM, OKIM..., WonderSwan, ...) to libvgmplugin.
--     Screenshots: each game's sibling .png member, keyed by the song zip URL
--     in data/vgmrips_screenshots.txt (getSongScreenshots offline path, like
--     Zophar; 3314/4145 matched). Bump forces a reindex.
-- 79: New top-level "Arcade" platform (TAB filter), split out of "Other
--     Platforms". The six "arcade" / "arcade (capcom|konami|namco|sega|taito)"
--     format strings now classify to the new ARCADE format byte instead of
--     OTHER; the sub-platform drill (buildSubPlatforms) is byte-parametrized so
--     Arcade browses its sub-boards exactly like Other Platforms does. Neo Geo /
--     pinball stay under Other Platforms. Reindex reclassifies the arcade songs.
-- 80: Fold "Atari Jaguar" into the Atari filter (ATARI byte; filter renamed
--     "Atari ST/STE/Falcon") and "Neo Geo" into the Arcade filter (ARCADE
--     byte, shown as the "Arcade (Neo Geo)" drill group). Neo Geo Pocket /
--     pinball stay under Other Platforms. Reindex reclassifies those songs.
-- 81: zxtunes.com collection (~7k net-new ZX Spectrum AY tunes). Built by
--     chipmachine/scripts/build_zxtunes.py from the zxtunes.com XML API (xml.php). Same AY
--     tracker formats as zxart (pt3/pt2/stc/stp/asc/... -> ayfly, .ay -> gme),
--     so it reuses the "Spectrum AY" -> ZXAY mapping; no new decoder/enum. Each
--     path is an extensionless downloads.php?id= URL (source="", ext column
--     routes it, like amp). Deduped title-stem vs zxart/modland/projectay/
--     modarchive (16.6k dropped as already-present). Reindex adds the songs.
-- 82: zxtunes fix -- store the BARE track id in `path` (source now prepends
--     downloads.php?id=) instead of the full URL. A full URL made
--     path_extension() deduce the bogus ext "php?id=N", which clobbered the
--     Content-Disposition/`ext`-column module ext and broke playback (format
--     shown as "Spectrum AY (php?id=N)"). Dedup is the strict
--     {title,composer,format} triple. Reindex rewrites the zxtunes paths.
-- 83: modarchive title cleanup -- the title column is the upstream
--     marchive-open-db composite "<filename>//<realtitle>"; the filename half
--     is dead weight (playback routes on the `ext` column + Content-Disposition,
--     nothing reads it) and it uglified the GUI + poisoned {title,composer,
--     format} dedup. parseStandard now keeps only the text after the first "//"
--     for id=="modarchive", and drops a trailing ".<ext>" for the ~5% of rows
--     whose uploader reused the filename as the title ("1394.it//1394.it" ->
--     "1394"). Reindex rewrites the modarchive titles.
-- 84: SMS Power! collection (smspower) -- the definitive Sega 8-bit VGM vault
--     (Master System / Game Gear / SG-1000 / ColecoVision, SN76489 PSG + some
--     YM2413 FM). 173 games kept after title-deduping vs modland's "Video Game
--     Music/Sega Master System" (184) + "Sega Game Gear" (33); SMS Power is far
--     more comprehensive, so mostly GG/SMS/SG-1000/Homebrew net-new. Each path is
--     a live smspower.org /uploads/Music/<game>-<console>.zip pack (source="",
--     browser-UA fetch passes Cloudflare); the ZIP-by-magic handler extracts the
--     .vgm tracks and plays them as subsongs, like Zophar/VGMRips. No new format:
--     GME's Vgm_Emu decodes SN76489 + YM2413. SMS/GG/SG-1000 -> SEGAMS; the 2
--     ColecoVision sets ride under OTHER (misc small consoles, not a Sega
--     platform). Catalog + screenshot existence read from the Wayback CDX
--     (no live crawl); built by chipmachine/scripts/build_smspower.py. Screenshots: each game's
--     sibling title-screen .png, keyed by the pack URL in
--     data/smspower_screenshots.txt (167/173). Bump forces a reindex.
-- 85: CPC-Power YM audiotheque FULL EXPANSION (51 -> 6429 tunes / 2537 games).
--     The site has no browsable music index, so the earlier build shipped only
--     the ~51 .ym files Wayback happened to capture. New approach: the Wayback
--     CDX cheaply enumerates the 2568 game fiches that have an onglet=zicym tab
--     (the expensive "which games have music" discovery), then we read just
--     those specific tabs from the LIVE site -- a targeted one-request-per-
--     music-game crawl, NOT the 20k blind sweep that was declined -- and parse
--     the JS liste_musique[] array for the .ym filenames + metadata. Song URLs
--     still point at the live cpc-power /YM/<name>.ym (LHA-wrapped, depacked by
--     stsoundplugin; no new format, same "Amstrad CPC" -> AMSTRAD route). Screen-
--     shots are now EXACT (fiche == detail num, handed to us for free by the
--     enumeration): 6337 shots for 2475/2522 games via the fiche=num endpoint,
--     replacing the old fragile difflib name-search (was 30/27). Rebuilt by
--     chipmachine/scripts/build_cpcpower.py (--build / --screenshots). Bump forces a reindex.
-- 86: mirsoft.info "World of Game MODs" collection (mirsoft) -- the tracker
--     modules used in games, one .zip per game (several .mod/.xm/.it/.s3m/.med
--     + info.txt). Spans many platforms (Amiga-dominant, then C64/PC/NES/SNES/
--     Mac/PlayStation/...) but by policy only mainstream tracker formats, so
--     NO new format: every module plays via OpenMPT/UADE and the ZIP-by-magic
--     subsong handler extracts them like zophar/vgmrips/smspower (Digital
--     Mugician .dmu now in that allow-list too; info.txt + the odd .mid/.dro/.mo3
--     are ignored). 1049 net-new games kept after platform-aware dedup: Amiga
--     names vs UnExoticA + modland "Video Game Music", C64 vs rko, CPC vs
--     cpcpower, consoles vs zophar/vgmrips/smspower, plus a byte (stem,size)
--     content match vs modland+amp. The 1601-game 2021 IA tarball is topped up
--     with 29 net-new games added to mirsoft since (fetched live via the site's
--     newest-additions delta, dates 2022-2025). CLASSIFIED BY GAME PLATFORM
--     (item 4): the `format`
--     column is a canonical platform label mapped in initFormats' mirsoft block
--     (Amiga->AMIGA, Commodore 64->SID, PC->PC, NES->NES, ...). SOURCE is the
--     Internet Archive item mirsoftJuly2021snapshot (a 982MB .tar.xz of the raw
--     gamemods/ tree), parsed offline by chipmachine/scripts/build_mirsoft.py; every fact
--     (platform/composer/format/tracks) comes from each game's info.txt -- no
--     live crawl. RUNTIME serves archive.org first (mirsoftJuly2021snapshot/
--     gamemods/<Game>.zip) with the live mirsoft host as fallback (generateIndex
--     mirsoft branch, like hvtc's Wayback+live). mirsoft hosts no screenshots,
--     so best-effort game shots are matched by name from sources we already ship
--     (gb64 for C64, Hall of Light/abime for Amiga, zophar/vgmrips/smspower/
--     cpcpower reuse) into data/mirsoft_screenshots.txt (274/1049), consumed by
--     the mirsoft branch of getSongScreenshots.
--     Bump forces a reindex.
-- 87: added Gyusyabu Retro PC radio station (data/radio.txt) -- gyusyabu.ddo.jp's
--     long-running SHOUTcast stream of NEC PC-98/PC-88 + Sharp X68000 game music
--     recorded from REAL hardware (PC-9821 + OPNA/YM2203). It is a live stream,
--     NOT a downloadable file archive (the site's MP3/ dir is 403 and the album
--     pages are just program info), so it onboards as one radio entry like The
--     Sid Station / VGM Radio -- direct 256kbps SHOUTcast MP3 at
--     http://gyusyabu.ddo.jp:8000/;stream.mp3 (plays via the radio::/ffmpeg
--     path; no new format). Station logo = the site's title.gif (stb decodes
--     GIF; STBI_NO_GIF is not set). Bump forces a reindex.
-- 88: chipmusic.org community chiptune archive (chipmusic collection) -- ~8637
--     tracks. Every track is served ONLY as a rendered MP3 on S3 (directly
--     hotlinkable, HTTP 206 range OK), no original module, so it's an MP3
--     STREAMING collection (ffmpeg playback, no new decode capability). The whole
--     catalog (title/artist/S3 MP3 URL) comes from ONE RSS request
--     (music/rss/feed.xml?limit=N); platform is classified per track from the
--     freeform tags read off each track page (a polite, cached, ~2 req/s crawl
--     that aborts on any Cloudflare 403-burst/429). Tag->platform: lsdj/gameboy
--     ->Game Boy, sid/c64->Commodore 64, nsf/famitracker->NES, ym/sndh->Atari ST,
--     amiga/protracker/ahx->Amiga, beeper/1bit->ZX Spectrum, impulse/schism->PC;
--     ~47% land on a real platform filter, the untagged remainder -> "Chipmusic"
--     -> the existing Unclassified MP3/OGG filter (format_map["chipmusic"]=MP3,
--     no new enum byte). Built by chipmachine/scripts/build_chipmusic.py (--rss /
--     --tags / --build). Bump forces a reindex.
-- 89: Battle of the Bits (battleofthebits.com) -- the live, active original-
--     composition chiptune community (2005-). 60165 net-new audio entries
--     onboarded from its public JSON API (on the .COM host; .org 404s for
--     /api). Playability is driven by the REAL file extension (from each
--     entry's view_url), NOT the API format_token (a compo/platform label):
--     only extensions one of our plugins actually decodes are kept -- nsf/it/
--     xm/mod/s3m/mptm (OpenMPT+GME), sid, spc, sap, gbs, hes, ay/pt3, vgm/vgz,
--     ftm/0cc/kftm (FamiTracker -- only .ftm actually plays; 0cc/kftm ingested for
--     completeness but playback SHELVED: tested, famitracker-cx SIGSEGVs on 0CC's
--     N163/5B channels, kftm is a different container -- needs an engine upgrade,
--     see botb-onboarding memory), dmf, sunvox, ptcop/pttune, org, bbsong,
--     ahx, sndh/ym, prg, s98,
--     a2m/rad/amd/dfm/snd (AdPlug), and the rendered remix/sample/vocal/allgear/
--     wildchip compos (mp3/ogg/wav/flac -> ffmpeg). Unplayable natives (renoise
--     .xrns, furnace .fur, klystrack .kt, mario paint .sho, raw ROMs .smc/.gb,
--     midi, tap, lgpt/snibbe/... ) are skipped -- BotB's mp3 PREVIEW endpoint is
--     login-gated, so there is no rendered fallback for those. NO dedup (BotB is
--     original battle output, not rips -- near-zero overlap with modland/
--     modarchive). Each path is a full STATIC, directly-hotlinkable
--     https://battleofthebits.com/disk/battle/<battle_id:08d>/<file> URL
--     (source=""; the API's EntryDonload/EntryPreview endpoints 403 anonymously,
--     but the raw disk asset is public), reconstructed from view_url + battle_id.
--     format column classifies to the TAB platform filter (build_botb.py); ext
--     column routes the decoder. Screenshots: each entry's battle cover art
--     (artwork.png / entry image), keyed by the song URL in
--     data/botb_screenshots.txt (~14.8k distinctive covers; the generic
--     One-Hour-Battle debris banners are dropped; getSongScreenshots botb
--     branch). New format_map labels adlib/
--     mptm/pxtone/vgm added in initFormats. Built by chipmachine/scripts/build_botb.py
--     (--crawl / --build). Bump forces a reindex.
-- 90: OverClocked ReMix (ocremix.org) -- the premier community archive of
--     *arranged* video-game music: ~5000 free downloadable MP3 remixes
--     (OCR00001..OCR050xx), each tied to a source game, its original composer
--     and the remixer. A rendered-MP3 STREAMING collection (ffmpeg, no new
--     decode capability), same class as amigaremix/chipmusic. ocremix.org
--     robots.txt disallows AI-training crawlers (ClaudeBot/GPTBot/CCBot) but has
--     no general block; with the user's explicit go-ahead, built by a POLITE,
--     THROTTLED (~2 req/s), browser-UA fetch of one detail page per remix
--     (chipmachine/scripts/build_ocremix.py --crawl/--build). Each path is a full STATIC,
--     directly-hotlinkable mirror URL
--     https://iterations.org/files/music/remixes/<file>.mp3 (source=""); the
--     filename uses an abbreviated game name (not derivable), so it comes from
--     the detail page's mp3 link. Clean OpenGraph tags drive extraction: og:title
--     -> game + remix title, og:description -> remixer, color-original ->
--     original game composer(s), og:image -> the source game's title-screen
--     image (screenshot). The `composer` column is "<remixer> / <original
--     composer(s)>" so a search for EITHER the arranger or the original game
--     composer surfaces the remix (e.g. "Nobuo Uematsu" -> 824 remixes).
--     `game` column = source game (searchable);
--     `format` column = the game's console (from the og:image system dir), which
--     classifies to the TAB platform filter (Super Nintendo/NES/Sega Genesis/
--     Playstation/N64/Arcade/...); unmapped/modern consoles -> MP3 (Unclassified).
--     Screenshots in data/ocremix_screenshots.txt keyed by the song URL
--     (getSongScreenshots ocremix branch). NO dedup (arranged remixes, a distinct
--     artifact type from every rip/module collection). Bump forces a reindex.
-- 91: keygenmusic (keygenmusic.tk/.org) -- the keygen/cracktro music scene, a
--     genuinely distinct subculture (tunes written for software keygens/cracks)
--     not otherwise in the DB. Original site dead (~2022); the full collection is
--     preserved on the Internet Archive item `keygen-music-2020-03-pack` (one
--     ~5.5k-file zip). 5388 net-new songs. Enumerated from ONE request to
--     archive.org's view_archive.php (reached via the stable /download/<item>/
--     <zip>/ redirect), which lists every inner file with its path AND byte size.
--     Each path is a STATIC archive.org zip-extraction URL
--     .../keygen-music-2020-03-pack.zip/<inner path> (source=""; same
--     archive.org-side extraction VGMRips uses, RemoteLoader follows the 302 and
--     plays the module directly). Playability + platform driven by the REAL ext
--     (xm/mod/it/s3m/mtm/mo3, sid/nsf/sap/spc, ahx, sc68/ym, v2m, d00/rad/hsc/amd,
--     fc13/fc14/bp, mp3/ogg/flac); .mid + junk skipped. Filename is
--     "<Artist> - <software title>.<ext>" -> composer=artist, title=software name.
--     Deduped vs modland (exact byte SIZE + normalized artist/stem, zero false
--     positives) and modarchive (filename/title) -> 35 dups dropped (keygen is a
--     distinct subculture; software-named titles rarely match, so net-new is
--     ~the whole set). NO screenshots (cracktro/keygen scene has no game art,
--     like AMP/chipmusic). Built by chipmachine/scripts/build_keygenmusic.py --build. Bump
--     forces a reindex.
-- 92: Syntax Error podcast artwork fixed. Its old collection artwork pointed at
--     archive.org services/img for a synthetic item id that does not exist, so it
--     only ever served a generic 160x110 placeholder. Repointed to the show's own
--     real logo banner (https://syntaxerror.nu/banner.gif). Bump forces a reindex
--     so the collection.artwork column is rewritten.
-- 93: Zophar's Domain FULL EXPANSION (Genesis pilot 335 -> 4463 games / 10
--     platforms). The other dev's data/misc/zophar.net/list.csv inventoried the
--     whole catalog (17712 games / 21 platforms). Ground truth (opening the
--     "(EMU).zophar.zip" of each) splits Zophar in two: SEQUENCED CHIP platforms
--     (homogeneous, all decodable) = NES nsf, SNES spc, Game Boy gbs, GBA
--     minigsf, DS 2sf, N64 usf, TurboGrafx-16 hes, Genesis/Master System vgm,
--     Game Gear sgc -- ONBOARDED; and a STREAMED-AUDIO tier (PS1/PS2/Saturn/
--     Dreamcast + all HD consoles) whose "originals" are recorded rips
--     (xa/asf/adx/at3/dsp/eam/genh/...), NOT chip -- HELD (only vgmstream could
--     play them; extensions logged to data/misc/zophar.net/unplayable_formats.txt
--     for a separate go/no-go). Deduped per platform vs the whole console corpus
--     (modland by ext + vgmrips + smspower + rsn + nsfe): modland is deep-but-
--     narrow per game while Zophar is game-complete, so most platforms are
--     substantially net-new (5493 corpus dups dropped). Reuses the existing
--     zip-by-magic subsong pipeline (songExt already covers every ext; shared
--     gsflib/usflib/2sflib extract alongside), NO new decoders. Screenshots from
--     the CSV's own image_url (thumbs_large). Built by the rewritten
--     chipmachine/scripts/build_zophar.py --build/--build-audio/--screenshots.
--     ALSO onboarded (--build-audio): the 126 Nintendo 3DS / Xbox 360 games whose
--     otherwise-streamed "(EMU)" zip carries an ffmpeg-playable ogg/wav/mp2/aac
--     member (per-game zip-central-dir scan; the zip handler's audioExt fallback
--     plays those and ignores the bcstm/xma/adx/... it can't). Both consoles have
--     no TAB filter -> classify to OTHER. 4589 rows total, 873 screenshots. Bump
--     forces a reindex.
-- 94: DEDUP REGRESSION FIX. Two bugs let recently-onboarded game-mod collections
--     (mirsoft especially) SHADOW iconic originals in search results -- e.g. Rob
--     Hubbard's C64 SIDs Delta / Monty on the Run / Commando / Sanxion were hidden
--     by mirsoft's Amiga MOD remixes. Nothing was ever deleted: both rows stay in
--     the DB; the loss was purely at search time. Root causes, both in
--     MusicDatabase::search's add_unique: (a) the dedup key used the COARSE
--     platform byte, so a mirsoft ".mod" mis-filed as "Commodore 64" (-> SID byte)
--     collided with the real ".sid"; (b) rows with an EMPTY composer were folded
--     together. Fix: add_unique now folds two rows only when {title, composer,
--     REAL module ext} all match AND composer is non-empty -- false negatives
--     (a visible duplicate) are always preferred over false positives (a hidden
--     song). A new per-song formatKey (interned real ext) carries the true format
--     into the index (serialized in read/writeIndex). ALSO: mirsoft is now
--     classified by the ACTUAL module format (mod-family -> Amiga, xm/it/s3m ->
--     PC) instead of the game's platform, so its remixes stop appearing under
--     Commodore 64 / NES / SNES filters (data/mirsoft.txt col3 relabelled;
--     build_mirsoft.py's format_label emits it on rebuild). ALSO NEW: a per-
--     collection `priority` field (this file) controls SEARCH PRECEDENCE -- the
--     order results surface AND which row wins a dedup fold. Higher = first;
--     default 0; negative sinks below the default mass. Stored in the collection
--     table's `priority` column, loaded every launch, applied by a stable_sort of
--     the title matches in MusicDatabase::search. Set here: hvsc = 100 (the
--     definitive SID archive), mirsoft/rko/amigaremix/ocremix = -100 (remixes/
--     arrangements). Tweak any collection by adding `priority = N`. Bump forces a
--     reindex.
-- 95: Zophar STREAMED TIER onboarded (the held set from v93, unblocked by the new
--     vgmstream plugin + its FFmpeg backend for ATRAC .at3/.oma). 7056 net-new
--     recorded-rip games across 11 consoles APPENDED into the same "Zophar"
--     collection (data/zophar.txt, one build_zophar.py --build now emits both
--     tiers): PS1 xa/str, PS2 asf/genh/aus/eam/ss2/musx, Saturn dvi, Dreamcast
--     adx/spsd, Xbox lwav/asf, Xbox360 lwav/wav, Wii dsp, GameCube dsp/eam, 3DS
--     bcwav/ogg, PS3 at3, PSP at3/oma. Deduped vs modland's PS1/PS2/Saturn/
--     Dreamcast Sound-Format dirs (game-title match); the rest are net-new. The
--     zip-by-magic handler now lists the vgmstream exts in songExt so the members
--     play as subsongs. SEVEN new platform bytes + TAB filters + logos: Nintendo
--     3DS / GameCube / Wii, Sony PS3 / PSP, Microsoft Xbox / Xbox 360 (formerly all
--     OTHER or folded into PlayStation); .musx stays content-routed (Archimedes
--     "MUSX" magic vs PS2 vgmstream). Logos to add under data/misc/
--     platformscreenshots/: "Nintendo 3DS.png", "Nintendo GameCube.png",
--     "Nintendo Wii.png", "PlayStation 3.png", "PlayStation Portable.png",
--     "Xbox.png", "Xbox 360.png". Bump forces a reindex.
-- 96: The single APPLE format byte (which fronted Apple IIGS + Macintosh + Mac OS
--     + iOS) is split into four bytes so the TAB "Apple" group can drill into all
--     four sub-platforms like Nintendo/Sony/Sega/Microsoft. APPLE now means Apple
--     IIGS only ("apple ii*"/soundsmith); new APPLEMAC (Macintosh classic /
--     PlayerPRO .mad), MACOS ("macos"/"macosx *"), IOS ("ios"). classifyFormat
--     reassigns these format strings, so their songs must be re-stamped -> the
--     bump forces a reindex. Optional logos under data/misc/platformscreenshots/:
--     "Original Apple Mac.png", "Mac OS.png", "iOS.png".
-- 97: `priority` is now set on EVERY collection (was: 4 of them), giving search
--     results a deliberate house order instead of DB-declaration order. Data-only
--     change -- no engine change; the existing stable_sort in
--     MusicDatabase::search already reads the column. The tiers, high to low:
--       100  manual patch, HVSC, Modland -- the definitive/corrective sources,
--            they must win every dedup fold.
--        95  single-chip native archives: sndh, asma, csdb, hvtc, bitworld,
--            unexotica, vampi, zxart, zxtunes, cpcpower, projectay, oplarchive,
--            nsfe, smspower.
--        90  modarchive, amp -- broad tracker mirrors that overlap Modland
--            heavily; below it so Modland (better metadata) wins the fold.
--        80  botb, chipmusic, keygenmusic -- original chip works, but community/
--            rendered rather than archival.
--        60  playlists.
--        50  mirsoft -- real game module rips, thin metadata (was -100; the v94
--            add_unique fix on {title, composer, real ext} is what actually
--            stopped it shadowing HVSC, so the -100 sledgehammer is no longer
--            needed -- but it stays under the archives).
--        40  video game music: gb64, rsn, vgmrips.
--        30  demoscene: sceneorg, demozoo.
--       -50  remixes / rendered MP3 / streamed: amigaremix, ocremix, rko,
--            scenesat, pouet, zophar.
--       -80  podcasts (all nine).
--      -100  radio streams.
--     Bump forces a reindex.
-- 98: Collection `source` URLs moved off ftp:// and http:// onto https:// where
--     the host actually serves it. Two motivations:
--       * Native: Modland was the last big ftp:// consumer. Mixing FTP and
--         HTTPS in one libcurl session is what triggers the HTTP/2 connection-
--         prune heap corruption (worked around by pinning CURLOPT_HTTP_VERSION),
--         and IGNORE_CONTENT_LENGTH is an FTP-only flag that stalls HTTP
--         downloads. Retiring FTP here removes the mix at its source.
--       * Web (WASM target): browsers dropped ftp:// entirely and hard-block
--         http:// from an https page, so only https sources are reachable at
--         all. Of these, modland/asma/amigaremix also send
--         `Access-Control-Allow-Origin: *`, so they are CORS-clean.
--     Switched (each verified against a real path from the song list -- 200/206
--     + Range): modland (ftp -> https://modland.com/pub/modules/), asma,
--     amigaremix, amp, sndh, rko, rsn, hvtc.
--     Deliberately NOT switched:
--       * unexotica -- stays ftp://. files.exotica.org.uk answers https on the
--         root but 404s the UnExoticA tree (the same path FTP serves fine), so
--         the https mirror is not path-compatible. Switching would break it.
--       * mirsoft, syntax -- hosts speak no TLS at all (connect fails), so
--         there is no https to move to. Both are therefore unreachable from a
--         browser regardless; native keeps working over http://.
--     The url lives in collection.url, and initDatabase() early-returns for a
--     collection that already exists -- so editing `source` does nothing until
--     the collection table is rebuilt. Hence the bump: it forces the reindex
--     that repopulates the urls. Data-only change; no engine change.
-- 99: rko (remix.kwed.org) and amigaremix now declare `ext = "mp3"`. Their paths
--     carry no extension (rko: a bare remix id, all 5217 rows; amigaremix: 3 rows
--     whose upstream url lost its ".mp3"), and neither list has an ext column, so
--     internExt had nothing to intern: formatKey came out 0 and add_unique's
--     "unknown format -> keep both" guard meant those rows could never fold
--     against the same title+composer elsewhere -- the duplicate stayed visible in
--     search. Engine change: parseStandard now honours a collection-level `ext` as
--     the fallback when the template has no ext column, exactly as it already does
--     for `format`/`composer`. Bump forces the reindex that repopulates song.ext.
--     A survey of the other 40 collections found the remaining formatKey==0 rows
--     are correct-as-is and deliberately left alone: pouet/manualpatch/radio
--     (32538) are youtube + stream urls with no file to extension; modland (310,
--     e.g. "SoundSmith/A.Fass/Coconut Champagne") and unexotica (6, filenames
--     ending in a dot) are genuinely extensionless and span too many formats for
--     any one default. None of them currently blocks a real fold.
-- 100: VGMRips "Other" bucket split into real platforms, and a new
--     "Nintendo Virtual Boy" platform on the TAB screen (9th under Nintendo).
--     VGMRips files by chip family, not vendor, so 44 game rips carried the bare
--     "Other" platform label and piled into the "Other Platforms" -> "Other"
--     catch-all, even though each names its hardware in the filename's trailing
--     parenthetical ("Bound High! (Nintendo Virtual Boy)"). build_vgmrips.py
--     already had TAG_PLATFORM for exactly this refinement -- it was just missing
--     the entries. Added: Nintendo Virtual Boy (17), Vectrex (15 -- these now
--     join the existing Vectrex sub-group rather than sitting in the catch-all),
--     Amstrad CPC + CPC+ (5), Atari 5200/400/800 -> Atari 8bit (3), Sega Game
--     1000 -> Sega SG-1000 (2), Atari 7800 (1), Intellivision (1). The bare
--     "Other" label is now unused by VGMRips.
--     Virtual Boy is a new format byte (VIRTUALBOY): its VSU logs decode only on
--     libvgm (vgm_opl_detect.h already routes chip 0xC4 there; testmus/libvgm/
--     virtualboy-vsu.vgz covers it), and pouet's 2 "Youtube (Virtual Boy)"
--     captures move off OTHER onto it too. Logo:
--     data/misc/platformscreenshots/Nintendo Virtual Boy.png.
--     Data-only reindex + one new platform byte; bump repopulates song.format.
--     NOTE: the rebuild also drops 17 Mega Drive rips that Zophar has since
--     onboarded -- the existing modland/Zophar dedup working on fresher data,
--     unrelated to the platform split.
-- 101: modland's "Capcom Q-Sound Format" (8 .miniqsf rips) moves from "Other
--     Platforms" to Arcade, folded into the existing "Arcade (Capcom)" group
--     rather than forming a second Capcom row. QSound IS Capcom's CPS-1/CPS-2
--     arcade sound hardware and all 8 are CPS board games (Street Fighter Alpha
--     2, 19XX, Cadillacs & Dinosaurs, The Punisher, Warriors of Fate, both
--     Mega Man arcade titles, Slam Masters), so "Other" was simply wrong.
--     Two parts, because the drill names groups by the raw format string:
--     format_map moves the label OTHER -> ARCADE, and buildSubPlatforms renames
--     it to "Arcade (Capcom)" (the same hook that already makes "Arcade" ->
--     "Arcade (Other)" and "Neo Geo" -> "Arcade (Neo Geo)").
--     The Capcom logo is unaffected: consoleSubLogos keys on the format string,
--     not the platform byte. Bump forces the reindex that rewrites the byte.
-- 102: demozoo's "Sony Playstation Portable (PSP)" MP3 tag now resolves to the
--     PSP platform (TAB: Sony -> Sony PSP) instead of "Other Platforms". The
--     MP3/OGG rescue map already resolved the identically-shaped sibling tags
--     ("Nintendo DS (NDS)" -> NDS, "Nintendo Game Boy Advance (GBA)" -> GBA),
--     so PSP -> OTHER was simply an oversight -- the PSP byte and its filter
--     already existed. One tune (Vowel-o, silvester21). Only MP3/OGG rows hit
--     that map, which is why the ~130 other demozoo "MS-Dos"/"Linux" rows were
--     never affected: they classify by extension (.xm/.s3m/.mod/.zip) instead.
-- 103: the top-level "YouTube audio (no platform)" filter is gone; its 1103
--     videos move into "Other Platforms". Consistency fix: 31512 of the 32615
--     YouTube captures ALREADY classified to a real platform byte (pouet's
--     "Youtube (<platform>)" tag -> platformNameToByte), so a separate top-level
--     YouTube bucket for the leftovers was the odd one out. The leftovers were
--     exactly three tags naming no hardware -- Animation/Video (1091, a rendered
--     video), mIRC (10, script art), Alambik (2, a dead multimedia player) --
--     which are the same kind of thing as Wild/JavaScript/PICO-8, already
--     mapped to OTHER. They now appear in the Other drill as their own
--     "Youtube (<tag>)" groups, next to the "Youtube (Wild)" (566) /
--     "Youtube (JavaScript)" (415) groups that were always there.
--     There is NO platform to recover for them: pouet names none. Combos still
--     resolve to the hardware ("Youtube (Amiga AGA,Animation/Video)" -> Amiga).
--     formatToByte's youtube fallback also changes YOUTUBE -> OTHER, so a future
--     unrecognised pouet tag surfaces as a named group in the drill instead of
--     carrying a byte that no filter matches (which would hide it entirely).
--     The YOUTUBE byte is now never produced; kept in the enum (removing it
--     would renumber every byte after it) but matched by no filter.
-- 104: format_map now knows "macos"/"macosx intel"/"macosx ppc" -> MACOS and
--     "ios" -> IOS. Those bytes were previously reachable ONLY via the YouTube
--     path (platformNameToByte), so the TAB "Mac OS" filter held 72 YouTube
--     videos and NONE of the 9 native macOS tunes (demozoo/scene.org
--     executable-music .zip compo entries) -- those fell through to the
--     extension fallback, where ".zip" keys nothing, so they reached no filter
--     at all. Mac OS: 72 -> 81, and the 9 natives are findable.
-- 105: the rest of the same asymmetry, found by auditing every
--     platformNameToByte name against format_map (74 names known to the YouTube
--     path were missing from format_map; these are the ones with real native
--     rows). format_map now knows: windows/ms-dos/ms-dos/gus/linux -> PC,
--     commodore plus/4 -> PRG, and atari lynx / commodore pet / commodore
--     vic-20 / pico-8 / tic-80 / microw8 / raspberry pi / browser / calculator /
--     custom hardware -> OTHER. "atari lynx" also corrects a MISFILE: the
--     startsWith(f,"atari") fallback had been claiming it for the Atari ST
--     filter. Songs reaching no platform filter: 18493 -> 18213 (the remainder
--     is the separate "Demoscene" issue below).
--     Dropped from the MP3/OGG rescue map the names format_map now resolves
--     (windows/ms-dos/linux/custom hardware) -- format_map is consulted first,
--     so they were unreachable there. Kept the names format_map does NOT know
--     (amiga ppc/rtg, gba/nds/psp, mobile, gamepark gp2x): gating those on
--     MP3/OGG stops them claiming a module that carries the same release tag.
--     NOT fixed: 18209 rows tagged "Demoscene" (16884 .zip) still reach no
--     filter -- demozoo records no platform for those productions and none is
--     recoverable offline (checked: song.metadata empty, ext="zip",
--     data/misc/demozoo-songs.md's platform column is blank for exactly these).
--     Deciding them needs the archive's member list; see cmtest
--     unclassified_songs for the live count.
VERSION = 105;

DB = {
{
	-- mirsoft.info "World of Game MODs" (id=mirsoft): game tracker modules, one
	-- .zip per game. source = live mirsoft base; generateIndex's mirsoft branch
	-- registers archive.org as PRIMARY and this live host as FALLBACK. The path
	-- column is the URL-encoded "<Game>.zip"; the ZIP-by-magic handler extracts
	-- the .mod/.xm/.it/.s3m/.med members as subsongs. Classified by the ACTUAL
	-- module format (mod-family -> Amiga, xm/it/s3m -> PC) via the `format`
	-- column, NOT the game's platform -- see VERSION 94 (classifying by game
	-- platform made these remixes shadow the real single-chip originals). Built
	-- offline from
	-- the Internet Archive mirsoftJuly2021snapshot .tar.xz by chipmachine/scripts/build_mirsoft.py.
	name = "World of Game MODs",
	id =  "mirsoft",
	priority = 50,  -- game tracker modules: real rips, but thin metadata -> below the archives
	source = "http://mirsoft.info/gamemods/",
	song_list = "data/mirsoft.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "unexotica",
	id =  "unexotica",
	priority = 95,  -- native Amiga game chip rips
	source = "ftp://files.exotica.org.uk/pub/exotica/media/audio/UnExoticA",
	song_list = "data/unexotica.txt",
        song_template = "title game format composer path",
	color = 0xfffff
},
{
	-- Vampi's MDX Database (Sharp X68000). Net-new-vs-modland subset; path is a
	-- full mdx.vampi.tech/data/<file> URL (source empty). Plays via mdxplugin.
	name = "Vampi MDX",
	id =  "vampi",
	priority = 95,  -- native X68000 MDX chip archive
	source = "",
	song_list = "data/vampi.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- Zophar's Domain console gamerips, 10 sequenced-chip platforms (see
	-- VERSION 93): NES/SNES/Game Boy/GBA/DS/N64/TG16/Genesis/Master System/Game
	-- Gear. Each path is a full "(EMU).zophar.zip" URL on the fi.zophar.net CDN;
	-- the zip-by-magic handler extracts it and plays the chip tracks (nsf/spc/
	-- gbs/gsf/2sf/usf/hes/vgm/sgc) as subsongs, shared *lib members alongside.
	-- The format column carries the per-platform label (classified via
	-- MusicDatabase format_map). source empty (full URLs in the path column).
	name = "Zophar",
	id =  "zophar",
	priority = -50,  -- mostly recorded rips (streamed tier) -> sink below the native chip sources
	source = "",
	song_list = "data/zophar.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- VGMRips (vgmrips.net) game rips via the Internet Archive
	-- "vgmrips-all-of-them" item. Each path is a full archive.org member URL of
	-- a game's inner .zip (source empty); the ZIP-by-magic handler extracts the
	-- .vgz tracks and plays them as subsongs. Non-Sega chips route to
	-- libvgmplugin (see vgm_opl_detect.h). Screenshots in
	-- data/vgmrips_screenshots.txt keyed by the song zip URL. Built by
	-- chipmachine/scripts/build_vgmrips.py.
	name = "VGMRips",
	id =  "vgmrips",
	priority = 40,  -- video game music
	source = "",
	song_list = "data/vgmrips.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- SMS Power! (smspower.org) -- definitive Sega 8-bit (Master System / Game
	-- Gear / SG-1000 / ColecoVision) VGM vault. Each path is a live
	-- /uploads/Music/<game>-<console>.zip pack (source empty, full URL in path);
	-- the ZIP-by-magic handler extracts the .vgm tracks and plays them as
	-- subsongs. SN76489 PSG (+ YM2413 FM) -> GME Vgm_Emu, no new format. Deduped
	-- vs modland Sega Master System / Game Gear. Screenshots in
	-- data/smspower_screenshots.txt keyed by the pack URL. Built by
	-- chipmachine/scripts/build_smspower.py.
	name = "SMS Power",
	id =  "smspower",
	priority = 95,  -- native Sega 8-bit VGM rips
	source = "",
	song_list = "data/smspower.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- CPC-Power Amstrad CPC YM audiotheque (full set: ~6.4k tunes / 2.5k games;
	-- see VERSION 85). Full URLs in the path column point at the live cpc-power
	-- /YM/ files; source is empty.
	name = "CPC-Power",
	id =  "cpcpower",
	priority = 95,  -- native Amstrad CPC YM archive
	source = "",
	song_list = "data/cpcpower.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- The OPL Archive (opl.wafflenet.com). Non-game OPL2/OPL3 chiptunes; each
	-- path is a full URI-encoded wafflenet .vgz URL (source empty). Plays via
	-- libvgmplugin (GME can't decode OPL); format "OPL Archive" -> AdLib/OPL.
	name = "OPL Archive",
	id =  "oplarchive",
	priority = 95,  -- native OPL2/OPL3 rips
	source = "",
	song_list = "data/oplarchive.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- Demozoo (demozoo.org) net-new demoscene music. Each path is a full URL
	-- (media.demozoo.org / files.zxdemo.org / files.scene.org / Fujiology), so
	-- source is empty. scene.org compo entries are zips containing a lone module
	-- that MusicPlayerList extracts by magic. Screenshots in
	-- data/demozoo_screenshots.txt, keyed by the song URL (handled in
	-- MusicDatabase::getSongScreenshots, like zxart). Built by
	-- chipmachine/scripts/build_demozoo.py.
	name = "Demozoo",
	id =  "demozoo",
	priority = 30,  -- demoscene music
	source = "",
	song_list = "data/demozoo.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- scene.org tracker modules (Spanish scenesp.org mirror + party archive),
	-- the net-new-vs-modland/modarchive slice. Each path is a full
	-- archive.scene.org/pub/ URL (source empty); .zip rows hold a lone module
	-- extracted by magic. composer = party/artist provenance. Built by
	-- chipmachine/scripts/build_sceneorg.py from data/misc/scene.org/*.tsv.
	name = "scene.org",
	id =  "sceneorg",
	priority = 30,  -- demoscene music
	source = "",
	song_list = "data/sceneorg.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "modarchive",
	id =  "modarchive",
	priority = 90,  -- broad tracker mirror: overlaps modland, let modland win folds
	source = "https://api.modarchive.org/downloads.php?moduleid=",
	song_list = "data/modarchive.txt",
	song_template = "title ext path format",
	color = 0xfffff
},
{
	-- zxart.ee music: id-addressed like modarchive, but each row's `path` is a
	-- full URL (originals on zxart.ee/file/id:, ogg fallbacks on
	-- music.zxart.ee/music/), so `source` is empty and the URL is used verbatim.
	name = "zxart.ee",
	id =  "zxart",
	priority = 95,  -- native ZX Spectrum AY archive
	source = "",
	song_list = "data/zxart.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- zxtunes.com: net-new ZX Spectrum AY tunes, deduped on the app-wide
	-- {title,composer,format} triple vs zxart/projectay/amp. Same AY tracker
	-- formats we already play, so it reuses the "Spectrum AY" -> ZXAY mapping.
	-- Each `path` is a BARE track id; `source` prepends the downloads.php?id=
	-- endpoint (the amp/modarchive moduleid idiom). The path MUST stay
	-- extensionless -- a full ".../downloads.php?id=N" URL makes path_extension
	-- deduce the bogus ext "php?id=N" and breaks playback. The real module ext
	-- comes from the `ext` column (+ Content-Disposition). Built by
	-- chipmachine/scripts/build_zxtunes.py.
	name = "zxtunes.com",
	id =  "zxtunes",
	priority = 95,  -- native ZX Spectrum AY archive
	source = "https://zxtunes.com/downloads.php?id=",
	song_list = "data/zxtunes.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "Playlists",
	id =  "pl",
	priority = 60,  -- user playlists: above the bulk archives, below the canonical chip sources
	local_dir = "data/playlists",
	color = 0xfffff
},
{
	name = "HVSC",
	id =  "hvsc",
	-- priority: search precedence & dedup winner (higher surfaces first, default
	-- 0, may be negative to sink below the default mass). Every collection now
	-- carries one -- see the VERSION 97 note for the tier scheme. HVSC is the
	-- definitive C64 SID archive -> top tier.
	priority = 100,
	source = "https://www.hvsc.c64.org/download/C64Music/",
	song_list = "data/hvsc.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/hvsc.txt",
	color = 0xfffff
},
{
	name = "Gamebase64",
	id =  "gb64",
	priority = 40,  -- video game music
	prod_list = "data/Games.csv",
	-- gb64.com is now behind a Cloudflare JS/Turnstile challenge that returns 403
	-- to every non-browser client (no User-Agent or header trick gets past it).
	-- The Wayback Machine still mirrors the original screenshots and isn't gated;
	-- the "2id_" modifier serves the raw latest-snapshot image, following the
	-- internal redirect, so the existing "<prefix><L/Name.png>" URLs keep working.
	screen_source = "https://web.archive.org/web/2id_/http://www.gb64.com/Screenshots/",
	index = "no",
	color = 0xfffff
},
{
	name = "CSDb",
	id =  "csdb",
	priority = 95,  -- native C64 demoscene chip rips
	local_dir = "",
	prod_list = "data/csdb.xml",
	color = 0xfffff
},
{
	name = "Modland",
	id =  "modland",
	priority = 100,  -- the definitive Amiga/tracker archive
	source = "https://modland.com/pub/modules/",
	song_list = "data/allmods.txt",
	local_dir = "/opt/Music/MODLAND",
	exclude_formats = "RealSID;PlaySID;Nintendo SPC;SNDH;Slight Atari Player;Super Nintendo Sound Format",
	color = 0xfffff
},
{
	-- Amiga Music Preservation (amp.dascene.net). Full catalogue, deduped vs
	-- modland/modarchive/demozoo/scene.org/unexotica (net-new only). Each path
	-- is a bare module id; source appends it to the downmod.php endpoint, which
	-- 302-redirects to the gzip module. MusicPlayerList inflates the gzip body
	-- by magic, then the `ext` column routes it. Built by chipmachine/scripts/build_amp.py.
	name = "Amp",
	id =  "amp",
	priority = 90,  -- broad tracker mirror: overlaps modland, let modland win folds
	source = "https://amp.dascene.net/downmod.php?index=",
	song_list = "data/amp.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "Bitworld",
	id =  "bitworld",
	priority = 95,  -- native Amiga demoscene chip rips
	prod_list = "data/bitworld.txt",
	screen_source = "http://kestra.exotica.org.uk/files/screenies/",
	color = 0xfffff
},
{
	name = "HVTC",
	id =  "hvtc",
	priority = 95,  -- native Plus/4 chip archive
	source = "https://plus4world.powweb.com/feat/tedsound/hvtc/",
	song_list = "data/hvtc.txt",
	-- this one has local files! (like nsfe -> music/Console)
	local_dir = "music/hvtc",
	color = 0xfffff
 },
 {
	-- Project AY / AY-EMUL (Sergey Bulba). Raw Z80 machine-code music rips in
	-- the ZXAYEMUL (.ay) container: Ironfist's ZX Spectrum game rips + Bulba's
	-- own rips ("Spectrum AY" -> ZX AY filter) and SoLO/CORPSE's Amstrad CPC
	-- demo rips ("Amstrad CPC" -> CPC filter). Shipped LOCAL (like nsfe/hvtc):
	-- Bulba only offers big archives and Ironfist's site is gone. Built by
	-- chipmachine/scripts/build_projectay.py from the embedded AY metadata; .ay plays via
	-- gmeplugin (Ayfly renders CPC rips silent). source empty (local files).
	name = "Project AY",
	id =  "projectay",
	priority = 95,  -- native ZX/CPC AY rips
	source = "",
	song_list = "data/projectay.txt",
	song_template = "title composer format path ext",
	local_dir = "music/projectay",
	color = 0xfffff
 },
 {
	name = "snesmusic.org",
	id =  "rsn",
	priority = 40,  -- video game music
	source = "https://snesmusic.org/v2/download.php?spcNow=",
	make_source = snes,
	song_list = "data/rsn.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rsn.txt",
	local_dir = "/opt/Music/spcsets",
	color = 0xfffff
},
{
	name = "sndh",
	id =  "sndh",
	priority = 95,  -- native Atari ST chip archive
	source = "https://sndh.atari.org/sndh/sndh_lf/",
	song_list = "data/sndh.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/sndh.txt",
	local_dir = "/opt/Music/sndh_lf",
	color = 0xfffff
},
{
	name = "asma",
	id =  "asma",
	priority = 95,  -- native Atari 8-bit POKEY archive
	source = "https://asma.atari.org/asma/",
	song_list = "data/asma.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/asma.txt",
	local_dir = "/opt/Music/asma",
	color = 0xfffff
},
{
	name = "remix.kwed.org",
	id =  "rko",
	priority = -50,  -- Remix.Kwed.Org: C64 SID remixes -> sink below HVSC
	source = "https://remix.kwed.org/download.php/",
	song_list = "data/rko.txt",
	utf8 = "no",
	song_template = "path sidname sidsong title composer rating",
	format = "MP3",
	-- Every rko path is a bare remix id ("5136") with no extension, and rko.txt
	-- carries no ext column, so internExt() had nothing to intern: formatKey came
	-- out 0 and add_unique's "unknown format -> keep both" guard meant no rko song
	-- could EVER fold against its twin elsewhere (all 5217 rows). Declared as a
	-- collection-level default rather than a 7th rko.txt column on purpose: the
	-- list is re-pulled from `remote_list` upstream, which has 6 columns, and
	-- parseStandard drops any row with fewer fields than the template -- a 7-token
	-- template would silently delete the whole collection on the next refresh.
	-- Verified by fetch: all sampled ids return ID3/MPEG-frame magic.
	ext = "mp3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rko.txt",
	local_dir = "/opt/Music/rko",
	color = 0xfffff
},
{
	name = "amigaremix",
	id =  "amigaremix",
	priority = -50,  -- Amiga remixes -> sink below originals
	source = "https://www.amigaremix.com/listen/",
	song_list = "data/amiremix.txt",
	song_template = "no path title composer",
	format = "MP3",
	-- Same formatKey==0 gap as rko, but from bad data rather than by design: 3 of
	-- the 1711 upstream rows lost the ".mp3" off their url (amiremix.txt:1451
	-- ".../soundspawner_-_-_amigaremix_03442"), so path_extension found nothing to
	-- intern and those 3 could never fold. They still PLAY -- the host routes on
	-- the slug and answers 206 audio/mpeg with or without the suffix -- so this is
	-- a dedup fix, not a playback one. No-op for the other 1708, which already
	-- derive "mp3" from the path; every row in the set is an mp3.
	ext = "mp3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/amiremix.txt",
	local_dir = "/opt/Music/amiremix",
	color = 0xfffff
},
{
	name = "scenesat",
	id =  "scenesat",
	priority = -50,  -- rendered/streamed audio -> sink below the native chip sources
	source = "https://static.scenesat.com/",
	song_list = "data/scenesat.txt",
	song_template = "composer game title format path",
	local_dir = "/opt/Music/scenesat",
	color = 0xfffff
},
{
	-- chipmusic.org community chiptune archive (see VERSION 87). Rendered-MP3
	-- streaming collection: path = full hotlinkable chipmusic.s3.amazonaws.com
	-- .mp3 URL (source=""), plays via ffmpeg. The `format` column is the platform
	-- classified from each track's tags (Game Boy/Commodore 64/NES/Atari ST/Amiga/
	-- ZX Spectrum/PC), routed to those TAB filters; untagged -> "Chipmusic" ->
	-- Unclassified MP3/OGG. Built by chipmachine/scripts/build_chipmusic.py.
	name = "Chipmusic",
	id =  "chipmusic",
	priority = 80,  -- community chiptune, rendered MP3 rather than the native module
	source = "",
	song_list = "data/chipmusic.txt",
	song_template = "title composer format path",
	color = 0xfffff
},
{
	-- Battle of the Bits (battleofthebits.com) -- original-composition chiptune
	-- community (see VERSION 89). Each path is a full STATIC
	-- https://battleofthebits.com/disk/battle/<id>/<file> URL (source=""); the
	-- `ext` column routes the decoder (native module/chip file, or a rendered
	-- mp3/ogg/wav compo -> ffmpeg), the `format` column the TAB platform filter.
	-- Screenshots = the entry's battle cover art in data/botb_screenshots.txt
	-- (getSongScreenshots botb branch). Built by scripts/build_botb.py.
	name = "Battle of the Bits",
	id =  "botb",
	priority = 80,  -- original community chiptunes, but composed-for-compo rather than archival
	source = "",
	song_list = "data/botb.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- OverClocked ReMix (ocremix.org) -- arranged video-game music, ~5000 free
	-- MP3 remixes (see VERSION 90). Each path is a full mirror
	-- iterations.org/files/music/remixes/<file>.mp3 URL (source=""), played via
	-- ffmpeg. `game` = source game (searchable), `composer` = remixer, `format`
	-- = the game's console -> TAB platform filter. Screenshots = the source
	-- game's title-screen image in data/ocremix_screenshots.txt
	-- (getSongScreenshots ocremix branch). Built by scripts/build_ocremix.py.
	name = "OC ReMix",
	id =  "ocremix",
	priority = -50,  -- OverClocked ReMix: arranged game-music remixes -> sink below originals
	source = "",
	song_list = "data/ocremix.txt",
	song_template = "title game composer format path",
	color = 0xfffff
},
{
	-- keygenmusic (keygen/cracktro scene) -- full collection preserved on the
	-- Internet Archive (see VERSION 91). Each path is a full archive.org
	-- zip-extraction URL .../keygen-music-2020-03-pack.zip/<inner path>
	-- (source=""); the `ext` column routes the native module/chip decoder.
	-- composer = artist, title = the cracked-software name. No screenshots.
	-- Built by scripts/build_keygenmusic.py.
	name = "keygenmusic",
	id =  "keygenmusic",
	priority = 80,  -- scene chip/tracker tunes, dupes modland/modarchive in part
	source = "",
	song_list = "data/keygenmusic.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},


-- this is dead
--{
--	name = "Bitjam",
--	id =  "bitjam",
--	type = "podcast",
--	source = "http://malus.exotica.org.uk/pub/",
--	remote_list = "http://www.bitfellas.org/podcast/podcast.xml",
--	color = 0xfffff,
--	screenshot = "http://www.bitfellas.org/e107_plugins/radio/images/bj_newlogo.jpg"
--},

{
	name = "Demovibes",
	id =  "demovibes",
	priority = -80,  -- podcast
	source = "https://www.demovibes.org/downloads/",
	song_list = "data/demovibes.txt",
	podcast = "yes",
	color = 0xfffff
},
{
	name = "Amigavibes",
	id =  "amigavibes",
	priority = -80,  -- podcast
	source = "https://stats.podcloud.fr/amigavibes/",
	song_list = "data/amigavibes.txt",
	song_template = "title format path",
	podcast = "yes",
	artwork = "https://uploads.podcloud.fr/uploads/covers/e6/6d/e66d7d3ce5a7b589acff2df7809904bcdcdfed7d.jpg",
	color = 0xfffff
},
{
	name = "Radio",
	id =  "radio",
	priority = -100,  -- live radio streams: last
	source = "",
	song_list = "data/radio.txt",
	color = 0xfffff
},
-- May 2026: Bitar site is dead -- removing these databases
--{
--	name = "Bitar till Kaffet (Live)",
--	id = "bitar",
--	type = "podcast",
--	source = "",
--	song_list = "data/bitar.xml",
--	remote_list = "http://www.bitartillkaffet.se/?feed=podcast",
--	color = 0xfffff
--},
--{
--	name = "Bitar till Kaffet (Archive)",
--	id =  "bitar2",
--	source = "http://www.bitartillkaffet.se/media/",
--	song_list = "data/bitar.txt",
--	color = 0xfffff
--},
--{
--	name = "This Week in Chiptune",
--	id = "weekchip",
--	type =  "podcast",
--	source = "",
--	presenter = "Dj CUTMAN",
--	song_list = "http://thisweekinchiptune.libsyn.com/rss",
--	color = 0xfffff
--},

-- this is dead
--{
--	name = "Gamewave Podcast",
--	id = "gamewave",
--	type = "podcast",
--	source = "",
--	song_list = "http://gamewave.yays.co/rss.xml",
--	color = 0xfffff
--},

{
	name = "Syntax Error",
	id =  "syntax",
	priority = -80,  -- podcast
	source = "http://se-ksd-01.files.syntaxerror.nu/mp3/",
	song_list = "data/syntax.txt",
	song_template = "path title",
	format = "MP3",
	podcast = "yes",
	-- The old archive.org "services/img/podcast_syntax-error-podcast_..." id is a
	-- synthetic item that does not exist (metadata returns {}), so it only served
	-- a generic 160x110 archive.org placeholder. Use the show's real logo banner
	-- from its own site instead.
	artwork = "https://syntaxerror.nu/banner.gif",
	-- presenter = "Sol"
	color = 0xfffff
},
{
        name = "NSFE",
        id = "nsfe",
        priority = 95,  -- native NES 2A03 rips
        source = "",
        song_list = "data/nsfe.txt",
        -- this one has local files!
        local_dir = "music/Console",
        song_template = "no title composer no path",
        format = "NSFE",
        color = 0xfffff
},
{
	name = "C64 Take-away",
	id = "takeaway",
	priority = -80,  -- podcast
	type =  "podcast",
	source = "",
	song_list = "data/c64takeaway.xml",
	remote_list = "https://c64takeaway.com/feed/",
	artwork = "https://c64takeaway.com/assets/C64Takeaway-banner-6581-1400x1400.png",
	color = 0xfffff
},
{
	name = "Completely Unnecessary Podcast",
	id = "cupodcast",
	priority = -80,  -- podcast
	type = "podcast",
	source = "",
	-- Full catalogue (395 eps, 2013-) snapshotted from the live Anchor feed;
	-- per-episode artwork comes from each item's <itunes:image>. Newer
	-- episodes are merged in at startup from remote_list (union by URL).
	song_list = "data/cupodcast.xml",
	remote_list = "https://anchor.fm/s/37360560/podcast/rss",
	artwork = "https://archive.org/services/img/completely-unnecessary-podcast-series",
	color = 0xfffff
},
{
	-- Chiptune music-mix show (Dj CUTMAN); ran 2013-2017, full archive.
	name = "This Week in Chiptune",
	id = "twic",
	priority = -80,  -- podcast
	type = "podcast",
	source = "",
	song_list = "data/twic.xml",
	remote_list = "http://thisweekinchiptune.libsyn.com/rss",
	artwork = "https://static.libsyn.com/p/assets/a/e/1/e/ae1e0001ed40227a/This-Week-In-Chiptune-Podcast-art-2015.jpg",
	color = 0xfffff
},
{
	-- Video game music podcast: discussion + tracks, with interviews.
	name = "Pixelated Audio",
	id = "pixelated",
	priority = -80,  -- podcast
	type = "podcast",
	source = "",
	song_list = "data/pixelated.xml",
	remote_list = "https://pixelatedaudio.com/feed/podcast",
	artwork = "http://www.pixelatedaudio.com/wp-content/uploads/2016/04/2016-PA-PodcastCover-final-boosted.jpg",
	color = 0xfffff
},
{
	-- KNGI Network VGM music show (original tracks, OSTs, chiptunes).
	name = "GameFuel",
	id = "gamefuel",
	priority = -80,  -- podcast
	type = "podcast",
	source = "",
	song_list = "data/gamefuel.xml",
	remote_list = "https://feeds.feedburner.com/GameFuel",
	artwork = "https://kngi.org/public_html/wp-content/uploads/GameFuelAlbum2016-1-1024x1024.png",
	color = 0xfffff
},
{
	-- KNGI Network video game music + remixes show.
	name = "Nitro Game Injection",
	id = "nitro",
	priority = -80,  -- podcast
	type = "podcast",
	source = "",
	song_list = "data/nitro.xml",
	remote_list = "https://feeds.feedburner.com/NitroGameInjection",
	artwork = "http://kngi.org/public_html/wp-content/uploads/powerpress/NGI2015AlbumArtiTunes1400-808.jpg",
	color = 0xfffff
},
{
	name = "Pouet/Youtube",
	id =  "pouet",
	priority = -50,  -- YouTube-backed demo audio -> sink below the native chip sources
	source = "",
	song_list = "data/pouet.txt",
	screen_source = "http://content.pouet.net/files/screenshots/",
	color = 0xfffff
},
{
	-- Hand-maintained extras. Each row is
	-- "name<TAB>platform<TAB>author<TAB>youtube_link<TAB>screenshot_link".
	-- Plays the YouTube link like Pouet; screenshot_link is a full URL stored
	-- verbatim per song (song_template `screenshot` keyword). source empty.
	name = "Manual Patch",
	id =  "manualpatch",
	priority = 100,  -- manual metadata corrections: must win every dedup fold
	source = "",
	song_list = "data/manualDatabasePatch.txt",
	song_template = "title format composer path screenshot info",
	color = 0xfffff
}
};
