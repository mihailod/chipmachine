#!/usr/bin/env python3
"""Build chipmachine/data/zophar.txt -- Zophar's Domain console gamerips.

Driven by data/misc/zophar.net/list.csv (17712 games / 21 platforms, scraped by
eliotbyte/zophar-downloader). We do NOT re-scrape Zophar -- the CSV already has
name + platform + image_url + game_page_url per game. The EMU (original chip) zip
URL is reconstructed from (slug,title) -- verified 200 across platforms (the CSV's
l1/l2 link columns are misaligned on rows with a missing link, so we only trust
the CSV for the game inventory, never for the URL).

SCOPE (2026-07-11, user decision "ship sequenced now, hold streamed"):
Zophar's platforms split by what the "(EMU).zophar.zip" actually contains --
verified by opening the zips:

  SEQUENCED CHIP (homogeneous, all decodable) -- ONBOARD:
    NES nsf, SNES spc, Game Boy gbs, Game Boy Advance minigsf, Nintendo DS 2sf,
    Nintendo 64 usf, TurboGrafx-16 hes, Genesis vgm, Master System vgm,
    Game Gear sgc.  (GBA/N64 look mp3-only on the game page -- that's a preview;
    the EMU zip holds real gsf/usf.)

  STREAMED AUDIO (recorded rips: xa/asf/adx/at3/dsp/eam/genh/ogg/wav/... ) -- HELD:
    PS1 (part psf, part xa/str), PS2 (asf/musx/genh), Saturn (part ssf, part dvi),
    Dreamcast (adx/genh, little dsf), 3DS, GameCube, Wii, Xbox, Xbox360, PS3, PSP.
    These are NOT chip formats -- their only playback path is vgmstream. Their
    extensions are written to unplayable_formats.txt for a separate go/no-go.

Dedup: per platform, {norm(game-title)} against the WHOLE console corpus mapped by
format family -- modland (allmods.txt, by extension), vgmrips, smspower, rsn
(SNES), nsfe (NES). Emitted rows are unique on {title, format}. modland's console
coverage is deep-but-narrow (few distinct games, many tracks each); Zophar is
game-complete, so most platforms are substantially net-new at the game level.

Play pipeline (already built for the Genesis pilot): MusicPlayerList detects the
downloaded zip by PK magic, extracts members, and plays the music-ext files as
local subsongs.

  --build        dedup list.csv vs corpus, write zophar.txt + unplayable_formats.txt
  --build-audio  APPEND the 3DS/Xbox360 games that carry an ffmpeg-playable
                 (ogg/wav/mp2/...) member -- per-game zip-central-dir scan
  --screenshots  key each emitted song URL to its game's image_url (thumbs_large),
                 write zophar_screenshots.txt
"""

import csv, html, os, re, struct, sys, time, urllib.parse, urllib.request

HERE = os.path.dirname(__file__)
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
CSV  = os.path.join(DATA, "misc", "zophar.net", "list.csv")
OUT       = os.path.join(DATA, "zophar.txt")
OUT_SHOTS = os.path.join(DATA, "zophar_screenshots.txt")
OUT_UNPL  = os.path.join(DATA, "misc", "zophar.net", "unplayable_formats.txt")
CDN = "https://fi.zophar.net/soundfiles/"

# slug -> (format label for db.lua classification (must resolve in the
# MusicDatabase format_map), row extension, dedup family-key). Only the
# sequenced-chip platforms; the streamed tier is intentionally absent.
PLATFORM = {
    "nintendo-nes-nsf":        ("Nintendo Sound Format",     "nsf", "NES"),
    "nintendo-snes-spc":       ("Super Nintendo",            "spc", "SNES"),
    "gameboy-gbs":             ("Nintendo Game Boy (GB)",    "gbs", "GB"),
    "gameboy-advance-gsf":     ("Gameboy Advance",           "gsf", "GBA"),
    "nintendo-ds-2sf":         ("Nintendo DS Sound Format",  "2sf", "DS"),
    "nintendo-64-usf":         ("Ultra64 Sound Format",      "usf", "N64"),
    "turbografx-16-hes":       ("HES",                       "hes", "TG16"),
    "sega-mega-drive-genesis": ("Sega Genesis",              "vgm", "GEN"),
    "sega-master-system-vgm":  ("Sega Master System",        "vgm", "SMS"),
    "sega-game-gear-sgc":      ("Sega Game Gear",            "sgc", "GG"),
}

