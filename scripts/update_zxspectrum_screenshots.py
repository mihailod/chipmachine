#!/usr/bin/env python3
"""
Build chipmachine/data/zxspectrum_screenshots.txt — ZX Spectrum game LOADING
screens for the ZX-AY tunes that live inside the big `modland` collection
(paths under "Spectrum/...", "AY Emul/...", "AY Amadeus/...", "AY STRC/...").

chipmachine has no dedicated ZX collection; the ZX tunes are organised by
MUSICIAN (the `game` field is empty), so this matches the tune *filename*
against the ZX game database by title. Same offline-prebuilt + Wayback-served
pattern as gb64 / sndh / hvtc / unexotica.

Sourcing is 100% OFFLINE at build time — no live hits to any ZX site:
  * Game titles + loading-screen file paths come from the ZXDB MySQL dump
    (https://github.com/zxdb/ZXDB, ZXDB_mysql.sql.zip, ~27 MB, one download).
  * The loading screen lives at the World of Spectrum path stored in ZXDB
    `scraps.file_link` (/pub/sinclair/screens/load/<x>/gif/<Name>.gif).

Serving host (investigated all three):
  * zxinfo.dk/media/zxscreens/<id>/<Name>-load.png — works, but NOT archived in
    Wayback and renders the .scr differently.
  * spectrumcomputing.co.uk (live WoS successor) — serves the exact
    `/pub/sinclair/screens/load/...` paths, BUT is HTTP/1.1-only. chipmachine's
    libcurl forces CURL_HTTP_VERSION_2TLS with no overall CURLOPT_TIMEOUT (only
    a 10s CONNECT timeout), so it STALLS FOREVER on the h2->h1.1 ALPN fallback —
    the GUI hangs with no image. (CLI curl falls back fine; the app's bundled
    libcurl does not.) Rejected for that reason.
  * worldofspectrum.org via Wayback "2id_" — CHOSEN. web.archive.org speaks
    HTTP/2, so the app fetches it exactly like the gb64/sndh/hvtc screenshots;
    2id_ 302-redirects (FOLLOWLOCATION) to the latest real .gif capture (~18.8k
    archived). No CDX needed (same as gb64). worldofspectrum.org is defunct, so
    a few 2id_ snapshots may be poisoned/404 -> graceful miss (WebJob deletes
    the target on non-200), not a hang.

Matching is STRICTLY exact (normalized tune basename == normalized game title,
incl. ZXDB aliases). Prefix/fuzzy was rejected on the other collections for
mass false positives. Even strict-exact single-word tune names coincide with a
game title (Popcorn, Feud, Zub); modland ZX is a MIX of game rips and demoscene
chip tunes, and a game DB has no art for the latter. Per the maintainer's call
we ship all strict-exact matches; the single-word subset goes to a .review
sidecar for eyeball pruning.

Output (TAB separated), keyed by the full modland song path:
    Spectrum/Pro Tracker 3/AER/ay3.pt3 <TAB> https://web.archive.org/web/2id_/http://www.worldofspectrum.org/pub/sinclair/screens/load/.../X.gif

Usage:
    # full offline build (reads modland paths from the live music.db,
    # parses a local ZXDB dump):
    python3 update_zxspectrum_screenshots.py --zxdb /path/ZXDB_mysql.sql

    # offline path list instead of music.db:
    python3 update_zxspectrum_screenshots.py --zxdb F --paths PATHS
"""

import re
import argparse
import sqlite3
import os.path
import urllib.parse
from pathlib import Path
from collections import defaultdict

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
OUT_FILE = DATA / "zxspectrum_screenshots.txt"
REVIEW_FILE = HERE / "zxspectrum_screenshots.review.txt"

DEFAULT_DB = Path(os.path.expanduser("~/Library/Caches/chipmachine/music.db"))

# Serve the World of Spectrum load .gif through the Internet Archive "2id_"
# latest-snapshot mirror (web.archive.org speaks HTTP/2, which chipmachine's
# libcurl requires — see the module docstring). Same pattern as gb64.
WAYBACK = "https://web.archive.org/web/2id_/http://www.worldofspectrum.org"

# ZXDB machinetype ids that are Spectrum / clones (machinetypes table). Anything
# that isn't a Spectrum-family machine (SAM Coupé excluded would lose a few, but
# keep it broad: the title match itself is the precision gate).
ZX_MACHINES = set(range(1, 17)) | {25, 31}

