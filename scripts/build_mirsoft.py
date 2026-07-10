#!/usr/bin/env python3
"""Build chipmachine/data/mirsoft.txt -- mirsoft.info "World of Game MODs".

mirsoft.info's World of Game MODs is a curated archive of the tracker modules
used in games, one .zip per game (several .mod/.xm/.it/.s3m/.med modules + an
info.txt). It spans many platforms -- overwhelmingly Amiga, then C64, PC (DOS/
Windows), NES, SNES, Macintosh, PlayStation, ... -- and by policy holds ONLY
mainstream tracker formats (its FAQ: "why exotic tracked formats are not in the
archive"), even for console games (those entries are tracker-format arrangements
/remixes, not native .sid/.nsf rips). So every module plays via OpenMPT/UADE and
there is NO new format. At runtime the host fetches one game zip and the
ZIP-by-magic subsong handler in MusicPlayerList extracts and plays its modules
as subsongs (same pipeline as zophar/vgmrips/smspower); info.txt and the odd
.mid/.dro/.dmu member are ignored by that handler's songExt set.

SOURCE (build-time, offline, no live crawl): the bulk is mirrored on the Internet
Archive as item `mirsoftJuly2021snapshot` (a 982MB .tar.xz of the raw gamemods/
tree, 1601 games). Download+extract it once, point --snapshot at the extracted
`gamemods/` directory; every fact (platform, composer, format, track list) comes
from each game's info.txt.

TOP-UP: mirsoft keeps adding games. The 2021 tarball was topped up with the games
added since (found via the site's "Newest additions" view, gamemods-archive.php?
order=timestamp&order_desc=1, filtered to dates after the snapshot; the game =
the tune title minus its ": <type>" suffix, and the whole-game zip is fetched
from /gamemods/<Name>.zip -- NOTE the archive DISPLAYS "Game: Subtitle" but the
zip name uses "Game - Subtitle"). Each delta zip is extracted into the --snapshot
tree under its real zip stem, then this script is re-run over the merged tree.
Only a handful per year, so this is a targeted live fetch, not a crawl.

RUNTIME playback points at archive.org first with the live mirsoft host as
fallback (registerSource in MusicDatabase::generateIndex, the mirsoft branch),
so mirsoft's server is barely touched. The archive.org item's per-file zip
derivatives are current through ~2023 (newer than the tarball), so only the most
recent additions actually fall through to the live-mirsoft fallback.

DEDUP (platform-aware): mirsoft's Amiga rips overlap heavily with UnExoticA (many
info.txt even say "Archiver: UnExoticA") and modland; its console/PC entries are
tracker remixes absent from the native-chip collections. A game is dropped when
its name matches a SAME-PLATFORM game collection already indexed, OR when every
one of its modules is a byte-plausible (basename-stem, size) match in modland+amp
(the only local indexes carrying sizes). ~1000 net-new games are kept.

CLASSIFICATION (item 4): by mirsoft's per-game Platform field (Amiga, C64, PC,
NES, SNES, Mac, ...). The emitted `format` column is a canonical platform label
that MusicDatabase::initFormats maps to a platform byte (see the mirsoft block).

  --build --snapshot <dir>   parse info.txt, dedup, classify, write mirsoft.txt
  --screenshots              (see build_mirsoft.py --screenshots; MobyGames-free
                             stub -- mirsoft hosts no screenshots, see README)
"""

import os
import re
import sys
import json
import urllib.parse
import collections

HERE = os.path.dirname(__file__)
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
OUT = os.path.join(DATA, "mirsoft.txt")

# Extensions the ZIP-by-magic subsong handler (MusicPlayerList songExt) will play.
# A game with none of these has nothing to play at runtime -> skip it.
PLAYABLE = {"mod", "xm", "it", "s3m", "mtm", "669", "far", "okt", "med", "mmd0",
            "mmd1", "mmd2", "mmd3", "dbm", "digi", "ahx", "hvl", "thx", "dmf",
            "ptm", "stm", "ult", "amf", "psm", "mt2", "gt2", "dtm", "fc", "fc13",
            "fc14", "aon", "smod", "dw", "cust", "mptm", "dmu", "dmu2"}


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


