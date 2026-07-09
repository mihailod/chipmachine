#!/usr/bin/env python3
"""Build chipmachine/data/demozoo.txt (+ demozoo_screenshots.txt) from Demozoo.

Demozoo (demozoo.org) publishes a daily full Postgres dump:
    https://data.demozoo.org/demozoo-export.sql.gz   (~196 MB gz / ~860 MB sql)

We onboard only the genuinely-new demoscene *music* (supertype='music') that is
NOT already reachable through an existing chipmachine collection, plus per-tune
screenshots (the production's own screens on media.demozoo.org).

Relevant tables (column order taken from the dump's COPY headers):
  productions_production            (id, title, ..., supertype[8], ...)
  productions_productionlink        (id, link_class, parameter, production_id,
                                     is_download_link[5], ..., source)
  productions_production_platforms  (id, production_id, platform_id)
  platforms_platform                (id, name, ...)
  productions_credit                (id, production_id, nick_id, role, category[5])
  demoscene_nick                    (id, releaser_id, name[3], ...)
  productions_screenshot            (id, production_id, original_url[3], ow, oh,
                                     thumb_url, tw, th, standard_url[9], ...)

DEDUPE (the whole point -- only truly-new content):
  A music production is SKIPPED entirely if ANY of its download links is one we
  already mirror: a ModlandFile (we mirror all of modland) or a BaseUrl pointing
  at a host we already ship as its own DB (HVSC, sndh.atari.org, asma, AMP,
  csdb, zxart, cpc-power). That dedups at the *tune* level: if the same tune is
  on modland AND scene.org, we drop it because modland already has it.
  A secondary pass drops kept rows whose (title, composer) already exists in
  zxart.txt (zxdemo<->zxart overlap) and media.demozoo .sndh whose basename is
  already in sndh.txt.

KEEP buckets (user choice): files.zxdemo.org + media.demozoo.org (BaseUrl),
FujiologyFile, SceneOrgFile. Long-tail file-sharing hosts (Google Drive,
Discord, YouTube, mediafire, dropbox, bit.ly, github pages) are dropped, as are
links with no plausible audio/archive file extension.

Link-class URL resolution:
  BaseUrl        -> parameter verbatim (already a full URL)
  SceneOrgFile   -> https://archive.scene.org/pub<parameter>  (direct HTTPS 200;
                    the files.scene.org/get/ redirector 302s to plain http://)
  FujiologyFile  -> https://ftp.untergrund.net/users/ltk_tscc/fujiology<parameter>

Output row (song_template = "title composer format path ext"):
  <title>\t<composer>\t<platform-label>\t<url>\t<ext>
Screenshots (demozoo_screenshots.txt, keyed by the emitted url):
  <url>\t<screenshot-url>

Usage:
  python3 chipmachine/scripts/build_demozoo.py --download   # (re)fetch the dump to /tmp
  python3 chipmachine/scripts/build_demozoo.py --build      # parse + dedup + write data files
"""

import gzip
import os
import re
import sys
import urllib.parse
import urllib.request
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
DUMP_URL = "https://data.demozoo.org/demozoo-export.sql.gz"
DUMP = os.environ.get("DEMOZOO_DUMP",
                      "/tmp/demozoo-export.sql.gz")  # large; keep out of repo
OUT = os.path.join(DATA, "demozoo.txt")
OUT_SHOTS = os.path.join(DATA, "demozoo_screenshots.txt")

# Hosts we already ship as their own collection -> a BaseUrl at one of these
# means we already have the tune; skip the whole production.
COVERED_HOSTS = {
    "hvsc.csdb.dk", "hvsc.perff.dk", "hvsc.etv.cx", "www.hvsc.c64.org",
    "sndh.atari.org", "sndhrecord.atari.org",
    "asma.atari.org", "asma.scene.pl",
    "amp.dascene.net",
    "csdb.dk", "www.csdb.dk", "noname.c64.org",
    "zxart.ee", "music.zxart.ee",
    "www.cpc-power.com",
    "ftp.modland.com", "modland.com", "ftp.amigascne.org",
}

