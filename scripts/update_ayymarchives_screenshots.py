#!/usr/bin/env python3
"""Build chipmachine/data/ayymarchives_screenshots.txt.

Two matchers, because the collection is two different corpora:

  * Atari ST .ym (YM Archive v5 / YM / faveym / VtxYmEtc) -> Atari Mania
    screenshots, the same source [[sndh]] uses. Names come from the Wayback CDX
    index of atarimania.com/st/screens (~67k files / ~11k distinct games).
  * ZX game/demo rows -> World of Spectrum loading screens via an offline ZXDB
    dump, the same path update_projectay_screenshots.py takes.

Per the screenshot policy only GAME/DEMO rows are eligible. That is the minority
here: ~11.8k of the 14.2k rows sit under Tr_Songs/Authors/ and are artist music,
which ships with no screenshot at all. Eligible = the .ym rows plus the ZX
rows filed under Games/ Demos/ Intros/ Groups/ Magazines/ Parties/ or Nostalgic/
(in those the PARENT FOLDER is the game or production name).

Run:
    python3 update_ayymarchives_screenshots.py --zxdb /path/ZXDB_mysql.sql \\
                                               --am-cdx /path/am_cdx.txt
`--am-cdx` is a plain list of atarimania screenshot URLs; refresh it with
    curl 'http://web.archive.org/cdx/search/cdx?url=atarimania.com/st/screens*\\
          &fl=original&collapse=urlkey&limit=200000&filter=statuscode:200'

Both hosts are served through the Wayback 2id_ mirror. NB Wayback throttles
bursts hard (20 rapid HEADs then connection failures), so verify output paced.
"""
import argparse
import html
import importlib.util
import re
import urllib.parse
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"
SONGS_TXT = DATA / "ayymarchives.txt"
OUT_FILE = DATA / "ayymarchives_screenshots.txt"
REVIEW_FILE = DATA / "ayymarchives_screenshots.review.txt"

# Reuse the ZXDB parser + Wayback prefix + generic denylist from the zxart
# script (which itself builds on update_zxspectrum_screenshots.py).
_spec = importlib.util.spec_from_file_location(
    "zxart_shots", HERE / "update_zxart_screenshots.py")
_za = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_za)
parse_zxdb, WAYBACK = _za.parse_zxdb, _za.WAYBACK
GENERIC_TITLES = _za.GENERIC_TITLES
match_keys, title_words = _za.match_keys, _za.title_words

# NB WAYBACK already embeds the worldofspectrum host (ZXDB stores site-relative
# links), so it is only usable for the ZX branch. Atari Mania URLs come out of
# the CDX index absolute, so they need the bare 2id_ prefix.
WB2ID = "https://web.archive.org/web/2id_/"

ZX_GAME_DIRS = re.compile(
    r"^(Nostalgic/|Tr_Songs/(Games|Demos|Intros|Groups|Magazines|Parties)/)")

# Folder names that are containers, not productions -- matching on them would
# attach one screenshot to a whole grab-bag.
GENERIC_DIRS = {
    "games", "miscgames", "misc", "miscellaneous", "other", "various",
    "demos", "intros", "unknown", "tunes", "music", "songs", "ym",
    "bigdemo", "ymarchivev5", "cybergothsyms", "menu", "menus", "compilation",
}


def norm(s):
    """Lowercase, drop extension and bracket/paren tags, alphanumerics only."""
    s = urllib.parse.unquote(s).lower()
    s = re.sub(r"\.[a-z0-9]{2,5}$", "", s)
    s = re.sub(r"\[[^\]]*\]|\([^)]*\)", " ", s)
    return re.sub(r"[^a-z0-9]+", " ", s).strip()


def toks(s):
    return [t for t in norm(s).split() if t]


# Trailing tokens that only ever index a tune WITHIN a production, so dropping
# them is safe. Sequel markers (2, ii, iii) are deliberately NOT here: dropping
# those turns "Double Dragon II" into "Double Dragon", a different game.
TUNE_WORDS = {
    "music", "tune", "theme", "title", "titles", "intro", "loader", "load",
    "ingame", "game", "menu", "mainmenu", "main", "song", "hiscore", "highscore",
    "end", "ending", "credits", "bonus", "level", "part", "remix", "version",
    "st", "ay", "ym", "mix", "fullscreen", "demo",
}
ROMAN = re.compile(r"^(i{1,3}|iv|v|vi{1,3}|ix|x)$")


def candidate_keys(name):
    """Keys to try, most specific first.

    Exact first, then progressively drop trailing tune-ish words or plain
    numbers -- "Nostalgic-O-Demo Fractals Music" -> "nostalgicodemo". A trailing
    ROMAN numeral or a token that is not tune-ish is never dropped, so sequels
    keep their identity.
    """
    t = toks(name)
    out = []
    if t:
        out.append("".join(t))
    while len(t) > 1:
        last = t[-1]
        if last in TUNE_WORDS or last.isdigit():
            t = t[:-1]
            out.append("".join(t))
        else:
            break
    return out


