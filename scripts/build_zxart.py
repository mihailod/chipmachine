#!/usr/bin/env python3
"""Build chipmachine/data/zxart.txt from the zxart.ee MUSIC database API.

zxart.ee exposes a paginated JSON REST API:

    https://zxart.ee/api/types:<T>/export:<T>/language:eng/start:<N>/limit:<M>/order:id,asc/

  T=zxMusic  -> tunes   (fields: id,title,type,year,authorIds[],compo,
                         originalUrl,originalFileName,mp3FilePath,...)
  T=author   -> authors (fields: id,title,realName,...)  -- to resolve authorIds

Each tune is offered both in its original community format (originalUrl) AND as
an ogg (mp3FilePath). ChipMachineAS plays a fixed set of ZX formats natively
(see SUPPORTED_EXT); for everything else we fall back to the ogg, routed through
the existing mp3/ogg streaming path.

  --histogram   page the whole DB, print extension / compo / type distributions
                and the native-vs-ogg split, then stop. Use this to confirm the
                EXT_CATEGORY mapping before generating the final list.
  --build       write chipmachine/data/zxart.txt

Raw API pages are cached under tools/zxart_cache/ so reruns don't re-hit the
site. Delete that dir to force a refresh.
"""

import argparse
import collections
import json
import os
import re
import sys
import time
import urllib.request

API = "https://zxart.ee/api"
CACHE = os.path.join(os.path.dirname(__file__), "zxart_cache")
OUT = os.path.join(os.path.dirname(__file__), "..", "data", "zxart.txt")
PAGE = 1000
SLEEP = 0.7  # be polite to zxart.ee

# --- ZX formats ChipMachineAS plays natively (case 1: index/play the ORIGINAL).
#     ayflyplugin + zxtuneplugin canHandle sets (no leading dot, lower case).
SUPPORTED_EXT = {
    # ayflyplugin
    "stp2", "ay", "psg", "asc", "stc", "psc", "sqt", "stp",
    "pt1", "pt2", "pt3", "vtx", "vt2", "zxs", "st13", "fxm", "amad",
    # zxtuneplugin
    "st11", "cop", "gtr", "chi", "tfe", "psm", "ftc",
}

# --- Classification (confirmed against the full --histogram run) -------------
# The `originalFileName` extension is garbled (URL-encoded/truncated), so we
# classify by the curated `type` field instead. `compo` is the demoparty
# competition category (NOT the chip type) but is the ONLY beeper signal zxart
# exposes per-tune, so it takes precedence for the 16/48 beeper bucket.
#
# format string -> category (wired into format_map in MusicDatabase.cpp):
#   "Spectrum AY"     -> ZXAY      ("ZX Spectrum 128K (AY)")
#   "Spectrum Beeper" -> ZXBEEPER  ("ZX Spectrum 16K/48K (Beeper)")
#   "Sam Coupe"       -> SAMCOUPE  (new "Sam Coupe" category)
#   "OGG"             -> OGG       ("MP3/OGG") -- pure digital / non-ZX tunes

BEEPER_COMPO = {"beeper", "realtimebeeper"}
SAMCOUPE_TYPE = {"COP", "SAA"}
# Pure digital or non-ZX-chip types: no AY/beeper identity, serve as ogg under
# the existing MP3/OGG filter.
DIGITAL_TYPE = {"MP3", "WAV", "VGM", "OGG", "FLAC", "FUR", "YM", "MOD"}


def classify(rec):
    """Return the `format` string (category) for a tune. This drives the F9
    platform filter only -- it is derived from the curated `type`/`compo`, which
    is fine for *grouping*. It must NOT decide the stored extension: zxart's
    `type` sometimes disagrees with the real file (e.g. an ETracker .etc tune
    tagged type=COP), and renaming the file to the type's extension feeds it to
    the wrong native player. native-vs-ogg + the stored ext come from the REAL
    file extension instead (see build())."""
    ty = (rec.get("type") or "").upper()
    if (rec.get("compo") or "") in BEEPER_COMPO:
        return "Spectrum Beeper"
    if ty in SAMCOUPE_TYPE:
        return "Sam Coupe"
    if ty in DIGITAL_TYPE:
        return "OGG"
    return "Spectrum AY"