# Hand-pruned false positives: strict-exact matched a game title but the tune is
# a chip cover / demo, not a game rip. Keyed by full song path. Seeded from the
# single-word review set; extend as you prune .review.
EXCLUDE = set()

# Generic single-word tune names that ALSO happen to be a ZX game title but are
# almost always demoscene/chip tunes here, not game rips (modland ZX is mostly
# musician tunes). Denied by normalized title across all their occurrences —
# reproducible, unlike per-path EXCLUDE. Conservative list: only obvious
# generics / known chip covers; real game names (zub, gryzor, stormlord, ...)
# are deliberately kept.
GENERIC_TITLES = {
    # original batch (musical / generic words, obvious chip-tune names)
    "fantasy", "prodigy", "music", "music1", "music2", "music3", "techno",
    "help", "gameover", "life", "fly", "brain", "time", "popcorn", "intro",
    "demo", "test", "hello", "sample", "song", "title", "theme", "chip",
    "rock", "disco", "trance", "dance", "jazz", "blues", "funk", "noise",
    # review-file prune: generic/abstract/musical single words whose ZXDB game
    # match is almost certainly coincidental (the tune is an original, not a
    # game rip). Distinctive game names (turrican, gryzor, deflektor, sokoban,
    # …) are deliberately KEPT even when single-word.
    "fire", "hate", "magic", "hard", "virus", "money", "drums", "chaos",
    "africa", "snake", "sea", "mirror", "hit", "core", "city", "attack",
    "action", "abyss", "storm", "tango", "score", "sound", "smash", "roulette",
    "pusher", "pool", "point", "panic", "oops", "maze", "mania", "logo",
    "lines", "impact", "imagination", "glass", "galaxy", "formula", "five",
    "filler", "expert", "escape", "entropy", "emergency", "electro", "duet",
    "double", "downtown", "color", "business", "bad", "bomb", "blaze", "danger",
    "paradox", "prelude", "inspiration", "style", "nonsense", "control",
    "crash", "cube", "emotion", "extreme", "war", "laser", "fish", "think",
    "trouble", "revolution", "western", "nonamed", "sigma", "morpheus",
}


def norm(s):
    """Lowercase, drop extension + bracket tags, keep alphanumerics only."""
    s = urllib.parse.unquote(s).lower()
    s = re.sub(r"\.[a-z0-9]{2,5}$", "", s)      # extension
    s = re.sub(r"\[[^\]]*\]", "", s)
    s = re.sub(r"\([^)]*\)", "", s)
    return re.sub(r"[^a-z0-9]+", "", s)


def words(s):
    s = re.sub(r"\.[a-z0-9]{2,5}$", "", s.lower())
    return len([w for w in re.split(r"[^a-z0-9]+", s) if w])


# ---------------------------------------------------------------- ZXDB parsing
def _unesc(s):
    return s.replace("\\'", "'").replace('\\"', '"').replace("\\\\", "\\")