# Streamed-tier platforms -- NOW ONBOARDED (2026-07-13) via the vgmstream plugin
# (adx/asf/at3/aus/bcwav/dsp/dvi/eam/genh/lwav/oma/spsd/ss2/str/xa + ATRAC once
# VGM_USE_FFMPEG was enabled) plus ffmpeg (ogg/wav) and the existing ssf/dsf. Each
# is emitted by --build (integrated with the sequenced tier) into the SAME
# zophar.txt collection. slug -> (format label -- MUST resolve in MusicDatabase's
# format_map -- , representative row extension for display, dedup family key or
# None). Family keys map to the modland console sound-format dirs in build_corpus;
# consoles modland lacks (Xbox/Wii/GameCube/3DS/PS3/PSP) dedup within-tier only.
STREAMED = {
    "playstation-psf":          ("Playstation",          "xa",    "PSX"),
    "playstation2-psf2":        ("Playstation 2",        "asf",   "PS2"),
    "sega-saturn-ssf":          ("Sega Saturn",          "dvi",   "SAT"),
    "sega-dreamcast-dsf":       ("Sega Dreamcast",       "adx",   "DC"),
    "nintendo-3ds-3sf":         ("Nintendo 3DS",         "bcwav", None),
    "nintendo-gamecube-gcn":    ("Nintendo GameCube",    "dsp",   None),
    "nintendo-wii":             ("Nintendo Wii",         "dsp",   None),
    "xbox":                     ("Xbox",                 "lwav",  None),
    "xbox-360":                 ("Xbox 360",             "lwav",  None),
    "playstation3-psf3":        ("Playstation 3",        "at3",   None),
    "playstation-portable-psp": ("Playstation Portable", "at3",   None),
}

# modland top-level dirs whose game titles dedup the streamed rips of the same
# console family (game-title match within family; see build_corpus).
CONSOLE_TOPDIR = {
    "Playstation Sound Format":   "PSX",
    "Playstation 2 Sound Format": "PS2",
    "Saturn Sound Format":        "SAT",
    "Dreamcast Sound Format":     "DC",
}


def norm(s):
    s = html.unescape(s or "").lower()
    s = re.sub(r"\((scd|32x|pico|mcd|sgx|gg|sms|md|arcade|emu|mp3|naomi|"
               r"system \d+)\)", "", s)
    return re.sub(r"[^a-z0-9]", "", s)


def plat_of(page):  return page.split("/music/")[-1].split("/")[0]
def slug_of(page):  return page.rstrip("/").split("/")[-1]


# ---------------------------------------------------------------- corpus ------
# family-key -> set(norm gamename) already in the DB, keyed by format family so a
# NES "Batman" never dedups against an SNES "Batman".
EXT2FAM = {
    "nsf":"NES", "nsfe":"NES", "spc":"SNES", "rsn":"SNES", "gbs":"GB",
    "gsf":"GBA", "minigsf":"GBA", "2sf":"DS", "mini2sf":"DS",
    "usf":"N64", "miniusf":"N64", "usflib":"N64", "hes":"TG16",
}
SEGA_SUB = {"Sega Megadrive":"GEN", "Sega Mega CD":"GEN", "Sega 32X":"GEN",
            "Sega Master System":"SMS", "Sega Game Gear":"GG",
            "Sega SG-1000":"SMS", "Sega SC-3000":"SMS"}


