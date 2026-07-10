#!/usr/bin/env python3
"""keygenmusic (keygenmusic.tk / .org) onboarding for ChipMachineAS.

The keygen/cracktro music scene -- a genuinely distinct subculture (tunes written
to accompany software keygens & cracks) not otherwise represented in the DB. The
original site is dead (~2022); the full collection is preserved on the Internet
Archive as the item `keygen-music-2020-03-pack` (the "2020-03 MusicPack",
~5.5k files in ONE zip). We onboard the audio tracks that our plugins decode.

Access: NO robots block on archive.org for this. The whole file listing comes from
ONE request to archive.org's `view_archive.php` (reached via the stable
`/download/<item>/<zip>/` redirect), which returns an HTML table of every inner
file with its path AND byte size. Each track is then a STATIC, directly-fetchable
archive.org zip-extraction URL:
    https://archive.org/download/keygen-music-2020-03-pack/keygen-music-2020-03-pack.zip/<inner-path>
(the same archive.org-side extraction mechanism VGMRips uses; the app's
RemoteLoader follows the 302 to the storage node and plays the module directly).

Structure: `.../KEYGENMUSiC MusicPack/<sub>/<Artist> - <title>.<ext>` -> composer
= artist (before " - "), title = the rest (usually the cracked-software name).
Playability + platform are driven by the REAL extension.

Dedup: keygen filenames are SOFTWARE-named (not musical titles), so title dedup vs
modarchive is a no-op; and modland hosts almost no keygen music (~49 hits). We do a
high-precision, zero-false-positive dedup vs modland by exact byte SIZE combined
with a normalized name match ((artist,size) or (stem,size) from allmods.txt, which
carries size). Net-new is ~the whole set.

No screenshots (cracktro/keygen scene has no game art; matches the artist-music
screenshot policy -- like AMP / chipmusic).

Usage:
    build_keygenmusic.py --build     # fetch listing -> data/keygenmusic.txt
"""
import html, os, re, sys, urllib.parse, urllib.request

ITEM = "keygen-music-2020-03-pack"
ZIP = ITEM + ".zip"
LISTING = "https://archive.org/download/%s/%s/" % (ITEM, ZIP)   # 302 -> view_archive
BASE = "https://archive.org/download/%s/%s/" % (ITEM, ZIP)      # + inner path
UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120 Safari/537.36")
HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "data"))

# Playable extension -> display/filter format string (lowercase-matched to
# format_map).  Presence == "we decode it".  midi and non-audio junk are skipped.
EXT_FORMAT = {
    "xm": "XM", "mod": "MOD", "it": "IT", "s3m": "S3M", "mtm": "MOD",
    "mo3": "MOD",   # MP3/OGG-compressed module -> OpenMPT
    "sid": "Commodore 64", "nsf": "Nintendo Sound Format", "sap": "Atari 8Bit",
    "spc": "Super Nintendo", "ahx": "Amiga", "hvl": "Amiga",
    "sc68": "Atari ST", "ym": "Atari ST",
    "v2m": "V2", "v2": "V2",
    "d00": "AdLib", "rad": "AdLib", "hsc": "AdLib", "amd": "AdLib",
    "fc13": "Amiga", "fc14": "Amiga", "fc": "Amiga", "bp": "Amiga",
    "mp3": "MP3", "ogg": "OGG", "flac": "MP3", "wav": "MP3",
}


def _get(url):
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=90) as r:
        return r.read().decode("utf-8", "replace")


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def load_listing():
    """[(inner_path, size)] from the view_archive HTML."""
    doc = _get(LISTING)
    out = []
    # Rows: href=".../<zip>/<encoded path>">...<td id="size">NNN
    pat = re.compile(
        r'href="//archive\.org/download/%s/%s/([^"]+)"'
        r'.*?<td id="size">(\d+)' % (re.escape(ITEM), re.escape(ZIP)),
        re.S)
    for m in pat.finditer(doc):
        path = html.unescape(urllib.parse.unquote(m.group(1)))
        out.append((path, int(m.group(2))))
    return out


