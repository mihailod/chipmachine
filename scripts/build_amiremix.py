#!/usr/bin/env python3
"""Build chipmachine/data/amigaremix_screenshots.txt for AmigaRemix (amiremix).

amiremix is Amiga game/demo music REMIXES (MP3s). data/amiremix.txt rows are
    <id>\t<mp3-url>\t<title>\t<arranger>
(song_template "no path title composer" -> path = col2, and db.lua `source =
http://amigaremix.com/listen/` is stripped at index time, so the stored song path
is "<id>/<file>.mp3" -- that is the screenshot key). The data carries NO link to
the original module/game (unlike rko's SID path), and amigaremix.com is a JS SPA
with no scrapeable cover, so we match the GAME NAME (parsed from the title) to its
Wikipedia article and borrow the infobox box-art -- the same approach as
build_vampi, but with a STRICTER filter: require an actual video-game infobox
(class `ib-video-game`), else a book/film/song of the same name (which also has a
"Publisher"/"Genre" row) slips through (e.g. "Hardwired" the novel).

Yield ~40% at high precision; the residual imperfection is series-number collapse
(Turrican 2/3 -> Turrican box art), which is thematically fine.

  python3 chipmachine/scripts/build_amiremix.py --screenshots
"""

import os
import re
import sys
import time
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build_vampi as bv  # reuse wp_search / _wp_get / WP_HTML

DATA = os.path.normpath(os.path.join(HERE, "..", "data"))
SRC = os.path.join(DATA, "amiremix.txt")
OUT = os.path.join(DATA, "amigaremix_screenshots.txt")
SOURCE = "http://amigaremix.com/listen/"   # db.lua strips this from the path

# Trailing descriptor words that are the tune/version, not the game name.
DESC = re.compile(
    r"\b(loader|title|theme|in-?game|sub-?tune\d*|subtune|level|intro|menu|"
    r"hi-?score|high score|main|ending|credits|remix|rmx|mix|edit|version|"
    r"tune|part|anthem|medley)\b.*$", re.I)


def game_of(title):
    """Best-effort game name from a remix title."""
    t = title.split(" - ")[0]          # game sits before the first " - "
    t = re.sub(r"\s*\(.*$", "", t)     # drop a "(... mix)" tail
    t = DESC.sub("", t)                # strip trailing tune/version words
    t = re.sub(r"\s+\d{4}$", "", t)    # trailing year (e.g. 2003)
    return t.strip(" -") or title


def wp_cover_game(title):
    """Infobox box-art URL, but ONLY from a video-game infobox (ib-video-game).

    Stricter than build_vampi.wp_cover (which accepts any infobox with a
    game-ish row) so novels/films/songs of the same name are rejected."""
    try:
        h = bv._wp_get(bv.WP_HTML + urllib.parse.quote(title.replace(" ", "_")))
    except Exception:
        return None
    m = re.search(r'<table class="infobox[^"]*ib-video-game[^"]*".*?</table>',
                  h, re.S)
    if not m:
        return None
    im = re.search(
        r'<img[^>]+src="(//upload\.wikimedia\.org/wikipedia/en/[^"]+)"',
        m.group(0))
    return ("https:" + im.group(1)) if im else None


def screenshots():
    # group song keys by game name (many remixes share a game -> one WP lookup)
    games = {}   # game_name -> [song-key, ...]
    with open(SRC, encoding="utf-8", errors="replace") as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 3 or not c[1]:
                continue
            key = c[1]
            if key.startswith(SOURCE):
                key = key[len(SOURCE):]
            games.setdefault(game_of(c[2]), []).append(key)
    names = list(games)
    sys.stderr.write(f"{len(names)} distinct game names\n")

    rows, matched = [], 0
    for i, g in enumerate(names, 1):
        art = bv.wp_search(g)
        cover = wp_cover_game(art) if art else None
        sys.stderr.write(f"\r[{i}/{len(names)}] {g[:34]:34} -> "
                         f"{('OK ' + art[:26]) if cover else 'miss':30}")
        sys.stderr.flush()
        if cover:
            for k in games[g]:
                rows.append(k + "\t" + cover)
            matched += 1
        time.sleep(0.2)
    sys.stderr.write("\n")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    total = sum(len(v) for v in games.values())
    print(f"wrote {len(rows)}/{total} screenshots for {matched}/{len(names)} "
          f"games -> {OUT}")


if __name__ == "__main__":
    if "--screenshots" in sys.argv:
        screenshots()
    else:
        print(__doc__)
