#!/usr/bin/env python3
"""Inspect demozoo.txt archive rows: drop the dead ones, classify the rest.

The builder works from the SQL dump and can't see inside archives, so this is a
separate post-pass over demozoo.txt. It does two jobs, both from the archive's
MEMBER LIST:

  --dead-rows  Drop rows with NO playable member. Many Fujiology
               (ftp.untergrund.net) zips are native-platform disk-images/ROMs
               (e.g. BOING.ZIP -> BOING.ATR, an Atari 8-bit disk image) with no
               decodable audio, so they show as dead "No playable tracks".
  --classify   Replace the generic "Demoscene" platform label with the member's
               real format ("XM", "MOD", "IT", "MP3", ...). Demozoo records no
               platform for these productions, so without this they carry a
               format string format_map can't key AND an archive extension it
               can't key either -- i.e. they reach NO platform filter at all
               (~17k songs). The uppercase-extension vocabulary matches what the
               keygenmusic/botb collections already put in that column, and
               format_map keys those directly, so no new label vocabulary.

Member lists are read with HTTP RANGE requests, NOT full downloads: a zip's
central directory lives at the END of the file (one ~4KB tail read regardless of
archive size -- 0.3% of a 1.6MB zip), while lha/rar headers sit at the FRONT.
That makes this ~100x lighter on archive.scene.org than the full-download pass
this replaced. .7z (header is LZMA-encoded at a far offset) and .tar.gz (needs
inflating to reach the tar header) are not range-peekable and pass through.

Playable member extensions mirror MusicPlayerList.cpp's songExt + audioExt.
By default only ftp.untergrund.net archive rows are inspected (--all-hosts to
inspect every archive row). Non-archive rows and rows on other hosts pass
through untouched.

Usage:
  # classify the ~17k "Demoscene" archives on archive.scene.org:
  python3 chipmachine/scripts/filter_demozoo_archives.py --classify --all-hosts
  python3 chipmachine/scripts/filter_demozoo_archives.py --classify --all-hosts --dry-run
  python3 chipmachine/scripts/filter_demozoo_archives.py --dead-rows   # the old job
"""
import argparse, concurrent.futures as cf, hashlib, io, json, os, re, struct, sys
import urllib.request, zipfile

# On-disk cache of the archive's MEMBER LIST per URL, so a re-run (or resuming an
# interrupted pass over ~17k archives) is cheap and doesn't re-fetch. Also means
# re-applying --classify after a build_demozoo.py --build (which regenerates
# demozoo.txt from the dump, wiping the labels) costs no network at all.
# Stores JSON: {"m": [member names]} or {"m": null} for undecidable.
CACHEDIR = os.path.join(os.path.dirname(__file__), "..", ".demozoo_archive_cache")

DEMOZOO = os.path.join(os.path.dirname(__file__), "..", "data",
                       "demozoo.txt")

def _cmtest(case):
    """Run a cmtest [.] dump case, or None if the binary isn't built."""
    import subprocess
    here = os.path.dirname(__file__)
    for exe in (os.path.join(here, "..", "..", "build", "cmtest"),
                os.path.join(here, "..", "build", "cmtest")):
        if not os.path.exists(exe):
            continue
        try:
            return subprocess.run([exe, case], capture_output=True, text=True,
                                  timeout=300).stdout
        except Exception:
            return None
    return None


def _app_picker_sets():
    """The picker's (song, audio) sets, straight from the app's plugin registry.

    Mirrors MusicPlayerList::archiveExtensions() rather than hand-copying it --
    a hand-copy is what drifted and made 129 live archives look dead.
    """
    out = _cmtest("archive_picker_exts")
    if not out:
        return None, None
    song = {l[5:].strip() for l in out.splitlines() if l.startswith("song:")}
    audio = {l[6:].strip() for l in out.splitlines() if l.startswith("audio:")}
    if len(song) < 100:
        return None, None
    sys.stderr.write(f"picker sets: {len(song)} song + {len(audio)} audio "
                     f"(from cmtest archive_picker_exts)\n")
    return song, audio


