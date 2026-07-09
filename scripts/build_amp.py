#!/usr/bin/env python3
"""Build data/amp.txt from the AMP (Amiga Music Preservation) catalogue.

Input : chipmachine/data/misc/amp/MODULES.csv  (the full 178k-row scrape:
        MODULE_ID, MODULE_TITLE, COMPOSER, COMPOSER_DETAIL_URL, FORMAT, SIZE, DL)
Output: chipmachine/data/amp.txt  (song_template "title composer format path ext")

No crawling required. Each row plays via the AMP download endpoint:

    source = "http://amp.dascene.net/downmod.php?index="   (in db.lua)
    path   = <MODULE_ID>

The app follows the 302 redirect, and MusicPlayerList's gzip-by-magic step
inflates the application/x-gzip body; the `ext` column (mapped from FORMAT)
then routes the module to OpenMPT/UADE/hively, exactly like modarchive's
"downloads.php?moduleid=" pattern.

Dedup: AMP overlaps heavily with modland / modarchive / demozoo / scene.org /
unexotica. We key on normalised (composer, title) -- AMP carries no md5 -- and
drop rows that already exist elsewhere, so AMP only contributes net-new tunes
to search. modarchive has no composer, so its (title-only) match is applied to
the four most-mirrored tracker formats where a title clash is almost certainly
the same tune.
"""
import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CHIP = os.path.join(HERE, "..")
DATA = os.path.join(CHIP, "data")
CSV_IN = os.path.join(DATA, "misc", "amp", "MODULES.csv")
OUT = os.path.join(DATA, "amp.txt")

# AMP FORMAT code (uppercased) -> file extension the decoders route on.
# STK/FST are Soundtracker/Startrekker .mod-family; OSS is OctaMED SoundStudio
# (MMD3) which libopenmpt decodes via the "med" ext by content.
FMT_EXT = {
    "MOD": "mod", "MOD3": "mod", "MOM": "mod", "STK": "mod", "FST": "mod",
    "STP": "mod", "STP2": "mod",
    "XM": "xm", "IT": "it", "S3M": "s3m", "STM": "stm",
    "MTM": "mtm", "DMF": "dmf", "PTM": "ptm", "ULT": "ult", "FAR": "far",
    "AMS": "ams", "DSM": "dsm", "MPTM": "mptm", "MPMT": "mptm", "MT2": "mt2",
    "669": "669", "MDL": "mdl", "AMF": "amf", "PLM": "plm",
    "MED": "med", "OSS": "med",
    "AHX": "ahx", "THX": "thx", "HVL": "hvl",
    "DBM": "dbm", "DIGI": "digi",
    "OKT": "okt", "OCT": "okt", "OK": "okt",
    "SFX": "sfx", "SFX2": "sfx",
    "AON": "aon", "GMC": "gmc", "ML": "ml", "DMU": "dmu", "DMU2": "dmu2",
    "EMOD": "emod", "JAM": "jam", "SA": "sa", "DSS": "dss", "MM8": "mm8",
    "FC13": "fc13", "AST": "ast", "ABK": "abk", "PRT": "prt", "ST26": "st26",
    "SID2": "sid2", "SID": "sid2", "FTM": "ftm", "DTM": "dtm", "TCB": "tcb",
    "GT2": "gt2",
}

# PC/multitracker formats where modarchive's title-only key is trustworthy.
MA_FMT = {"MOD", "XM", "IT", "S3M", "STM", "669", "FAR", "ULT", "MTM", "MDL",
          "DMF", "PTM", "AMF", "MOD3", "DSM", "MPTM", "MT2", "AMS", "OK", "OKT"}


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def load_ct():
    """(composer,title) keys from the collections that overlap AMP, plus the
    modarchive title-only set."""
    ct = set()

    with open(os.path.join(DATA, "allmods.txt"), encoding="utf-8",
              errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 2:
                continue
            q = p[1].split("/")
            if len(q) < 3:
                continue
            ct.add((norm(q[1]), norm(re.sub(r"\.[^.]+$", "", q[-1]))))

    for fn in ("demozoo", "sceneorg"):
        with open(os.path.join(DATA, fn + ".txt"), encoding="utf-8",
                  errors="replace") as f:
            for line in f:
                p = line.rstrip("\n").split("\t")
                if len(p) >= 2:
                    ct.add((norm(p[1]), norm(p[0])))

    with open(os.path.join(DATA, "unexotica.txt"), encoding="utf-8",
              errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 4:
                ct.add((norm(p[3]), norm(p[0])))

    ma = set()
    with open(os.path.join(DATA, "modarchive.txt"), encoding="utf-8",
              errors="replace") as f:
        for line in f:
            f0 = line.split("\t")[0]
            ma.add(norm(f0.split("//", 1)[1] if "//" in f0 else ""))

    return ct, ma


def main():
    ct, ma = load_ct()

    total = kept = dup = 0
    by_fmt_new = {}
    rows = []
    with open(CSV_IN, encoding="utf-8", errors="replace") as f:
        for row in csv.DictReader(f):
            total += 1
            mid = (row["MODULE_ID"] or "").strip()
            if not mid.isdigit():
                continue
            fmt = (row["FORMAT"] or "").strip()
            title = (row["MODULE_TITLE"] or "").strip().replace("\t", " ")
            composer = (row["COMPOSER"] or "").strip().replace("\t", " ")

            c, t = norm(composer), norm(title)
            if (c, t) in ct:
                dup += 1
                continue
            if fmt.upper() in MA_FMT and t in ma:
                dup += 1
                continue

            ext = FMT_EXT.get(fmt.upper(), fmt.lower())
            if not title:
                title = "(untitled)"
            if not composer:
                composer = "Unknown"
            # title \t composer \t format \t path(=id) \t ext
            rows.append("\t".join([title, composer, fmt, mid, ext]))
            kept += 1
            by_fmt_new[fmt] = by_fmt_new.get(fmt, 0) + 1

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")

    print(f"AMP catalogue rows : {total}")
    print(f"dropped as duplicate: {dup}")
    print(f"kept (net-new)     : {kept}  -> {OUT}")
    print()
    print(f"{'FMT':7}{'kept':>8}")
    for fmt in sorted(by_fmt_new, key=lambda x: -by_fmt_new[x]):
        if by_fmt_new[fmt] >= 10:
            print(f"{fmt:7}{by_fmt_new[fmt]:8}")


if __name__ == "__main__":
    main()
