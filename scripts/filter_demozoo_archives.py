#!/usr/bin/env python3
"""Drop demozoo.txt archive rows that contain NO playable member.

Many Fujiology (ftp.untergrund.net) zips are native-platform disk-images/ROMs
(e.g. BOING.ZIP -> BOING.ATR, an Atari 8-bit disk image) with no decodable
audio, so they show up in the app as dead "No playable tracks" entries. The
builder works from the SQL dump and can't see inside archives, so this is a
separate post-pass that DOWNLOADS the archive rows and keeps only the ones
holding a member the app can actually play.

Playable member extensions mirror MusicPlayerList.cpp's songExt + audioExt.
By default only ftp.untergrund.net archive rows are inspected (--all-hosts to
inspect every archive row). Non-archive rows and rows on other hosts pass
through untouched.

Usage:
  python3 chipmachine/scripts/filter_demozoo_archives.py            # inspect + rewrite in place
  python3 chipmachine/scripts/filter_demozoo_archives.py --dry-run  # report only, no rewrite
  python3 chipmachine/scripts/filter_demozoo_archives.py --all-hosts
"""
import argparse, concurrent.futures as cf, hashlib, io, os, sys, urllib.request, zipfile

# On-disk cache of the tiny "is there a playable member?" verdict per URL, so a
# re-run (or resuming an interrupted all-hosts pass over ~19k zips) is cheap and
# doesn't re-download. Stores "1"/"0"/"?" (undecidable -> keep).
CACHEDIR = os.path.join(os.path.dirname(__file__), "..", ".demozoo_archive_cache")

DEMOZOO = os.path.join(os.path.dirname(__file__), "..", "data",
                       "demozoo.txt")

# Mirror MusicPlayerList.cpp songExt + audioExt (the ZIP-by-magic track picker).
SONG_EXT = {
    "vgm","vgz","nsf","nsfe","spc","gbs","hes","kss","sgc","ay","gym","usf",
    "miniusf","gsf","minigsf","psf","minipsf","2sf","mini2sf","ssf","dsf","sid",
    "psid","sndh","sap","ym","sc68","pt3","pt2","pt1","stc","stp","sqt","asc",
    "vtx","psc","mod","xm","it","s3m","mtm","669","far","okt","med","mmd0",
    "mmd1","mmd2","mmd3","dbm","digi","ahx","hvl","thx","dmf","ptm","stm","ult",
    "amf","psm","mt2","gt2","dtm","fc","fc13","fc14","aon","smod","dw","cust",
    "mptm",
}
AUDIO_EXT = {"mp3","ogg","flac","wav","mp2","m4a","aac","opus"}
PLAYABLE = SONG_EXT | AUDIO_EXT

ARCHIVE_EXT = {"zip"}  # only zip is inspectable here; lha/rar/7z pass through

UA = ("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120 Safari/537.36")


def ext_of(name):
    b = name.rsplit("/", 1)[-1]
    return b.rsplit(".", 1)[-1].lower() if "." in b else ""


def zip_has_playable(data):
    try:
        z = zipfile.ZipFile(io.BytesIO(data))
    except Exception:
        return None  # not a real zip / corrupt -> undecidable
    for n in z.namelist():
        if n.endswith("/"):
            continue
        base = n.rsplit("/", 1)[-1]
        if base.startswith(".") or n.startswith("__MACOSX/"):
            continue
        if ext_of(n) in PLAYABLE:
            return True
    return False


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


def inspect(url):
    """(keep: bool, reason: str)"""
    key = os.path.join(CACHEDIR, hashlib.sha1(url.encode()).hexdigest())
    if os.path.exists(key):
        v = open(key).read().strip()
        if v == "1":
            return True, "has-playable (cached)"
        if v == "0":
            return False, "no-playable-member (cached)"
        return True, "undecidable (cached) -> keep"
    try:
        data = fetch(url)
    except Exception as e:
        return True, f"download-failed({e}) -> keep"  # don't drop on transient
    r = zip_has_playable(data)
    if r is None:
        open(key, "w").write("?")
        return True, "not-a-zip/corrupt -> keep"
    open(key, "w").write("1" if r else "0")
    return (r, "has-playable" if r else "no-playable-member")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--all-hosts", action="store_true")
    # --native-only: skip the "Demoscene" and "Amiga" platforms, whose zips are
    # module/mp3 compo entries (playable). The unplayable program-zips (ROMs/disk
    # images/exes) are on the native/console platforms, so this cuts the inspect
    # set from ~19k to ~2k -- much gentler on archive.scene.org.
    ap.add_argument("--native-only", action="store_true")
    # Keep concurrency LOW: we're a guest on archive.scene.org. 4 parallel small
    # requests is plenty and won't look like an attack / get us rate-limited.
    ap.add_argument("--jobs", type=int, default=4)
    args = ap.parse_args()
    PASS_THROUGH_PLATFORMS = {"Demoscene", "Amiga"}

    os.makedirs(CACHEDIR, exist_ok=True)
    lines = open(DEMOZOO, encoding="utf-8").read().splitlines()
    targets = []  # (index, url)
    for i, line in enumerate(lines):
        c = line.split("\t")
        if len(c) < 5:
            continue
        url, ext, platform = c[3], c[4].lower(), (c[2] if len(c) > 2 else "")
        if ext not in ARCHIVE_EXT:
            continue
        if not args.all_hosts and "ftp.untergrund.net" not in url:
            continue
        if args.native_only and platform in PASS_THROUGH_PLATFORMS:
            continue
        targets.append((i, url))

    print(f"inspecting {len(targets)} archive rows "
          f"({'all hosts' if args.all_hosts else 'ftp.untergrund.net only'})...",
          file=sys.stderr)

    drop = set()
    results = {}
    with cf.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        fut = {ex.submit(inspect, url): (i, url) for i, url in targets}
        done = 0
        for f in cf.as_completed(fut):
            i, url = fut[f]
            keep, reason = f.result()
            results[i] = (keep, reason, url)
            if not keep:
                drop.add(i)
            done += 1
            if done % 50 == 0:
                print(f"  {done}/{len(targets)}", file=sys.stderr)

    kept = len(targets) - len(drop)
    print(f"\nresult: keep {kept}, DROP {len(drop)} of {len(targets)}",
          file=sys.stderr)
    for i in sorted(drop):
        print(f"  DROP {results[i][2]}", file=sys.stderr)

    if args.dry_run:
        print("(dry-run: demozoo.txt not modified)", file=sys.stderr)
        return

    out = [ln for j, ln in enumerate(lines) if j not in drop]
    with open(DEMOZOO, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print(f"rewrote demozoo.txt: {len(lines)} -> {len(out)} rows", file=sys.stderr)


if __name__ == "__main__":
    main()
