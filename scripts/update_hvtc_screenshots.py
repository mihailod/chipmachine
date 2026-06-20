#!/usr/bin/env python3
"""
Build chipmachine/data/hvtc_screenshots.txt — game screenshots for the HVTC
(High Voltage TED Collection) Plus/4 music, sourced from Commodore Plus/4 World.

We do NOT hit plus4world.powweb.com directly: it's a flaky shared host that
rate-limits / black-holes under load. Instead we enumerate its screenshots from
the Internet Archive's Wayback CDX index (one query, reliable) and the runtime
serves the images through the Wayback "2id_" mirror — exactly like the gb64
screenshot fix. (See memory: gb64-screenshots-cloudflare-wayback.)

Only HVTC songs whose path is prefixed "games/" are matched (demos/musicians/
other aren't game entries).

Matching: HVTC is hosted ON Plus/4 World, so our .prg basenames
("games/mega_chase.prg") already align with the screenshot stems
("/dl/screenshots/m/mega_chase_title.gif"). We pick the best archived file for
each basename using, in order:
  1. exact stem + suffix variant (_title, _main, plain, ...)
  2. stem with trailing tune-words stripped (corman_death -> corman)
  3. progressive trailing-word drop (indoor_sports_bowling -> indoor_sports)
  4. substring containment across the index (orosz-a_szavak... contains the stem)
  5. a small hand-curated override table for genuine renames.
Digit-initial stems live in the "0" bucket, not a digit-named dir.

Output (TAB separated), one line per matched song:
    games/<name>.prg <TAB> https://web.archive.org/web/2id_/http://plus4world...gif
Full absolute Wayback URLs are stored (like csdb stores full csdb.dk URLs),
because the hvtc collection's `url` column is taken by its song-download source,
so the runtime can't derive the screenshot host from it.

Usage:
    python3 update_hvtc_screenshots.py            # fetch CDX, match, write file
    python3 update_hvtc_screenshots.py --cdx F    # use a local CDX dump instead
    python3 update_hvtc_screenshots.py --verify    # also HEAD-check matches via Wayback
"""

import re
import sys
import argparse
import urllib.request
import urllib.error
from pathlib import Path

CDX_URL = ("http://web.archive.org/cdx/search/cdx?"
           "url=plus4world.powweb.com/dl/screenshots&matchType=prefix&"
           "collapse=urlkey&fl=original&filter=statuscode:200&limit=100000")
WAYBACK = "https://web.archive.org/web/2id_/http://plus4world.powweb.com"

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
SONG_LIST = DATA / "hvtc.txt"
OUT_FILE = DATA / "hvtc_screenshots.txt"

# Trailing words denoting a sub-tune of a game rather than the game name.
TUNE = {"title", "ingame", "intro", "jingle", "instructions", "crack", "early",
        "digi", "text", "loader", "hiscore", "highscore", "main", "menu",
        "ending", "tune", "music", "demo", "game", "preview", "death", "in",
        "c16", "c-16", "p4", "plus4"}
SUFFIXES = ["_title", "_main", "", "_ingame", "_1", "_01", "_pic", "_bag",
            "_alt", "_intro"]
# Genuine renames the heuristics can't derive (stem -> screenshot rel path).
OVERRIDES = {
    "way_of_the_exploding_fist": "/dl/screenshots/e/exploding_fist_+4.gif",
}


def fetch_cdx():
    req = urllib.request.Request(CDX_URL, headers={"User-Agent": "hvtc-shots/1.0"})
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read().decode("utf-8", errors="replace")


def head_ok(url):
    try:
        req = urllib.request.Request(url, method="HEAD",
                                     headers={"User-Agent": "hvtc-shots/1.0"})
        with urllib.request.urlopen(req, timeout=30) as r:
            return r.status == 200
    except Exception:
        return False


def bucket(stem):
    return "0" if stem[0].isdigit() else stem[0].lower()


def strip_tune(stem):
    w = stem.split("_")
    while len(w) > 1 and w[-1] in TUNE:
        w.pop()
    return "_".join(w)


def pick_in_dir(index, l, stem):
    """Best file for an exact stem in directory l: suffix variants, then loose."""
    for sfx in SUFFIXES:
        rel = f"/dl/screenshots/{l}/{stem}{sfx}.gif"
        if rel in index:
            return rel
    pre = f"/dl/screenshots/{l}/{stem}"
    loose = sorted(x for x in index if x.startswith(pre + "_") or x == pre + ".gif")
    for want in ("_title.gif", "_main.gif"):
        for x in loose:
            if x.endswith(want):
                return x
    return loose[0] if loose else ""


def match(index, by_dir, stem):
    if stem in OVERRIDES:
        return OVERRIDES[stem]
    l = bucket(stem)
    # 1+2: exact stem, then tune-stripped stem
    for s in dict.fromkeys([stem, strip_tune(stem)]):
        r = pick_in_dir(index, bucket(s), s)
        if r:
            return r
    # 3: progressive trailing-word drop (keep >=1 word, stem stays specific)
    words = stem.split("_")
    while len(words) > 1:
        words.pop()
        s = "_".join(words)
        if len(s) >= 3:
            r = pick_in_dir(index, bucket(s), s)
            if r:
                return r
    # 4: substring containment (e.g. orosz-<stem>); prefer _title, shortest
    if len(stem) >= 8:
        cand = [x for x in by_dir.get(l, ()) if stem in x]
        cand += [x for x in by_dir.get(bucket("o"), ()) if stem in x]  # orosz- bucket
        if cand:
            cand = sorted(set(cand), key=lambda x: (("_title" not in x),
                                                    ("_main" not in x), len(x)))
            return cand[0]
    return ""


def load_games():
    out = []
    for line in SONG_LIST.read_text(encoding="utf-8", errors="replace").splitlines():
        c = line.split("\t")
        if len(c) >= 5 and c[-1].strip().startswith("games/"):
            out.append((c[-1].strip(), c[0].strip()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cdx", help="local CDX dump (skip network)")
    ap.add_argument("--verify", action="store_true",
                    help="HEAD-check each match via the Wayback mirror")
    args = ap.parse_args()

    raw = Path(args.cdx).read_text() if args.cdx else fetch_cdx()
    index = set(x.lower() for x in
                re.findall(r"/dl/screenshots/[a-z0-9]/[^\s]+?\.(?:gif|png|jpg)",
                           raw, re.IGNORECASE))
    by_dir = {}
    for x in index:
        by_dir.setdefault(x.split("/")[3], []).append(x)
    print(f"CDX index: {len(index)} distinct screenshots")

    games = load_games()
    results, misses = {}, []
    for path, title in games:
        stem = Path(path).stem.lower()
        rel = match(index, by_dir, stem)
        if rel and args.verify and not head_ok(WAYBACK + rel):
            print(f"  verify FAIL {rel}", file=sys.stderr)
            rel = ""
        if rel:
            results[path] = rel
        else:
            misses.append(title)

    lines = [f"{p}\t{WAYBACK}{s}" for p, s in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_FILE}: {len(results)}/{len(games)} matched")
    if misses:
        print("Unmatched (no archived screenshot):")
        for t in misses:
            print(f"  {t}")


if __name__ == "__main__":
    main()
