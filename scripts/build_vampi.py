#!/usr/bin/env python3
"""Build chipmachine/data/vampi.txt from Vampi's MDX Database (mdx.vampi.tech).

Sharp X68000 MDX chiptunes (YM2151 OPM + OKIM6258 ADPCM). Vampi has 9321 MDX files
vs modland's 7449, so we onboard ONLY the genuinely-new ones (user choice):
best-effort dedup by (basename, size) against modland's MDX -- both come from the
same rips, so a matching original filename + exact byte size is a confident dup.
(Descriptively-renamed modland files can't match and may slip through as new; md5
dedup isn't possible -- allmods.txt has size but no md5.)

API (custom AngularJS backend, leaks SQL but works):
  GET api/mdx_files?fields[]=id&fields[]=filename&fields[]=title&fields[]=md5
      &fields[]=size&limit=N&skip=M     -> {results:[...], total:"9321"}
Download: https://mdx.vampi.tech/data/<filename>  (verified; the .mdx binary).
The filename embeds metadata: "<Game> (<year>)(<company>)(<composer>)/<file>.MDX".

  --build        page the API, dedup vs modland, write vampi.txt
  --screenshots  match each game on Wikipedia, harvest the infobox cover/flyer,
                 write chipmachine/data/vampi_screenshots.txt (keyed by song URL).
                 Optional: --limit N (sample the first N distinct games).
"""

import collections
import difflib
import html
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

API = "https://mdx.vampi.tech/api/mdx_files"
DATA = "https://mdx.vampi.tech/data/"
ALLMODS = os.path.join(os.path.dirname(__file__), "..", "data",
                       "allmods.txt")
OUT = os.path.join(os.path.dirname(__file__), "..", "data",
                   "vampi.txt")
UA = {"User-Agent": "Mozilla/5.0 chipmachine-vampi/1.0"}
PAGE = 1000


def modland_mdx_keys():
    """(basename_lower, size) for every modland MDX file. allmods line = size\\tpath."""
    keys = set()
    with open(ALLMODS, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 2:
                continue
            size, path = parts[0], parts[1]
            if not path.startswith("MDX/"):
                continue
            base = path.split("/")[-1].lower()
            if size.isdigit():
                keys.add((base, int(size)))
    return keys


def fetch_page(skip):
    fields = "".join("fields%5B%5D=" + f + "&"
                     for f in ("id", "filename", "title", "md5", "size"))
    url = f"{API}?{fields}limit={PAGE}&skip={skip}"
    req = urllib.request.Request(url, headers=UA)
    import json
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.load(r)


def clean(s):
    # MDX titles carry raw header padding incl. NUL/control bytes (would truncate
    # in SQLite and garble the display); strip control chars, collapse whitespace.
    s = re.sub(r"[\x00-\x1f\x7f]", " ", s or "")
    return re.sub(r"\s+", " ", s).strip()


def parse_meta(filename):
    """game, composer from '<Game> (<year>)(<company>)(<composer>)/<file>.MDX'."""
    d = filename.split("/")[0]
    game = re.sub(r"\s*\(.*$", "", d).strip() or d
    parens = re.findall(r"\(([^()]*)\)", d)
    composer = parens[-1].strip() if parens else ""
    if re.fullmatch(r"\d{4}|\d{2}xx|", composer, re.I):  # a year, not a composer
        composer = ""
    return game, composer


def build():
    dup = modland_mdx_keys()
    sys.stderr.write(f"modland MDX keys: {len(dup)}\n")
    rows, skipped, skip = [], 0, 0
    cats = collections.Counter()
    while True:
        d = fetch_page(skip)
        results = d.get("results", [])
        # NB: the API's "total" is unreliable (returns the page size, not
        # FOUND_ROWS), so page until a short/empty page instead.
        if not results:
            break
        for r in results:
            fn = r.get("filename") or ""
            if not fn or "\t" in fn:
                continue
            base = fn.split("/")[-1].lower()
            size = r.get("size", "")
            key = (base, int(size)) if str(size).isdigit() else None
            if key in dup:
                skipped += 1
                continue
            game, composer = parse_meta(fn)
            composer = clean(composer)
            title = clean(r.get("title")) or clean(game) or base
            path = DATA + urllib.parse.quote(fn)
            rows.append("\t".join([title, composer, "MDX", path, "mdx"]))
            cats[game] += 1
        skip += PAGE
        sys.stderr.write(f"\rfetched={skip}  kept={len(rows)} dup={skipped}")
        sys.stderr.flush()
        if len(results) < PAGE:
            break
        time.sleep(0.4)
    sys.stderr.write("\n")
    out = os.path.normpath(OUT)
    with open(out, "w") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} net-new rows ({skipped} modland dups skipped) -> {out}")
    print(f"distinct games: {len(cats)}")


