#!/usr/bin/env python3
"""Build chipmachine/data/zxtunes.txt from the zxtunes.com XML API.

zxtunes.com exposes a clean XML API (reverse-engineered from the ZXTune Android
player's app/zxtune/fs/zxtunes/RemoteCatalog.kt), so no page scraping is needed:

    authors:  https://zxtunes.com/xml.php?scope=authors&fields=nickname,name,tracks,photo
    tracks:   https://zxtunes.com/xml.php?scope=tracks&fields=filename,title,duration,date&author_id=N
    download: https://zxtunes.com/downloads.php?id=N   (raw module binary)

Every format on the site is a ZX AY tracker format we already play natively
(pt3/pt2/stc/stp/asc/sqt/psc/vtx/pt1 via ayflyplugin, .ay via gmeplugin) -- there
is NO ogg fallback and NO new decoder. The stored `path` is the extensionless
downloads.php URL (source="" in db.lua, used verbatim like amp); the `ext` column
routes it to the right plugin (amp idiom).

Heavy overlap with the already-onboarded zxart.ee (28.5k) + Project AY + modland
AY corpus: same formats, same demoscene sources. We keep only net-new tunes via a
fuzzy normalized-title dedup against those collections (see load_existing_stems).

  --histogram   parse the cache, print ext / dedup stats, then stop.
  --build       write chipmachine/data/zxtunes.txt

Metadata cache (authors.xml + author_<id>.xml) lives under chipmachine/scripts/zxtunes_cache/;
populate it with the sibling crawl (curl loop) or --crawl. Delete to refresh.
"""

import argparse
import collections
import html
import os
import re
import sys
import time
import urllib.request
import xml.etree.ElementTree as ET

HERE = os.path.dirname(__file__)
CACHE = os.path.join(HERE, "zxtunes_cache")
OUT = os.path.join(HERE, "..", "data", "zxtunes.txt")
DATA = os.path.join(HERE, "..", "data")
API = "https://zxtunes.com/xml.php"
DL = "https://zxtunes.com/downloads.php?id="
SLEEP = 0.7  # be polite (robots.txt Crawl-delay:10 is for crawlers; API is light)

# Formats we play natively (ayflyplugin + gmeplugin .ay). .sna is a ZX snapshot,
# not a tune -> dropped. Everything zxtunes hosts is in this set except .sna.
SUPPORTED_EXT = {
    # ayflyplugin
    "stp2", "ay", "psg", "asc", "stc", "psc", "sqt", "stp",
    "pt1", "pt2", "pt3", "vtx", "vt2", "zxs", "st13", "fxm", "amad",
    # zxtuneplugin
    "st11", "gtr", "chi", "tfe", "psm", "ftc",
    # stsoundplugin (YM2149 logged register dumps)
    "ym",
}
# .fls (Flash Tracker, no open replayer -- parked) and .sna (ZX RAM snapshots,
# not tunes) are intentionally NOT here: they drop and the tunes Skip.

# Dedup rule (app-wide invariant): the triple {title, composer, format} must be
# unique across every collection. We drop a zxtunes track whose triple already
# exists in another "title composer format path ext" collection. Only ZX sources
# that also label format "Spectrum AY"/"Spectrum Beeper" can actually collide
# (modland/modarchive tag the SAME tune "Pro Tracker"/"Sound Tracker" etc, a
# DIFFERENT triple -> legitimately kept). zxart is the dominant overlap.
TRIPLE_SOURCES = ["zxart.txt", "projectay.txt", "amp.txt"]


def crawl():
    os.makedirs(CACHE, exist_ok=True)
    af = os.path.join(CACHE, "authors.xml")
    if not os.path.exists(af):
        url = f"{API}?scope=authors&fields=nickname,name,tracks,photo"
        urllib.request.urlretrieve(url, af)
        time.sleep(SLEEP)
    ids = [a.get("id") for a in ET.parse(af).getroot().iter("author")]
    for i, aid in enumerate(ids):
        tf = os.path.join(CACHE, f"author_{aid}.xml")
        if os.path.exists(tf) and os.path.getsize(tf) > 0:
            continue
        url = f"{API}?scope=tracks&fields=filename,title,duration,date&author_id={aid}"
        urllib.request.urlretrieve(url, tf)
        sys.stderr.write(f"\rcrawl {i + 1}/{len(ids)}")
        sys.stderr.flush()
        time.sleep(SLEEP)
    sys.stderr.write("\n")


def norm(s):
    """Alnum-lowercase key. Strips a trailing file extension but NOT
    parentheticals -- for zxtunes a "(group)" in a filename is part of the name,
    and over-stripping caused false collisions."""
    s = re.sub(r"\.[a-z0-9]{1,4}$", "", (s or "").lower())
    return re.sub(r"[^a-z0-9]", "", s)


