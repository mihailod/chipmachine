#!/usr/bin/env python3
"""Battle of the Bits (battleofthebits.com) onboarding for ChipMachineAS.

BotB is a live, active *original-composition* chiptune community (2005-).  Its
public JSON API (on the .com host; .org 404s for /api) exposes every "entry" of
every "battle".  We onboard only the **audio** entries whose original uploaded
file is in a format one of our plugins can actually decode natively -- routed by
the REAL file extension taken from each entry's `view_url` (the API's
`format_token` is a *platform/compo* label, not the file type).  Rendered-audio
compos (remix/sample/vocal/...) upload an mp3/ogg/wav/flac original which streams
via ffmpeg.  Formats with no open replayer here (renoise/buzz/klystrack/furnace/
lgpt/...) are skipped -- BotB's mp3 *preview* endpoint is login-gated, so there
is no rendered fallback for those.

Stable playback URL: the API's file endpoints (EntryDonload/EntryPreview) require
login and 403 anonymously, but every original file is also served as a STATIC,
directly-hotlinkable asset at
    https://battleofthebits.com/disk/battle/<battle_id:08d>/<original-filename>
which we reconstruct from `view_url` (true filename+ext) + `battle_id`.  Verified
HTTP 200 for .mod/.xm/.it/.s3m/.nsf/.sid/.spc/.ftm/.mp3 and artwork.png.

Screenshots: each entry belongs to a battle whose `battle.cover_art_url` is the
compo's cover art -- used as this collection's per-song artwork (like an album
cover), keyed by the song URL in data/botb_screenshots.txt.

Usage:
    build_botb.py --crawl      # full API crawl -> scratchpad cache (jsonl)
    build_botb.py --tally      # print (format_token, ext, medium) distribution
    build_botb.py --build      # cache -> data/botb.txt + data/botb_screenshots.txt
"""
import json, os, sys, time, urllib.parse, urllib.request

API = "https://battleofthebits.com/api/v1/entry/list/{page}/{length}"
DISK = "https://battleofthebits.com/disk/battle/{bid:08d}/{fname}"
UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120 Safari/537.36")
PAGE_LEN = 500          # API hard cap (501+ returns invalid)
HERE = os.path.dirname(os.path.abspath(__file__))
CACHE = os.environ.get("BOTB_CACHE",
        "/private/tmp/claude-502/-Users-mihailod-Documents-chipmachine-as/"
        "5c8b19f4-f34f-4c96-9d33-7990b2de3fa0/scratchpad/botb_entries.jsonl")
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))


def _get(url, tries=4):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=60) as r:
                return json.loads(r.read().decode("utf-8", "replace"))
        except Exception as e:
            if i == tries - 1:
                raise
            time.sleep(2 * (i + 1))
    return None


def real_filename(view_url):
    # view_url = /player/View/<id>/<ORIGINAL FILENAME WITH EXT>
    parts = view_url.split("/", 4)
    return parts[4] if len(parts) >= 5 else ""


def crawl():
    # Paginate by offset until a page comes back EMPTY.  Do NOT stop on a merely
    # short page -- a transient hiccup can return <PAGE_LEN mid-stream and would
    # truncate the crawl (observed: a flaky page 9).  A short page is retried
    # once; only a genuinely empty page ends the crawl.
    seen, page = 0, 0
    with open(CACHE, "w") as out:
        while True:
            url = API.format(page=page, length=PAGE_LEN)
            batch = _get(url)
            if batch is None:
                batch = []
            if len(batch) == 0:
                # confirm end-of-list with one retry (guard against a flaky empty)
                time.sleep(1.0)
                batch = _get(url) or []
                if len(batch) == 0:
                    break
            for e in batch:
                fmt = e.get("format", {}) or {}
                rec = {
                    "id": e.get("id"),
                    "title": e.get("title", ""),
                    "authors_display": e.get("authors_display", ""),
                    "format_token": e.get("format_token", ""),
                    "medium": fmt.get("medium", ""),
                    "battle_id": e.get("battle_id"),
                    "view_url": e.get("view_url", ""),
                    "cover_art_url": (e.get("battle") or {}).get("cover_art_url", ""),
                    "plays": e.get("plays", "0"),
                    "favs": e.get("favs", "0"),
                }
                out.write(json.dumps(rec, ensure_ascii=False) + "\n")
            seen += len(batch)
            sys.stderr.write("\rpage %d  entries %d" % (page, seen))
            sys.stderr.flush()
            page += 1
            time.sleep(0.4)          # be polite
    sys.stderr.write("\ndone: %d entries -> %s\n" % (seen, CACHE))


def iter_cache():
    with open(CACHE) as f:
        for line in f:
            line = line.strip()
            if line:
                yield json.loads(line)


