#!/usr/bin/env python3
"""Build chipmachine/data/oplarchive.txt from The OPL Archive (opl.wafflenet.com).

Non-game OPL2/OPL3 chiptunes (demoscene originals + game-tune covers), organised
by artist. Every file is a VGM log gzipped to .vgz; a random sample of the archive
is 100% YM3812 (OPL2) / YMF262 (OPL3), i.e. the AdLib / Sound Blaster PC chips.

GME's Vgm_Emu cannot decode OPL (silent, or aborts on OPL2), so these route to
libvgmplugin (ValleyBell libvgm), which reads the gzipped .vgz directly. The
songs classify as "OPL Archive" -> ADPLUG -> the "IBM PC (AdLib/OPL)" filter.

Input:  chipmachine/data-notbundled/misc/opl/files.csv  (columns: Length,Path,Composer,Title)
Output: chipmachine/data/oplarchive.txt      (song_template: title composer format path ext)

The path column is a full, URI-encoded wafflenet URL (db.lua `source` is empty);
per the archive README the path must be URI-encoded when the download URL is
formed. Files are kept as .vgz (no gunzip -- libvgm handles gzip).
"""

import csv
import os
import re
import sys
import urllib.parse

BASE = "https://opl.wafflenet.com/vgm/"
HERE = os.path.dirname(__file__)
# Scrape input: build-time only -> data-notbundled/ (not copied into the .app).
CSV = os.path.join(HERE, "..", "data-notbundled", "misc", "opl", "files.csv")
OUT = os.path.join(HERE, "..", "data", "oplarchive.txt")


def clean(s):
    # Strip control chars (would truncate in SQLite / garble display) and any
    # tab (our column separator); collapse whitespace.
    s = re.sub(r"[\x00-\x1f\x7f]", " ", s or "")
    return re.sub(r"\s+", " ", s).strip()


def main():
    rows = []
    skipped = 0
    with open(os.path.normpath(CSV), encoding="utf-8", errors="replace") as f:
        for rec in csv.DictReader(f):
            path = (rec.get("Path") or "").strip()
            if not path or not path.lower().endswith(".vgz"):
                skipped += 1
                continue
            composer = clean(rec.get("Composer"))
            title = clean(rec.get("Title"))
            if not title:
                # Fall back to the file's basename without extension.
                title = clean(os.path.splitext(os.path.basename(path))[0])
            url = BASE + urllib.parse.quote(path)
            rows.append("\t".join([title, composer, "OPL Archive", url, "vgz"]))

    out = os.path.normpath(OUT)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    sys.stderr.write(f"wrote {len(rows)} songs to {out} (skipped {skipped})\n")


if __name__ == "__main__":
    main()
