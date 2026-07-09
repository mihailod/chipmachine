#!/usr/bin/env python3
"""Build chipmachine/data/cpcpower.txt -- the Amstrad CPC YM audiotheque.

CPC-Power (cpc-power.com) hosts per-game AY-3-8912 chiptune rips as LHA-wrapped
.ym files under /YM/<name>.ym, where the filename carries the metadata:
    <Game> (<year>)(<publisher>)(<composer>)(<format-tag>...).ym

There is NO browsable music index on the live site (/YM/ is 403, no music-list
page, the games DB is 10/page over 20,107 fiches, and the search form has no
"has music" filter). Crawling all 20k live game pages blindly to find the ones
with music would hammer cpc-power. So we split discovery from harvesting:

  * Each game with music has an `onglet=zicym` tab whose HTML holds a JS array
        liste_musique[i] = "./YM/<Game> (year)(publisher)(composer).ym"
  * DISCOVERY (cheap, one call): the Wayback CDX has archived ~2,568 distinct
    fiches that have a zicym tab (HTTP 200). We enumerate those game ids from it
    -- that is the "which games have music" answer, without touching cpc-power.
  * HARVEST: we then read just those ~2,568 specific tabs from the LIVE site
    (one request per music-game, not a 20k blind sweep) and parse the .ym
    filenames + metadata. Live tabs are current and stay consistent with the
    live /YM/ files -- Wayback snapshots could list since-deleted files.
  * Song URLs point at the still-live https://www.cpc-power.com/YM/<name>.ym
    (verified 200 + -lh5- LHA magic); stsoundplugin depacks them natively, so
    no extraction step is needed. Routed to the "Amstrad CPC" (AMSTRAD) filter.

The zicym enumeration hands us the game fiche id (== the detail `num`) for free,
and the in-game screenshot endpoint is keyed by that same id, so screenshots are
an exact fiche=num lookup -- no fuzzy name search.

  --build        enumerate music fiches (Wayback CDX), read live tabs, write
                 data/cpcpower.txt
                 (+ data/cpcpower_fiche.tsv mapping song URL -> game fiche id)
  --screenshots  for each game fiche, probe its in-game screenshot and write
                 data/cpcpower_screenshots.txt (keyed by song URL)
"""

import html
import os
import re
import sys
import time
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

HERE = os.path.dirname(__file__)
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
OUT = os.path.join(DATA, "cpcpower.txt")
OUT_FICHE = os.path.join(DATA, "cpcpower_fiche.tsv")
OUT_SHOTS = os.path.join(DATA, "cpcpower_screenshots.txt")

LIVE = "https://www.cpc-power.com/YM/"
# CDX enumerates which fiches have a zicym tab (the expensive discovery, done
# once & cheaply). We then read each of those specific tabs from the LIVE site
# -- targeted (one request per music-game, not a 20k blind crawl), current, and
# guaranteed consistent with the live /YM/ files we point playback URLs at.
CDX = ("http://web.archive.org/cdx/search/cdx?url=cpc-power.com&matchType=domain"
       "&filter=original:.*onglet=zicym.*&filter=statuscode:200"
       "&fl=timestamp,original&collapse=urlkey&output=text&limit=100000")
TAB = "https://www.cpc-power.com/index.php?page=detail&onglet=zicym&num=%s"
# In-game screenshot, keyed by the game fiche id (== detail num). image/png when
# present, 0-byte text/html when the game has none.
SHOT = ("https://www.cpc-power.com/extra_lire_fichier.php?"
        "extra=cpcold&fiche=%s&slot=2&part=A&type=.png")

UA = {"User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36"}
LISTE = re.compile(r'liste_musique\[\d+\]\s*=\s*"([^"]+)"')
NUM = re.compile(r"[?&]num=(\d+)")
WORKERS = 4            # kept busy to hide latency; PACE caps the actual rate
PACE = 0.3             # min seconds between live requests (global, polite)
CACHE = os.path.join(os.environ.get("TMPDIR", "/tmp"), "cpcpower_zicym_live")

# Parenthetical tags that are NOT a composer name.
NON_COMPOSER = re.compile(
    r"^(public domain|\d{2,4}|\d{2}xx|19xx|20xx|"
    r"st-?128 module|st-?module ?128|st module|starkos|arkos\w*|"
    r"protracker.*|soundtracker.*|digitracker.*|basic|mast|unknown|"
    r"\?+|cpc|amstrad)$", re.I)


import threading
_pace_lock = threading.Lock()
_last = [0.0]


def _throttle():
    """Global rate limiter so we stay under Wayback's per-IP cap."""
    with _pace_lock:
        wait = PACE - (time.time() - _last[0])
        if wait > 0:
            time.sleep(wait)
        _last[0] = time.time()


def get(url, timeout=60):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode("utf-8", "replace")


def parse(name):
    """(title, composer) from '<Game> (...)(...)....ym'."""
    base = re.sub(r"\.ym5?$", "", name, flags=re.I)
    parens = re.findall(r"\(([^()]*)\)", base)
    title = re.sub(r"\s*\(.*$", "", base).strip() or base
    composer = ""
    for p in reversed(parens):
        p = p.strip()
        if p and not NON_COMPOSER.match(p):
            composer = p
            break
    return title, composer


