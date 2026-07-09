#!/usr/bin/env python3
"""Build chipmachine/data/rko_screenshots.txt for the remix.kwed.org (rko) set.

rko is C64 SID *remixes* (MP3s). Each row in data/rko.txt is
    <id>\t<hvsc-sid-path>\t<sidsong>\t<title>\t<composer>\t<rating>
(song_template "path sidname sidsong title composer rating" -> col1 = the remix
id, which is what the app stores as the song path, so screenshots are keyed by
col1). Col2 is the ORIGINAL HVSC SID the remix covers -- a direct link we exploit
to borrow a screenshot of the source game/tune:

  1. gb64 (Games.csv): SidFilename column carries the same HVSC path (backslash
     form). Its ScrnshotFilename + the gb64 Wayback screen_source gives the actual
     GAME screenshot -- the right image for a game-music remix. (~73% of rows.)
  2. else hvsc_screenshots.txt (demozoo-augmented SID screenshots, keyed by the
     same HVSC path): a demo that uses the tune. (~another 9%.)

Combined ~82%. Emits data/rko_screenshots.txt keyed by the remix id (col1), values
full http URLs (gb64 Wayback png or media.demozoo.org), consumed by the "rko"
branch of MusicDatabase::getSongScreenshots.

  python3 chipmachine/scripts/build_rko.py --screenshots
"""

import csv
import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
RKO = os.path.join(DATA, "rko.txt")
GAMES = os.path.join(DATA, "Games.csv")
HVSC_SHOTS = os.path.join(DATA, "hvsc_screenshots.txt")
OUT = os.path.join(DATA, "rko_screenshots.txt")
# Same Wayback mirror db.lua uses for gb64 (gb64.com is Cloudflare-walled live).
GB64_PREFIX = ("https://web.archive.org/web/2id_/"
               "http://www.gb64.com/Screenshots/")


def _norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def gb64_map():
    """HVSC sid path -> [(game_name, screenshot_url), ...].

    A single SID is frequently referenced by MANY gb64 entries (the original
    game plus cracks/hacks/other games that reuse the tune), so we keep ALL of
    them and let the caller disambiguate by the remix's title -- picking the
    lowest GA_Id blindly lands on the wrong game (Commando -> AFL Boulder Dash)."""
    m = {}
    with open(GAMES, encoding="utf-8", errors="replace") as f:
        for r in csv.reader(f):
            if len(r) < 13 or r[0] == "GA_Id":
                continue
            sid = r[12].strip().replace("\\", "/")     # SidFilename
            shot = r[6].strip().replace("\\", "/")     # ScrnshotFilename
            if sid and shot and shot != "0":
                m.setdefault(sid, []).append((r[1], GB64_PREFIX + shot))
    return m


def hvsc_map():
    """HVSC sid path -> screenshot URL (demozoo-augmented set)."""
    m = {}
    if os.path.exists(HVSC_SHOTS):
        with open(HVSC_SHOTS, encoding="utf-8", errors="replace") as f:
            for line in f:
                t = line.find("\t")
                if t > 0:
                    m[line[:t]] = line[t + 1:].rstrip("\n")
    return m


def screenshots():
    gb64 = gb64_map()
    hvsc = hvsc_map()
    sys.stderr.write(f"gb64 sid->shot: {len(gb64)}, hvsc sid->shot: {len(hvsc)}\n")
    rows, gb, hv = [], 0, 0
    with open(RKO, encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 4:
                continue
            rid, sid = c[0], c[1]
            # rko title (col4) is the game name; strip a remixer's "(... mix)" /
            # "[...]" suffix so it compares against the gb64 game name.
            title = _norm(re.sub(r"\s*[\(\[].*$", "", c[3]))
            shot = None
            cands = gb64.get(sid)
            if cands:
                # Disambiguate SID reuse: pick the gb64 game whose name best
                # matches the remix title. Only trust it when it's a real match
                # (exact / substring / ratio>=0.6); otherwise the SID is reused
                # and none of these games is the remix's -> fall back to the
                # SID-keyed demo shot (correct tune) instead of a wrong game.
                best, score = None, 0.0
                for name, url in cands:
                    n = _norm(name)
                    s = 1.0 if n == title else \
                        0.9 if (title and (title in n or n in title)) else \
                        difflib.SequenceMatcher(None, title, n).ratio()
                    if s > score:
                        best, score = url, s
                if score >= 0.6:
                    shot, gb = best, gb + 1
            if not shot:
                shot = hvsc.get(sid)
                if shot:
                    hv += 1
            if shot:
                rows.append(rid + "\t" + shot)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    total = sum(1 for _ in open(RKO, encoding="utf-8", errors="replace"))
    print(f"wrote {len(rows)}/{total} screenshots ({gb} gb64 + {hv} hvsc) -> {OUT}")


if __name__ == "__main__":
    if "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
