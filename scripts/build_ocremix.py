#!/usr/bin/env python3
"""OverClocked ReMix (ocremix.org) onboarding for ChipMachineAS.

OCReMix is the premier community archive of *arranged* video-game music --
~5000 free, downloadable MP3 remixes (OCR00001..OCR050xx), each tied to a source
game, its original composer, and the remixer.  This is a rendered-MP3 STREAMING
collection (ffmpeg playback, no new decode capability), the same class as
amigaremix / chipmusic.

Access note: ocremix.org/robots.txt disallows AI-training crawlers (ClaudeBot,
GPTBot, CCBot, ...) but has NO general User-agent:* block; there is no public API
or data dump.  With the user's explicit go-ahead we do a POLITE, THROTTLED,
browser-UA metadata fetch (one detail page per remix, cached/resumable) -- the
same hosts the app already streams from at play time.

Each remix's stable, directly-hotlinkable audio URL lives on the mirror network
(iterations.org / ocrmirror.org / ocr.blueblue.fr) at
    https://iterations.org/files/music/remixes/<Abbrev_Game>_<Title>_OC_ReMix.mp3
The filename uses an ABBREVIATED game name that is NOT derivable from metadata, so
the detail page (which also carries clean OpenGraph tags) is fetched per remix.

Screenshots: each remix's og:image is the source game's title-screen/box image on
ocremix.org (a genuine game shot), keyed by the song URL in
data/ocremix_screenshots.txt (getSongScreenshots ocremix branch).

Usage:
    build_ocremix.py --crawl     # detail pages -> scratchpad jsonl cache
    build_ocremix.py --systems   # tally the game-system tokens (design the map)
    build_ocremix.py --build     # cache -> data/ocremix.txt + ocremix_screenshots.txt
"""
import html, json, os, re, sys, time, urllib.parse, urllib.request

FEED = "https://ocremix.org/feeds/ten20/"
DETAIL = "https://ocremix.org/remix/OCR{id:05d}"
UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120 Safari/537.36")
HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
CACHE = os.environ.get("OCR_CACHE",
        "/private/tmp/claude-502/-Users-mihailod-Documents-chipmachine-as/"
        "5c8b19f4-f34f-4c96-9d33-7990b2de3fa0/scratchpad/ocremix_entries.jsonl")

# Prefer this mirror for the stored URL (all three carry identical paths).
MIRROR = "https://iterations.org"

_OG = lambda prop, s: (re.search(
    r'<meta property="og:%s" content="([^"]*)"' % prop, s) or [None, None])[1]


def _get(url, tries=3):
    for i in range(tries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=45) as r:
                return r.read().decode("utf-8", "replace"), r.getcode()
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return None, 404
            if i == tries - 1:
                return None, e.code
            time.sleep(2 * (i + 1))
        except Exception:
            if i == tries - 1:
                return None, 0
            time.sleep(2 * (i + 1))
    return None, 0


def max_ocr_id():
    body, _ = _get(FEED)
    ids = re.findall(r"OCR(\d{5})", body or "")
    return max(int(x) for x in ids) if ids else 5046


def parse_detail(ocr_id, s):
    og_title = _OG("title", s) or ""
    og_desc = _OG("description", s) or ""
    og_image = _OG("image", s) or ""
    og_title = html.unescape(og_title)
    og_desc = html.unescape(og_desc)

    # og:title = '<Game> "<Title>" OC ReMix'  -> split on the quoted title.
    m = re.search(r'^(.*?)\s*"(.+)"\s*OC ReMix\s*$', og_title)
    game = html.unescape(m.group(1)).strip() if m else og_title.strip()
    title = html.unescape(m.group(2)).strip() if m else ""

    # og:description = '...MP3: <game> "<title>" by <remixer(s)>'
    dm = re.search(r'\bby\s+(.+?)\s*$', og_desc)
    remixer = html.unescape(dm.group(1)).strip() if dm else ""

    # Original composer(s): the <a class="color-original">NAME</a> links.
    comps = [html.unescape(c).strip()
             for c in re.findall(r'class="color-original"[^>]*>([^<]+)', s)]
    # dedupe, keep order
    seen = set(); composers = []
    for c in comps:
        if c and c not in seen:
            seen.add(c); composers.append(c)

    # Platform from the game-image system dir: /files/images/games/<sys>/...
    pm = re.search(r"/files/images/games/([a-z0-9-]+)/", s)
    system = pm.group(1) if pm else ""

    # Stable MP3 URL (any mirror path is identical); rehost onto MIRROR.
    fm = re.search(r'https?://[^"]+/files/music/remixes/([^"]+\.mp3)', s)
    if not fm:
        return None
    fname = fm.group(1)
    url = MIRROR + "/files/music/remixes/" + fname

    if not og_image.startswith("http"):
        og_image = ""

    return {
        "id": ocr_id, "game": game, "title": title, "remixer": remixer,
        "composers": composers, "system": system, "url": url, "art": og_image,
    }


def _done_ids():
    done = set()
    if os.path.exists(CACHE):
        with open(CACHE) as f:
            for line in f:
                try:
                    done.add(json.loads(line)["id"])
                except Exception:
                    pass
    return done


