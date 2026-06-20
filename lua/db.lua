
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
VERSION = 34;

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
	name = "modarchive",
	id =  "modarchive",
	source = "https://api.modarchive.org/downloads.php?moduleid=",
	song_list = "data/modarchive.txt",
	song_template = "title ext path format",
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