def build_corpus():
    fam = {}
    def add(k, n):
        if n: fam.setdefault(k, set()).add(n)
    def ext_of(p):
        m = re.search(r"\.([a-z0-9]+)$", p, re.I); return m.group(1).lower() if m else ""

    with open(os.path.join(DATA, "allmods.txt"), encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.split("\t", 1)[-1].strip(); parts = p.split("/"); e = ext_of(p)
            top = parts[0]
            if top in CONSOLE_TOPDIR:
                # <Sound Format dir>/<composer>/<game>/<track>.ext (depth>=4) ->
                # game=parts[-2]; shallower rips fall back to the filename stem.
                g = parts[-2] if len(parts) >= 4 else re.sub(r"\.[a-z0-9]+$", "", parts[-1])
                add(CONSOLE_TOPDIR[top], norm(g)); continue
            if top == "Megadrive GYM" or e == "gym":
                # Megadrive GYM/<composer>/<game>.gym
                add("GEN", norm(re.sub(r"\.[a-z0-9]+$", "", parts[-1]))); continue
            if top == "Video Game Music" and len(parts) > 1:
                sysk = SEGA_SUB.get(parts[1])
                if sysk:
                    # Video Game Music/<System>/<Composer>/<Game>/<track>.vgz (depth 5)
                    g = parts[3] if len(parts) >= 5 else re.sub(r"\.[a-z0-9]+$", "", parts[-1])
                    add(sysk, norm(g))
                continue
            # composer/game/track (depth>=4) -> game=parts[-2]; else filename stem
            g = parts[-2] if len(parts) >= 4 else re.sub(r"\.[a-z0-9]+$", "", parts[-1])
            k = EXT2FAM.get(e)
            if k: add(k, norm(g))
            if e == "sgc":  # SGC/<composer>/<game>.sgc -> Master System + Game Gear
                add("SMS", norm(g)); add("GG", norm(g))

    def tab(fn):
        try:
            return [l.rstrip("\n").split("\t")
                    for l in open(os.path.join(DATA, fn), encoding="utf-8", errors="replace")]
        except FileNotFoundError:
            return []
    for c in tab("rsn.txt"):   # \t title \t composer \t plat \t file
        if len(c) >= 2: add("SNES", norm(c[1]))
    for c in tab("nsfe.txt"):
        if len(c) >= 2: add("NES", norm(c[1]))
    for c in tab("vgmrips.txt"):  # "Title (System)" \t \t Arcade \t url \t vgz
        if c and c[0]:
            t = c[0]; n = norm(re.sub(r"\s*\([^)]*\)\s*$", "", t)); low = t.lower()
            if any(w in low for w in ("mega drive", "genesis", "megadrive")): add("GEN", n)
            elif "master system" in low: add("SMS", n)
            elif "game gear" in low: add("GG", n)
    for c in tab("smspower.txt"):
        if c and c[0]: add("SMS", norm(c[0])); add("GG", norm(c[0]))
    return fam


# ---------------------------------------------------------------- build -------
def emu_url(plat, slug, title):
    return (CDN + plat + "/" + urllib.parse.quote(slug) + "/"
            + urllib.parse.quote(f"{title} (EMU).zophar.zip"))


def build():
    corpus = build_corpus()
    rows, dropped = [], 0
    seen = set()                       # {norm title, format} uniqueness
    stats = {}                         # plat -> [total, kept, dup]
    # streamed-tier uniqueness is per-platform (a PS3 and a PSP game can share a
    # title -> keyed by (norm title, slug), not by the shared representative ext).
    seen_streamed = set()
    with open(CSV, newline="") as f:
        for row in csv.DictReader(f):
            plat = plat_of(row["game_page_url"])
            title = html.unescape(row["name"]).strip()
            n = norm(title)

            sm = STREAMED.get(plat)
            if sm:                                    # streamed tier (vgmstream)
                label, ext, fam = sm
                st = stats.setdefault(plat, [0, 0, 0]); st[0] += 1
                if fam and n in corpus.get(fam, ()):  # already in modland
                    st[2] += 1; dropped += 1; continue
                key = (n, plat)
                if key in seen_streamed:
                    continue
                seen_streamed.add(key)
                url = emu_url(plat, slug_of(row["game_page_url"]), title)
                rows.append("\t".join([title, "", label, url, ext]))
                st[1] += 1
                continue

            meta = PLATFORM.get(plat)
            if not meta:
                continue               # a platform we do not onboard
            label, ext, fam = meta
            st = stats.setdefault(plat, [0, 0, 0]); st[0] += 1
            if n in corpus.get(fam, ()):        # already in the corpus
                st[2] += 1; dropped += 1; continue
            key = (n, ext)
            if key in seen:
                continue
            seen.add(key)
            url = emu_url(plat, slug_of(row["game_page_url"]), title)
            rows.append("\t".join([title, "", label, url, ext]))
            st[1] += 1

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")

    print(f"wrote {len(rows)} rows ({dropped} corpus dups dropped) -> {OUT}")
    sys.stderr.write(f"\n{'platform':28}{'total':>7}{'kept':>7}{'dup':>7}  label\n")
    tot = [0, 0, 0]
    for p in sorted(stats):
        t, k, d = stats[p]; tot = [tot[0]+t, tot[1]+k, tot[2]+d]
        label = (PLATFORM.get(p) or STREAMED.get(p))[0]
        tier = "seq" if p in PLATFORM else "str"
        sys.stderr.write(f"{p:28}{t:7d}{k:7d}{d:7d}  [{tier}] {label}\n")
    sys.stderr.write(f"{'TOTAL':28}{tot[0]:7d}{tot[1]:7d}{tot[2]:7d}\n")


# ---------------------------------------------------- ogg/wav audio pass ------
# 3DS and Xbox 360 "(EMU)" zips are heterogeneous streamed-audio grab-bags; only
# a minority of games carry a member ffmpegplugin can decode. We keep exactly
# those games (the zip-by-magic handler's audioExt fallback plays the ogg/wav/mp2
# members and ignores the bcstm/xma/adx/... it can't). Reading only each zip's
# central directory via an HTTP Range tail keeps this to ~64KB/game regardless of
# the (often 100MB+) payload.
AUDIO_PLATFORM = {   # slug -> format label (both classify to OTHER)
    "nintendo-3ds-3sf": "Nintendo 3DS",
    "xbox-360":         "Xbox 360",
}
FFMPEG_EXT = ("ogg", "wav", "mp2", "mp3", "flac", "m4a", "aac", "opus")
UA = {"User-Agent": "Mozilla/5.0 chipmachine-zophar/2.0"}


def zip_central_names(url):
    """Member filenames from a remote zip's central directory (Range tail)."""
    req = urllib.request.Request(url, headers={**UA, "Range": "bytes=-131072"})
    try:
        data = urllib.request.urlopen(req, timeout=25).read()
    except Exception:
        return None
    names, i, sig = [], 0, b"PK\x01\x02"
    while True:
        j = data.find(sig, i)
        if j < 0:
            break
        try:
            nlen = struct.unpack_from("<H", data, j + 28)[0]
            elen = struct.unpack_from("<H", data, j + 30)[0]
            clen = struct.unpack_from("<H", data, j + 32)[0]
            names.append(data[j + 46:j + 46 + nlen].decode("utf-8", "replace"))
            i = j + 46 + nlen + elen + clen
        except Exception:
            i = j + 4
    return names


def build_audio():
    # SUPERSEDED (db.lua v95): 3DS and Xbox 360 are now full streamed-tier platforms
    # emitted by --build (STREAMED map), so this ogg/wav-only pass would duplicate
    # them. Kept for reference; refuse to run so it can't append dup rows.
    sys.exit("--build-audio is superseded by --build's streamed tier (db.lua v95); "
             "3DS/Xbox 360 are emitted there. Nothing to do.")
    games = []
    with open(CSV, newline="") as f:
        for row in csv.DictReader(f):
            plat = plat_of(row["game_page_url"])
            if plat in AUDIO_PLATFORM:
                games.append((plat, row["game_page_url"], html.unescape(row["name"]).strip()))
    rows, kept, scanned = [], 0, 0
    per = {}
    for plat, page, title in games:
        scanned += 1
        url = emu_url(plat, slug_of(page), title)
        names = zip_central_names(url)
        if names is None:
            time.sleep(0.3); continue
        # count ffmpeg-playable members; pick the most common as the row ext
        counts = {}
        for n in names:
            if "." in n:
                e = n.rsplit(".", 1)[-1].lower()
                if e in FFMPEG_EXT:
                    counts[e] = counts.get(e, 0) + 1
        if counts:
            ext = max(counts, key=counts.get)
            rows.append("\t".join([title, "", AUDIO_PLATFORM[plat], url, ext]))
            kept += 1
            per[plat] = per.get(plat, 0) + 1
        if scanned % 50 == 0:
            sys.stderr.write(f"\rscanned {scanned}/{len(games)}, kept {kept}")
            sys.stderr.flush()
        time.sleep(0.25)
    sys.stderr.write("\n")
    # APPEND to zophar.txt (keep the sequenced rows already written by --build)
    with open(OUT, "a", encoding="utf-8") as f:
        if rows:
            f.write("\n".join(rows) + "\n")
    print(f"audio pass: kept {kept}/{scanned} games "
          f"({', '.join(f'{k}={v}' for k,v in per.items())}) -> appended to {OUT}")


# ----------------------------------------------------------- screenshots ------
def screenshots():
    # image_url already in the CSV; upgrade to the hi-res sibling; key by the
    # (platform, slug) embedded in each emitted EMU URL.
    img = {}
    with open(CSV, newline="") as f:
        for row in csv.DictReader(f):
            u = (row["image_url"] or "").strip()
            if u:
                img[(plat_of(row["game_page_url"]), slug_of(row["game_page_url"]))] = \
                    u.replace("/thumbs_small/", "/thumbs_large/")
    out, matched, total = [], 0, 0
    for line in open(OUT, encoding="utf-8", errors="replace"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 4 or not c[3]:
            continue
        total += 1
        seg = urllib.parse.urlparse(c[3]).path.split("/")
        try:
            i = seg.index("soundfiles")
            key = (seg[i+1], urllib.parse.unquote(seg[i+2]))
        except (ValueError, IndexError):
            continue
        u = img.get(key)
        if u:
            out.append(c[3] + "\t" + u); matched += 1
    with open(OUT_SHOTS, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print(f"wrote {matched}/{total} screenshots -> {OUT_SHOTS}")


if __name__ == "__main__":
    if "--build" in sys.argv: build()
    elif "--build-audio" in sys.argv: build_audio()
    elif "--screenshots" in sys.argv: screenshots()
    else: print(__doc__)
