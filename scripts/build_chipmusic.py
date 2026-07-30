#!/usr/bin/env python3
"""Build chipmachine/data/chipmusic.txt -- the chipmusic.org rendered-MP3 archive.

chipmusic.org is a community chiptune platform. Every track is served ONLY as a
rendered MP3 on S3 (directly hotlinkable, HTTP 206 range OK -> ffmpeg-streamable);
no original module download exists. So this onboards as an MP3 STREAMING
collection (like scenesat/demovibes), adding no new decode capability.

Two data sources, split to keep the site load tiny:
  * SPINE (1 request): the RSS feed honours ?limit=N and returns the WHOLE
    catalog in one document -- per item: <title> "artist - title", <link> track
    page, <enclosure url> the S3 MP3, <pubDate>, <itunes:author>. This is all we
    need to PLAY every track.
  * TAGS (per-track, cached): RSS carries no <category>, so to CLASSIFY by
    platform we read each track page's freeform tags (`/music?s=tag:NAME`) and
    map gear tags -> platform (lsdj->Game Boy, sid->C64, nsf->NES, ym->Atari ST,
    amiga/protracker/ahx->Amiga, beeper/1bit->ZX Spectrum, impulse/schism->PC).
    Untagged / unmatched -> the generic "Chipmusic" bucket, so a partial crawl
    still yields a COMPLETE, correct collection. chipmusic.org is behind
    Cloudflare: the crawl is single-digit req/s, disk-cached (resume), and ABORTS
    on any 403/429 so we never hammer or get banned.

  --rss     fetch the RSS spine -> data-notbundled/misc/chipmusic/rss.tsv
  --tags    crawl track pages for tags (cached) -> data-notbundled/misc/chipmusic/tags.tsv
  --build   join spine+tags, classify, write data/chipmusic.txt
"""

import html
import os
import re
import sys
import time
import threading
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

HERE = os.path.dirname(__file__)
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
# Intermediate scrape artifacts: build-time only -> data-notbundled/ (the .app
# bundle gets data/ only, see package_app.sh).
NOTBUNDLED = os.path.normpath(os.path.join(HERE, "..", "data-notbundled"))
MISC = os.path.join(NOTBUNDLED, "misc", "chipmusic")
RSS_OUT = os.path.join(MISC, "rss.tsv")
TAGS_OUT = os.path.join(MISC, "tags.tsv")
CACHE = os.path.join(os.environ.get("TMPDIR", "/tmp"), "chipmusic_tag_cache")
OUT = os.path.join(DATA, "chipmusic.txt")

RSS = "https://chipmusic.org/music/rss/feed.xml?limit=20000"
UA = {"User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36"}
WORKERS = 2
PACE = 0.5          # global min seconds between track-page requests (~2 req/s)

# gear/source tag -> canonical platform label (must match format_map in
# MusicDatabase.cpp initFormats). Order matters: first match wins.
PLATFORM = [
    ("Game Boy",     r"\b(lsdj|little ?sound ?dj|nanoloop|gameboy|game ?boy|"
                     r"gbc|gba|dmg|mgb|nitrotracker)\b"),
    ("Commodore 64", r"\b(sid|c64|commodore ?64|goattracker|cynthcart|defmon|"
                     r"sidtracker)\b"),
    ("NES",          r"\b(nsf|famitracker|famicom|2a03|nintendo)\b"),
    ("Atari ST",     r"\b(atari ?st|ym2149|sndh|maxymiser|\bym\b)\b"),
    ("ZX Spectrum",  r"\b(zx ?spectrum|beeper|1 ?bit|1bit|\bay\b|zx)\b"),
    ("Amiga",        r"\b(amiga|protracker|octamed|milkytracker|\bahx\b|paula|"
                     r"\bmod\b)\b"),
    ("PC",           r"\b(impulse ?tracker|schism|fasttracker|adlib|opl[23]?|"
                     r"renoise|scream ?tracker)\b"),
]
TAG_RE = re.compile(r'/music\?s=tag:([^"]+)"')


# --- gentle global rate limiter + abort-on-block --------------------------
# chipmusic.org (Cloudflare) 403s track URLs whose artist path contains ENCODED
# reserved chars (%3F=?, %2F=/, %23=#) -- ~208 pathological URLs, unrelated to
# rate. So an isolated 403 is SKIPPED (that track -> generic bucket); we only
# ABORT on a BURST of consecutive 403s, which signals a real rate block.
_lock = threading.Lock()
_last = [0.0]
_abort = threading.Event()
_streak = [0]                 # consecutive 403s
BURST_403 = 8                 # this many in a row -> real block, stop


def _throttle():
    with _lock:
        w = PACE - (time.time() - _last[0])
        if w > 0:
            time.sleep(w)
        _last[0] = time.time()


def get(url, timeout=30):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.getcode(), r.read().decode("utf-8", "replace")