def fetch(t, start, limit):
    os.makedirs(CACHE, exist_ok=True)
    cf = os.path.join(CACHE, f"{t}_{start}_{limit}.json")
    if os.path.exists(cf):
        with open(cf, "rb") as f:
            return json.load(f)
    url = f"{API}/types:{t}/export:{t}/language:eng/start:{start}/limit:{limit}/order:id,asc/"
    req = urllib.request.Request(url, headers={"User-Agent": "chipmachine-zxart-builder/1.0"})
    with urllib.request.urlopen(req, timeout=120) as r:
        data = json.load(r)
    with open(cf, "w") as f:
        json.dump(data, f)
    time.sleep(SLEEP)
    return data


def fetch_all(t):
    first = fetch(t, 0, PAGE)
    total = int(first.get("totalAmount", 0))
    key = next(iter(first["responseData"]))
    recs = list(first["responseData"][key])
    start = PAGE
    while start < total:
        d = fetch(t, start, PAGE)
        recs.extend(d["responseData"][key])
        sys.stderr.write(f"\r{t}: {len(recs)}/{total}")
        sys.stderr.flush()
        start += PAGE
    sys.stderr.write(f"\r{t}: {len(recs)}/{total}\n")
    return recs


def ext_of(rec):
    fn = rec.get("originalFileName") or ""
    e = os.path.splitext(fn)[1].lower().lstrip(".")
    return e


def authors_map():
    m = {}
    for a in fetch_all("author"):
        name = a.get("title") or a.get("realName") or ""
        m[int(a["id"])] = name
    return m


def histogram():
    tunes = fetch_all("zxMusic")
    exts = collections.Counter(ext_of(t) for t in tunes)
    compo = collections.Counter(t.get("compo") for t in tunes)
    types = collections.Counter(t.get("type") for t in tunes)
    native = sum(1 for t in tunes if ext_of(t) in SUPPORTED_EXT)
    print(f"\nTUNES: {len(tunes)}")
    print(f"NATIVE (play original): {native}   OGG fallback: {len(tunes) - native}")
    print("\nEXT (count | native?):")
    for e, c in exts.most_common():
        nat = "native" if e in SUPPORTED_EXT else "ogg   "
        print(f"  {e or '(none)':8} {c:6}  {nat}")
    cats = collections.Counter(classify(t) for t in tunes)
    print("\nCATEGORY (format string):", dict(cats.most_common()))
    print("\nCOMPO:", dict(compo.most_common()))
    print("\nTYPE:", dict(types.most_common(40)))
    # cross-tab: which exts ever appear under compo=beeper (beeper-format hints)
    beeper_exts = collections.Counter(
        ext_of(t) for t in tunes if t.get("compo") == "beeper")
    print("\nEXT where compo=beeper:", dict(beeper_exts.most_common()))


def clean(s):
    """Make a field safe for the tab-separated list file."""
    return (s or "").replace("\t", " ").replace("\n", " ").replace("\r", " ").strip()


# A multi-part game soundtrack on zxart is stored as separate tunes whose titles
# follow "<game> - <part> (<chip>) <N>" (e.g. "The Arc of Yesod - In-Game (AY) 2").
# The (chip) label is authoritative for beeper-vs-AY even when the API `type` is
# a generic "AY" .ay rip. We fold each such series into one MULTI: umbrella.
SUB_RE = re.compile(r"^(?P<game>.+?) - .+ \((?P<chip>[^()]*)\) (?P<num>\d+)\s*$")