# The only BaseUrl hosts we ingest: the two curated, reliable single-file audio
# CDNs the user selected ("zxdemo + media.demozoo"). Everything else under
# BaseUrl is the long-tail the user chose not to ingest (party-archive mirrors,
# personal sites, and unreliable file-sharing/streaming hosts).
BASEURL_KEEP_HOSTS = {"media.demozoo.org", "files.zxdemo.org"}

# Extensions we will NOT treat as a playable file (pages, images, video, docs).
DENY_EXT = {
    "html", "htm", "php", "asp", "aspx", "cgi", "jsp", "pl",
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "ico",
    "pdf", "txt", "nfo", "diz", "doc", "rtf",
    "mp4", "webm", "avi", "mkv", "mov", "flv", "wmv", "m4v",
    "json", "xml", "css", "js",
}

# Demozoo platform name -> chipmachine filter/format label. Aligns with the
# strings existing collections already use (hvsc "Commodore 64", sndh/asma
# "Atari ST" / "Atari 8Bit", zxart "ZX Spectrum"), so new tunes land in the
# existing F9 filter categories instead of spawning near-empty ones.
PLATFORM_MAP = {
    "Commodore 64": "Commodore 64",
    "Commodore 64-DTV": "Commodore 64",
    "Commodore 128": "Commodore 64",
    "Commodore VIC-20": "Commodore VIC-20",
    "Commodore PET": "Commodore PET",
    "Commodore 16/Plus 4": "Commodore Plus/4",
    "Amiga OCS/ECS": "Amiga",
    "Amiga AGA": "Amiga",
    "Atari ST/E": "Atari ST",
    "Atari Falcon": "Atari ST",
    "Atari TT": "Atari ST",
    "Atari 8 bit": "Atari 8Bit",
    "ZX Spectrum": "ZX Spectrum",
    "ZX Spectrum Enhanced": "ZX Spectrum",
    "ZX81": "ZX Spectrum",
    "Amstrad CPC": "Amstrad CPC",
    "Amstrad Plus": "Amstrad CPC",
    "MSX": "MSX",
    "Sharp X68000": "MDX",
}

# Many demozoo "music" productions carry no platform (just a music file), so the
# platform join is blank. Infer a chip/format label from unambiguous file
# extensions; anything cross-platform (xm/it/s3m) or streamed (mp3/ogg/zip)
# falls through to the "Demoscene" catch-all bucket below.
EXT_FORMAT = {
    "sid": "Commodore 64", "psid": "Commodore 64",
    "sndh": "Atari ST", "sc68": "Atari ST", "ym": "Atari ST",
    "sap": "Atari 8Bit",
    "pt3": "ZX Spectrum", "pt2": "ZX Spectrum", "pt1": "ZX Spectrum",
    "sqt": "ZX Spectrum", "stc": "ZX Spectrum", "stp": "ZX Spectrum",
    "asc": "ZX Spectrum", "vtx": "ZX Spectrum", "psc": "ZX Spectrum",
    "ay": "ZX Spectrum", "tsd": "ZX Spectrum", "psg": "ZX Spectrum",
    "ahx": "Amiga", "hvl": "Amiga", "thx": "Amiga", "mod": "Amiga",
    "med": "Amiga", "mmd0": "Amiga", "mmd1": "Amiga", "mmd2": "Amiga",
    "mmd3": "Amiga", "dbm": "Amiga", "okt": "Amiga", "fc": "Amiga",
    "fc13": "Amiga", "fc14": "Amiga", "aon": "Amiga", "smod": "Amiga",
    "dw": "Amiga", "cust": "Amiga", "digi": "Amiga",
}
GENERIC_FORMAT = "Demoscene"


# ---------------------------------------------------------------------------
def download():
    sys.stderr.write(f"downloading {DUMP_URL} -> {DUMP}\n")
    req = urllib.request.Request(DUMP_URL, headers={"User-Agent":
                                                    "chipmachine-demozoo/1.0"})
    with urllib.request.urlopen(req, timeout=120) as r, open(DUMP, "wb") as f:
        while True:
            chunk = r.read(1 << 20)
            if not chunk:
                break
            f.write(chunk)
    sys.stderr.write(f"  {os.path.getsize(DUMP)} bytes\n")


