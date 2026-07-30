#!/usr/bin/env python3
"""Build chipmachine/data/sceneorg.txt from the scene.org file dumps.

The four TSVs in chipmachine/data-notbundled/misc/scene.org/ (mod/xm/it/s3m) list every
tracker module on files.scene.org -- but .mod/.xm/.it/.s3m are the four most
heavily-mirrored formats we ship (modland ~165k + modarchive ~155k of exactly
these). So onboarding all of scene.org would be almost entirely duplicate.

SCOPE (user choice): only the genuinely-net-new slice that modland/modarchive
cover *worst* -- the Spanish demoscene mirror (mirrors/scenesp.org/) and the
party archive (parties/). The amigascne / klosz / hornet mirrors are modland's
home turf and are dropped.

DEDUPE: there is no md5 anywhere (not in the TSV, the files.scene.org/view page,
or the documented API), so dedup is by lowercased basename (.zip stripped) vs
modland (allmods.txt), modarchive (modarchive.txt) and our own demozoo.txt
(which already ships ~27k scene.org *stream* URLs). Basename match is fuzzy both
ways, but it's the only signal short of downloading all ~7k files.

EMIT (song_template "title composer format path ext"):
  title    cleaned basename (no extension)
  composer the party / collection the file sits in (provenance, searchable)
  format   MOD | XM | IT | S3M  -> routes into the existing per-format filters
  path     https://archive.scene.org/pub/<path>  (direct HTTPS 200; the
           files.scene.org/get/ redirector 302s to plain http:// and hangs the
           in-app curl -- same lesson as demozoo v67). source empty.
  ext      module ext, or "zip" for archived modules (host extracts by magic)

  python3 chipmachine/scripts/build_sceneorg.py --build
"""

import os
import re
import sys
import urllib.parse

HERE = os.path.dirname(__file__)
DATA = os.path.join(HERE, "..", "data")
# Scrape input: build-time only -> data-notbundled/ (not copied into the .app).
NOTBUNDLED = os.path.join(HERE, "..", "data-notbundled")
SRC = os.path.join(NOTBUNDLED, "misc", "scene.org")
ALLMODS = os.path.join(DATA, "allmods.txt")
MODARCHIVE = os.path.join(DATA, "modarchive.txt")
DEMOZOO = os.path.join(DATA, "demozoo.txt")
OUT = os.path.join(DATA, "sceneorg.txt")

BASE = "https://archive.scene.org/pub/"
FORMATS = ("mod", "xm", "it", "s3m")
# only these path roots are in scope
SCOPE = re.compile(r"^(parties/|mirrors/scenesp\.org/)")
# instrument/sample siblings and pre-rendered streams -- never standalone songs
SKIP_EXT = (".iti", ".its", ".xms", ".mp3", ".ogg", ".wav", ".flac", ".diz",
            ".nfo", ".txt", ".info")


def base_of(path):
    """Lowercased basename with a trailing .zip removed (the dedup key)."""
    b = path.split("/")[-1].lower()
    if b.endswith(".zip"):
        b = b[:-4]
    return b


