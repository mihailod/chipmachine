#!/usr/bin/env python3
"""Build chipmachine/data/vgmrips.txt -- VGMRips (vgmrips.net) game rips.

Source: the Internet Archive item "vgmrips-all-of-them", a single outer zip
(VGMRips_all_of_them_2024-05-18.zip). archive.org serves any member of that zip
directly via ".../<outerzip>.zip/<url-encoded-member>". Each game is one inner
".zip" (per-track .vgz + a .png screenshot + .m3u/.txt); there is also a sibling
top-level ".png" for the game. The manifest is data/misc/vgmrips/files.csv
(path, date, size, top-platform, url-encoded-suffix), left by an earlier pass.

The host downloads the inner game zip and the ZIP-by-magic handler in
MusicPlayerList extracts the .vgz tracks and plays them as local subsongs (the
same path Zophar uses). So each vgmrips.txt row is a single song whose path is
the full archive.org member URL of the game zip.

Routing: VGM is a multi-chip container. GME's Vgm_Emu only decodes Sega/AY logs;
the rest (NES APU, GameBoy, HuC6280, the OPN family, Neo Geo / arcade sample
chips, ...) route to libvgmplugin via the chip gate in vgm_opl_detect.h.

Dedup: modland's "Video Game Music" is ~99% Sega (Mega Drive / SMS / GG) and
Zophar already onboarded ~335 Genesis games, so the only real overlap is the
MegaDrive subset -- we title-dedup that vs modland Sega + zophar and keep every
other platform (arcade, PC-98/88, X68000, FM Towns, TurboGrafx, Neo Geo, NES,
GameBoy, WonderSwan, ...), which are genuinely net-new.

  --build        parse files.csv, dedup MegaDrive, write vgmrips.txt
  --screenshots  pair each game zip to its sibling .png, write
                 vgmrips_screenshots.txt keyed by the song URL in vgmrips.txt
"""

import csv
import os
import re
import sys
import urllib.parse

HERE = os.path.dirname(__file__)
CSV = os.path.join(HERE, "..", "data", "misc", "vgmrips",
                   "files.csv")
ALLMODS = os.path.join(HERE, "..", "data", "allmods.txt")
ZOPHAR = os.path.join(HERE, "..", "data", "zophar.txt")
OUT = os.path.join(HERE, "..", "data", "vgmrips.txt")
OUT_SHOTS = os.path.join(HERE, "..", "data",
                         "vgmrips_screenshots.txt")

BASE = ("https://archive.org/download/vgmrips-all-of-them/"
        "VGMRips_all_of_them_2024-05-18.zip")


def norm(s):
    """Normalize a game title for dedup (lowercase, alnum only)."""
    return re.sub(r"[^a-z0-9]", "", s.lower())


# Top-level CSV platform -> the display platform shown in search results. The
# "Computers/*" and "Other" buckets are refined per-file from the hardware tag in
# the filename's trailing parenthetical (VGMRips names are consistent about it).
PLATFORM = {
    "MegaDrive": "Sega Mega Drive",
    "NES": "NES",
    "GameBoy": "Game Boy",
    "TurboGrafx": "PC Engine",
    "NeoGeo": "Neo Geo",
    "NeoGeoPocket": "Neo Geo Pocket",
    "WonderSwan": "WonderSwan",
    "SegaPico": "Sega Pico",
    "Pinball": "Pinball",
    "Arcade": "Arcade",
    "Arcade/Capcom": "Arcade (Capcom)",
    "Arcade/Konami": "Arcade (Konami)",
    "Arcade/Namco": "Arcade (Namco)",
    "Arcade/SegaSys": "Arcade (Sega)",
    "Arcade/Taito": "Arcade (Taito)",
    "Computers/MSX": "MSX",
    "Computers/NEC": "NEC PC-88/98",
    "Computers/Sharp": "Sharp X68000",
    "Computers/Fujitsu": "FM Towns",
    "Computers/IBM_PC": "IBM PC",
    "Computers/Atari": "Atari ST",
    "Computers/ZX_Spectrum": "ZX Spectrum",
    "Other": "Other",
}

# Hardware-tag substrings (as they appear in the filename parenthetical) -> a
# cleaner platform label, used to refine the "Other" and "Computers/*" buckets.
TAG_PLATFORM = [
    ("PC-9801", "NEC PC-98"), ("PC-98", "NEC PC-98"),
    ("PC-8801", "NEC PC-88"), ("PC-8001", "NEC PC-80"), ("PC-88", "NEC PC-88"),
    ("X68000", "Sharp X68000"), ("X1", "Sharp X1"),
    ("FM Towns", "FM Towns"), ("FM-7", "Fujitsu FM-7"), ("FM-77", "Fujitsu FM-7"),
    ("Apple IIgs", "Apple IIgs"), ("Apple II", "Apple II"),
    ("Atari ST", "Atari ST"), ("Commodore 64", "Commodore 64"),
    ("BBC Micro", "BBC Micro"), ("ZX Spectrum", "ZX Spectrum"),
    ("MSX", "MSX"),
    # Consoles that VGMRips files under "Other" (it buckets by chip family, not
    # vendor). Without these they keep the bare "Other" label and pile up in the
    # "Other Platforms" -> "Other" catch-all instead of their real platform.
    ("Nintendo Virtual Boy", "Nintendo Virtual Boy"), # VSU
    ("Vectrex", "Vectrex"),                           # AY8910
    ("Amstrad CPC", "Amstrad CPC"),                   # also matches "CPC+"
    ("Sega Game 1000", "Sega SG-1000"),               # VGMRips' name for SG-1000
    ("Atari 5200", "Atari 8bit"),                     # POKEY, same as 400/800
    ("Atari 400", "Atari 8bit"),                      # tag is "(Atari 400, 800)"
    ("Atari 7800", "Atari 7800"),
    ("Intellivision", "Intellivision"),
]


