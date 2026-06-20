#!/usr/bin/env python3
"""
Build chipmachine/data/unexotica_screenshots.txt — Amiga game screenshots for the
UnExoticA /Game/ subset, sourced from Hall of Light (amiga.abime.net).

We do NOT scrape Hall of Light directly (the live site is behind an Anubis
proof-of-work anti-scraper wall, and the maintainer asked to avoid it). Both the
game-page index and the screenshot images are taken from the Internet Archive
Wayback machine; images are served at runtime via Wayback. HOL itself is never hit.

Pipeline (see the long investigation in memory: hall-of-light is harder than
gb64/hvtc/sndh because HOL keys screenshots by NUMERIC game id, not name):
  1. game name -> slug (lowercase, '-' separators), matched against the set of
     archived HOL `games/view/<slug>` pages (one Wayback CDX query).
  2. fetch each matched HOL game page from Wayback (GENTLE 5s pacing — 2s tripped
     archive.org rate-limiting) and read the screenshot path from its <og:image>
     meta tag (the `/screen/<range>/<id>_screenN.png` scheme).
  3. join that path to a GOOD archived image capture: HOL's most recent Wayback
     captures are 238-byte Anubis junk, so we must pick an explicit timestamp of
     a real image/png capture (from a second CDX query, filtered mimetype=image).
  4. emit "<game .lha key>\t<explicit-timestamp Wayback image URL>". The runtime
     (MusicDatabase getSongScreenshots, unexotica branch) extracts the
     "/Game/<composer>/<game>.lha" key from the song path and looks it up.

Inputs (pre-pulled to avoid re-querying; the script also fetches them if absent):
  --cdx-slugs   F   CDX dump of amiga.abime.net/games/view (fl=original)
  --cdx-screens F   CDX dump of amiga.abime.net/screen (fl=original,timestamp,length, mimetype image)
  --limit N         cap the number of games harvested this run (partial coverage)
  --resume          reuse the page-id cache, skip already-harvested games
"""

import re
import sys
import time
import json
import argparse
import urllib.request
from pathlib import Path
from collections import defaultdict

UA = {"User-Agent": ("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                     "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4.1 "
                     "Safari/605.1.15")}
WB = "https://web.archive.org/web/"
HOL_PAGE = "https://amiga.abime.net/games/view/"
HOL = "https://amiga.abime.net"
PACING = 5.0   # seconds between page fetches (archive.org rate-limits faster)

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
SONG_LIST = DATA / "unexotica.txt"
OUT_FILE = DATA / "unexotica_screenshots.txt"
CACHE = HERE / "unexotica_screenshots_cache.json"   # slug -> og:image path or ""


def slugify(name):
    return re.sub(r"[^a-z0-9-]", "", name.lower().replace("_", "-"))


def load_games():
    """Return {slug: lha_key} and {slug: display} for /Game/ entries.
    lha_key = '/Game/<composer>/<game>.lha' (the per-game runtime lookup key)."""
    keys, names = {}, {}
    for line in SONG_LIST.read_text(encoding="utf-8", errors="replace").splitlines():
        c = line.split("\t")
        if len(c) < 5:
            continue
        path = c[-1].strip()
        m = re.search(r"(/Game/[^/]+/([^/]+)\.lha)", path)
        if not m:
            continue
        lha_key, game = m.group(1), m.group(2)
        sl = slugify(game)
        if sl and sl not in keys:
            keys[sl] = lha_key
            names[sl] = c[1].strip() or game
    return keys, names


def fetch(url, tries=3):
    for t in range(tries):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=UA),
                                        timeout=45) as r:
                return r.read().decode("utf-8", "replace")
        except Exception as e:
            last = str(e)
            time.sleep(5 * (t + 1))
    sys.stderr.write(f"  unreachable {url}: {last}\n")
    return ""


def cdx(args_url):
    import urllib.parse
    return fetch("http://web.archive.org/cdx/search/cdx?" + args_url)