# mirsoft Platform field (first comma-token, normalized) -> canonical format
# label emitted in the `format` column. The label maps to a platform byte in
# MusicDatabase::initFormats (mirsoft block). Unmatched/blank -> "PC" fallback.
def platform_label(raw):
    p = norm(raw.split(",")[0])
    if not p or p.startswith("year") or p in ("online", "internet"):
        return "PC"
    if "amiga" in p:
        return "Amiga"
    if p in ("c64", "c64128", "commodore64128", "commodore64"):
        return "Commodore 64"
    if "amstrad" in p or "cpc" in p:
        return "Amstrad CPC"
    if "zxspectrum" in p or p == "spectrum":
        return "ZX Spectrum"
    if "mastersystem" in p:
        return "Sega Master System"
    if "megadrive" in p or "genesis" in p or "megacd" in p or p == "segacd":
        return "Sega Mega Drive"
    if "saturn" in p:
        return "Sega Saturn"
    if "dreamcast" in p:
        return "Dreamcast"
    if "jaguar" in p:
        return "Atari Jaguar"
    if "falcon" in p:
        return "Atari Falcon"
    if p.startswith("atari"):
        return "Atari ST"
    if "nintendo64" in p or p == "n64":
        return "Nintendo 64"
    if "snes" in p or "supernes" in p or "supernintendo" in p or "superfamicom" in p:
        return "Super Nintendo"
    if ("gameboy" in p or p in ("gbc", "gba")):
        return "Game Boy"
    if "nes" in p or "nintendoentertainment" in p:
        return "NES"
    if "turbografx" in p or "pcengine" in p:
        return "PC Engine"
    if "neogeo" in p or "arcade" in p:
        return "Arcade"
    if "psx" in p or "playstation" in p:
        return "PlayStation"
    if "mac" in p:
        return "Macintosh"
    if "pc" in p:            # PC, PC Dos, PC Windows, PC-DOS, Pocket PC
        return "PC"
    return "PC"


# Which already-indexed game collections to name-dedup against, per label.
DEDUP_COLLECTIONS = {
    "Amiga": ["unexotica"],
    "Commodore 64": ["rko"],
    "Amstrad CPC": ["cpcpower"],
}
CONSOLE_LABELS = {"NES", "Super Nintendo", "Game Boy", "Nintendo 64",
                  "Sega Master System", "Sega Mega Drive", "Sega Saturn",
                  "Dreamcast", "Atari Jaguar", "PC Engine", "Arcade",
                  "PlayStation"}
for _l in CONSOLE_LABELS:
    DEDUP_COLLECTIONS[_l] = ["zophar", "vgmrips", "smspower"]


def load_titles(fn, col):
    s = set()
    p = os.path.join(DATA, fn)
    if not os.path.exists(p):
        return s
    for line in open(p, encoding="utf-8", errors="replace"):
        c = line.rstrip("\n").split("\t")
        if len(c) > col and c[col].strip():
            s.add(norm(c[col]))
    return s


def load_name_sets():
    sets = {k: set() for grp in DEDUP_COLLECTIONS.values() for k in grp}
    cols = {"unexotica": 1, "rko": 0, "cpcpower": 0, "zophar": 0,
            "vgmrips": 0, "smspower": 0}
    for name, col in cols.items():
        sets[name] = load_titles(name + ".txt", col)
    # modland "Video Game Music/<system>/<composer>/<game>/..." game names,
    # keyed by normalized system, for the Amiga subtree.
    vgm = collections.defaultdict(set)
    am = os.path.join(DATA, "allmods.txt")
    for line in open(am, encoding="utf-8", errors="replace"):
        p = line.split("\t", 1)[-1].strip()
        if p.startswith("Video Game Music/"):
            parts = p.split("/")
            if len(parts) >= 5:
                vgm[norm(parts[1])].add(norm(parts[3]))
    return sets, vgm