# ---------------------------------------------------------------------------
# Screenshots. vampi.tech has no images, MobyGames is Cloudflare-walled, and the
# Wikipedia pageimages API omits non-free covers -- but the article HTML still
# embeds the infobox cover/flyer, served from upload.wikimedia.org. Crucially a
# game's box/flyer is NON-FREE, so it lives under /wikipedia/en/ (local upload),
# whereas icons/flags/rating-logos are free Commons files under
# /wikipedia/commons/. So "first infobox <img> under /wikipedia/en/" is both the
# cover AND a strong is-this-a-real-game-article filter. Combined with a title
# similarity guard this matches the famous (mostly western/arcade) games while
# skipping JP-only titles with no EN article.
OUT_SHOTS = os.path.join(os.path.dirname(__file__), "..", "data",
                         "vampi_screenshots.txt")
WP_API = "https://en.wikipedia.org/w/api.php?"
WP_HTML = "https://en.wikipedia.org/api/rest_v1/page/html/"
WP_UA = {"User-Agent": "chipmachine-vampi-shots/1.0 (mihailod@me.com)"}
GAME_MARKER = re.compile(
    r"Developer|Publisher|Platform|Genre|Mode\(s\)|Composer|Arcade system",
    re.I)


def _norm(s):
    return re.sub(r"[^a-z0-9]", "", html.unescape(s or "").lower())


def base_game(d):
    """Game name from a vampi dir: drop the ' (year)(...)' tail and '[tags]'."""
    return re.sub(r"\s*[\(\[].*$", "", d).strip()


def _wp_get(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=WP_UA),
                                timeout=30) as r:
        return r.read().decode("utf-8", "replace")


def _wp_search_hits(query):
    try:
        d = json.loads(_wp_get(WP_API + urllib.parse.urlencode({
            "action": "query", "format": "json", "list": "search",
            "srsearch": query, "srlimit": 5})))
        return [h["title"] for h in d.get("query", {}).get("search", [])]
    except Exception:
        return []


def wp_search(game):
    """Best video-game-ish article title for a game name, or None.

    Searches both the bare name and "<name> video game": a bare query is often
    dominated by the year/character/film of the same name and never surfaces the
    game article, while the qualified query brings it in. Similarity ranking +
    the cover guard keep precision."""
    titles = _wp_search_hits(game)
    for t in _wp_search_hits(game + " video game"):
        if t not in titles:
            titles.append(t)
    n = _norm(game)
    best, score = None, 0.0
    for t in titles:
        nt = _norm(re.sub(r"\s*\(.*\)$", "", t))   # drop "(video game)" qualifier
        if not nt:
            continue
        lr = min(len(n), len(nt)) / max(len(n), len(nt))
        if nt == n:
            s = 1.0
        elif (n in nt or nt in n) and lr >= 0.7:
            # substring only counts as a strong match when the lengths are close
            # -- otherwise "Bullet" matches "100 Bullets", "Syndrome" matches
            # "Alien Syndrome", etc.
            s = 0.85 + 0.1 * lr
        else:
            s = difflib.SequenceMatcher(None, n, nt).ratio()
        # A bare title ("1942" = the year, "Jax" = a character) ties the game
        # article ("1942 (video game)"); a small nudge breaks the tie toward the
        # game without overriding a better full match. The /wikipedia/en/ cover
        # guard still rejects non-games.
        if re.search(r"\((?:video |arcade )?game\)$", t, re.I):
            s += 0.05
        if s > score:
            best, score = t, s
    return best if score >= 0.82 else None


def wp_cover(title):
    """The infobox cover URL (non-free /wikipedia/en/ image) for an article."""
    try:
        h = _wp_get(WP_HTML + urllib.parse.quote(title.replace(" ", "_")))
    except Exception:
        return None
    m = re.search(r'<table class="infobox.*?</table>', h, re.S)
    if not m:
        return None
    box = m.group(0)
    if not GAME_MARKER.search(box):          # not a game infobox -> reject
        return None
    im = re.search(r'<img[^>]+src="(//upload\.wikimedia\.org/wikipedia/en/[^"]+)"',
                   box)
    return ("https:" + im.group(1)) if im else None


def screenshots():
    limit = None
    if "--limit" in sys.argv:
        limit = int(sys.argv[sys.argv.index("--limit") + 1])

    games = collections.OrderedDict()   # base game -> [song urls]
    with open(os.path.normpath(OUT), encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 4 or not c[3]:
                continue
            path = urllib.parse.unquote(urllib.parse.urlparse(c[3]).path)
            d = path.split("/data/", 1)[-1].split("/")[0]
            games.setdefault(base_game(d), []).append(c[3])
    names = list(games)
    if limit:
        names = names[:limit]
    sys.stderr.write(f"{len(names)} distinct games to look up\n")

    rows, matched = [], 0
    for i, g in enumerate(names, 1):
        cover = None
        t = wp_search(g)
        if t:
            cover = wp_cover(t)
        sys.stderr.write(f"\r[{i}/{len(names)}] {g[:38]:38} -> "
                         f"{('OK ' + t[:30]) if cover else ('miss' if not t else 'no-cover ' + t[:22])}")
        sys.stderr.flush()
        if cover:
            for u in games[g]:
                rows.append(u + "\t" + cover)
            matched += 1
        time.sleep(0.25)
    sys.stderr.write("\n")
    out = os.path.normpath(OUT_SHOTS)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} screenshots for {matched}/{len(names)} games -> {out}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