def triple(title, composer, fmt):
    """The app-wide uniqueness key {title, composer, format}, normalized. html
    entities (zxart escapes '&#039;' etc) are unescaped so both sides align."""
    return (norm(html.unescape(title)), norm(html.unescape(composer)),
            norm(fmt))


def load_existing_triples():
    """Set of {title,composer,format} triples from the composer+format-bearing
    ("title composer format path ext") collections."""
    keys = set()
    for name in TRIPLE_SOURCES:
        p = os.path.join(DATA, name)
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                cols = line.rstrip("\n").split("\t")
                if len(cols) < 3:
                    continue
                keys.add(triple(cols[0], cols[1], cols[2]))
    return keys


def parse_authors():
    af = os.path.join(CACHE, "authors.xml")
    out = {}
    for a in ET.parse(af).getroot().iter("author"):
        aid = a.get("id")
        nick = (a.findtext("nickname") or "").strip()
        out[aid] = nick or f"#{aid}"
    return out


def iter_tracks():
    authors = parse_authors()
    for aid, nick in authors.items():
        tf = os.path.join(CACHE, f"author_{aid}.xml")
        if not os.path.exists(tf) or os.path.getsize(tf) == 0:
            continue
        try:
            root = ET.parse(tf).getroot()
        except ET.ParseError:
            continue
        for t in root.iter("track"):
            tid = t.get("id")
            fn = (t.findtext("filename") or "").strip()
            if not tid or not fn:
                continue
            yield nick, tid, fn


def clean(s):
    return (s or "").replace("\t", " ").replace("\n", " ").replace("\r", " ").strip()


FORMAT = "Spectrum AY"


def analyze():
    existing = load_existing_triples()
    exts = collections.Counter()
    dropped = collections.Counter()
    seen = set()
    kept = dup = selfdup = 0
    for nick, tid, fn in iter_tracks():
        ext = os.path.splitext(fn)[1].lower().lstrip(".")
        exts[ext] += 1
        if ext not in SUPPORTED_EXT:
            dropped["unsupported ext"] += 1
            continue
        title = re.sub(r"\.[a-z0-9]+$", "", fn)
        key = triple(title, nick, FORMAT)
        if key in existing:
            dup += 1
            continue
        if key in seen:
            selfdup += 1
            continue
        seen.add(key)
        kept += 1
    return exts, kept, dup, selfdup, dropped


def build():
    existing = load_existing_triples()
    rows = []
    dup = selfdup = 0
    dropped = collections.Counter()
    seen = set()   # zxtunes-internal {title,composer,format} uniqueness
    for nick, tid, fn in iter_tracks():
        ext = os.path.splitext(fn)[1].lower().lstrip(".")
        if ext not in SUPPORTED_EXT:
            dropped[ext or "(none)"] += 1
            continue
        title = clean(re.sub(r"\.[a-z0-9]+$", "", fn)) or f"#{tid}"
        key = triple(title, nick, FORMAT)
        if key in existing:            # duplicates another collection's triple
            dup += 1
            continue
        if key in seen:                # duplicates an earlier zxtunes row
            selfdup += 1
            continue
        seen.add(key)
        # path = BARE track id (db.lua `source` prepends downloads.php?id=). It
        # MUST be extensionless: MusicPlayerList derives the routing extension
        # from path_extension(path), and a full ".../downloads.php?id=N" URL
        # yields the bogus ext "php?id=N", clobbering the Content-Disposition
        # ext and breaking playback (the amp / modarchive moduleid idiom).
        rows.append("\t".join([title, clean(nick), FORMAT, tid, ext]))
    out = os.path.normpath(OUT)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} net-new rows -> {out}")
    print(f"deduped vs other collections (triple): {dup}")
    print(f"deduped within zxtunes (triple): {selfdup}")
    print("dropped exts:", dict(dropped))


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--crawl", action="store_true")
    ap.add_argument("--histogram", action="store_true")
    ap.add_argument("--build", action="store_true")
    args = ap.parse_args()
    if args.crawl:
        crawl()
    elif args.histogram:
        exts, kept, dup, selfdup, dropped = analyze()
        total = sum(exts.values())
        print(f"tracks in cache: {total}")
        print(f"kept (net-new): {kept}   deduped vs others: {dup}   "
              f"self-dup: {selfdup}   dropped: {dict(dropped)}")
        print("EXT:")
        for e, c in exts.most_common():
            tag = "native" if e in SUPPORTED_EXT else "DROP  "
            print(f"  {e or '(none)':8} {c:6}  {tag}")
    elif args.build:
        build()
    else:
        ap.print_help()