def build_am_index(cdx_path):
    """norm-name -> screenshot URL, for atarimania /st/screens files.

    Shot files are "<game>[_<publisher>][_<n>].gif"; the publisher tag and the
    shot number are stripped so every shot of a game folds onto one key. The
    LOWEST-numbered shot wins (that is the title screen far more often than not).
    """
    exact = {}
    for line in open(cdx_path, encoding="utf-8", errors="replace"):
        u = line.strip()
        if not u or not re.search(r"\.(gif|png|jpg)$", u, re.I):
            continue
        raw = u.rsplit("/", 1)[-1]
        n = re.sub(r"\s+\d+$", "", norm(raw)).replace(" ", "")
        if not n:
            continue
        prev = exact.get(n)
        if prev is None or len(raw) < len(prev[0]):
            exact[n] = (raw, u)
    return {k: v[1] for k, v in exact.items()}


def am_lookup(index, prefix_index, name):
    """Exact/suffix-trimmed match, then a guarded PREFIX match.

    The prefix pass exists for cases where the SHOT name is longer than the
    song's: "1st Division 1..8" are tunes from "1st Division Manager". It only
    fires when exactly one game starts with the key and the key is long enough
    to be distinctive -- otherwise "Turrican" would silently claim "Turrican 2".
    """
    for k in candidate_keys(name):
        if len(k) >= 4 and k in index:
            return index[k], "exact"
    for k in candidate_keys(name):
        if len(k) < 8:
            continue
        hits = prefix_index.get(k[:8], ())
        cands = {kk for kk in hits if kk.startswith(k)}
        if len(cands) == 1:
            return index[next(iter(cands))], "prefix"
    return None, None


def load_songs():
    rows = []
    for line in SONGS_TXT.read_text(encoding="utf-8",
                                    errors="replace").splitlines():
        c = line.split("\t")
        if len(c) >= 5 and c[3].strip():
            rows.append((c[3].strip(), html.unescape(c[0].strip()), c[4].strip()))
    return rows


def parent_dir(path):
    p = path.split("/")
    return p[-2] if len(p) >= 2 else ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zxdb", required=True, help="ZXDB_mysql.sql (unzipped)")
    ap.add_argument("--am-cdx", required=True, help="atarimania CDX url list")
    args = ap.parse_args()

    am = build_am_index(args.am_cdx)
    prefix_index = defaultdict(list)
    for k in am:
        if len(k) >= 8:
            prefix_index[k[:8]].append(k)
    print(f"Atari Mania distinct games: {len(am)}")

    title2ids, entry_load_link = parse_zxdb(args.zxdb)
    print(f"ZXDB titles with a loading screen: {len(title2ids)}")

    rows = load_songs()
    results, review = {}, []
    stats = defaultdict(int)

    for path, title, ext in rows:
        if ext == "ym":
            folder = parent_dir(path)
            cands = []
            if norm(folder).replace(" ", "") not in GENERIC_DIRS:
                cands.append(("folder", folder))
            cands += [("title", title), ("file", Path(path).stem)]
            for src, c in cands:
                url, how = am_lookup(am, prefix_index, c)
                if url:
                    results[path] = WB2ID + url
                    stats[f"ym:{src}:{how}"] += 1
                    if how == "prefix":
                        review.append((f"ym/{how}", c, path, results[path]))
                    break
            else:
                stats["ym:MISS"] += 1
        elif ZX_GAME_DIRS.match(path):
            # The game/production name is the file's PARENT dir. Do not index a
            # fixed depth: "Nostalgic/<game>/x.vtx" is one level shallower than
            # "Tr_Songs/Games/<game>/x.asc", and hardcoding p[2] silently blanked
            # every Nostalgic row -- the most matchable set in the collection.
            folder = parent_dir(path)
            hit = False
            for src, c in (("folder", folder), ("title", title)):
                if not c or norm(c).replace(" ", "") in GENERIC_DIRS:
                    continue
                k = next((kk for kk in match_keys(c)
                          if len(kk) >= 3 and kk in title2ids
                          and kk not in GENERIC_TITLES), None)
                if k is None:
                    continue
                link = entry_load_link.get(min(title2ids[k]))
                if not link:
                    continue
                results[path] = WAYBACK + urllib.parse.quote(link,
                                                             safe="/().-_")
                stats[f"zx:{src}"] += 1
                if title_words(c) < 2:
                    review.append(("zx/single-word", c, path, results[path]))
                hit = True
                break
            if not hit:
                stats["zx:MISS"] += 1
        else:
            stats["not eligible (artist music)"] += 1

    OUT_FILE.write_text(
        "\n".join(f"{p}\t{u}" for p, u in sorted(results.items())) + "\n",
        encoding="utf-8")
    REVIEW_FILE.write_text(
        "\n".join("\t".join(r) for r in sorted(review)) + "\n",
        encoding="utf-8")
    for k in sorted(stats):
        print(f"  {k:<34} {stats[k]}")
    print(f"\nWrote {OUT_FILE}: {len(results)} song matches "
          f"({100*len(results)/max(1,len(rows)):.1f}% of the collection); "
          f"{len(review)} low-confidence -> {REVIEW_FILE.name}")


if __name__ == "__main__":
    main()