def modland_dedup_index():
    """From allmods.txt (`<size>\\t<Format>/<Composer>/.../<file>`): sets of
    (composer_norm, size) and (title_stem_norm, size) for exact-dup detection."""
    by_comp = set(); by_stem = set()
    p = os.path.join(DATA, "allmods.txt")
    if not os.path.exists(p):
        sys.stderr.write("WARN: no allmods.txt -> no dedup\n")
        return by_comp, by_stem
    with open(p, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if "\t" not in line:
                continue
            size_s, path = line.split("\t", 1)
            try:
                size = int(size_s)
            except ValueError:
                continue
            parts = path.split("/")
            if len(parts) >= 3:
                by_comp.add((norm(parts[1]), size))
            stem = os.path.splitext(parts[-1])[0]
            by_stem.add((norm(stem), size))
    return by_comp, by_stem


def modarchive_names():
    """Normalized filename-stems + titles from modarchive.txt (col1 =
    `<filename>//<title>`). keygen titles are software names, so this catches
    only the rare tune re-uploaded to modarchive under the same name."""
    names = set()
    p = os.path.join(DATA, "modarchive.txt")
    if not os.path.exists(p):
        return names
    with open(p, encoding="utf-8", errors="replace") as f:
        for line in f:
            col1 = line.split("\t", 1)[0]
            fn, _, title = col1.partition("//")
            names.add(norm(os.path.splitext(fn)[0]))
            if title:
                names.add(norm(title))
    names.discard("")
    return names


def build():
    from collections import Counter
    files = load_listing()
    sys.stderr.write("listing: %d inner files\n" % len(files))
    by_comp, by_stem = modland_dedup_index()
    ma_names = modarchive_names()

    rows = []; kept = Counter(); skipped_ext = Counter(); dups = 0
    internal = 0; seen = set()
    for path, size in files:
        base = path.split("/")[-1]
        ext = os.path.splitext(base)[1].lower().lstrip(".")
        if ext not in EXT_FORMAT:
            skipped_ext[ext or "(none)"] += 1
            continue
        stem = os.path.splitext(base)[0]
        # "<Artist> - <title>"
        if " - " in stem:
            artist, title = stem.split(" - ", 1)
        else:
            artist, title = "", stem
        artist = artist.strip(); title = title.strip()
        # internal dedup: the same tune re-filed under multiple subfolders.
        key = (norm(stem), size)
        if key in seen:
            internal += 1
            continue
        seen.add(key)
        # dedup vs modland (exact size + normalized name); zero false positives.
        if (artist and (norm(artist), size) in by_comp) or \
           (norm(stem), size) in by_stem or (norm(title), size) in by_stem:
            dups += 1
            continue
        # dedup vs modarchive (same filename-stem or title uploaded there too).
        if norm(stem) in ma_names or (title and norm(title) in ma_names):
            dups += 1
            continue
        url = BASE + urllib.parse.quote(path)
        fmt = EXT_FORMAT[ext]
        rows.append((title or base, artist, fmt, url, ext))
        kept[fmt] += 1

    out = os.path.join(DATA, "keygenmusic.txt")
    with open(out, "w", encoding="utf-8") as f:
        for r in rows:
            f.write("\t".join(r) + "\n")
    print("wrote %d songs -> %s (modland dups %d, internal dups %d)"
          % (len(rows), out, dups, internal))
    print("\n== kept by format ==")
    for k, v in kept.most_common():
        print("%6d  %s" % (v, k))
    print("\n== skipped ext (no decoder) ==")
    for k, v in skipped_ext.most_common(15):
        print("%6d  .%s" % (v, k))


if __name__ == "__main__":
    if (sys.argv[1:] or [""])[0] == "--build":
        build()
    else:
        print(__doc__); sys.exit(1)