def subtune(title):
    """(game, num, is_beeper) if title is a "<game> - <part> (<chip>) <N>" part,
    else None. is_beeper only when the chip label is exactly 'Beeper' (AY, Sample
    and 'AY & Beeper' all count as the AY/128 side)."""
    m = SUB_RE.match(title)
    if not m:
        return None
    chip = m.group("chip").replace("&amp;", "&").strip().lower()
    return m.group("game").strip(), int(m.group("num")), (chip == "beeper")


def playable(r):
    """(path, ext) for a tune: the native original when our plugins support the
    real extension, else the ogg fallback. None if neither URL exists / is clean."""
    original = clean(r.get("originalUrl"))
    ogg = clean(r.get("mp3FilePath"))
    real_ext = ext_of(r)
    if real_ext in SUPPORTED_EXT and original:
        path, ext = original, real_ext
    elif ogg:
        path, ext = ogg, "ogg"
    else:
        return None
    if "\t" in path or " " in path:
        return None
    return path, ext


def build():
    names = authors_map()
    tunes = fetch_all("zxMusic")
    rows = []
    skipped = collections.Counter()
    cats = collections.Counter()

    def composer_of(r):
        return clean(" & ".join(names.get(int(a), "")
                                for a in (r.get("authorIds") or [])
                                if names.get(int(a), "")))

    def emit(title, composer, fmt, path, ext):
        rows.append("\t".join([title, composer, fmt, path, ext]))
        cats[fmt] += 1

    # Pass 1: split tunes into fold-able subtune series vs standalone tunes.
    # groups[(game, composer)] -> list of {num, beeper, path, ext}
    groups = collections.defaultdict(list)
    singles = []
    for r in tunes:
        pe = playable(r)
        if pe is None:
            skipped["no playable url"] += 1
            continue
        path, ext = pe
        title = clean(r.get("title")) or f"#{r.get('id')}"
        composer = composer_of(r)
        info = subtune(title)
        if info is None:
            singles.append((title, composer, classify(r), path, ext))
        else:
            game, num, beeper = info
            groups[(game, composer)].append(
                {"num": num, "beeper": beeper, "path": path, "ext": ext,
                 "title": title, "fmt": classify(r)})

    # Pass 2: emit ONE entry per chip class. zxart lists each internal subsong of
    # a game's rip as a separate tune-id, but every "part" in a chip class is the
    # SAME multi-subsong file (verified: identical md5 across a game's parts). So
    # we emit a single URL, not a MULTI: list -- the player loads the file once and
    # GME exposes its internal subsongs, making next/prev an instant mp.seek()
    # instead of reloading an identical file per step. If a game+composer has both
    # beeper and AY parts they are SEPARATE files, so split into "<game> (Beeper)"
    # and "<game> (AY)"; a single class is just "<game>".
    for (game, composer), subs in groups.items():
        mixed = len({s["beeper"] for s in subs}) > 1
        for beeper in sorted({s["beeper"] for s in subs}):
            cls = sorted((s for s in subs if s["beeper"] == beeper),
                         key=lambda s: s["num"])
            # Parts that disagree on extension can't be the same file -- keep those
            # as individual rows rather than dropping the odd ones.
            exts = {s["ext"] for s in cls}
            if len(exts) != 1:
                for s in cls:
                    emit(s["title"], composer, s["fmt"], s["path"], s["ext"])
                continue
            suffix = (" (Beeper)" if beeper else " (AY)") if mixed else ""
            fmt = "Spectrum Beeper" if beeper else "Spectrum AY"
            emit(game + suffix, composer, fmt, cls[0]["path"], cls[0]["ext"])

    for s in singles:
        emit(*s)

    out = os.path.normpath(OUT)
    with open(out, "w") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} rows ({len(groups)} folded game-groups, "
          f"{len(singles)} singles) -> {out}")
    print("categories:", dict(cats.most_common()))
    print("skipped:", dict(skipped))


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--histogram", action="store_true")
    ap.add_argument("--build", action="store_true")
    args = ap.parse_args()
    if args.histogram:
        histogram()
    elif args.build:
        build()
    else:
        ap.print_help()
