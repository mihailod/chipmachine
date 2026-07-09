#!/usr/bin/env python3
"""Build chipmachine/data/zophar.txt -- Zophar's Domain console gamerips.

PILOT SCOPE: Sega Genesis / Mega Drive (VGM). This is the genuinely net-new part
of Zophar -- modland already has SNES/GB/PS/N64/Saturn/NES/Dreamcast (tens of
thousands), but has NO VGM and NO arcade. modland's only Genesis music is 265
"Megadrive GYM" tunes, so we dedup against those and keep the rest.

Each game is a directory under https://fi.zophar.net/soundfiles/<platform>/<slug>/
holding "<Title> (EMU).zophar.zip" -- a zip of per-track .vgm files. The download
URL is reconstructable from the (slug, title) pairs on the platform listing pages
(verified), so enumeration is one listing fetch per page, no per-game requests.
The host extracts the zip and plays the .vgm tracks as subsongs (see the
MusicPlayerList zip-game handler).

  --build        page the Genesis listing, dedup vs modland GYM, write zophar.txt
  --screenshots  page the listing, harvest each game's soundcover image, write
                 zophar_screenshots.txt keyed by the song URL in zophar.txt
"""

import html
import os
import re
import sys
import time
import urllib.parse
import urllib.request

PLATFORM = "sega-mega-drive-genesis"
LISTING = "https://www.zophar.net/music/" + PLATFORM
CDN = "https://fi.zophar.net/soundfiles/" + PLATFORM + "/"
ALLMODS = os.path.join(os.path.dirname(__file__), "..", "data",
                       "allmods.txt")
OUT = os.path.join(os.path.dirname(__file__), "..", "data",
                   "zophar.txt")
OUT_SHOTS = os.path.join(os.path.dirname(__file__), "..", "data",
                         "zophar_screenshots.txt")
UA = {"User-Agent": "Mozilla/5.0 chipmachine-zophar/1.0"}
SLEEP = 0.6

GAME_RE = re.compile(
    r'<a href="/music/' + re.escape(PLATFORM) + r'/([^"/]+)">([^<]+)</a>')


def norm(s):
    """Normalize a game title for dedup matching (lowercase, alnum only)."""
    return re.sub(r"[^a-z0-9]", "", html.unescape(s).lower())


# modland ALSO has these systems in VGM/VGZ under "Video Game Music/<system>/"
# (10961 Sega Megadrive files = 621 games, + Mega CD / 32X / GYM). Dedup against
# all of them so we keep only games modland lacks.
MODLAND_DUP_PREFIXES = (
    "Video Game Music/Sega Megadrive/",
    "Video Game Music/Sega Mega CD/",
    "Video Game Music/Sega 32X/",
    "Megadrive GYM/",
    "Megadrive CYM/",
)


def modland_dups():
    """Normalized Sega-Megadrive game names already in modland (any of the dirs
    above). Path layout: Video Game Music/<system>/<composer>/<game>/<track>."""
    names = set()
    try:
        with open(ALLMODS, encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.split("\t", 1)[-1].strip()
                if not p.startswith(MODLAND_DUP_PREFIXES):
                    continue
                parts = p.split("/")
                # ".../Sega Megadrive/<composer>/<game>/<track>"  -> game = parts[3]
                # "Megadrive GYM/<composer>/<game>.gym"           -> game = parts[2]
                game = parts[3] if p.startswith("Video Game Music/") and len(parts) >= 5 \
                    else re.sub(r"\.[a-z0-9]+$", "", parts[-1], flags=re.I)
                names.add(norm(game))
    except FileNotFoundError:
        pass
    return names


def fetch(url):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read().decode("utf-8", "replace")


def build():
    dup = modland_dups()
    rows, skipped, page = [], 0, 1
    seen = set()
    while True:
        url = LISTING if page == 1 else f"{LISTING}?page={page}"
        try:
            html_text = fetch(url)
        except Exception as e:
            sys.stderr.write(f"\npage {page} failed: {e}\n")
            break
        games = GAME_RE.findall(html_text)
        if not games:
            break
        new_on_page = 0
        for slug, title in games:
            if slug in seen:
                continue
            seen.add(slug)
            new_on_page += 1
            title = html.unescape(title).strip()
            # match modland by the base name (drop the (SCD)/(32X) hardware tag)
            base = re.sub(r"\s*\((SCD|32X|Pico|MCD)\)\s*$", "", title)
            if norm(base) in dup:
                skipped += 1
                continue
            fname = urllib.parse.quote(f"{title} (EMU).zophar.zip")
            path = CDN + urllib.parse.quote(slug) + "/" + fname
            # song_template = "title composer format path ext"
            rows.append("\t".join([title, "", "Sega Genesis", path, "vgm"]))
        sys.stderr.write(f"\rpage {page}: {len(rows)} kept, {skipped} dup")
        sys.stderr.flush()
        if new_on_page == 0:
            break
        page += 1
        time.sleep(SLEEP)
    sys.stderr.write("\n")
    out = os.path.normpath(OUT)
    with open(out, "w") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {len(rows)} rows ({skipped} modland-GYM dups skipped) -> {out}")


# Each game's soundcover lives in a listing-row image cell:
#   <td class="image"><a href="/music/<platform>/<slug>"><img src="...soundcovers/
#   <platform>/thumbs_small/<file>.jpg" ...></a></td>
# Games without a cover render an empty cell (no <img>), so they just don't match.
COVER_RE = re.compile(
    r'<td class="image"><a href="/music/' + re.escape(PLATFORM) +
    r'/([^"/]+)"><img src="(https://fi\.zophar\.net/soundcovers/[^"]+)"')


def slug_of(url):
    """The <slug> segment of a fi.zophar.net soundfiles song URL, or ''.

    Unquoted so alternate-title slugs (stored URL-encoded in zophar.txt as
    ...%5Balt%5D) match the listing hrefs (raw brackets)."""
    parts = urllib.parse.urlparse(url).path.split("/")
    try:
        return urllib.parse.unquote(parts[parts.index(PLATFORM) + 1])
    except (ValueError, IndexError):
        return ""


def screenshots():
    # 1) page the listing, build slug -> cover URL (upgrade the small thumb to the
    #    large one -- same filename, just a different sibling dir on the CDN).
    slug_cover, page, seen = {}, 1, set()
    while True:
        url = LISTING if page == 1 else f"{LISTING}?page={page}"
        try:
            html_text = fetch(url)
        except Exception as e:
            sys.stderr.write(f"\npage {page} failed: {e}\n")
            break
        found = COVER_RE.findall(html_text)
        links = GAME_RE.findall(html_text)
        new = [s for s, _ in links if s not in seen]
        for s, _ in links:
            seen.add(s)
        for slug, cover in found:
            slug_cover.setdefault(
                urllib.parse.unquote(slug),
                cover.replace("/thumbs_small/", "/thumbs_large/"))
        sys.stderr.write(f"\rpage {page}: {len(slug_cover)} covers")
        sys.stderr.flush()
        if not links or not new:
            break
        page += 1
        time.sleep(SLEEP)
    sys.stderr.write("\n")

    # 2) key each zophar.txt song URL to its game's cover.
    rows, matched, total = [], 0, 0
    with open(os.path.normpath(OUT), encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 4 or not c[3]:
                continue
            total += 1
            cover = slug_cover.get(slug_of(c[3]))
            if cover:
                rows.append(c[3] + "\t" + cover)
                matched += 1
    out = os.path.normpath(OUT_SHOTS)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print(f"wrote {matched}/{total} screenshots -> {out}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    elif "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