def _app_playable():
    """The extensions the APP can actually play, straight from the built binary.

    MusicPlayerList's ZIP picker derives its allowlist from the registered
    plugins, so hand-copying it here would drift right back out of sync -- which
    is the bug this whole pass exists to clean up (a zip holding only an Organya
    .org was called "dead" although OrgPlugin decodes it). Ask cmtest for the
    real plugin extension map instead, minus the not-supported list. Falls back
    to the static set below if the binary isn't built.
    """
    import subprocess
    here = os.path.dirname(__file__)
    for exe in (os.path.join(here, "..", "..", "build", "cmtest"),
                os.path.join(here, "..", "build", "cmtest")):
        if not os.path.exists(exe):
            continue
        try:
            out = subprocess.run([exe, "priority_map"], capture_output=True,
                                 text=True, timeout=300).stdout
        except Exception:
            continue
        exts = {l.split()[0].lstrip(".").lower()
                for l in out.splitlines() if l.startswith(".") and l.split()}
        if len(exts) < 100:
            continue
        ns = set()
        p = os.path.join(here, "..", "data", "misc",
                         "not_supported_extensions.txt")
        if os.path.exists(p):
            for line in open(p):
                s = line.strip()
                if s.startswith("."):
                    ns.add(s[1:].lower())
        sys.stderr.write(f"playable set: {len(exts - ns)} extensions "
                         f"(from cmtest priority_map)\n")
        return exts - ns
    sys.stderr.write("WARNING: cmtest not found -- falling back to the static "
                     "list, which UNDERSTATES what the app plays\n")
    return None


# Fallback only (see _app_playable): mirrors MusicPlayerList.cpp's old hand-kept
# songExt + audioExt. Kept so the script still runs without a build tree.
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

# The two jobs need DIFFERENT sets, and conflating them is a trap:
#
#   PLAYABLE  -- "can the app play any member?" (the --dead-rows verdict).
#     Must be the app's FULL set, or we delete rows holding music we can decode
#     (129 archives here held .org/.mdl/.mo3/.a2m and looked dead).
#
#   LABELABLE -- "what format do we call it?" (the --classify label).
#     Must be the NARROW, verified set. The app's full set includes formats
#     format_map can't key (so the label classifies to nothing -- no better than
#     "Demoscene", but now displayed as a bare "ORG") and, worse, UADE's
#     prefix-style format names: it claims .dat/.js/.md/.pm/.x, so a plain
#     "readme.dat" would be labelled "DAT". Every code below is asserted to
#     resolve by the cmtest "peek_labels" dump.
APP_SONG, APP_AUDIO = _app_picker_sets()
PLAYABLE = (APP_SONG | APP_AUDIO) if APP_SONG else (_app_playable() or
                                                    (SONG_EXT | AUDIO_EXT))
# Formats we ship a plugin for AND whose bare code format_map keys, so the label
# resolves to a real platform. Verified by the cmtest "peek_labels" dump, which
# asserts every code actually written here classifies to something.
# Deliberately NOT here: "ftm" (FamiTracker NES vs OpenMPT's FTMN share the
# extension and are magic-gated) and "mix" (StSound: Atari ST YM vs Amstrad CPC)
# -- an extension-keyed label would misfile one of the two.
LABELABLE = SONG_EXT | AUDIO_EXT | {
    "mdl", "mo3", "a2m", "mid", "prg", "sunvox",
    # rendered-audio containers ffmpeg decodes (AUDIO_EXT above is the old,
    # narrower hand list); format_map keys each of these to the MP3/OGG bucket
    "mp4", "aif", "aiff", "wma", "ac3", "mpeg",
    # chip/tracker formats whose bare code format_map now keys
    "ams", "v2m", "bbsong", "hsc", "rad", "ptcop",
}

# Range-peekable archives. .7z and .gz/.tar.gz are NOT: 7z's header is LZMA-
# encoded at an offset near EOF, and a .tar.gz must be inflated to reach the tar
# header. Both are a rounding error here (39 + 33 rows) -- they pass through.
ARCHIVE_EXT = {"zip", "lha", "lzh", "rar"}

UA = ("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120 Safari/537.36")


def ext_of(name, allow=None, prefix_ok=False):
    """Format token of an archive member, as the APP would read it.

    SUFFIX ONLY unless prefix_ok. Which is right depends on the CONTAINER, and
    mirrors what the app does with each:

    * ZIP  -> suffix only. MusicPlayerList's picker reads path_extension, so a
      prefix-named member inside a zip is unreachable no matter what we label the
      row. Allowing prefixes here also fires on ordinary filenames, because the
      set has ~1200 entries: "AD.EXE" -> ad, "SE.COM" -> se, "CP.CFG" -> cp,
      "sss.tap" -> sss -- all DOS junk, and they outranked real .xm/.it members.
    * LHA  -> prefixes allowed. These carry Amiga/modland naming
      ("mod.Kwern akk"), which the app resolves via the ".lha/<member>" path
      (RemoteLoader + MusicDatabase::resolveExtension both special-case it).
      Reading those suffix-only calls the format "akk" and loses the row.
    """
    allow = PLAYABLE if allow is None else allow
    b = name.rsplit("/", 1)[-1].lower()
    if "." not in b:
        return ""
    suffix = b.rsplit(".", 1)[-1]
    if suffix in allow:
        return suffix
    if prefix_ok:
        prefix = b.split(".", 1)[0]
        if prefix in allow:
            return prefix
    return suffix