def crawl():
    mx = max_ocr_id()
    sys.stderr.write("max OCR id = %d\n" % mx)
    done = _done_ids()
    sys.stderr.write("already cached: %d\n" % len(done))
    kept = 0; miss = 0
    with open(CACHE, "a") as out:
        for i in range(1, mx + 1):
            if i in done:
                continue
            body, code = _get(DETAIL.format(id=i))
            if code == 404 or not body:
                miss += 1
            else:
                rec = parse_detail(i, body)
                if rec:
                    out.write(json.dumps(rec, ensure_ascii=False) + "\n")
                    out.flush()
                    kept += 1
                else:
                    miss += 1
            if i % 20 == 0:
                sys.stderr.write("\rOCR%05d  kept %d  miss %d" % (i, kept, miss))
                sys.stderr.flush()
            time.sleep(0.5)                 # polite: ~2 req/s
    sys.stderr.write("\ndone: kept %d, miss %d -> %s\n" % (kept, miss, CACHE))


def iter_cache():
    with open(CACHE) as f:
        for line in f:
            line = line.strip()
            if line:
                yield json.loads(line)


# OCReMix game-system token (from the /files/images/games/<sys>/ dir) ->
# ChipMachine `format` string (F9 platform filter).  Tokens with a real chip
# filter map to it; modern/CD-DA consoles that have no chip filter -> "MP3"
# (Unclassified), honest for rendered arranged audio.  Every string here is a
# format_map key (see initFormats), except "playstation 2" which this onboarding
# adds.  Derived from the actual --systems tally.
SYSTEM_FORMAT = {
    # Nintendo
    "snes": "Super Nintendo", "nes": "NES", "n64": "Nintendo 64",
    "gb": "Nintendo Game Boy (GB)", "gbc": "Nintendo Game Boy (GB)",
    "gba": "Nintendo Game Boy (GB)", "nds": "Nintendo DS Sound Format",
    "gcn": "MP3", "wii": "MP3", "wiiu": "MP3", "3ds": "MP3",
    "swtch": "MP3", "vb": "MP3",
    # Sega
    "gen": "Sega Genesis", "sms": "Sega Master System", "gg": "Sega Game Gear",
    "32x": "Sega 32X", "scd": "Sega Mega CD", "sat": "Sega Saturn",
    "dc": "Dreamcast", "pico": "Sega Genesis",
    # Sony
    "ps1": "Playstation", "ps2": "Playstation 2", "ps3": "MP3", "ps4": "MP3",
    "ps5": "MP3", "psp": "MP3", "vita": "MP3",
    # Microsoft
    "xbox": "MP3", "xb360": "MP3", "xbone": "MP3", "xbsx": "MP3",
    # PC / home computers
    "win": "PC", "dos": "PC", "mac": "Macintosh", "amiga": "Amiga",
    "c64": "Commodore 64", "spec": "ZX Spectrum", "cpc": "Amstrad CPC",
    "msx": "MSX", "x68k": "Sharp X68000", "pc88": "NEC PC-88",
    "aplii": "Apple II", "st": "Atari ST", "atari": "Atari 8Bit",
    # NEC PC Engine / TurboGrafx
    "tg16": "PC Engine", "tgcd": "PC Engine",
    # Arcade / Neo Geo / misc
    "arc": "Arcade", "ng": "Arcade", "ngcd": "Arcade", "jag": "Atari Jaguar",
    "cdi": "MP3", "ios": "MP3", "ngage": "MP3", "3do": "MP3", "mobile": "MP3",
}


def systems():
    from collections import Counter
    c = Counter(e.get("system", "") for e in iter_cache())
    for k, v in c.most_common():
        mapped = SYSTEM_FORMAT.get(k, "??MP3")
        print("%6d  %-14s -> %s" % (v, k or "(none)", mapped))


def build():
    from collections import Counter
    rows = []; shots = {}; fmt_count = Counter(); seen = set()
    for e in iter_cache():
        url = e.get("url", "")
        if not url or url in seen:
            continue
        seen.add(url)
        title = (e.get("title") or "").strip().replace("\t", " ")
        game = (e.get("game") or "").strip().replace("\t", " ")
        if not title:
            title = game or ("OCR%05d" % e["id"])
        # composer = "<remixer> / <original composer(s)>" so a search for either
        # the OCR arranger OR the original game composer surfaces the remix. The
        # original composers come from the detail page's color-original links.
        remixer = (e.get("remixer") or "").strip().replace("\t", " ")
        orig = ", ".join(c.replace("\t", " ") for c in (e.get("composers") or []))
        if remixer and orig:
            composer = remixer + " / " + orig
        else:
            composer = remixer or orig
        fmt = SYSTEM_FORMAT.get(e.get("system", ""), "MP3")
        rows.append((title, game, composer, fmt, url))
        fmt_count[fmt] += 1
        art = (e.get("art") or "").strip()
        if art.startswith("http"):
            shots[url] = art

    with open(os.path.join(DATA, "ocremix.txt"), "w", encoding="utf-8") as f:
        for r in rows:
            f.write("\t".join(r) + "\n")
    with open(os.path.join(DATA, "ocremix_screenshots.txt"), "w",
              encoding="utf-8") as f:
        for url, art in shots.items():
            f.write(url + "\t" + art + "\n")

    print("wrote %d remixes -> data/ocremix.txt" % len(rows))
    print("wrote %d screenshots -> data/ocremix_screenshots.txt" % len(shots))
    print("\n== by platform ==")
    for k, v in fmt_count.most_common():
        print("%6d  %s" % (v, k))


if __name__ == "__main__":
    arg = sys.argv[1] if len(sys.argv) > 1 else ""
    if arg == "--crawl":
        crawl()
    elif arg == "--systems":
        systems()
    elif arg == "--build":
        build()
    else:
        print(__doc__); sys.exit(1)
