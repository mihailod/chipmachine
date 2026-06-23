
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
--     tools/build_zxart.py from the zxart API; routes by chip type (AY ->
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
--     Built by tools/build_cpcpower.py; format "Amstrad CPC" -> AMSTRAD filter.
-- 56: Zophar's Domain pilot -- Sega Genesis VGM gamerips (the genuinely net-new
--     slice; modland already has SNES/GB/PS/N64/Saturn/NES/Dreamcast but NO VGM
--     and NO arcade). Each row's path is a "<game> (EMU).zophar.zip" of per-track
--     .vgm files; MusicPlayerList detects the ZIP by magic, extracts it, and plays
--     the tracks as local subsongs. Built by tools/build_zophar.py (dedup vs the
--     265 modland Megadrive GYM games). format "Sega Genesis" -> MEGADRIVE filter.
-- 57: zophar.txt deduped properly. v56 wrongly deduped Genesis only vs "Megadrive
--     GYM" (265) and missed modland's huge "Video Game Music/Sega Megadrive" tree
--     (10961 files / 621 games), so 480 of the 815 were dups. Now dedups vs the
--     Video Game Music Sega dirs -> 335 genuinely-new games (215 MD + 119 Sega CD
--     + 1 32X). Also fixed the libcurl HTTP/2 prune crash (web.cpp FORBID_REUSE)
--     and the ZipFile extractor (archive.cpp) that this collection first exposed.
VERSION = 57;

DB = {
{
	name = "unexotica",
	id =  "unexotica",
	source = "ftp://files.exotica.org.uk/pub/exotica/media/audio/UnExoticA",
	song_list = "data/unexotica.txt",
        song_template = "title game format composer path",
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
	-- CPC-Power Amstrad CPC YM audiotheque (Wayback-known subset). Full URLs in
	-- the path column point at the live cpc-power /YM/ files; source is empty.
	name = "CPC-Power",
	id =  "cpcpower",
	source = "",
	song_list = "data/cpcpower.txt",
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
	name = "Amp",
	id =  "amp",
	source = "http://amp.dascene.net/modules/",
	song_template = "path",
	index = "no",
	song_list = "data/amp.txt",
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
	local_dir = "/opt/Music/hvtc",
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
	color = 0xfffff
},
{
	name = "Amigavibes",
	id =  "amigavibes",
	source = "https://stats.podcloud.fr/amigavibes/",
	song_list = "data/amigavibes.txt",
	song_template = "title format path",
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
	color = 0xfffff
},
{
	name = "Pouet/Youtube",
	id =  "pouet",
	source = "",
	song_list = "data/pouet.txt",
	screen_source = "http://content.pouet.net/files/screenshots/",
	color = 0xfffff
}
};
