#!/usr/bin/env python3
"""
Build chipmachine/data/sndh_screenshots.txt — Atari ST game screenshots for the
sndh (sndh.atari.org) music collection, sourced from Atari Mania.

We do NOT crawl atarimania.com. Its screenshots are enumerated from the Internet
Archive Wayback CDX index in a SINGLE bulk query and served at runtime through
the Wayback "2id_" mirror — same polite approach as the gb64/hvtc fixes
([[gb64-screenshots-cloudflare-wayback]], [[hvtc-plus4-screenshots-wayback]]).
Zero load on atarimania.

Matching is STRICTLY exact (normalized title == normalized atarimania game stem,
publisher suffix stripped). Prefix/fuzzy matching was tried and rejected — it
mass-produces false positives (easy->easyrider, Jumps->jumpster, MIST->misty).
Even strict exact has ~15% residual error from short single-word tune titles that
coincide with a game name (Popcorn, Dreams, Dentro) — sndh is a MIX of game rips
and demoscene/chip-musician tunes, and a game DB has no art for the latter. Per
the maintainer's call we ship all strict-exact matches (max coverage); the
single-word subset is written to a .review sidecar for eyeball pruning.

Output (TAB separated), keyed by the song's "<composer>/<game>.sndh" path:
    <composer>/<game>.sndh <TAB> https://web.archive.org/web/2id_/http://www.atarimania.com/st/screens/<file>

Usage:
    python3 update_sndh_screenshots.py            # fetch CDX, match, write file
    python3 update_sndh_screenshots.py --cdx F    # use a local CDX dump
"""

import re
import sys
import argparse
import urllib.parse
import urllib.request
from pathlib import Path
from collections import defaultdict

CDX_URL = ("http://web.archive.org/cdx/search/cdx?"
           "url=atarimania.com/st/screens&matchType=prefix&"
           "collapse=urlkey&fl=original&filter=statuscode:200&limit=200000")
WAYBACK = "https://web.archive.org/web/2id_/http://www.atarimania.com"

# Hand-pruned false positives: strict-exact matched a game name but the tune is
# actually a chip-musician cover / demo-intro, not a game rip (sndh is a mix).
# Keyed by song path. Reviewed from the single-word match set.
EXCLUDE = {
    "Kennedy_Matt/Batdance.sndh",            # Prince cover
    "Mad_Max/Demos/Best_In_Galaxy/Zoolook.sndh",  # JMJ cover
    "Tao/Zoolook.sndh",                      # JMJ cover
    "505/Oxygene.sndh",                      # JMJ cover
    "Techno/Dentro.sndh",                    # "dentro" = demo/intro
    "Xrwwr/Dentro.sndh",
    "Epic/Phototro.sndh",                    # -tro = intro
    "AJT/Beachtro.sndh",                     # -tro = intro
    "505/Trace_Intro.sndh",                  # intro
    "Furax/Preview.sndh",                    # generic/demo
    "Junosix/DMA/Rave.sndh",                 # Falcon demo
    "505/DMA/Again.sndh",                    # demo
    "AJT/Colorz.sndh",                       # demo
    "Excellence_In_Art/Spunge.sndh",         # demo-group abstract
    "ACC/Popcorn.sndh",                      # Hot Butter cover (chip staple)
    "Herceg_Dorde/Popcorn.sndh",
    "Marcer/Popcorn.sndh",
    "Modmate/Popcorn.sndh",
    "Kenet/Gameboy.sndh",                    # demo
    "Kenet/Oldiez.sndh",                     # oldies medley
    "Dalecki_Adrian/Logo.sndh",              # generic
    "Stu/Player.sndh",                       # generic
    "Modmate/Xmas.sndh",                     # chip tune
    "505/Xmas.sndh",
    "Dma-Sc/Dread.sndh",                     # chip tune (Dma-Sc)
}

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
SONG_LIST = DATA / "sndh.txt"
OUT_FILE = DATA / "sndh_screenshots.txt"
REVIEW_FILE = HERE / "sndh_screenshots.review.txt"


def fetch_cdx():
    req = urllib.request.Request(CDX_URL, headers={"User-Agent": "sndh-shots/1.0"})
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read().decode("utf-8", errors="replace")


def norm(s):
    """Normalize a title/filename to a comparison key: decode, lowercase, drop
    [publisher] brackets, keep alphanumerics only."""
    s = urllib.parse.unquote(s).lower()
    s = re.sub(r"\[[^\]]*\]", "", s)
    return re.sub(r"[^a-z0-9]+", "", s)


def words(s):
    return len(re.sub(r"[^a-z0-9 ]", " ", s.lower()).split())


def build_game_index(raw):
    """norm(game stem) -> sorted list of screenshot rel-paths. The atarimania
    filename is '<game>_<publisher>[_N].<ext>'; we strip the _N shot numbering
    but keep the publisher in the stored filename (it's the real archived path)."""
    files = set(re.findall(r"/st/screens/[^\s?]+?\.(?:gif|png|jpg)", raw, re.I))
    games = defaultdict(list)
    for rel in files:
        rel = rel.lower()
        fn = urllib.parse.unquote(rel).rsplit("/", 1)[-1]
        stem = re.sub(r"\.(gif|png|jpg)$", "", fn, flags=re.I)
        stem = re.sub(r"_\d+$", "", stem)          # drop _2/_3 shot numbering
        games[norm(stem)].append(rel)
    for k in games:
        games[k].sort()                            # base shot (no _N) sorts first
    return games


def load_songs():
    out = []
    for line in SONG_LIST.read_text(encoding="utf-8", errors="replace").splitlines():
        c = line.split("\t")
        if len(c) >= 5 and c[-1].strip().endswith(".sndh"):
            out.append((c[0].strip(), c[-1].strip()))   # (title, path)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cdx", help="local CDX dump (skip network)")
    ap.add_argument("--minlen", type=int, default=4,
                    help="min normalized-title length to match (kills coincidences)")
    args = ap.parse_args()

    raw = Path(args.cdx).read_text() if args.cdx else fetch_cdx()
    games = build_game_index(raw)
    print(f"atarimania ST index: {len(games)} distinct games "
          f"({sum(len(v) for v in games.values())} screenshots)")

    songs = load_songs()
    results = {}          # path -> rel screenshot
    single = []           # (title, path) single-word matches (lower confidence)
    for title, path in songs:
        if path in EXCLUDE:
            continue
        key = norm(title)
        if len(key) >= args.minlen and key in games:
            results[path] = games[key][0]
            if words(title) < 2:
                single.append((title, path))

    lines = [f"{p}\t{WAYBACK}{rel}" for p, rel in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")

    rev = [f"{t}\t{p}\t{WAYBACK}{results[p]}" for t, p in sorted(single)]
    REVIEW_FILE.write_text("\n".join(rev) + "\n", encoding="utf-8")

    print(f"Wrote {OUT_FILE}: {len(results)} matches "
          f"({len(results)-len(single)} multi-word high-confidence, "
          f"{len(single)} single-word -> {REVIEW_FILE.name} for review)")


if __name__ == "__main__":
    main()