# --- classification -------------------------------------------------------
# Playability is driven by the REAL file extension (from view_url), NOT the
# BotB `format_token` (which is a compo/platform label).  Only extensions one of
# our plugins actually decodes are onboarded; everything else (renoise .xrns,
# furnace .fur, klystrack .kt, mario paint .sho, raw ROMs .smc/.gb, midi, tap,
# text/image junk, ...) is skipped -- BotB's mp3 *preview* is login-gated, so
# there is no rendered fallback for unplayable natives.

# Rendered audio containers -> ffmpeg (FFMPEGPlugin gate: mp3/mp4/m4a/aac/ogg +
# the widened wav/flac/aif/mp2 lossless set).
RENDERED = {"mp3", "mp2", "ogg", "m4a", "aac", "wav", "flac", "aif", "aiff"}

# Rendered-audio platform by compo token (so a Game Boy LSDj mp3 still lands in
# the Game Boy filter); the cross-platform free compos fall through to "MP3".
RENDER_PLATFORM = {
    "gameboy": "Nintendo Game Boy (GB)", "lsdj": "Nintendo Game Boy (GB)",
    "sms": "Sega Master System", "sgen": "Sega Genesis",
    "sid": "Commodore 64",
    "nsf": "Nintendo Sound Format", "nsfplus": "Nintendo Sound Format",
    "nsf_classic": "Nintendo Sound Format", "famitracker": "Nintendo Sound Format",
    "sap": "Atari 8Bit", "sapx2": "Atari 8Bit",
    "spc": "Super Nintendo", "hes": "HES",
    "aym": "Spectrum AY", "zxbeep": "Spectrum Beeper",
    "adlib": "AdLib", "opl2": "AdLib", "opl3": "AdLib",
    "ym2151": "Sharp X68000", "x68k": "Sharp X68000", "pc-x801": "Sharp X68000",
    "ted": "TED", "sunvox": "SunVox", "deflemask": "DefleMask",
}

# Native extension -> display/filter format string (lowercase-matched to
# format_map).  Presence here == "we can decode it".  .vgm/.vgz platform depends
# on the compo token (see VGM_PLATFORM).
NATIVE_FORMAT = {
    # GME consoles
    "nsf": "Nintendo Sound Format", "nsfe": "Nintendo Sound Format",
    "spc": "Super Nintendo", "gbs": "Nintendo Game Boy (GB)", "hes": "HES",
    "sap": "Atari 8Bit", "ay": "Spectrum AY", "pt3": "Spectrum AY",
    # OpenMPT trackers
    "it": "IT", "xm": "XM", "mod": "MOD", "nst": "MOD", "s3m": "S3M",
    "stm": "S3M", "mptm": "MPTM",
    # dedicated plugins
    "ftm": "FamiTracker",
    # 0CC-FamiTracker (fork of FamiTracker; same "FamiTracker Module" magic).
    # Ingested with their real ext -- playback needs "0cc"/"kftm" added to the
    # famitrackerplugin supported_ext (currently {"ftm"} only); follow-up session.
    "0cc": "FamiTracker", "kftm": "FamiTracker",
    "dmf": "DefleMask", "sunvox": "SunVox",
    "ptcop": "PxTone", "pttune": "PxTone", "org": "Organya",
    "bbsong": "Beepola", "sid": "Commodore 64", "ahx": "Amiga",
    "sndh": "Atari ST", "ym": "Atari ST", "prg": "TED", "s98": "S98",
    # AdPlug OPL
    "a2m": "AdLib", "a2t": "AdLib", "rad": "AdLib", "amd": "AdLib",
    "dfm": "AdLib", "snd": "AdLib", "cff": "AdLib", "sa2": "AdLib",
    # libvgm / GME multi-chip logs
    "vgm": "VGM", "vgz": "VGM",
}
VGM_PLATFORM = {
    "sms": "Sega Master System", "sgen": "Sega Genesis",
    "ym2151": "Sharp X68000", "x68k": "Sharp X68000", "pc-x801": "Sharp X68000",
}

# Per-URL VGM platform override, keyed by the reconstructed disk URL. Built by
# reading each file's chip-clock header (scripts/vgm_peek analysis, 2026-07-16):
# the BotB compo token classifies only ~5 tokens, leaving ~1039 as generic "VGM".
# The header names the actual sound chip(s), which map to a real platform far more
# reliably than the token. This side-file (data/botb_vgm_platforms.txt) is the
# committed result; it wins over VGM_PLATFORM's token guess for any .vgm/.vgz.
def load_vgm_overrides():
    path = os.path.join(DATA, "botb_vgm_platforms.txt")
    m = {}
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for ln in f:
                ln = ln.rstrip("\n")
                if not ln:
                    continue
                u, _, fmt = ln.partition("\t")
                if u and fmt:
                    m[u] = fmt
    return m


VGM_BY_URL = load_vgm_overrides()