def clean(s):
    return s.replace("\t", " ").replace("\n", " ").strip()


# ---------------------------------------------------------------------------
def zicym_map():
    """{num: (timestamp, original_url)} for every Wayback-archived zicym tab."""
    best = {}
    for line in get(CDX, timeout=180).splitlines():
        p = line.split()
        if len(p) < 2:
            continue
        ts, url = p[0], p[1]
        m = NUM.search(url)
        if not m:
            continue
        num = m.group(1)
        # keep the earliest capture per game (they are stable content anyway)
        if num not in best or ts < best[num][0]:
            best[num] = (ts, url)
    return best


def fetch_tab(num):
    """Live zicym tab HTML, from disk cache or cpc-power (retry w/ backoff)."""
    cf = os.path.join(CACHE, num + ".html")
    if os.path.exists(cf):
        with open(cf, encoding="utf-8") as f:
            return f.read()
    last = None
    for attempt in range(5):
        _throttle()
        try:
            h = get(TAB % num)
            os.makedirs(CACHE, exist_ok=True)
            with open(cf, "w", encoding="utf-8") as f:
                f.write(h)
            return h
        except Exception as e:
            last = e
            time.sleep(2 * (attempt + 1))                    # back off if throttled
    raise last


def ym_files(num):
    """[.ym filename, ...] parsed from a live zicym tab (or [])."""
    try:
        h = fetch_tab(num)
    except Exception as e:
        sys.stderr.write(f"\n[{num}] fetch failed: {e}\n")
        return None                             # None = unfetched (retryable)
    out = []
    for raw in LISTE.findall(h):
        # entries look like "./YM/<name>.ym" -- keep .ym/.ym5 only (skip mp3/wav)
        raw = html.unescape(raw).replace("\\'", "'").replace("\\\\", "\\")
        m = re.search(r"/YM/(.+\.ym5?)$", raw, re.I)
        if m:
            out.append(m.group(1))
    return out


def build():
    nums = sorted(zicym_map(), key=int)
    sys.stderr.write(f"{len(nums)} music fiches; fetching live tabs "
                     f"(cache={CACHE})\n")
    # num -> [filenames]; None result = unfetched, retried in the next round
    games, pending = {}, set(nums)
    for rnd in range(1, 9):
        if not pending:
            break
        failed = set()
        done = 0
        with ThreadPoolExecutor(max_workers=WORKERS) as ex:
            futs = {ex.submit(ym_files, n): n for n in pending}
            for f in as_completed(futs):
                n = futs[f]
                files = f.result()
                if files is None:
                    failed.add(n)
                elif files:
                    games[n] = files
                done += 1
                if done % 50 == 0:
                    sys.stderr.write(f"\r  round {rnd}: {done}/{len(pending)}, "
                                     f"{sum(len(v) for v in games.values())} tunes")
                    sys.stderr.flush()
        sys.stderr.write(f"\n  round {rnd}: {len(failed)} still failed\n")
        pending = failed
        if pending:
            time.sleep(20)                      # cool-off before the next round
    if pending:
        sys.stderr.write(f"WARNING: {len(pending)} tabs unfetched after retries\n")

    rows, fiche_rows, seen = [], [], set()
    for num in sorted(games, key=int):
        for name in games[num]:
            if name in seen:                    # same .ym listed on >1 fiche
                continue
            seen.add(name)
            title, composer = parse(name)
            url = LIVE + urllib.parse.quote(name)
            rows.append("\t".join([clean(title), clean(composer),
                                   "Amstrad CPC", url, "ym"]))
            fiche_rows.append(url + "\t" + num)

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    with open(OUT_FICHE, "w", encoding="utf-8") as f:
        f.write("\n".join(fiche_rows) + "\n")
    print(f"wrote {len(rows)} tunes from {len(games)} games -> {OUT}")
    print(f"wrote fiche map -> {OUT_FICHE}")


# ---------------------------------------------------------------------------
def shot_is_real(fid):
    """True if the screenshot endpoint serves an actual image (not 0-byte html)."""
    try:
        req = urllib.request.Request(SHOT % fid, headers=UA)
        with urllib.request.urlopen(req, timeout=60) as r:
            return r.headers.get_content_maintype() == "image" and bool(r.read(64))
    except Exception:
        return False


def screenshots():
    # url -> fiche id, grouped so each game is probed once
    url_num, games = {}, {}
    with open(OUT_FICHE, encoding="utf-8") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) == 2:
                url_num[c[0]] = c[1]
                games.setdefault(c[1], []).append(c[0])
    sys.stderr.write(f"{len(games)} distinct games\n")

    real = {}
    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futs = {ex.submit(shot_is_real, n): n for n in games}
        done = 0
        for f in as_completed(futs):
            real[futs[f]] = f.result()
            done += 1
            if done % 50 == 0:
                sys.stderr.write(f"\r  {done}/{len(games)} probed, "
                                 f"{sum(real.values())} with shots")
                sys.stderr.flush()
    sys.stderr.write("\n")

    rows, matched = [], 0
    for num in sorted(games, key=int):
        if real.get(num):
            matched += 1
            for u in games[num]:
                rows.append(u + "\t" + (SHOT % num))
    with open(OUT_SHOTS, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} screenshots for {matched}/{len(games)} games "
          f"-> {OUT_SHOTS}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