# Archive info files that LOOK like music by extension. We only have member
# NAMES here (the range peek never fetches contents), so unlike the app -- which
# asks the plugins and lets OrgPlugin's "Org-0x" magic check decline -- we cannot
# tell "scene.org" (a plain text file present in ~1800 archive.scene.org zips)
# from a real Organya tune. Name them explicitly.
INFO_FILES = {"scene.org", "file_id.org", "file_id.diz", "descript.ion"}


def _is_junk(n):
    base = n.rsplit("/", 1)[-1]
    if base.lower() in INFO_FILES:
        return True
    return (n.endswith("/") or base.startswith(".") or n.startswith("__MACOSX/"))


def rng(url, spec, timeout=45):
    """One HTTP Range read. `spec` is a Range value ("-4096", "0-8191")."""
    req = urllib.request.Request(url, headers={"User-Agent": UA,
                                               "Range": f"bytes={spec}"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def zip_members(url):
    """Member names from the zip central directory, via a tail read.

    The End Of Central Directory record is last, and the central directory sits
    just before it -- so a tail read gets both for any archive whose directory
    fits. Retry wider when it doesn't (many-membered zips).
    """
    for size in (4096, 65536, 262144, 1048576):
        d = rng(url, f"-{size}")
        eocd = d.rfind(b"PK\x05\x06")
        if eocd == -1:
            continue                       # EOCD not in this slice -> widen
        cd_size = struct.unpack("<I", d[eocd + 12:eocd + 16])[0]
        # Parse ONLY the real central directory. It ends exactly where the EOCD
        # begins, so its bounds are known -- scanning the whole tail for the
        # PK\x01\x02 signature instead would also match the central directory of
        # any NESTED zip stored near the end of this one, inventing members that
        # do not exist at this level (a real case: b98mhs22.zip holds
        # "Bbr-iltm.zip", whose own directory leaked in as .xm/.exe/.pcb members
        # -- and the app cannot reach inside a nested archive anyway).
        start = eocd - cd_size
        if start < 0:
            continue                       # directory truncated -> widen
        cd = d[start:eocd]
        names, j = [], 0
        while True:
            k = cd.find(b"PK\x01\x02", j)
            if k == -1:
                break
            nlen = struct.unpack("<H", cd[k + 28:k + 30])[0]
            names.append(cd[k + 46:k + 46 + nlen].decode("utf8", "replace"))
            j = k + 4
        return names
    return None


def lha_members(url):
    """Member names from LHA/LZH headers, which sit at the FRONT of the file."""
    d = rng(url, "0-16383")
    names, p = [], 0
    while p + 22 < len(d) and d[p] != 0:
        if not d[p + 2:p + 7].startswith(b"-lh"):
            break
        hs = d[p]
        try:
            csize = struct.unpack("<I", d[p + 7:p + 11])[0]
            nlen = d[p + 21]
            names.append(d[p + 22:p + 22 + nlen].decode("latin1"))
        except Exception:
            break
        p += hs + 2 + csize          # next header follows the compressed data
    return names or None


def rar_members(url):
    """Member names from RAR headers at the FRONT of the file.

    RAR's header layout differs between v4 and v5, so rather than parse both,
    pull filename-shaped strings out of the header region and let the PLAYABLE
    allowlist decide -- a false positive would have to be a playable extension
    appearing inside the first 16KB of headers.
    """
    d = rng(url, "0-16383")
    if not d.startswith(b"Rar!"):
        return None
    cand = re.findall(rb"[\w\-. !()\[\]]{3,80}\.[A-Za-z0-9]{2,5}", d)
    return [c.decode("latin1") for c in cand] or None


def peek_members(url):
    """Member names via range reads, or None if undecidable. Cached."""
    key = os.path.join(CACHEDIR, hashlib.sha1(url.encode()).hexdigest() + ".json")
    if os.path.exists(key):
        try:
            return json.load(open(key))["m"]
        except Exception:
            pass
    e = url.rsplit(".", 1)[-1].lower()
    fn = {"zip": zip_members, "lha": lha_members, "lzh": lha_members,
          "rar": rar_members}.get(e)
    if fn is None:
        return None
    m = fn(url)                       # may raise -> caller treats as transient
    json.dump({"m": m}, open(key, "w"))
    return m


def playable_of(members, allow=None, prefix_ok=False):
    """The member the app would play, or None. Junk/metadata files ignored."""
    allow = PLAYABLE if allow is None else allow
    if not members:
        return None
    for n in members:
        if _is_junk(n):
            continue
        if ext_of(n, allow, prefix_ok) in allow:
            return n
    return None


def labelable_of(members, prefix_ok=False):
    """The member whose format we're willing to WRITE as the label (see
    LABELABLE). None means: leave the row's existing label alone."""
    return playable_of(members, LABELABLE, prefix_ok)


def app_pick(members, prefix_ok=False):
    """The member the APP would actually play -- mirrors MusicPlayerList's ZIP
    branch: collect "song" (chip/module) members and "audio" (ffmpeg rendering)
    members, prefer songs, sort, take the first. Getting this wrong would write
    an `ext` for a member the app never plays.
    """
    if not members or not APP_SONG:
        return playable_of(members, None, prefix_ok)
    songs, audio = [], []
    for n in members:
        if _is_junk(n):
            continue
        if ext_of(n, APP_SONG, prefix_ok) in APP_SONG:
            songs.append(n)
        elif ext_of(n, APP_AUDIO, prefix_ok) in APP_AUDIO:
            audio.append(n)
    tracks = songs or audio
    return sorted(tracks)[0] if tracks else None


def first_member_ext(members):
    """Ext of the first non-junk member of a DEAD archive. Written to the `ext`
    column so not_supported_extensions.txt can gate the row at index time (that
    list is matched against `ext`, which is "zip" for every archive row and so
    was unreachable). The row stays in demozoo.txt: enabling the format later is
    then just deleting a not_supported line + a reindex, not a re-onboarding.
    """
    JUNK = {"txt", "diz", "nfo", "readme", "jpg", "png", "gif", "bmp", "doc",
            "html", "htm", "bat", "ini", "url", "me", "1st", "log", "cfg"}
    # NEVER report a nested container. We DO support .lha/.lzx as a top-level
    # row (263 of them here), so writing one into `ext` would invite a
    # not_supported line that silently kills those instead. A container nested
    # inside a zip is simply not peeked -- leave the row alone.
    CONTAINER = {"zip", "lha", "lzh", "lzx", "rar", "7z", "gz", "tar", "z", "arj"}
    # UADE's prefix-only tokens (see MusicPlayerList::archiveExtensions): as a
    # SUFFIX these are JavaScript/data/Markdown files, not music. Writing one
    # into `ext` would invite a not_supported line for ".js" that also hides any
    # legitimate row carrying that ext -- and would name the wrong file anyway.
    PREFIX_ONLY = {"js", "dat", "md", "ml", "pm", "ps", "di", "db", "pat", "cp"}
    # A real (non-junk) extension always wins. But an archive whose ONLY content
    # is a readme is itself the finding -- ~100 scene.org compo entries are a
    # lone .txt, the tune never uploaded -- so fall back to reporting that, which
    # lets the ".txt" line in not_supported_extensions.txt gate the row. Without
    # this they keep ext="zip", the list can't match, and they stay indexed as
    # dead search hits.
    fallback = ""
    for n in members or []:
        if _is_junk(n):
            continue
        e = os.path.splitext(n.rsplit("/", 1)[-1])[1].lower().lstrip(".")
        # Archive member names carry mojibake and control characters (real
        # example: "x.iff ·)» i£$ whq!..."), which would otherwise be written
        # into the ext column verbatim. Only accept a plausible extension.
        if not e or not re.fullmatch(r"[a-z0-9]{1,8}", e):
            continue
        if e in CONTAINER or e in PREFIX_ONLY:
            continue
        if e in JUNK:
            if not fallback:
                fallback = e     # remember, but keep looking for something real
            continue
        return e
    return fallback


def inspect(url):
    """(members|None, reason). Never raises: a transient error is undecidable."""
    try:
        return peek_members(url), "ok"
    except Exception as e:
        return None, f"peek-failed({e})"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--all-hosts", action="store_true")
    # The two jobs. --dead-rows is the original behaviour (drop rows with no
    # playable member); --classify rewrites the platform label from the member's
    # real format. Both read the same member list, so they can run together.
    ap.add_argument("--classify", action="store_true")
    ap.add_argument("--dead-rows", action="store_true")
    # Only rows whose current label is this get reclassified (the generic bucket
    # demozoo leaves when a production has no platform). Rows that already name a
    # platform are left alone -- demozoo knows better than the archive does.
    ap.add_argument("--relabel-only", default="Demoscene")
    ap.add_argument("--limit", type=int, default=0, help="sample N rows (test)")
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
        # --classify only touches the generic bucket: a row that already names a
        # platform came from demozoo's own data, which is better evidence than
        # whatever the archive happens to contain.
        if args.classify and not args.dead_rows and platform != args.relabel_only:
            continue
        targets.append((i, url))
    if args.limit:
        targets = targets[:args.limit]

    print(f"inspecting {len(targets)} archive rows "
          f"({'all hosts' if args.all_hosts else 'ftp.untergrund.net only'})...",
          file=sys.stderr)

    import collections
    drop, relabel, setext = set(), {}, {}
    stats = collections.Counter()
    fmts = collections.Counter()
    deadexts = collections.Counter()
    with cf.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        fut = {ex.submit(inspect, url): (i, url) for i, url in targets}
        done = 0
        for f in cf.as_completed(fut):
            i, url = fut[f]
            members, reason = f.result()
            done += 1
            if done % 200 == 0:
                print(f"  {done}/{len(targets)}", file=sys.stderr)
            if members is None:
                stats["undecidable -> untouched"] += 1
                continue
            # ONE member decides both columns, so `ext` can never name a
            # different file than `format` does.
            # LHA members are Amiga/modland prefix-named; ZIP members are not
            # (and the app's zip picker could not reach a prefix-named one).
            prefix_ok = url.lower().endswith((".lha", ".lzh"))
            member = app_pick(members, prefix_ok)
            if member is None:
                stats["no playable member"] += 1
                if args.dead_rows:
                    drop.add(i)
                elif args.classify:
                    # Not deleted: record what IS inside, so the existing
                    # not_supported_extensions.txt gate can drop it at index time
                    # and a future plugin can revive it by deleting one line.
                    e = first_member_ext(members)
                    if e:
                        setext[i] = e
                        deadexts[e] += 1
                continue
            stats["playable"] += 1
            if args.classify:
                e = ext_of(member, PLAYABLE, prefix_ok)
                setext[i] = e            # the real inner format, not "zip"
                if e in LABELABLE:
                    # Uppercase extension: the vocabulary keygenmusic/botb
                    # already use in this column, which format_map keys directly.
                    label = e.upper()
                    relabel[i] = label
                    fmts[label] += 1
                else:
                    # Playable, but a format we won't NAME (see LABELABLE) --
                    # leave the existing label rather than write one that
                    # classifies to nothing. The ext column is still corrected.
                    stats["playable, format not labelable -> label left"] += 1

    print(f"\n-- inspected {len(targets)} --", file=sys.stderr)
    for k, v in stats.most_common():
        print(f"  {v:6d}  {k}", file=sys.stderr)
    if args.classify:
        print(f"\n-- resolved formats ({len(relabel)} rows) --", file=sys.stderr)
        for k, v in fmts.most_common(25):
            print(f"  {v:6d}  {k}", file=sys.stderr)
        print(f"\n-- `ext` column set from the archive ({len(setext)} rows) --",
              file=sys.stderr)
        if deadexts:
            print("   of which UNPLAYABLE (add to not_supported_extensions.txt "
                  "to stop them being indexed):", file=sys.stderr)
            for k, v in deadexts.most_common(20):
                print(f"     {v:5d}  .{k}", file=sys.stderr)
    if args.dead_rows:
        print(f"\nDROP {len(drop)} rows with no playable member", file=sys.stderr)

    if args.dry_run:
        print("\n(dry-run: demozoo.txt not modified)", file=sys.stderr)
        return
    if not (args.classify or args.dead_rows):
        print("(no --classify/--dead-rows: nothing written)", file=sys.stderr)
        return

    out = []
    for j, ln in enumerate(lines):
        if j in drop:
            continue
        if j in relabel or j in setext:
            c = ln.split("\t")
            if j in relabel:
                c[2] = relabel[j]
            if j in setext and len(c) > 4:
                c[4] = setext[j]   # the real inner format, not the "zip" wrapper
            ln = "\t".join(c)
        out.append(ln)
    with open(DEMOZOO, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print(f"\nrewrote demozoo.txt: {len(lines)} -> {len(out)} rows, "
          f"{len(relabel)} relabelled", file=sys.stderr)


if __name__ == "__main__":
    main()