def classify(token, ext):
    """(format_string, keep) for an audio entry, or (None, False) to skip."""
    if ext in RENDERED:
        return (RENDER_PLATFORM.get(token, "MP3"), True)
    if ext in NATIVE_FORMAT:
        if ext in ("vgm", "vgz"):
            return (VGM_PLATFORM.get(token, "VGM"), True)
        return (NATIVE_FORMAT[ext], True)
    return (None, False)


def build():
    from collections import Counter
    disk = "https://battleofthebits.com/disk/battle/{bid:08d}/{fname}"
    rows = []            # (title, composer, format, url, ext)
    shots = {}           # url -> cover_art_url
    skipped = Counter(); kept_fmt = Counter()
    seen_url = set()
    for e in iter_cache():
        if e["medium"] != "audio":
            continue
        fname = real_filename(e["view_url"])
        if not fname:
            continue
        ext = os.path.splitext(fname)[1].lower().lstrip(".")
        fmt, keep = classify(e["format_token"], ext)
        if not keep:
            skipped[ext or "(noext)"] += 1
            continue
        try:
            bid = int(e["battle_id"])
        except (TypeError, ValueError):
            continue
        url = disk.format(bid=bid, fname=urllib.parse.quote(fname))
        if url in seen_url:
            continue
        seen_url.add(url)
        # Header-derived platform wins over the compo-token guess for VGM logs.
        # An unclassified VGM (not in the side-file, token unknown) is DROPPED
        # rather than shipped as a generic "VGM" row: the one such file (an
        # extra-header-only log, "Visible Confusion") was silent, and the "VGM"
        # Other sub-platform was removed entirely (2026-07-16).
        if ext in ("vgm", "vgz"):
            fmt = VGM_BY_URL.get(url, fmt)
            if fmt == "VGM":
                skipped["vgm-unclassified"] += 1
                continue
        title = (e.get("title") or "").strip().replace("\t", " ")
        if not title:                       # some entries have a blank title
            title = os.path.splitext(fname)[0].strip() or ("BotB " + str(e.get("id")))
        composer = (e.get("authors_display") or "").strip().replace("\t", " ")
        rows.append((title, composer, fmt, url, ext))
        kept_fmt[fmt] += 1
        cover = (e.get("cover_art_url") or "").strip()
        # Keep only DISTINCTIVE per-battle cover art (disk/battle/<id>/...). The
        # bulk of BotB audio is One/Two/Four-Hour Battles whose cover_art_url is
        # a generic series banner under /disk/debris/ (ohb600.jpg/2hb600/4hb600,
        # ~75% of entries) -- a placeholder, not real event art, so we drop it
        # (matches the "meaningful imagery only" screenshot policy).
        if cover.startswith("http") and "/disk/debris/" not in cover:
            shots[url] = cover

    out_txt = os.path.join(DATA, "botb.txt")
    out_shots = os.path.join(DATA, "botb_screenshots.txt")
    with open(out_txt, "w", encoding="utf-8") as f:
        for r in rows:
            f.write("\t".join(r) + "\n")
    with open(out_shots, "w", encoding="utf-8") as f:
        for url, cover in shots.items():
            f.write(url + "\t" + cover + "\n")

    print("wrote %d songs -> %s" % (len(rows), out_txt))
    print("wrote %d screenshots -> %s" % (len(shots), out_shots))
    print("\n== kept by format ==")
    for k, v in kept_fmt.most_common():
        print("%7d  %s" % (v, k))
    print("\n== skipped by ext (top 25, no decoder) ==")
    for k, v in skipped.most_common(25):
        print("%7d  .%s" % (v, k))


def tally():
    from collections import Counter
    by_fmt = Counter(); by_ext = Counter(); by_fmt_ext = Counter()
    audio = 0; total = 0
    for e in iter_cache():
        total += 1
        if e["medium"] != "audio":
            continue
        audio += 1
        ext = os.path.splitext(real_filename(e["view_url"]))[1].lower().lstrip(".")
        by_fmt[e["format_token"]] += 1
        by_ext[ext] += 1
        by_fmt_ext[(e["format_token"], ext)] += 1
    print("total=%d  audio=%d\n" % (total, audio))
    print("== by format_token ==")
    for k, v in by_fmt.most_common():
        print("%6d  %s" % (v, k))
    print("\n== by extension ==")
    for k, v in by_ext.most_common():
        print("%6d  .%s" % (v, k))
    print("\n== (format_token, ext) pairs, top 80 ==")
    for (f, x), v in by_fmt_ext.most_common(80):
        print("%6d  %-16s .%s" % (v, f, x))


if __name__ == "__main__":
    arg = sys.argv[1] if len(sys.argv) > 1 else ""
    if arg == "--crawl":
        crawl()
    elif arg == "--tally":
        tally()
    elif arg == "--build":
        build()
    else:
        print(__doc__)
        sys.exit(1)