def load_existing():
    """Every basename we already mirror, for dedup."""
    seen = set()
    # modland: "<size>\t<path>"
    with open(ALLMODS, encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 2:
                seen.add(base_of(p[1]))
    # modarchive: "<file>//<title>\t..."
    with open(MODARCHIVE, encoding="utf-8", errors="replace") as f:
        for line in f:
            name = line.split("\t", 1)[0].split("//", 1)[0]
            if name:
                seen.add(name.lower())
    # demozoo: full URL in column 4
    with open(DEMOZOO, encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 4 and "://" in p[3]:
                b = urllib.parse.unquote(p[3]).split("?")[0]
                seen.add(base_of(b))
    return seen


# years like 1997 / '98 / 2004 -- a party dir is the segment that names the event
YEAR = re.compile(r"^('?\d{2}|\d{4}|\d{4}-\d{2})$", re.I)
# directory names that carry no provenance value (compo bins, media buckets)
GENERIC = {
    "music", "mods", "mod", "modules", "tunes", "compo", "compos", "compo tunes",
    "chiptunes", "chip", "various", "misc", "stuff", "normal", "fast", "wild",
    "amiga", "pc", "pc artists a-m", "pc artists n-z", "amiga artists",
    "pc artists", "artists", "pc demos", "amiga demos", "pc intros", "intros",
    "demos", "demodulate", "oldschool", "oldschool music", "executable music",
    "cd1", "cd2", "cd3", "bonuscd", "graphics", "selected", "unselected",
    "music from mag", "m4ch", "4ch", "8ch", "multichannel", "4channel",
}


def _meaningful(segs):
    """Deepest path segment that actually names something (artist / event)."""
    for s in reversed(segs):
        norm = s.replace("_", " ").strip().lower()
        if norm and norm not in GENERIC and not YEAR.match(s):
            return s.replace("_", " ").strip()
    return None


def provenance(path):
    """A human, searchable label for where the file lives (party / artist)."""
    segs = path.split("/")[:-1]  # drop filename
    if path.startswith("parties/"):
        # parties/<year>/<event>/...  -> the event (segment right after the year)
        body = segs[1:]
        if body and YEAR.match(body[0]) and len(body) > 1:
            return body[1].replace("_", " ")
        return _meaningful(body) or "scene.org parties"
    # mirrors/scenesp.org/...  -> the deepest real dir (usually the artist)
    return _meaningful(segs[2:]) or "scenesp.org"


def title_of(path):
    b = path.split("/")[-1]
    if b.lower().endswith(".zip"):
        b = b[:-4]
    for fx in FORMATS:
        if b.lower().endswith("." + fx):
            b = b[: -(len(fx) + 1)]
            break
    b = b.replace("_", " ").strip()
    return b or path.split("/")[-1]


def clean(s):
    return s.replace("\t", " ").replace("\n", " ").strip()


def build():
    seen = load_existing()
    sys.stderr.write(f"existing basenames: {len(seen)}\n")
    rows = []
    stats = {"scope": 0, "dup": 0, "skipext": 0, "kept": 0}
    by_fmt = {}
    emitted = set()
    for fx in FORMATS:
        with open(os.path.join(SRC, fx + ".tsv"), encoding="utf-8",
                  errors="replace") as f:
            for line in f:
                c = line.rstrip("\n").split("\t")
                if len(c) < 4:
                    continue
                typ, path = c[0], c[1]
                if "music-module" not in typ and "archive" not in typ:
                    continue  # streams / text / instrument siblings
                if not SCOPE.match(path):
                    continue
                stats["scope"] += 1
                low = path.lower()
                # the real module ext must survive a trailing .zip -- drops
                # mislabeled siblings the TSV tagged as modules (.jpg.zip,
                # .iti, odd ".mod.telepation" junk, pre-rendered streams).
                inner = low[:-4] if low.endswith(".zip") else low
                if not inner.endswith("." + fx):
                    stats["skipext"] += 1
                    continue
                key = base_of(path)
                if key in seen or key in emitted:
                    stats["dup"] += 1
                    continue
                emitted.add(key)
                ext = "zip" if low.endswith(".zip") else fx
                row = "\t".join([
                    clean(title_of(path)),
                    clean(provenance(path)),
                    fx.upper(),
                    BASE + urllib.parse.quote(path),
                    ext,
                ])
                rows.append(row)
                by_fmt[fx] = by_fmt.get(fx, 0) + 1
                stats["kept"] += 1
    out = os.path.normpath(OUT)
    with open(out, "w") as f:
        f.write("\n".join(rows) + "\n")
    sys.stderr.write(
        f"in-scope={stats['scope']} skip-ext={stats['skipext']} "
        f"dup={stats['dup']} kept={stats['kept']}\n")
    sys.stderr.write("  by format: " +
                     ", ".join(f"{k.upper()}={v}" for k, v in by_fmt.items()) + "\n")
    print(f"wrote {len(rows)} net-new rows -> {out}")


if __name__ == "__main__":
    if "--build" in sys.argv:
        build()
    else:
        print(__doc__)