# Postgres COPY text-format unescape (\t \n \r \\ ...). \N == SQL NULL.
_UNESC = {"t": "\t", "n": "\n", "r": "\r", "\\": "\\"}


def unesc(s):
    if s == r"\N":
        return None
    if "\\" not in s:
        return s
    out, i, n = [], 0, len(s)
    while i < n:
        c = s[i]
        if c == "\\" and i + 1 < n:
            out.append(_UNESC.get(s[i + 1], s[i + 1]))
            i += 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def iter_copy(table):
    """Yield each data row (list of raw, still-escaped fields) of one COPY block.

    Streams the gz once per call; the dump is read multiple times (a handful of
    tables) which is simpler and fast enough (~tens of seconds each)."""
    needle = f"COPY public.{table} "
    with gzip.open(DUMP, "rt", encoding="utf-8", errors="replace",
                   newline="\n") as f:
        grab = False
        for line in f:
            if not grab:
                if line.startswith(needle):
                    grab = True
                continue
            if line.startswith("\\."):
                return
            yield line.rstrip("\n").split("\t")


def host_of(url):
    m = re.match(r"[a-zA-Z][a-zA-Z0-9+.-]*://([^/]+)", url)
    return m.group(1).lower() if m else ""


def ext_of(url):
    path = urllib.parse.urlparse(url).path.rstrip("/")
    m = re.search(r"\.([A-Za-z0-9]{1,5})$", path)
    return m.group(1).lower() if m else ""


def norm(s):
    return re.sub(r"[^a-z0-9]", "", (s or "").lower())


# ---------------------------------------------------------------------------
def resolve(link_class, parameter):
    """(url, ext) for a kept link, or (None, None) to drop it."""
    if link_class == "BaseUrl":
        url = parameter
        h = host_of(url)
        if h in COVERED_HOSTS:
            return None, None
        # Only the two curated audio CDNs the user selected; every other BaseUrl
        # host is the "long-tail" bucket that was explicitly not chosen.
        if h not in BASEURL_KEEP_HOSTS:
            return None, None
    elif link_class == "SceneOrgFile":
        # Use the archive host that serves the file DIRECTLY over HTTPS (200, no
        # redirect). The obvious files.scene.org/get/ redirector 302s to a plain
        # http:// mirror (http.us.scene.org), and the in-app curl fails that
        # HTTPS->HTTP downgrade (RemoteLoader CODE -1 / hang).
        url = "https://archive.scene.org/pub" + parameter
    elif link_class == "FujiologyFile":
        url = "https://ftp.untergrund.net/users/ltk_tscc/fujiology" + parameter
    else:
        # ModlandFile handled as covered (skip prod); others (Untergrund/Padua/
        # Amigascne/Wayback) are tiny + the user didn't select them.
        return None, None

    # A handful of scene.org link parameters carry a stray trailing ';' (e.g.
    # ".../keith303_-_over&out.xrns;"). Left in, it both breaks the download URL
    # and -- because SongInfo treats "path;<digits>" as a subtune selector -- used
    # to feed stoi("") and abort the whole collection's indexing. Strip it here.
    url = url.rstrip(";")

    ext = ext_of(url)
    # Archives are playable (host extracts by magic); modules by extension.
    if not ext or ext in DENY_EXT:
        # media.demozoo / zxdemo are curated audio CDNs: keep even if the ext is
        # unusual, but never keep an explicitly-denied (page/image) ext.
        if ext in DENY_EXT:
            return None, None
        h = host_of(url)
        if h not in ("media.demozoo.org", "files.zxdemo.org"):
            return None, None
    return url, ext


def is_covered_link(link_class, parameter):
    if link_class == "ModlandFile":
        return True
    if link_class == "BaseUrl" and host_of(parameter) in COVERED_HOSTS:
        return True
    return False


