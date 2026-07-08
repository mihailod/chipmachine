#!/usr/bin/env python3
"""
Build chipmachine/data/zxtunes_screenshots.txt — ZX Spectrum game loading screens
for the `zxtunes` collection (zxtunes.com AY music, ~7k net-new tunes).

Same offline ZXDB / World-of-Spectrum-via-Wayback pipeline as the sibling
update_zxart_screenshots.py; see that file (and update_zxspectrum_screenshots.py)
for the ZXDB parse, the WAYBACK "2id_" HTTP/2 serving requirement, and the
GENERIC_TITLES prune.

zxtunes differs from zxart in ONE way: its `title` column is the raw module
FILENAME stem, and multi-subsong rips carry a trailing "_<N>" subsong index
(e.g. "Joe Blade 2_7", "The New Zealand Story_15"). We strip that suffix before
matching so real game titles resolve. Yield is low (~2%): the net-new zxtunes set
is mostly demoscene/chip originals — the game-titled rips were largely deduped
against zxart/modland (which already carry their screenshots).

Key the output by the song's full zxtunes.com downloads.php?id= URL (== song.path,
the runtime lookup key in getSongScreenshots).

Usage:
    python3 update_zxtunes_screenshots.py --zxdb /path/ZXDB_mysql.sql
    python3 update_zxtunes_screenshots.py --zxdb /path/ZXDB_mysql.sql --songs data/zxtunes.txt
"""

import re
import html
import argparse
import sqlite3
import os.path
import importlib.util
import urllib.parse
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
OUT_FILE = DATA / "zxtunes_screenshots.txt"
REVIEW_FILE = HERE / "zxtunes_screenshots.review.txt"
SONGS_TXT = DATA / "zxtunes.txt"
DEFAULT_DB = Path(os.path.expanduser("~/Library/Caches/chipmachine/music.db"))

# Reuse the ZXDB parser + Wayback serving + generic-title denylist + the zxart
# match_keys/title_words canonicalization (roman<->arabic sequels, &->and, ...).
_zaspec = importlib.util.spec_from_file_location(
    "zxartshots", HERE / "update_zxart_screenshots.py")
_za = importlib.util.module_from_spec(_zaspec)
_zaspec.loader.exec_module(_za)
parse_zxdb, norm, WAYBACK = _za.parse_zxdb, _za.norm, _za.WAYBACK
GENERIC_TITLES = _za.GENERIC_TITLES
match_keys, title_words = _za.match_keys, _za.title_words

# Trailing "_<digits>" subsong index that zxtunes filename stems carry.
SUBIDX_RE = re.compile(r"_\d+\s*$")

# zxtunes-LOCAL prune (kept out of the SHARED GENERIC_TITLES so it can't affect
# zxart/modland matches). These are single-word titles from the review sidecar
# that normalize onto a real ZX game key but are actually non-game tokens: a
# leaked format name (asc), musical terms (snare/track/tilt/solo/pops), junk
# demoscene stubs (zzzz/bis/sos/msx10/lsd/math/fax/ram/their), and bare years.
# Distinctive game names (exolon, wizball, renegade, quazatron, ...) are KEPT.
LOCAL_DENY = {
    "asc", "zzzz", "sos", "bis", "msx10", "lsd", "math", "fax", "snare",
    "pops", "ram", "solo", "track", "tilt", "their", "1999", "180",
}


def clean_title(title):
    return SUBIDX_RE.sub("", title).strip()


def load_songs(args):
    """Return list of (url, title). url == song.path (the runtime key)."""
    if args.songs:
        out = []
        for line in Path(args.songs).read_text(
                encoding="utf-8", errors="replace").splitlines():
            c = line.split("\t")
            if len(c) >= 4 and c[3].strip():
                out.append((c[3].strip(), html.unescape(c[0].strip())))
        return out
    db = Path(args.db)
    if not db.exists():
        raise SystemExit(f"music.db not found at {db}; pass --songs")
    con = sqlite3.connect(str(db))
    rows = con.execute(
        "SELECT path,title FROM song WHERE collection="
        "(SELECT ROWID FROM collection WHERE id='zxtunes')").fetchall()
    con.close()
    return [(r[0], r[1]) for r in rows]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zxdb", required=True, help="ZXDB_mysql.sql (unzipped)")
    ap.add_argument("--songs", help="data/zxtunes.txt (skip music.db)")
    ap.add_argument("--db", default=str(DEFAULT_DB), help="music.db for songs")
    args = ap.parse_args()

    title2ids, entry_load_link = parse_zxdb(args.zxdb)
    songs = load_songs(args)
    print(f"zxtunes songs: {len(songs)}")

    results = {}      # url -> wayback screenshot url
    single = []       # (title, url) single-word matches (lower confidence)
    matched_games = set()
    for url, raw in songs:
        title = clean_title(raw)
        k = next((kk for kk in match_keys(title)
                  if len(kk) >= 3 and kk in title2ids
                  and kk not in GENERIC_TITLES and kk not in LOCAL_DENY),
                 None)
        if k is None:
            continue
        eid = min(title2ids[k])           # canonical/original release
        link = entry_load_link.get(eid)
        if not link:
            continue
        results[url] = WAYBACK + urllib.parse.quote(link, safe="/().-_")
        matched_games.add(k)
        if title_words(title) < 2:
            single.append((title, url))

    lines = [f"{u}\t{s}" for u, s in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + ("\n" if lines else ""),
                        encoding="utf-8")
    rev = [f"{t}\t{u}\t{results[u]}" for t, u in sorted(single)]
    REVIEW_FILE.write_text("\n".join(rev) + ("\n" if rev else ""),
                           encoding="utf-8")
    print(f"Wrote {OUT_FILE}: {len(results)} song matches "
          f"across {len(matched_games)} distinct games "
          f"({100*len(results)/max(1,len(songs)):.1f}% of songs); "
          f"{len(single)} single-word -> {REVIEW_FILE.name}")


if __name__ == "__main__":
    main()
