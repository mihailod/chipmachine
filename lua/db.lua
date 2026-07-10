
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
-- 33: added Karolina radio station
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
--     show under the F9 Podcasts category alongside C64 Take-away / CU Podcast.
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
--     the Atari/console F9 filters. chipmachine/scripts/filter_demozoo_archives.py downloads the
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
-- 79: New top-level "Arcade" platform (F9 filter), split out of "Other
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
VERSION = 86;

DB = {
{
	-- mirsoft.info "World of Game MODs" (id=mirsoft): game tracker modules, one
	-- .zip per game. source = live mirsoft base; generateIndex's mirsoft branch
	-- registers archive.org as PRIMARY and this live host as FALLBACK. The path
	-- column is the URL-encoded "<Game>.zip"; the ZIP-by-magic handler extracts
	-- the .mod/.xm/.it/.s3m/.med members as subsongs. Classified by game platform
	-- via the `format` column (see initFormats mirsoft block). Built offline from
	-- the Internet Archive mirsoftJuly2021snapshot .tar.xz by chipmachine/scripts/build_mirsoft.py.
	name = "World of Game MODs",
	id =  "mirsoft",
	source = "http://mirsoft.info/gamemods/",
	song_list = "data/mirsoft.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "unexotica",
	id =  "unexotica",
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
	source = "",
	song_list = "data/vampi.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	-- Zophar's Domain (pilot: Sega Genesis VGM). Each path is a full EMU zip URL
	-- on the fi.zophar.net CDN; the host extracts it and plays the .vgm tracks as
	-- subsongs. source empty (full URLs in the path column).
	name = "Zophar",
	id =  "zophar",
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
	source = "",
	song_list = "data/sceneorg.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "modarchive",
	id =  "modarchive",
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
	source = "https://zxtunes.com/downloads.php?id=",
	song_list = "data/zxtunes.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "Playlists",
	id =  "pl",
	local_dir = "data/playlists",
	color = 0xfffff
},
{
	name = "HVSC",
	id =  "hvsc",
	source = "https://www.hvsc.c64.org/download/C64Music/",
	song_list = "data/hvsc.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/hvsc.txt",
	color = 0xfffff
},
{
	name = "Gamebase64",
	id =  "gb64",
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
	local_dir = "",
	prod_list = "data/csdb.xml",
	color = 0xfffff
},
{
	name = "Modland",
	id =  "modland",
	source = "ftp://ftp.modland.com/pub/modules/",
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
	source = "http://amp.dascene.net/downmod.php?index=",
	song_list = "data/amp.txt",
	song_template = "title composer format path ext",
	color = 0xfffff
},
{
	name = "Bitworld",
	id =  "bitworld",
	prod_list = "data/bitworld.txt",
	screen_source = "http://kestra.exotica.org.uk/files/screenies/",
	color = 0xfffff
},
{
	name = "HVTC",
	id =  "hvtc",
	source = "http://plus4world.powweb.com/feat/tedsound/hvtc/",
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
	source = "",
	song_list = "data/projectay.txt",
	song_template = "title composer format path ext",
	local_dir = "music/projectay",
	color = 0xfffff
 },
 {
	name = "snesmusic.org",
	id =  "rsn",
	source = "http://snesmusic.org/v2/download.php?spcNow=",
	make_source = snes,
	song_list = "data/rsn.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rsn.txt",
	local_dir = "/opt/Music/spcsets",
	color = 0xfffff
},
{
	name = "sndh",
	id =  "sndh",
	source = "http://sndh.atari.org/sndh/sndh_lf/",
	song_list = "data/sndh.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/sndh.txt",
	local_dir = "/opt/Music/sndh_lf",
	color = 0xfffff
},
{
	name = "asma",
	id =  "asma",
	source = "http://asma.atari.org/asma/",
	song_list = "data/asma.txt",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/asma.txt",
	local_dir = "/opt/Music/asma",
	color = 0xfffff
},
{
	name = "remix.kwed.org",
	id =  "rko",
	source = "http://remix.kwed.org/download.php/",
	song_list = "data/rko.txt",
	utf8 = "no",
	song_template = "path sidname sidsong title composer rating",
	format = "MP3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/rko.txt",
	local_dir = "/opt/Music/rko",
	color = 0xfffff
},
{
	name = "amigaremix",
	id =  "amigaremix",
	source = "http://amigaremix.com/listen/",
	song_list = "data/amiremix.txt",
	song_template = "no path title composer",
	format = "MP3",
	remote_list = "http://raw.githubusercontent.com/sasq64/cmds/master/amiremix.txt",
	local_dir = "/opt/Music/amiremix",
	color = 0xfffff
},
{
	name = "scenesat",
	id =  "scenesat",
	source = "https://static.scenesat.com/",
	song_list = "data/scenesat.txt",
	song_template = "composer game title format path",
	local_dir = "/opt/Music/scenesat",
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
	source = "https://www.demovibes.org/downloads/",
	song_list = "data/demovibes.txt",
	podcast = "yes",
	color = 0xfffff
},
{
	name = "Amigavibes",
	id =  "amigavibes",
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
	source = "http://se-ksd-01.files.syntaxerror.nu/mp3/",
	song_list = "data/syntax.txt",
	song_template = "path title",
	format = "MP3",
	podcast = "yes",
	artwork = "https://archive.org/services/img/podcast_syntax-error-podcast_1481271871",
	-- presenter = "Sol"
	color = 0xfffff
},
{
        name = "NSFE",
        id = "nsfe",
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
	source = "",
	song_list = "data/manualDatabasePatch.txt",
	song_template = "title format composer path screenshot info",
	color = 0xfffff
}
};