def page_ts_index(dump):
    """slug -> timestamp of the LARGEST archived capture. HOL's recent `2id_`
    captures are often the Anubis anti-scraper shell (~20KB, no <og:image>); the
    real server-rendered page (~170KB) is an older, larger capture. Picking max
    length per slug gives the good one. Returns {slug: timestamp}."""
    raw = Path(dump).read_text() if dump else cdx(
        "url=amiga.abime.net/games/view&matchType=prefix&filter=statuscode:200&"
        "filter=mimetype:text/html&fl=original,timestamp,length&limit=300000")
    best = {}
    for line in raw.splitlines():
        p = line.split()
        if len(p) < 3:
            continue
        m = re.search(r"games/view/([a-z0-9_-]+)", p[0], re.I)
        if not m:
            continue
        slug, length = m.group(1).lower(), int(p[2]) if p[2].isdigit() else 0
        if slug not in best or length > best[slug][1]:
            best[slug] = (p[1], length)
    return {s: ts for s, (ts, _) in best.items()}


def screen_index(dump):
    """path(no query) -> best (timestamp) image capture, picking max length."""
    raw = Path(dump).read_text() if dump else cdx(
        "url=amiga.abime.net/screen&matchType=prefix&filter=statuscode:200&"
        "filter=mimetype:image/.*&fl=original,timestamp,length&limit=300000")
    # Group captures by the query-stripped path, but KEEP the original archived
    # URL (with its ?v=NNN query string) — Wayback only has a capture at the
    # exact archived URL, so dropping the query yields a 0-byte miss.
    best = {}   # stripped_path -> (timestamp, length, original_full_url)
    for line in raw.splitlines():
        p = line.split()
        if len(p) < 3:
            continue
        m = re.search(r"/screen/[0-9-]+/(\d+)_screen\d+\.(?:png|jpg)", p[0])
        if not m:
            continue
        stripped = p[0].split("?", 1)[0]
        length = int(p[2]) if p[2].isdigit() else 0
        if stripped not in best or length > best[stripped][1]:
            best[stripped] = (p[1], length, p[0])    # (ts, length, full url)
    by_id = defaultdict(list)
    for path in best:
        gid = re.search(r"/(\d+)_screen", path).group(1)
        by_id[gid].append(path)
    for gid in by_id:
        by_id[gid].sort(key=lambda x: (int(re.search(r"_screen(\d+)", x).group(1)), x))
    return best, by_id


def og_image_id(html):
    m = re.search(r'og:image"\s*content="([^"]+?_screen\d+\.(?:png|jpg))', html)
    if not m:
        return ""
    gid = re.search(r"/(\d+)_screen", m.group(1))
    return gid.group(1) if gid else ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cdx-pages", help="CDX dump of games/view (original,timestamp,length)")
    ap.add_argument("--cdx-screens")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--no-harvest", action="store_true",
                    help="skip fetching; just rebuild the data file from cache")
    args = ap.parse_args()

    keys, names = load_games()
    page_ts = page_ts_index(args.cdx_pages)
    best, by_id = screen_index(args.cdx_screens)
    matched = sorted(s for s in keys if s in page_ts)
    print(f"{len(keys)} unexotica games, {len(page_ts)} HOL pages, "
          f"{len(matched)} name matches; {len(best)} archived images")

    cache = json.loads(CACHE.read_text()) if ((args.resume or args.no_harvest)
                                              and CACHE.exists()) else {}
    todo = [] if args.no_harvest else \
        [s for s in matched if not (args.resume and s in cache)]
    if args.limit:
        todo = todo[:args.limit]
    print(f"harvesting {len(todo)} game pages (5s pacing, explicit timestamps)...")

    for i, slug in enumerate(todo, 1):
        # explicit largest-capture timestamp dodges the Anubis shell capture
        gid = og_image_id(fetch(f"{WB}{page_ts[slug]}id_/{HOL_PAGE}{slug}"))
        cache[slug] = gid
        CACHE.write_text(json.dumps(cache))
        print(f"[{i}/{len(todo)}] {'id=' + gid if gid else 'NO-ID':10} {slug}",
              flush=True)
        time.sleep(PACING)

    # join: slug -> id -> best archived image (explicit timestamp) -> URL
    results = {}
    for slug, gid in cache.items():
        if not gid or gid not in by_id:
            continue
        path = by_id[gid][0]                       # prefer _screen0
        ts, _, original = best[path]
        results[keys[slug]] = f"{WB}{ts}id_/{original}"   # original keeps ?v=NNN

    lines = [f"{k}\t{u}" for k, u in sorted(results.items())]
    OUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_FILE}: {len(results)} games with screenshots")


if __name__ == "__main__":
    main()