def load_content():
    """(stem, size) pairs from modland (allmods.txt) + amp (amp.txt) -- the only
    local indexes that carry file sizes -- for byte-plausible content dedup."""
    content = set()
    for line in open(os.path.join(DATA, "allmods.txt"), encoding="utf-8",
                     errors="replace"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 2:
            continue
        try:
            sz = int(c[0])
        except ValueError:
            continue
        base = c[1].split("/")[-1]
        stem = base.rsplit(".", 1)[0] if "." in base else base
        content.add((norm(stem), sz))
    ampf = os.path.join(DATA, "amp.txt")
    if os.path.exists(ampf):
        for line in open(ampf, encoding="utf-8", errors="replace"):
            c = line.rstrip("\n").split("\t")
            if len(c) >= 4:
                try:
                    sz = int(c[3])
                except ValueError:
                    continue
                content.add((norm(c[0]), sz))
    return content


def parse_info(path):
    meta = {}
    if not os.path.exists(path):
        return meta
    t = open(path, encoding="latin-1").read()
    # [^\S\n]* = horizontal whitespace only: a *blank* field must not let \s*
    # swallow the newline and slurp the following line's value.
    for key in ("Type", "Format", "Original Composer", "Tracker", "Name", "Platform"):
        m = re.search(r"^" + re.escape(key) + r":[^\S\n]*(.*)$", t, re.M)
        if m:
            meta[key] = m.group(1).strip()
    return meta


def build(snapshot):
    root = snapshot
    if os.path.isdir(os.path.join(root, "gamemods")):
        root = os.path.join(root, "gamemods")
    name_sets, vgm = load_name_sets()
    content = load_content()
    rows = []
    kept = dropped_dup = dropped_empty = 0
    plat_ct = collections.Counter()
    for g in sorted(os.listdir(root)):
        d = os.path.join(root, g)
        if not os.path.isdir(d):
            continue
        meta = parse_info(os.path.join(d, "info.txt"))
        mods = [f for f in os.listdir(d)
                if os.path.isfile(os.path.join(d, f))
                and not f.lower().endswith(".txt")]
        exts = {f.rsplit(".", 1)[-1].lower() for f in mods if "." in f}
        if not (exts & PLAYABLE):                       # nothing runtime can play
            dropped_empty += 1
            continue
        label = platform_label(meta.get("Platform", ""))
        name = meta.get("Name", "").strip() or g
        n = norm(name)
        # platform-aware name dedup
        namematch = any(n in name_sets.get(c, set())
                        for c in DEDUP_COLLECTIONS.get(label, []))
        if label == "Amiga":
            namematch = namematch or (n in vgm.get("amiga", set()))
        # content dedup: every module byte-plausibly already in modland/amp
        dup = 0
        for f in mods:
            try:
                sz = os.path.getsize(os.path.join(d, f))
            except OSError:
                continue
            stem = f.rsplit(".", 1)[0] if "." in f else f
            if (norm(stem), sz) in content:
                dup += 1
        fullcontent = mods and dup == len(mods)
        if namematch or fullcontent:
            dropped_dup += 1
            continue
        # dominant playable ext (for the row's ext column)
        ecount = collections.Counter(f.rsplit(".", 1)[-1].lower()
                                     for f in mods if "." in f
                                     and f.rsplit(".", 1)[-1].lower() in PLAYABLE)
        ext = ecount.most_common(1)[0][0] if ecount else "mod"
        composer = meta.get("Original Composer", "").strip()
        if not composer:                                # mirsoft credits the tracker
            composer = meta.get("Tracker", "").strip()
        if composer.lower() in ("unknown", "?", "n/a", "-"):
            composer = ""
        path = urllib.parse.quote(g + ".zip")
        rows.append("\t".join([name, composer, label, path, ext]))
        kept += 1
        plat_ct[label] += 1
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {kept} games -> {OUT}")
    print(f"  dropped {dropped_dup} dups, {dropped_empty} with no playable track")
    print("  by platform label:")
    for k, v in plat_ct.most_common():
        print(f"    {v:4d}  {k}")


GB64_PREFIX = ("https://web.archive.org/web/2id_/"
               "http://www.gb64.com/Screenshots/")


def shot_norm(s):
    """Game-name key for screenshot matching (articles normalized)."""
    s = s.lower().strip()
    s = re.sub(r",\s*(the|a|an)$", "", s)   # "Pyramid, The" -> "pyramid"
    s = re.sub(r"^(the|a|an)\s+", "", s)    # "The Pyramid"  -> "pyramid"
    return re.sub(r"[^a-z0-9]", "", s)


def _load_shot_file(coll):
    """data/<coll>_screenshots.txt as {key: url}."""
    m = {}
    p = os.path.join(DATA, coll + "_screenshots.txt")
    if os.path.exists(p):
        for line in open(p, encoding="utf-8", errors="replace"):
            t = line.find("\t")
            if t > 0:
                m[line[:t]] = line[t + 1:].rstrip("\n")
    return m


def gb64_names():
    """C64 game name -> gb64 title-screen URL (Wayback), from data/Games.csv."""
    import csv
    m = {}
    gp = os.path.join(DATA, "Games.csv")
    if not os.path.exists(gp):
        return m
    with open(gp, encoding="utf-8", errors="replace") as f:
        for r in csv.reader(f):
            if len(r) < 13 or r[0] == "GA_Id":
                continue
            shot = r[6].strip().replace("\\", "/")      # ScrnshotFilename
            if shot and shot != "0":
                m.setdefault(shot_norm(r[1]),
                             GB64_PREFIX + urllib.parse.quote(shot))
    return m


def amiga_names():
    """Amiga game name -> Hall of Light / abime.net shot, by joining
    unexotica.txt (game col1, /Game/.../<g>.lha path col4) with the abime URLs
    already harvested in unexotica_screenshots.txt (keyed on that .lha id)."""
    ushots = _load_shot_file("unexotica")
    m = {}
    up = os.path.join(DATA, "unexotica.txt")
    if not os.path.exists(up):
        return m
    for line in open(up, encoding="utf-8", errors="replace"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 5:
            continue
        path = c[4]
        g, lha = path.find("/Game/"), path.find(".lha")
        if g >= 0 and lha >= 0 and path[g:lha + 4] in ushots:
            m.setdefault(shot_norm(c[1]), ushots[path[g:lha + 4]])
    return m


def reuse_names(coll):
    """game name -> shot, reusing an existing collection's offline screenshots
    (its .txt has game in col0 and the shot-key URL in col3)."""
    shots = _load_shot_file(coll)
    m = {}
    p = os.path.join(DATA, coll + ".txt")
    if shots and os.path.exists(p):
        for line in open(p, encoding="utf-8", errors="replace"):
            c = line.rstrip("\n").split("\t")
            if len(c) >= 4 and c[3] in shots:
                m.setdefault(shot_norm(c[0]), shots[c[3]])
    return m


# console/arcade/handheld platform labels -> the collections that carry title
# shots for those games (all already offline in data/*_screenshots.txt).
CONSOLE_SHOT = {
    "NES", "Super Nintendo", "Game Boy", "Nintendo 64", "Sega Master System",
    "Sega Mega Drive", "Sega Saturn", "Dreamcast", "PC Engine", "Arcade",
    "Atari Jaguar", "PlayStation"}


def screenshots():
    """Best-effort game screenshots for mirsoft games, drawn only from sources we
    already ship offline: gb64 (C64, data/Games.csv), Hall of Light / abime.net
    (Amiga, via the unexotica join), and the zophar/vgmrips/smspower/cpcpower
    per-game shots. Matched by normalized game name, platform-native source first,
    then a cross-title fallback (a same-named game's shot on another platform --
    usually the same multiplatform release). Keyed by the mirsoft song path (the
    <Game>.zip column of mirsoft.txt); consumed by the "mirsoft" branch of
    MusicDatabase::getSongScreenshots. mirsoft itself hosts no screenshots."""
    gb64 = gb64_names()
    amiga = amiga_names()
    console = {}
    for coll in ("zophar", "vgmrips", "smspower", "cpcpower"):
        for k, v in reuse_names(coll).items():
            console.setdefault(k, v)
    sys.stderr.write(f"gb64:{len(gb64)} amiga:{len(amiga)} console:{len(console)}\n")

    def pick(label, n):
        # platform-native source first
        if label == "Commodore 64" and n in gb64:
            return gb64[n]
        if label == "Amiga" and n in amiga:
            return amiga[n]
        if label == "Amstrad CPC" and n in console:
            return console[n]
        if label in CONSOLE_SHOT and n in console:
            return console[n]
        # cross-title best-effort fallback (same game on another platform)
        if n in gb64:
            return gb64[n]
        if n in amiga:
            return amiga[n]
        if n in console:
            return console[n]
        return None

    rows, matched, total = [], 0, 0
    for line in open(OUT, encoding="utf-8", errors="replace"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 5:
            continue
        total += 1
        url = pick(c[2], shot_norm(c[0]))
        if url:
            rows.append(c[3] + "\t" + url)
            matched += 1
    outp = os.path.join(DATA, "mirsoft_screenshots.txt")
    with open(outp, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {matched}/{total} screenshots -> {outp}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        snap = None
        if "--snapshot" in sys.argv:
            snap = sys.argv[sys.argv.index("--snapshot") + 1]
        if not snap or not os.path.isdir(snap):
            sys.exit("usage: build_mirsoft.py --build --snapshot <extracted gamemods dir>")
        build(snap)
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
