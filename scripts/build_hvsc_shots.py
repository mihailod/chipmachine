#!/usr/bin/env python3
"""Overlay gb64 GAME artwork onto data/hvsc_screenshots.txt (prefer game > demo).

hvsc_screenshots.txt is otherwise demozoo-augmented DEMO screens -- a demo that
happens to use the tune as soundtrack. For a game SID (Commando, Comic Bakery,
Delta, Monty on the Run...) the RIGHT image is the game's own screenshot, which
gb64 (Games.csv) links by the very same HVSC SID path (its SidFilename column).
This overlays gb64 game art as the PREFERRED shot and keeps the demozoo demo
screen only where gb64 has no confident match.

Disambiguation: one SID is referenced by MANY gb64 entries (the original game +
cracks/hacks/other games reusing the tune), so blindly taking the first lands on
e.g. "AFL Boulder Dash" for Commando. We pick the candidate whose game name best
matches the HVSC song title (col1) or the SID basename (>=0.6), same idea as
build_rko.py.

Run AFTER build_demozoo.py --augment (which writes the demo baseline this reads).
  python3 chipmachine/scripts/build_hvsc_shots.py --build
"""

import difflib
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_rko import gb64_map, _norm       # shared gb64 Games.csv parsing

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
HVSC = os.path.join(DATA, "hvsc.txt")
SHOTS = os.path.join(DATA, "hvsc_screenshots.txt")


def _best(cands, *names):
    """Best (score, url) among gb64 candidates vs any of the given norm names."""
    best, score = None, 0.0
    for gname, url in cands:
        g = _norm(gname)
        for n in names:
            if not n:
                continue
            s = 1.0 if g == n else \
                0.9 if (n in g or g in n) else \
                difflib.SequenceMatcher(None, n, g).ratio()
            if s > score:
                best, score = url, s
    return best, score


def build():
    gb = gb64_map()                                  # sid -> [(name, url)]
    # demo baseline (whatever is currently in the file)
    demo = {}
    if os.path.exists(SHOTS):
        with open(SHOTS, encoding="utf-8", errors="replace") as f:
            for line in f:
                t = line.find("\t")
                if t > 0:
                    demo[line[:t]] = line[t + 1:].rstrip("\n")

    out = dict(demo)
    gbn = over = add = 0
    with open(HVSC, encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 5:
                continue
            title, path = c[0], c[4]
            cands = gb.get(path)
            if not cands:
                continue
            base = path.rsplit("/", 1)[-1]
            base = re.sub(r"\.sid$", "", base, flags=re.I).replace("_", " ")
            url, score = _best(cands, _norm(title), _norm(base))
            if url and score >= 0.6:
                if path not in demo:
                    add += 1
                elif demo[path] != url:
                    over += 1
                out[path] = url
                gbn += 1

    with open(SHOTS, "w", encoding="utf-8") as f:
        for k in sorted(out):
            f.write(k + "\t" + out[k] + "\n")
    print(f"wrote {len(out)} hvsc screenshots -> {SHOTS}")
    print(f"  gb64 game art: {gbn} ({over} replaced a demo screen, {add} new); "
          f"demo screens kept for the rest")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    else:
        print(__doc__)
