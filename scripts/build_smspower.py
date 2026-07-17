#!/usr/bin/env python3
"""Build chipmachine/data/smspower.txt -- SMS Power! Sega 8-bit VGM gamerips.

SMS Power (smspower.org) is the definitive Sega Master System / Game Gear /
SG-1000 / ColecoVision music vault. Each game has a pre-packaged zip of per-track
.vgm files at /uploads/Music/<Game>-<Console>[-FM|-PSG].zip, plus a sibling
title-screen <...>.png. The host fetches the zip (browser-UA disguise passes
Cloudflare), extracts it by magic and plays the .vgm tracks as subsongs -- the
same pipeline as zophar/vgmrips. All VGM here is SN76489 PSG (some SMS games add
YM2413 FM); both play via GME's Vgm_Emu, so there is no new format.

DEDUP: modland already has "Video Game Music/Sega Master System" (184 games) and
"Sega Game Gear" (33 games) -- ~217 total, all we already index. SMS Power is far
more comprehensive, so we keep only the games modland lacks (~165+ net-new,
skewing GG/SMS/SG-1000/Homebrew/ColecoVision).

ENUMERATION is polite: the whole catalog is read from the Wayback Machine CDX
index of /uploads/Music/* (no live-site crawl). The path column then points at
the LIVE smspower.org URL, fetched on demand only when a user plays a game.

  --build        read the CDX, dedup vs modland, write smspower.txt
  --screenshots  emit smspower_screenshots.txt keyed by the pack URL -> the live
                 title-screen .png (only for games whose .png is archived)
"""

import os
import re
import sys
import time
import urllib.parse
import urllib.request

BASE = "https://www.smspower.org/uploads/Music/"
CDX = ("http://web.archive.org/cdx/search/cdx?url=smspower.org/uploads/Music*"
       "&output=text&fl=original,statuscode,mimetype&collapse=urlkey")
ALLMODS = os.path.join(os.path.dirname(__file__), "..", "data",
                       "allmods.txt")
OUT = os.path.join(os.path.dirname(__file__), "..", "data",
                   "smspower.txt")
OUT_SHOTS = os.path.join(os.path.dirname(__file__), "..", "data",
                         "smspower_screenshots.txt")
UA = {"User-Agent": "Mozilla/5.0 chipmachine-smspower/1.0"}

# Trailing tokens on a pack stem. Console tokens decide the platform/format
# label; chip tokens (FM/PSG) mark a same-game sound-chip variant.
CONSOLE = {"SMS": "Sega Master System", "GG": "Sega Game Gear",
           "SG": "Sega SG-1000", "SC3000": "Sega SG-1000",
           "ColecoVision": "ColecoVision", "Coleco": "ColecoVision",
           "Homebrew": "Sega Master System"}
CHIP = {"FM", "PSG", "YM2413", "Stereo"}
TAGS = set(CONSOLE) | CHIP


def norm(s):
    """Normalize a game name for dedup matching (lowercase, alnum only)."""
    return re.sub(r"[^a-z0-9]", "", s.lower())


def camel_split(stem):
    """A readable title from a CamelCase/underscore pack stem, e.g.
    'AlexKiddHighTechWorld' -> 'Alex Kidd High Tech World'. Best-effort: the
    real per-track names live inside the zip and show during playback."""
    s = stem.replace("_", " ")
    s = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", s)      # aB   -> a B
    s = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", " ", s)    # ABc  -> A Bc
    s = re.sub(r"(?<=[A-Za-z])(?=[0-9])", " ", s)      # a1   -> a 1
    return re.sub(r"\s+", " ", s).strip()


def parse_pack(stem):
    """(game_stem, console_label, chip) from a pack stem, or None if it has no
    recognized console suffix (drops tool/source uploads like vgmplayer045)."""
    toks = stem.split("-")
    if len(toks) < 2 or toks[-1] not in TAGS:
        return None
    console = None
    chip = ""
    while len(toks) > 1 and toks[-1] in TAGS:
        t = toks.pop()
        if t in CONSOLE and console is None:
            console = CONSOLE[t]
        elif t in CHIP and not chip:
            chip = t
    if console is None:                                 # only a chip tag, no console
        return None
    return "-".join(toks), console, chip