# --- RSS spine ------------------------------------------------------------
def rss():
    os.makedirs(MISC, exist_ok=True)
    _, x = get(RSS, timeout=120)
    items = re.findall(r"<item>(.*?)</item>", x, re.S)
    rows = []
    for it in items:
        link = re.search(r"<link>([^<]+)</link>", it)
        mp3 = re.search(r'<enclosure url="([^"]+)"', it)
        title = re.search(r"<title><!\[CDATA\[(.*?)\]\]></title>", it, re.S)
        author = re.search(r"<itunes:author><!\[CDATA\[(.*?)\]\]>", it, re.S)
        if not (link and mp3 and title):
            continue
        t = html.unescape(title.group(1)).strip()
        artist = html.unescape(author.group(1)).strip() if author else ""
        # <title> is "artist - track"; prefer the itunes:author for artist and
        # strip it off the front of the title to get a clean track name.
        name = t
        if artist and t.lower().startswith(artist.lower() + " - "):
            name = t[len(artist) + 3:]
        rows.append("\t".join([link.group(1).strip(), mp3.group(1).strip(),
                               clean(artist), clean(name)]))
    with open(RSS_OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"rss: {len(rows)} tracks -> {RSS_OUT}")


def clean(s):
    return s.replace("\t", " ").replace("\n", " ").strip()


# --- per-track tag crawl (cached, throttled, abort-on-block) ---------------
def tags_for(url):
    if _abort.is_set():
        return None
    key = os.path.join(CACHE, re.sub(r"[^a-zA-Z0-9]", "_", url)[-180:] + ".txt")
    if os.path.exists(key):
        with open(key, encoding="utf-8") as f:
            return f.read().splitlines()
    for attempt in range(4):
        if _abort.is_set():
            return None
        _throttle()
        try:
            code, h = get(url)
            tags = [html.unescape(t).strip().lower() for t in TAG_RE.findall(h)]
            os.makedirs(CACHE, exist_ok=True)
            with open(key, "w", encoding="utf-8") as f:
                f.write("\n".join(tags))
            _streak[0] = 0
            return tags
        except urllib.error.HTTPError as e:
            if e.code == 429:               # hard rate signal -> stop now
                sys.stderr.write(f"\n!! HTTP 429 on {url} -- ABORTING (rate)\n")
                _abort.set()
                return None
            if e.code == 403:
                _streak[0] += 1
                if _streak[0] >= BURST_403:  # many in a row -> real block
                    sys.stderr.write(f"\n!! {BURST_403} consecutive 403s -- "
                                     f"ABORTING (likely rate block)\n")
                    _abort.set()
                    return None
                # isolated 403 = pathological encoded URL -> skip (untagged)
                os.makedirs(CACHE, exist_ok=True)
                open(key, "w").close()
                return []
            time.sleep(2 * (attempt + 1))
        except Exception:
            time.sleep(2 * (attempt + 1))
    return None


def crawl_tags():
    urls = [l.split("\t")[0] for l in open(RSS_OUT, encoding="utf-8")
            if l.strip()]
    done = {}
    if os.path.exists(TAGS_OUT):
        for l in open(TAGS_OUT, encoding="utf-8"):
            c = l.rstrip("\n").split("\t")
            done[c[0]] = c[1] if len(c) > 1 else ""
    todo = [u for u in urls if u not in done]
    sys.stderr.write(f"tags: {len(done)} cached, {len(todo)} to fetch "
                     f"(cache={CACHE})\n")
    n = 0
    with open(TAGS_OUT, "a", encoding="utf-8") as out:
        with ThreadPoolExecutor(max_workers=WORKERS) as ex:
            futs = {ex.submit(tags_for, u): u for u in todo}
            for f in as_completed(futs):
                u = futs[f]
                tags = f.result()
                if tags is None:            # aborted / failed -> leave for resume
                    continue
                out.write(u + "\t" + ",".join(tags) + "\n")
                out.flush()
                n += 1
                if n % 100 == 0:
                    sys.stderr.write(f"\r  fetched {n}/{len(todo)}")
                    sys.stderr.flush()
    sys.stderr.write(f"\ntags: wrote {n} new (abort={_abort.is_set()})\n")


def classify(tagstr):
    t = " " + tagstr.replace(",", " ") + " "
    for name, pat in PLATFORM:
        if re.search(pat, t):
            return name
    return "Chipmusic"


def build():
    tags = {}
    if os.path.exists(TAGS_OUT):
        for l in open(TAGS_OUT, encoding="utf-8"):
            c = l.rstrip("\n").split("\t")
            tags[c[0]] = c[1] if len(c) > 1 else ""
    rows = []
    import collections
    dist = collections.Counter()
    for l in open(RSS_OUT, encoding="utf-8"):
        c = l.rstrip("\n").split("\t")
        if len(c) < 4:
            continue
        url, mp3, artist, title = c[0], c[1], c[2], c[3]
        plat = classify(tags.get(url, ""))
        dist[plat] += 1
        # song_template "title composer format path" (path = full S3 URL,
        # collection source="" like cpcpower/oplarchive)
        rows.append("\t".join([title, artist, plat, mp3]))
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"build: {len(rows)} tracks -> {OUT}")
    for k, v in dist.most_common():
        print(f"  {k:13} {v}")


if __name__ == "__main__":
    if "--rss" in sys.argv:
        rss()
    elif "--tags" in sys.argv:
        crawl_tags()
    elif "--build" in sys.argv:
        build()
    else:
        print(__doc__)
