#!/usr/bin/env python3
"""
Build chipmachine/data/projectay_screenshots.txt — ZX Spectrum game loading
screens for the `projectay` collection (Ironfist / Bulba .ay game rips).

Project AY is HIGH-signal for this: the ZX rips carry the real GAME NAME as the
title (Ironfist's PMisc, e.g. "Arkanoid", "Chase HQ 2") with composer attribution,
so a strict ZXDB title match lands ~4 in 5 (vs zxart's mixed 4%). Only the
"Spectrum AY" rows are matched; the "Amstrad CPC" rows are demoscene demos/mags
and ZXDB is ZX-only, so they get no screenshot (graceful blank).

Shares the whole pipeline with update_zxart_screenshots.py:
  * parse_zxdb: offline ZXDB MySQL dump -> title/alias -> entry-with-load-screen,
    entry -> World of Spectrum loading-screen file_link.
  * match_keys: strict norm(title) plus safe roman<->arabic sequel / '&'->and /
    dropped-leading-"The" canonicalizations.
  * GENERIC_TITLES prune for coincidental single-word matches; the survivors are
    written to a .review sidecar for a manual audit.
  * Serve via the worldofspectrum.org Wayback "2id_" mirror (web.archive.org
    speaks HTTP/2, which chipmachine's libcurl requires).

Unlike zxart (keyed by the full zxart.ee URL), the runtime key here is the song
PATH stored in the DB (col 3 of projectay.txt, e.g. "ironfist/arkanoid.ay") --
getSongScreenshots looks up parts[1] of "projectay::<path>".

Usage:
    python3 update_projectay_screenshots.py --zxdb /path/ZXDB_mysql.sql
"""

import argparse
import html
import importlib.util
import urllib.parse
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
OUT_FILE = DATA / "projectay_screenshots.txt"
REVIEW_FILE = HERE / "projectay_screenshots.review.txt"
SONGS_TXT = DATA / "projectay.txt"

# Reuse the ZXDB parser + Wayback serving + generic denylist + match_keys/
# title_words helpers from the zxart script (which itself imports the base from
# update_zxspectrum_screenshots.py).
_spec = importlib.util.spec_from_file_location(
    "zxart_shots", HERE / "update_zxart_screenshots.py")
_za = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_za)
parse_zxdb, WAYBACK = _za.parse_zxdb, _za.WAYBACK
GENERIC_TITLES = _za.GENERIC_TITLES
match_keys, title_words = _za.match_keys, _za.title_words


def load_zx_songs():
    """(path, title) for every 'Spectrum AY' row; CPC rows are skipped."""
    out = []
    for line in SONGS_TXT.read_text(encoding="utf-8",
                                    errors="replace").splitlines():
        c = line.split("\t")
        if len(c) >= 4 and c[2].strip() == "Spectrum AY" and c[3].strip():
            out.append((c[3].strip(), html.unescape(c[0].strip())))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zxdb", required=True, help="ZXDB_mysql.sql (unzipped)")
    args = ap.parse_args()

    title2ids, entry_load_link = parse_zxdb(args.zxdb)
    songs = load_zx_songs()
    print(f"projectay ZX songs: {len(songs)}")

    results = {}          # path -> wayback screenshot url
    single = []           # (title, path) single-word matches (lower confidence)
    matched_games = set()
    for path, title in songs:
        k = next((kk for kk in match_keys(title)
                  if len(kk) >= 3 and kk in title2ids and kk not in GENERIC_TITLES),
                 None)
        if k is None:
            continue
        eid = min(title2ids[k])           # canonical/original release
        link = entry_load_link.get(eid)
        if not link:
            continue
        results[path] = WAYBACK + urllib.parse.quote(link, safe="/().-_")
        matched_games.add(k)
        if title_words(title) < 2:
            single.append((title, path))

    lines = [f"{p}\t{s}" for p, s in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + ("\n" if lines else ""),
                        encoding="utf-8")
    rev = [f"{t}\t{p}\t{results[p]}" for t, p in sorted(single)]
    REVIEW_FILE.write_text("\n".join(rev) + ("\n" if rev else ""),
                           encoding="utf-8")
    print(f"Wrote {OUT_FILE}: {len(results)} song matches "
          f"across {len(matched_games)} distinct games "
          f"({100*len(results)/max(1,len(songs)):.1f}% of ZX songs); "
          f"{len(single)} single-word -> {REVIEW_FILE.name}")


if __name__ == "__main__":
    main()