MODLAND_DUP_PREFIXES = (
    "Video Game Music/Sega Master System/",
    "Video Game Music/Sega Game Gear/",
    # ColecoVision too: modland already carries the (playable) .vgz rips under
    # "Video Game Music/Colecovision/<composer>/<game>/". Without this prefix,
    # Antarctic Adventure & M.A.S.H re-entered smspower.txt as duplicates whose
    # live /uploads/Music/*-ColecoVision.zip packs 500 on smspower's server --
    # dead rows shadowing the working modland entries in the ColecoVision drill.
    "Video Game Music/Colecovision/",
)


def modland_dups():
    """Normalized SMS/GG game names already in modland. Path layout:
    Video Game Music/<system>/<composer>/<game>/<track> -> game = parts[3]."""
    names = set()
    try:
        with open(ALLMODS, encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.split("\t", 1)[-1].strip()
                if not p.startswith(MODLAND_DUP_PREFIXES):
                    continue
                parts = p.split("/")
                if len(parts) >= 5:
                    names.add(norm(parts[3]))
    except FileNotFoundError:
        pass
    return names


def fetch(url, tries=4):
    last = None
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=120) as r:
                return r.read().decode("utf-8", "replace")
        except Exception as e:                          # Wayback CDX is flaky
            last = e
            time.sleep(2 * (i + 1))
    raise last


def cdx_files(ext):
    """Set of url-encoded '<stem>.<ext>' basenames archived (status 200) under
    /uploads/Music/, from the Wayback CDX."""
    text = fetch(CDX)
    out = {}
    for line in text.splitlines():
        c = line.split(" ")
        if len(c) < 2 or c[1] != "200":
            continue
        m = re.search(r"/uploads/Music/([^?/ ]+\." + ext + r")$", c[0])
        if m:
            out.setdefault(m.group(1), True)            # keep first (any) capture
    return set(out)


def build():
    dup = modland_dups()
    sys.stderr.write(f"modland SMS/GG games: {len(dup)}\n")
    packs = cdx_files("zip")
    sys.stderr.write(f"archived packs: {len(packs)}\n")
    rows, kept, skipped, noncanon, seen = [], 0, 0, 0, set()
    for enc in sorted(packs):
        stem = urllib.parse.unquote(enc)[:-4]           # strip ".zip"
        parsed = parse_pack(stem)
        if not parsed:
            noncanon += 1
            continue
        game_stem, console, chip = parsed
        if norm(game_stem) in dup:
            skipped += 1
            continue
        key = (norm(game_stem), chip)                   # keep FM & PSG variants both
        if key in seen:
            continue
        seen.add(key)
        title = camel_split(game_stem)
        if chip:
            title += f" ({chip})"
        url = BASE + enc
        # song_template = "title composer format path ext"
        rows.append("\t".join([title, "", console, url, "vgm"]))
        kept += 1
    with open(os.path.normpath(OUT), "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {kept} rows -> {os.path.normpath(OUT)} "
          f"({skipped} modland dups, {noncanon} non-canonical skipped)")


def screenshots():
    pngs = cdx_files("png")
    sys.stderr.write(f"archived title screens: {len(pngs)}\n")
    rows, matched, total = [], 0, 0
    with open(os.path.normpath(OUT), encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 4 or not c[3]:
                continue
            total += 1
            enc_png = c[3][len(BASE):-4] + ".png"        # <stem>.zip -> <stem>.png
            if enc_png in pngs:
                rows.append(c[3] + "\t" + BASE + enc_png)
                matched += 1
    with open(os.path.normpath(OUT_SHOTS), "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {matched}/{total} screenshots -> {os.path.normpath(OUT_SHOTS)}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
