#!/usr/bin/env python3
"""
Build chipmachine/data/zxart_screenshots.txt — ZX Spectrum game loading screens
for the `zxart` collection (zxart.ee chip music, ~28.5k tunes).

Unlike the modland ZX subset (matched by tune *filename*, see
update_zxspectrum_screenshots.py), zxart entries carry the real GAME TITLE in
the title column plus a composer, so matching is high-precision. We re-run the
FULL ZXDB title match here (NOT a reuse of zxspectrum_screenshots.txt) because
zxart covers games the modland pass never hit.

Pipeline (shares parse_zxdb / WAYBACK serving with update_zxspectrum_screenshots):
  * Parse the offline ZXDB MySQL dump -> title/alias -> entry-with-load-screen,
    entry -> World of Spectrum loading-screen file_link.
  * For each zxart song: strict-exact match norm(title) (norm() already strips
    the trailing "(AY)" / "(Beeper)" / "(AY & Beeper)" chip-type tag and all
    punctuation) against ZXDB. Key the output by the song's full zxart.ee URL
    (== song.path), so each AY/Beeper variant of a game gets the same shot.
  * Serve via the worldofspectrum.org Wayback "2id_" mirror (web.archive.org
    speaks HTTP/2, which chipmachine's libcurl requires — see the sibling
    script's docstring for the HTTP/1.1-only-host hang).

zxart is a MIX: real game rips (Chase HQ / Jonathan Dunn) AND original demoscene
chiptunes (a compo track named "circus" or "fly"). So we reuse the SAME
GENERIC_TITLES prune as the modland pass to drop generic/abstract single words
whose ZXDB game match is coincidental; single-word matches that survive go to a
.review sidecar. Distinctive game names (Zaxxon, Sanxion, …) are kept.

Usage:
    python3 update_zxart_screenshots.py --zxdb /path/ZXDB_mysql.sql
    python3 update_zxart_screenshots.py --zxdb F --songs data/zxart.txt
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
OUT_FILE = DATA / "zxart_screenshots.txt"
REVIEW_FILE = HERE / "zxart_screenshots.review.txt"
SONGS_TXT = DATA / "zxart.txt"
DEFAULT_DB = Path(os.path.expanduser("~/Library/Caches/chipmachine/music.db"))

# Reuse the ZXDB parser + Wayback serving + generic-title denylist from the
# sibling script.
_spec = importlib.util.spec_from_file_location(
    "zxshots", HERE / "update_zxspectrum_screenshots.py")
_zx = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_zx)
parse_zxdb, norm, WAYBACK = _zx.parse_zxdb, _zx.norm, _zx.WAYBACK
GENERIC_TITLES = _zx.GENERIC_TITLES


def title_words(title):
    """Word count of the game title with the trailing (AY)/(Beeper) chip tag
    and other parentheticals removed."""
    t = re.sub(r"\([^)]*\)", "", title)
    return len([w for w in re.split(r"[^a-z0-9]+", t.lower()) if w])


_ROMAN = {"i": "1", "ii": "2", "iii": "3", "iv": "4", "v": "5", "vi": "6",
          "vii": "7", "viii": "8", "ix": "9", "x": "10"}
_ARABIC = {v: k for k, v in _ROMAN.items()}


def match_keys(title):
    """Normalized lookup keys for a game title: the strict norm() plus a few
    safe canonicalizations that recover real games differing only by formatting
    — sequel numbers as roman vs arabic (Turrican 2 == Turrican II), '&'/'+'/"'n'"
    vs 'and' (Fire & Ice), and a dropped leading 'The'. Strict key first."""
    yield norm(title)
    t = re.sub(r"\([^)]*\)", "", title.lower())
    t = t.replace("&", " and ").replace("+", " and ").replace("'n'", " and ")
    t = re.sub(r"^\s*the\s+", "", t)
    toks = [w for w in re.split(r"[^a-z0-9]+", t) if w]
    yield "".join(toks)
    yield "".join(_ROMAN.get(w, w) for w in toks)          # arabic sequel form
    if toks and toks[-1] in _ARABIC:                       # roman sequel form
        yield "".join(toks[:-1] + [_ARABIC[toks[-1]]])


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
        "(SELECT ROWID FROM collection WHERE id='zxart')").fetchall()
    con.close()
    return [(r[0], r[1]) for r in rows]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zxdb", required=True, help="ZXDB_mysql.sql (unzipped)")
    ap.add_argument("--songs", help="data/zxart.txt (skip music.db)")
    ap.add_argument("--db", default=str(DEFAULT_DB), help="music.db for songs")
    args = ap.parse_args()

    title2ids, entry_load_link = parse_zxdb(args.zxdb)
    songs = load_songs(args)
    print(f"zxart songs: {len(songs)}")

    results = {}      # url -> wayback screenshot url
    single = []       # (title, url) single-word matches (lower confidence)
    matched_games = set()
    for url, title in songs:
        k = next((kk for kk in match_keys(title)
                  if len(kk) >= 3 and kk in title2ids and kk not in GENERIC_TITLES),
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