# priority for choosing one link when a prod has several keepable ones
def link_priority(link_class, url):
    h = host_of(url)
    if h == "media.demozoo.org":
        return 0
    if h == "files.zxdemo.org":
        return 1
    if link_class == "FujiologyFile":
        return 2
    if link_class == "BaseUrl":
        return 3
    if link_class == "SceneOrgFile":
        return 4
    return 5


def load_dedup_sets():
    """(title,composer) keys from zxart.txt and basenames from sndh.txt."""
    zx = set()
    p = os.path.join(DATA, "zxart.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) >= 2:
                    zx.add((norm(c[0]), norm(c[1])))
    sndh = set()
    p = os.path.join(DATA, "sndh.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if c and c[-1]:
                    sndh.add(os.path.basename(c[-1]).lower())
    return zx, sndh


# ---------------------------------------------------------------------------
def build():
    # 1) music production ids + titles
    sys.stderr.write("pass 1/6 productions_production (music) ...\n")
    title = {}
    for r in iter_copy("productions_production"):
        if len(r) < 8:
            continue
        if r[7] == "music":                      # supertype col (0-based 7)
            title[r[0]] = unesc(r[1]) or ""
    sys.stderr.write(f"  music productions: {len(title)}\n")

    # 2) platform id -> name, then prod -> platform label
    sys.stderr.write("pass 2/6 platforms ...\n")
    pname = {}
    for r in iter_copy("platforms_platform"):
        if len(r) >= 2:
            pname[r[0]] = unesc(r[1]) or ""
    prod_plat = {}
    for r in iter_copy("productions_production_platforms"):
        if len(r) >= 3 and r[1] in title and r[1] not in prod_plat:
            prod_plat[r[1]] = pname.get(r[2], "")

    # 3) nick id -> name, then prod -> composer. For a standalone music tune the
    #    artist is the production's *byline* (author_nicks), not a role-credit
    #    (productions_credit's "Music" rows are overwhelmingly demos' musicians,
    #    whose production_id isn't in our music set). Use the byline, joining
    #    co-authors with " & "; fall back to a Music credit if there's no byline.
    sys.stderr.write("pass 3/6 credits/nicks ...\n")
    nick = {}
    for r in iter_copy("demoscene_nick"):
        if len(r) >= 3:
            nick[r[0]] = unesc(r[2]) or ""
    prod_authors = defaultdict(list)
    for r in iter_copy("productions_production_author_nicks"):
        if len(r) >= 3 and r[1] in title:
            nm = nick.get(r[2], "")
            if nm:
                prod_authors[r[1]].append(nm)
    prod_comp = {pid: " & ".join(ns) for pid, ns in prod_authors.items()}
    for r in iter_copy("productions_credit"):
        if len(r) >= 5 and r[1] in title and r[4] == "Music" \
                and r[1] not in prod_comp:
            nm = nick.get(r[2], "")
            if nm:
                prod_comp[r[1]] = nm

    # 4) screenshots (standard_url, fall back to original_url). Keep ALL
    #    productions' shots, not just the music ones: a standalone tune rarely has
    #    its own screenshot, but the demos that use it as soundtrack almost always
    #    do, and those demo prods are not in `title` (music-only).
    sys.stderr.write("pass 4/6 screenshots ...\n")
    prod_shot = {}
    for r in iter_copy("productions_screenshot"):
        if len(r) >= 9 and r[1] not in prod_shot:
            shot = unesc(r[8]) or unesc(r[2]) or ""
            if shot:
                prod_shot[r[1]] = shot
    # music tune -> the demos that use it (those carry the visual); same source
    # the augment() pass uses for our other collections.
    music_to_demos = defaultdict(list)
    for r in iter_copy("productions_soundtracklink"):
        if len(r) >= 3:
            music_to_demos[r[2]].append(r[1])

    # 5) download links grouped per production
    sys.stderr.write("pass 5/6 production links ...\n")
    links = defaultdict(list)            # prod_id -> [(link_class, parameter)]
    for r in iter_copy("productions_productionlink"):
        if len(r) < 5:
            continue
        pid, lc, param, isdl = r[3], r[1], unesc(r[2]) or "", r[4]
        if isdl != "t" or pid not in title:
            continue
        links[pid].append((lc, param))

    # 6) join + dedup + emit
    sys.stderr.write("pass 6/6 dedup + emit ...\n")
    zx_keys, sndh_base = load_dedup_sets()
    rows, shots = [], []
    stats = defaultdict(int)
    seen_url = set()
    for pid, ls in links.items():
        if any(is_covered_link(lc, p) for lc, p in ls):
            stats["skip_covered"] += 1
            continue
        cands = []
        for lc, p in ls:
            url, ext = resolve(lc, p)
            if url:
                cands.append((link_priority(lc, url), lc, url, ext))
        if not cands:
            stats["skip_nolink"] += 1
            continue
        cands.sort(key=lambda c: c[0])
        _, lc, url, ext = cands[0]
        if url in seen_url:
            stats["skip_dupurl"] += 1
            continue

        t = title.get(pid, "").strip()
        comp = (prod_comp.get(pid) or "").strip()
        plat_raw = prod_plat.get(pid, "")
        plat = PLATFORM_MAP.get(plat_raw, plat_raw)
        if not plat:
            # No platform tag on demozoo -> infer from the file extension, else
            # the generic demoscene-music bucket.
            plat = EXT_FORMAT.get(ext, GENERIC_FORMAT)

        # secondary dedup vs existing collections
        if host_of(url) == "files.zxdemo.org" and (norm(t), norm(comp)) in zx_keys:
            stats["skip_zxart_dup"] += 1
            continue
        if ext == "sndh" and os.path.basename(
                urllib.parse.urlparse(url).path).lower() in sndh_base:
            stats["skip_sndh_dup"] += 1
            continue

        seen_url.add(url)
        # sanitise tabs/newlines out of free-text fields
        t = re.sub(r"\s+", " ", t) or "(unknown)"
        comp = re.sub(r"\s+", " ", comp)
        rows.append("\t".join([t, comp, plat, url, ext]))
        stats["kept"] += 1
        stats["host:" + (host_of(url) or lc)] += 1
        # own screenshot wins; else borrow one from a demo that uses this tune.
        s = prod_shot.get(pid)
        if s:
            stats["shot:own"] += 1
        else:
            for demo in music_to_demos.get(pid, []):
                if demo in prod_shot:
                    s = prod_shot[demo]
                    stats["shot:via-demo"] += 1
                    break
        if s:
            shots.append(url + "\t" + s)

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    with open(OUT_SHOTS, "w", encoding="utf-8") as f:
        f.write("\n".join(shots) + "\n")

    sys.stderr.write("\n=== done ===\n")
    print(f"wrote {len(rows)} songs -> {OUT}")
    print(f"wrote {len(shots)} screenshots -> {OUT_SHOTS}")
    print("stats:")
    for k in sorted(stats, key=lambda k: (-stats[k], k)):
        if k.startswith("host:") and stats[k] < 20:
            continue
        print(f"  {stats[k]:7d}  {k}")


# ===========================================================================
# Screenshot augmentation for our OTHER (non-demozoo) collections.
#
# Demozoo links a music production to the demos/games that use it as their
# soundtrack (productions_soundtracklink), and those productions almost always
# have a screenshot. Our existing collections (modland/HVSC/sndh/asma) reach the
# same files via the SAME hosts/paths demozoo records as download links, so we
# can: match our song -> a demozoo music production by its download link, then
# borrow a screenshot (the production's own, else one from a demo that uses it).
# Writes data/<collection>_screenshots.txt keyed by OUR song path (the same key
# MusicDatabase::getSongScreenshots looks up). Verified match rates: modland
# 97.7%, HVSC 99.0%, asma 98.8%, sndh 85.2% (the sndh miss = dynamic dl.php?ID=
# links that carry no filename). sceneorg matches by its archive.scene.org URL
# (only the net-new parties/scenesp slice is in sceneorg.txt, so the rate is low
# by design -- most demozoo scene.org links point outside our scope). amp matches
# by AMP's numeric module index: demozoo's amp.dascene.net links carry either
# downmod.php?index=<N> (direct) or /modules/<Comp>/<fmt>.<title>.gz which amp.txt
# maps back to the index via (composer,title); the low rate is inherent -- most
# AMP tunes are standalone music never used in a screenshotted production.
# ===========================================================================
HVSC_HOSTS = {"hvsc.csdb.dk", "hvsc.perff.dk", "www.hvsc.c64.org", "hvsc.etv.cx"}


def _our_key(link_class, param):
    """(collection, our-song-path-key) for a download link, or (None, None).

    Returns the path key in the exact form the collection stores (so it matches
    the screenshot lookup): full modland/HVSC/asma path, or for sndh the file
    basename (resolved to the full sndh.txt path by the caller)."""
    if link_class == "ModlandFile":
        key = urllib.parse.unquote(param)
        # Most params are "/pub/modules/<path>"; a minority are already "/<path>"
        # (no pub/modules prefix). Strip the prefix if present, then any leading
        # slash, so the key matches our slash-less allmods.txt paths.
        key = re.sub(r"^/?pub/modules/", "", key).lstrip("/")
        return "modland", key
    if link_class == "BaseUrl":
        h = host_of(param)
        path = urllib.parse.unquote(urllib.parse.urlparse(param).path)
        if h in HVSC_HOSTS:
            key = re.sub(r"^/(download/)?C64Music/", "", path).lstrip("/")
            return "hvsc", key
        if h == "asma.atari.org":
            return "asma", re.sub(r"^/asma/", "", path).lstrip("/")
        if h == "sndh.atari.org":
            base = os.path.basename(path).lower()
            return ("sndh", base) if base.endswith(".sndh") else (None, None)
        if h == "amp.dascene.net":
            # amp.txt keys songs by AMP's numeric module index (the song path),
            # so return a token the caller resolves to that index. Two link forms:
            #   downmod.php?index=<N>      -> the index directly
            #   /modules/<L>/<Comp>/<fmt>.<title>.gz -> (composer, title), which
            #   amp.txt maps back to the index (AMP's own naming, so it matches).
            mi = re.search(r"index=(\d+)", param)
            if mi:
                return "amp", ("idx", mi.group(1))
            mp = re.search(r"/modules/[^/]+/([^/]+)/([^/]+)\.gz$", path)
            if mp:
                title = re.sub(r"^[A-Za-z0-9]{1,8}\.", "", mp.group(2))
                return "amp", ("ct", norm(mp.group(1)), norm(title))
            return None, None
    if link_class == "ModarchiveModule":
        # param is modarchive.org's numeric module id == our modarchive.txt path
        # key. (These are reference links, is_download_link='f' -- the caller must
        # let them through its download-only filter.)
        mid = (param or "").strip()
        return ("modarchive", mid) if mid.isdigit() else (None, None)
    if link_class == "SceneOrgFile":
        # param is the archive path ("/parties/...", "/mirrors/scenesp.org/...").
        # Our sceneorg.txt stores the song as https://archive.scene.org/pub<path>
        # (URL-quoted). Return a normalized lookup key matching _sceneorg_index;
        # the caller resolves it to the exact stored URL.
        return "sceneorg", urllib.parse.unquote("/pub" + param).lower()
    return None, None


def _sceneorg_index():
    """normalized-archive-path -> exact sceneorg.txt URL (the screenshot key).

    sceneorg.txt only carries the net-new parties/ + scenesp.org slice, so most
    demozoo SceneOrgFile links won't be present (returns None -> skipped)."""
    m = {}
    p = os.path.join(DATA, "sceneorg.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) >= 4 and "archive.scene.org" in c[3]:
                    key = urllib.parse.unquote(
                        urllib.parse.urlparse(c[3]).path).lower()
                    m.setdefault(key, c[3])
    return m


def _sndh_basename_to_path():
    m = {}
    p = os.path.join(DATA, "sndh.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if c and c[-1].endswith(".sndh"):
                    m.setdefault(os.path.basename(c[-1]).lower(), c[-1])
    return m


def _modarchive_ids():
    """The set of modarchive.txt module ids (col3 = the song path key)."""
    ids = set()
    p = os.path.join(DATA, "modarchive.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) >= 3 and c[2].isdigit():
                    ids.add(c[2])
    return ids


def _amp_maps():
    """(valid-index set, (norm_composer,norm_title)->[indices]) from amp.txt.

    amp.txt row = title \\t composer \\t format \\t index \\t ext. The demozoo link
    gives us either the index or (composer,title); both resolve to the index(es)
    that are the amp screenshot key."""
    idx = set()
    ct = defaultdict(list)
    p = os.path.join(DATA, "amp.txt")
    if os.path.exists(p):
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) >= 4 and c[3].isdigit():
                    idx.add(c[3])
                    ct[(norm(c[1]), norm(c[0]))].append(c[3])
    return idx, ct


def augment():
    # prod -> a screenshot url (standard_url, else original_url)
    sys.stderr.write("aug 1/3 screenshots ...\n")
    shot = {}
    for r in iter_copy("productions_screenshot"):
        if len(r) >= 9 and r[1] not in shot:
            u = unesc(r[8]) or unesc(r[2])
            if u:
                shot[r[1]] = u

    # music soundtrack prod -> demo prods that use it (those carry the visual)
    sys.stderr.write("aug 2/3 soundtracklinks ...\n")
    music_to_demos = defaultdict(list)
    for r in iter_copy("productions_soundtracklink"):
        if len(r) >= 3:
            music_to_demos[r[2]].append(r[1])

    # download links -> our collection keys, then pick a screenshot
    sys.stderr.write("aug 3/3 links + emit ...\n")
    sndh_paths = _sndh_basename_to_path()
    sceneorg_idx = _sceneorg_index()
    amp_idx, amp_ct = _amp_maps()
    ma_ids = _modarchive_ids()
    out = {"modland": {}, "hvsc": {}, "sndh": {}, "asma": {}, "sceneorg": {},
           "amp": {}, "modarchive": {}}
    stats = defaultdict(int)
    for r in iter_copy("productions_productionlink"):
        if len(r) < 5:
            continue
        # ModarchiveModule links are reference links (is_download_link='f') but
        # carry the module id we key on; every other collection matches a real
        # download link.
        if r[1] != "ModarchiveModule" and r[4] != "t":
            continue
        pid, lc, param = r[3], r[1], unesc(r[2]) or ""
        coll, key = _our_key(lc, param)
        if not coll:
            continue
        # own screenshot wins, else the first demo that has one
        s = shot.get(pid)
        if not s:
            for demo in music_to_demos.get(pid, []):
                if demo in shot:
                    s = shot[demo]
                    stats[coll + ":via-demo"] += 1
                    break
        else:
            stats[coll + ":own"] += 1
        if not s:
            continue
        if coll == "sndh":
            key = sndh_paths.get(key)        # basename -> full sndh.txt path
            if not key:
                stats["sndh:unmatched-basename"] += 1
                continue
        elif coll == "sceneorg":
            key = sceneorg_idx.get(key)      # archive path -> exact stored URL
            if not key:
                stats["sceneorg:unmatched-path"] += 1
                continue
        elif coll == "amp":
            # key is ("idx",N) or ("ct",comp,title); resolve to AMP index(es).
            if key[0] == "idx":
                idxs = [key[1]] if key[1] in amp_idx else []
            else:
                idxs = amp_ct.get((key[1], key[2]), [])
            if not idxs:
                stats["amp:unmatched"] += 1
                continue
            for k in idxs:                   # (composer,title) can have variants
                out["amp"].setdefault(k, s)
            continue
        elif coll == "modarchive":
            if key not in ma_ids:            # only tunes we actually host
                stats["modarchive:unmatched"] += 1
                continue
        out[coll].setdefault(key, s)         # first link wins, stable

    for coll, m in out.items():
        path = os.path.join(DATA, f"{coll}_screenshots.txt")
        # MERGE, never clobber: sndh_screenshots.txt already ships curated Atari
        # Mania matches (and re-runs would otherwise drop them). Existing entries
        # win; Demozoo only fills keys we don't already have. modland/hvsc/asma
        # have no prior file, so they're all-Demozoo. (modland's ZX subset lives
        # in the separate zxspectrum_screenshots.txt, loaded alongside.)
        merged, kept = {}, 0
        if os.path.exists(path):
            with open(path, encoding="utf-8", errors="replace") as f:
                for line in f:
                    tab = line.find("\t")
                    if tab != -1:
                        merged[line[:tab]] = line[tab + 1:].rstrip("\n")
            kept = len(merged)
        added = 0
        for k, v in m.items():
            if k not in merged:
                merged[k] = v
                added += 1
        with open(path, "w", encoding="utf-8") as f:
            for k in sorted(merged):
                f.write(k + "\t" + merged[k] + "\n")
        print(f"wrote {len(merged):6d} screenshots ({kept} existing + {added} "
              f"new demozoo) -> {path}")
    print("breakdown:")
    for k in sorted(stats):
        print(f"  {stats[k]:7d}  {k}")


# ===========================================================================
# Regenerate ONLY demozoo_screenshots.txt against the CURRENT demozoo.txt, without
# rewriting demozoo.txt. Use this after demozoo.txt already exists (e.g. a newer
# dump/day than the one that built it): we key screenshots to the song URLs that
# are actually present, so nothing drifts. Same own-else-via-soundtrack-demo logic
# build() now uses, but membership-gated by the existing song URLs -- this is what
# lifts coverage far past the ~1k tunes that have their OWN screenshot.
# ===========================================================================
def screenshots():
    urls = set()
    with open(OUT, encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) >= 4 and "://" in c[3]:
                urls.add(c[3])
    sys.stderr.write(f"demozoo.txt song URLs: {len(urls)}\n")

    sys.stderr.write("1/3 screenshots (all prods) ...\n")
    prod_shot = {}
    for r in iter_copy("productions_screenshot"):
        if len(r) >= 9 and r[1] not in prod_shot:
            u = unesc(r[8]) or unesc(r[2])
            if u:
                prod_shot[r[1]] = u
    sys.stderr.write("2/3 soundtracklinks ...\n")
    music_to_demos = defaultdict(list)
    for r in iter_copy("productions_soundtracklink"):
        if len(r) >= 3:
            music_to_demos[r[2]].append(r[1])

    # map each present song URL back to its production via the same resolve() the
    # build uses, so the key matches demozoo.txt col 4 exactly.
    sys.stderr.write("3/3 links -> url -> pid, emit ...\n")
    url2pid = {}
    for r in iter_copy("productions_productionlink"):
        if len(r) < 5 or r[4] != "t":
            continue
        pid, lc, param = r[3], r[1], unesc(r[2]) or ""
        url, _ = resolve(lc, param)
        if url and url in urls and url not in url2pid:
            url2pid[url] = pid

    shots, own, via = [], 0, 0
    for url, pid in url2pid.items():
        s = prod_shot.get(pid)
        if s:
            own += 1
        else:
            for demo in music_to_demos.get(pid, []):
                if demo in prod_shot:
                    s = prod_shot[demo]
                    via += 1
                    break
        if s:
            shots.append(url + "\t" + s)
    shots.sort()
    with open(OUT_SHOTS, "w", encoding="utf-8") as f:
        f.write("\n".join(shots) + "\n")
    print(f"wrote {len(shots)} screenshots -> {OUT_SHOTS}")
    print(f"  matched {len(url2pid)}/{len(urls)} song URLs to a production; "
          f"of those with a shot: {own} own + {via} via-soundtrack-demo")


if __name__ == "__main__":
    if "--download" in sys.argv:
        download()
    if "--build" in sys.argv:
        if not os.path.exists(DUMP):
            sys.exit(f"dump not found at {DUMP}; run --download first")
        build()
    if "--screenshots" in sys.argv:
        if not os.path.exists(DUMP):
            sys.exit(f"dump not found at {DUMP}; run --download first")
        screenshots()
    if "--augment" in sys.argv:
        if not os.path.exists(DUMP):
            sys.exit(f"dump not found at {DUMP}; run --download first")
        augment()
    if len(sys.argv) == 1:
        print(__doc__)
