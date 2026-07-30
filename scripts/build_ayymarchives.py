#!/usr/bin/env python3
"""Build the AY/YM Music Archives song list from Sergey Bulba's ay.strangled.net
archives (Tr_Songs, YM Archive v5, YM, faveym, VtxYmEtc, Nostalgic).

Only the NET-NEW files are shipped -- everything byte-identical to zxart, present
in modland by (basename,size), or already in the projectay ZIP is dropped, the
same net-new-only convention used for Vampi MDX / Demozoo / AMP / VGMRips.

Metadata, in priority order:
  title    module (via `cm --dump-metadata`, i.e. the real decoders) -> .ayl
           Name= -> filename stem
  composer module -> .ayl Author= -> the Authors/<name>/ folder -> "Unknown"

Emitting the module's own title matters: hand-rolled header offsets get .stc
wrong (every file yields the literal format id "SONG BY ST COMPILE"), while the
plugins parse it properly -- so the list agrees with what the app displays.

format is the coarse platform string: "Spectrum AY" for the ZX formats, which
generateIndex re-specialises per real extension (the v128 allowlist covers
pt1/pt2/pt3/asc/stc/stp/sqt/psc/vtx/ftc/fxm/gtr), and "Atari ST" for .ym.

Output template: "title composer format path ext"; path is the member path
inside bulba-projectay-plus.zip.
"""
import json, os, sys, re

SP = sys.argv[1]
OUT = sys.argv[2]
ZIPLIST = sys.argv[3]

ZX = {"pt1","pt2","pt3","asc","stc","stp","sqt","psc","vtx","ftc","fxm","gtr","ay","psm","org"}
ATARI = {"ym"}
# Files the decoders could not load at all -- they would only ever show "Error".
SKIP = set()

ayl = json.load(open(f"{SP}/ayl_index.json"))
def aylkey(p):
    q = p.replace("\\", "/")
    if q.startswith("Tr_Songs/"): q = q[len("Tr_Songs/"):]
    return os.path.normpath(q).lower()

meta = {}
for ln in open(f"{SP}/netnew_meta.tsv", "rb").read().decode("utf-8", "replace").splitlines()[1:]:
    r = ln.split("\t")
    if len(r) < 5: continue
    if r[1] != "ok":
        SKIP.add(r[0]); continue
    meta[r[0]] = (r[3], r[4])

def clean(s):
    s = re.sub(r"[\t\r\n]", " ", s or "")
    s = re.sub(r"\s+", " ", s).strip()
    # a title that is only punctuation/garbage is worse than the filename
    return s if re.search(r"[0-9A-Za-zÀ-ӿ]", s) else ""

rows, ziplist = [], []
for p in (l.strip() for l in open(f"{SP}/netnew_list.txt") if l.strip()):
    if p in SKIP: continue
    ext = p.rsplit(".", 1)[-1].lower()
    if ext not in ZX and ext not in ATARI: continue
    mt, mc = meta.get(p, ("", ""))
    a = ayl.get(aylkey(p), {})
    parts = p.replace("\\", "/").split("/")
    folder = parts[2] if len(parts) > 3 and parts[0] == "Tr_Songs" and parts[1] == "Authors" else ""
    stem = os.path.splitext(os.path.basename(p))[0]
    # NB the last resort is the RAW stem, not clean(stem): a handful of tunes are
    # named ";-(" / "+++" / "_", and clean() would reduce those to an empty title.
    title = clean(mt) or clean(a.get("Name", "")) or clean(stem) or stem or "(untitled)"
    comp  = clean(mc) or clean(a.get("Author", "")) or clean(folder) or "Unknown"
    fmt   = "Atari ST" if ext in ATARI else "Spectrum AY"
    rows.append("\t".join([title, comp, fmt, p, ext]))
    ziplist.append(p)

with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(rows) + "\n")
with open(ZIPLIST, "w", encoding="utf-8") as f:
    f.write("\n".join(ziplist) + "\n")
print(f"skipped (would not load): {len(SKIP)}")
print(f"wrote {len(rows)} songs -> {OUT}")
