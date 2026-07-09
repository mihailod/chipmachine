#!/usr/bin/env python3
"""Build chipmachine/data/projectay.txt from the shipped Project AY .ay files.

The Project AY / AY-EMUL archive (Sergey Bulba, bulba.untergrund.net) distributes
raw Z80 machine-code music rips in the ZXAYEMUL (.ay) container. We ship three of
its .AY sub-collections locally under chipmachine/music/projectay/ (like the
nsfe -> music/Console and hvtc -> music/hvtc collections), because Bulba only
offers big archives and Ironfist's own site is gone:

  ironfist/  Ironfist's ZX Spectrum game rips (210)      -> "Spectrum AY" (ZXAY)
  bulba/     Bulba's own ZX rips/rebuilds (30)           -> "Spectrum AY" (ZXAY)
  cpc/       SoLO/CORPSE's Amstrad CPC demo rips (373)    -> "Amstrad CPC" (AMSTRAD)

Playback: .ay is owned by gmeplugin (Ay_Emul lineage) -- Ayfly renders the CPC
rips silent. Many .ay carry multiple subsongs (host next/prev navigates them).

Each .ay embeds metadata: PAuthor (0x0C, the composer) and PMisc (0x0E, usually
the game/publisher). The CPC rips were stripped of metadata by SoLO, so we fall
back to the folder/filename and credit the ripper.

Output template: "title composer format path ext"; path is relative to the
collection's local_dir (music/projectay), source is empty (files are local).
"""

import glob
import os
import re
import struct
import sys

HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, "..")
MUSIC = os.path.join(ROOT, "music", "projectay")
OUT = os.path.join(ROOT, "data", "projectay.txt")


def clean(s):
    s = re.sub(r"[\x00-\x1f\x7f]", " ", s or "")
    return re.sub(r"\s+", " ", s).strip()


def parse_ay(path):
    """Return (author, misc) decoded from the ZXAYEMUL header, or ('','')."""
    with open(path, "rb") as f:
        d = f.read()
    if d[:8] != b"ZXAYEMUL":
        return "", ""

    def cstr(off):
        v = struct.unpack(">h", d[off:off + 2])[0]
        p = off + v
        if p < 0 or p >= len(d):
            return ""
        e = d.find(b"\x00", p)
        return clean(d[p:e if e >= 0 else len(d)].decode("latin-1", "replace"))

    return cstr(0x0C), cstr(0x0E)


def strip_copyright(s):
    # "Arkanoid (c) Imagine 1987" -> "Arkanoid"
    return re.sub(r"\s*\(c\).*$", "", s, flags=re.I).strip()


# Some rips (a handful of Bulba's) put a descriptive sentence in PMisc rather than
# a bare game name ("Music from Kwik Snax, the game by Codemasters Ltd, 1990").
# Those make poor titles and never match ZXDB; the .ay filename is the clean game
# name there ("Kwik Snax.ay"), so prefer it when the misc string looks prose-y.
_PROSE = re.compile(r"\b(the game|game by|music from|from intro|ripped|rebuilt)\b",
                    re.I)


def looks_prose(s):
    return bool(_PROSE.search(s)) or s.count(",") >= 1 or len(s.split()) > 6


def title_case_stem(stem):
    # "TribalMag5_00" -> "TribalMag5 00"; leave readable
    return clean(stem.replace("_", " "))


def main():
    rows = []
    for f in sorted(glob.glob(os.path.join(MUSIC, "**", "*.ay"), recursive=True)):
        rel = os.path.relpath(f, MUSIC)             # e.g. "ironfist/arkanoid.ay"
        top = rel.split(os.sep)[0]                  # ironfist | bulba | cpc
        author, misc = parse_ay(f)
        stem = os.path.splitext(os.path.basename(f))[0]

        if top == "cpc":
            fmt = "Amstrad CPC"
            # SoLO stripped titles/authors; use the demo/mag folder + filename.
            folder = rel.split(os.sep)[1] if len(rel.split(os.sep)) > 2 else ""
            game = strip_copyright(misc)
            title = game or clean(f"{folder} {title_case_stem(stem)}".strip())
            composer = author or "SoLO/CORPSE"
        else:
            fmt = "Spectrum AY"
            game = strip_copyright(misc)
            # Prefer the clean filename when PMisc is empty or a prose blurb.
            if not game or looks_prose(game):
                game = title_case_stem(stem)
            title = game
            composer = author
            if composer in ("", "?"):
                composer = "Unknown"

        if composer == "?":
            composer = "Unknown"
        # path column uses forward slashes regardless of host
        pathcol = rel.replace(os.sep, "/")
        rows.append("\t".join([title, composer, fmt, pathcol, "ay"]))

    with open(os.path.normpath(OUT), "w", encoding="utf-8") as fh:
        fh.write("\n".join(rows) + "\n")
    sys.stderr.write(f"wrote {len(rows)} songs to {os.path.normpath(OUT)}\n")


if __name__ == "__main__":
    main()