def display_platform(top, stem):
    """Resolve the display platform for a game, refining generic buckets."""
    base = PLATFORM.get(top, top)
    if top == "Other" or top.startswith("Computers/"):
        m = re.search(r"\(([^()]*)\)\s*$", stem.replace("_", " "))
        tag = m.group(1) if m else ""
        for needle, label in TAG_PLATFORM:
            if needle.lower() in tag.lower():
                return label
    return base


def game_title(stem):
    """Human title from a zip stem: underscores -> spaces, keep the paren tag."""
    return stem.replace("_", " ").strip()


def modland_sega_dups():
    """Normalized Sega Mega Drive game names already in modland + zophar."""
    names = set()
    prefixes = ("Video Game Music/Sega Megadrive/",
                "Video Game Music/Sega Mega CD/",
                "Video Game Music/Sega 32X/",
                "Megadrive GYM/", "Megadrive CYM/")
    try:
        with open(ALLMODS, encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.split("\t", 1)[-1].strip()
                if not p.startswith(prefixes):
                    continue
                parts = p.split("/")
                game = parts[3] if p.startswith("Video Game Music/") and \
                    len(parts) >= 5 else re.sub(r"\.[a-z0-9]+$", "", parts[-1],
                                                flags=re.I)
                names.add(norm(game))
    except FileNotFoundError:
        pass
    # Zophar's already-onboarded Genesis titles (title is column 1).
    try:
        with open(ZOPHAR, encoding="utf-8", errors="replace") as f:
            for line in f:
                c = line.split("\t")
                if c:
                    names.add(norm(strip_tag(c[0])))
    except FileNotFoundError:
        pass
    return names


def strip_tag(title):
    """Drop the trailing hardware parenthetical for cross-source title matching,
    e.g. '3 Ninjas Kick Back (Mega Drive, Genesis)' -> '3 Ninjas Kick Back'."""
    return re.sub(r"\s*\([^()]*\)\s*$", "", title).strip()


def rows_from_csv():
    """Yield (top_platform, stem, zip_url) for every game zip in files.csv."""
    with open(CSV, newline="", encoding="utf-8") as fh:
        for r in csv.reader(fh):
            path, _date, _size, top, suffix = r[0], r[1], r[2], r[3], r[4]
            if not path.lower().endswith(".zip"):
                continue
            stem = os.path.splitext(os.path.basename(path))[0]
            # suffix is the pre-URL-encoded member path (leading '/'), verified to
            # resolve on archive.org; use it verbatim rather than re-encoding.
            url = BASE + suffix
            yield top, stem, url


def build():
    dup = modland_sega_dups()
    rows, skipped = [], 0
    for top, stem, url in rows_from_csv():
        if top == "MegaDrive" and norm(strip_tag(game_title(stem))) in dup:
            skipped += 1
            continue
        title = game_title(stem)
        plat = display_platform(top, stem)
        # song_template = "title composer format path ext"
        rows.append("\t".join([title, "", plat, url, "vgz"]))
    out = os.path.normpath(OUT)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} rows ({skipped} MegaDrive dups skipped) -> {out}")


def screenshots():
    # Index every .png member: exact stem -> url, and norm(stem) -> url (for the
    # ~20% of zips whose sibling png differs only by a region/naming suffix).
    exact, fuzzy, fuzzy_dupe = {}, {}, set()
    with open(CSV, newline="", encoding="utf-8") as fh:
        for r in csv.reader(fh):
            path, suffix = r[0], r[4]
            if not path.lower().endswith(".png"):
                continue
            stem = os.path.splitext(os.path.basename(path))[0]
            url = BASE + suffix
            exact[stem] = url
            k = norm(stem)
            if k in fuzzy and fuzzy[k] != url:
                fuzzy_dupe.add(k)   # ambiguous -> don't fuzzy-match this key
            fuzzy.setdefault(k, url)

    # Only key screenshots to games actually kept in vgmrips.txt (build() drops
    # the MegaDrive dups), so the map has no orphan entries.
    kept = set()
    try:
        with open(os.path.normpath(OUT), encoding="utf-8") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) >= 4:
                    kept.add(c[3])
    except FileNotFoundError:
        sys.exit("run --build first (vgmrips.txt missing)")

    rows, matched, total = [], 0, 0
    for top, stem, zip_url in rows_from_csv():
        if zip_url not in kept:
            continue
        total += 1
        png = exact.get(stem)
        if png is None:
            k = norm(stem)
            if k not in fuzzy_dupe:
                png = fuzzy.get(k)
        if png:
            rows.append(zip_url + "\t" + png)
            matched += 1
    out = os.path.normpath(OUT_SHOTS)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {matched}/{total} screenshots -> {out}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