def parse_zxdb(sql_path):
    """Return (title2ids, entry_load_link):
       title2ids[norm_title] -> set(entry_id) for ZX entries that have a
                                loading-screen scrap.
       entry_load_link[entry_id] -> WoS file_link of its loading screen."""
    text_iter = lambda: open(sql_path, encoding="utf-8", errors="replace")

    # entries: id, title, is_xrated, machinetype_id, ... (we need id+title+mt)
    entry_re = re.compile(
        r"\((\d+),\s*'((?:[^'\\]|\\.)*)',\s*\d+,\s*(NULL|\d+),")
    entries = {}      # id -> (title, machinetype_id)
    intable = False
    for line in text_iter():
        if line.startswith("INSERT INTO `entries`"):
            intable = True
        elif intable and line.startswith("INSERT INTO"):
            break
        if not intable:
            continue
        for m in entry_re.finditer(line):
            eid = int(m.group(1))
            title = _unesc(m.group(2))
            mt = int(m.group(3)) if m.group(3) != "NULL" else 0
            entries[eid] = (title, mt)

    # scraps: find loading-screen file_links (/pub/sinclair/screens/load/.../X.gif)
    # row prefix: (id, entry_id, release_seq, file_link, ...)
    scrap_re = re.compile(
        r"\(\d+,\s*(NULL|\d+),\s*(?:NULL|\d+),\s*'(/pub/sinclair/screens/load/[^']*?)'")
    entry_load_link = {}
    intable = False
    for line in text_iter():
        if line.startswith("INSERT INTO `scraps`"):
            intable = True
        elif intable and line.startswith("INSERT INTO"):
            break
        if not intable:
            continue
        for m in scrap_re.finditer(line):
            if m.group(1) == "NULL":
                continue
            eid = int(m.group(1))
            link = _unesc(m.group(2))
            # prefer the first (lowest scrap id) load screen per entry
            entry_load_link.setdefault(eid, link)

    # title -> entry ids, restricted to ZX entries that have a load screen
    title2ids = defaultdict(set)
    have = 0
    for eid, (title, mt) in entries.items():
        if eid in entry_load_link and mt in ZX_MACHINES:
            have += 1
            k = norm(title)
            if k:
                title2ids[k].add(eid)

    # aliases (alternate / localized titles): entry_id, release_seq, lang, title
    alias_re = re.compile(
        r"\((\d+),\s*\d+,\s*'[^']*',\s*'((?:[^'\\]|\\.)*)'\)")
    intable = False
    for line in text_iter():
        if line.startswith("INSERT INTO `aliases`"):
            intable = True
        elif intable and line.startswith("INSERT INTO"):
            break
        if not intable:
            continue
        for m in alias_re.finditer(line):
            eid = int(m.group(1))
            if eid in entry_load_link and entries.get(eid, (None, 0))[1] in ZX_MACHINES:
                k = norm(_unesc(m.group(2)))
                if k:
                    title2ids[k].add(eid)

    print(f"ZXDB: {len(entries)} entries, "
          f"{have} ZX entries with a loading screen, "
          f"{len(title2ids)} distinct normalized titles (incl aliases)")
    return title2ids, entry_load_link


# ------------------------------------------------------------------- modland songs
def load_paths(args):
    if args.paths:
        return [l.strip() for l in Path(args.paths).read_text(
            encoding="utf-8", errors="replace").splitlines() if l.strip()]
    db = Path(args.db)
    if not db.exists():
        raise SystemExit(f"music.db not found at {db}; pass --paths or --db")
    con = sqlite3.connect(str(db))
    rows = con.execute(
        "SELECT path FROM song WHERE collection=(SELECT ROWID FROM collection "
        "WHERE id='modland') AND (path LIKE 'Spectrum/%' OR path LIKE 'AY Emul/%' "
        "OR path LIKE 'AY Amadeus/%' OR path LIKE 'AY STRC/%')").fetchall()
    con.close()
    return [r[0] for r in rows]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zxdb", required=True, help="ZXDB_mysql.sql (unzipped)")
    ap.add_argument("--paths", help="local modland ZX path list (skip music.db)")
    ap.add_argument("--db", default=str(DEFAULT_DB), help="music.db for song paths")
    args = ap.parse_args()

    title2ids, entry_load_link = parse_zxdb(args.zxdb)
    paths = load_paths(args)
    print(f"modland ZX songs: {len(paths)}")

    results = {}      # song path -> screenshot url
    single = []       # (basename, path) single-word matches (low confidence)
    for path in paths:
        if path in EXCLUDE:
            continue
        base = path.rsplit("/", 1)[-1]
        k = norm(base)
        if len(k) < 3 or k not in title2ids:
            continue
        if k in GENERIC_TITLES:
            continue
        # pick the lowest-id entry with this title (canonical/original release)
        eid = min(title2ids[k])
        link = entry_load_link.get(eid)
        if not link:
            continue
        # file_link is already URL-encoded-safe path text; spaces/() exist in a
        # few names — quote everything except the path separators.
        results[path] = WAYBACK + urllib.parse.quote(link, safe="/().-_")
        if words(base) < 2:
            single.append((base, path))

    lines = [f"{p}\t{u}" for p, u in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    rev = [f"{b}\t{p}\t{results[p]}" for b, p in sorted(single)]
    REVIEW_FILE.write_text("\n".join(rev) + ("\n" if rev else ""), encoding="utf-8")

    print(f"Wrote {OUT_FILE}: {len(results)} matches "
          f"({len(results)-len(single)} multi-word high-confidence, "
          f"{len(single)} single-word -> {REVIEW_FILE.name})")


if __name__ == "__main__":
    main()
