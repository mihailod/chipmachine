#!/usr/bin/env python3
"""
Comprehensive scanner + downloader for missing cmtest extensions.

The key insight: most missing UADE extensions are prefix-form Amiga formats.
In Modland, these live in directories named after the tracker, e.g.:
  - "thx" → AHX/<author>/<file>.ahx  (or thx.<name>)
  - "p61a" → The Player 6.1a/<author>/p61a.<name>
  - "tfmx" → TFMX/<author>/mdat.<name>  (with smpl.<name>)

We need to:
1. Map UADE extension → Modland format directory name(s)
2. Search allmods.txt for files in those directories
3. Pick small files (no smpl/sample companions needed for test)
4. Download via HTTP mirror

For AdLib, OpenMPT, RSN, SC68 etc, we use suffix-match on extensions.
"""

import os
import sys
import subprocess
import urllib.parse
from collections import defaultdict

PROJ    = os.path.dirname(os.path.abspath(__file__))
DATA    = os.path.join(os.path.dirname(PROJ), "chipmachine", "data")
TESTMUS = os.path.join(os.path.dirname(PROJ), "chipmachine", "testmus")
ALLMODS  = os.path.join(DATA, "allmods.txt")
MODLAND_HTTP = "https://ftp.modland.com/pub/modules/"

MAX_PER_EXT = 3

# ── Map UADE extension → Modland directory name(s) ──────────────────────
# These are many-to-many: one extension may appear in multiple directories,
# and files may use either prefix (ext.songname) or suffix (songname.ext).
UADE_EXT_TO_MODLAND_DIR = {
    # ProPacker/ProRunner family (prefix: e.g. pp20.songname)
    "!pm!":  ["Promizer"],
    "40a":   ["Promizer 0.1"],
    "40b":   ["Promizer 0.1"],
    "41a":   ["Promizer 1.0c"],
    "50a":   ["Promizer 2.0"],
    "60a":   ["Promizer 4.0"],
    "61a":   ["Promizer 4.0"],
    "ac1":   ["AC1D Packer"],
    "ac1d":  ["AC1D Packer"],
    "adpcm": ["ADPCM Mono"],
    "agi":   ["Actionamics"],
    "aon4":  ["Art Of Noise 4V"],
    "aon8":  ["Art Of Noise 8V"],
    "aval":  ["Promizer"],
    "bfc":   ["FutureComposer BSI"],
    "bye":   ["Delitracker Custom"],
    "cin":   ["Delitracker Custom"],
    "cplx":  ["Promizer"],
    "crb":   ["Promizer"],
    "custom":["CustomMade", "Delitracker Custom"],
    "di":    ["Promizer"],
    "dl_deli":["Delitracker Custom"],
    "dlm1":  ["Delta Music"],
    "dlm2":  ["Delta Music 2"],
    "dmu2":  ["Digital Mugician 2"],
    "dwold": ["David Whittaker"],
    "emsv6": ["EMS"],
    "eu":    ["Eureka Packer"],
    "fc-bsi":["FutureComposer BSI"],
    "fc-m":  ["FutureComposer"],
    "fc3":   ["FutureComposer"],
    "fc4":   ["FutureComposer"],
    "fcm":   ["FutureComposer"],
    "fuz":   ["Promizer"],
    "gm":    ["Graoumf Tracker"],
    "gv":    ["Promizer"],
    "hmc":   ["Hippel-CoSo"],
    "hn":    ["Major Synth"],
    "hrt":   ["Heatseeker", "Delitracker Custom"],
    "hrt!":  ["Heatseeker"],
    "it1":   ["Ice Tracker"],
    "jcb":   ["JamCracker"],
    "jcbo":  ["JamCracker"],
    "jp":    ["JasonPage"],
    "jpnd":  ["JasonPage"],
    "jpold": ["JasonPage"],
    "js":    ["Delitracker Custom"],
    "kef7":  ["Kef7 Tracker"],
    "kim":   ["Delitracker Custom", "Kris Hatlelid"],
    "krs":   ["Kris Hatlelid"],
    "lax":   ["Promizer"],
    "mcmd_org":["MCMD"],
    "mdst":  ["TFMX ST"],
    "mexxmp":["Promizer"],
    "mkiio": ["Mark II"],
    "mod15": ["Soundtracker"],
    "mod15_mst":["Soundtracker"],
    "mod15_st-iv":["Soundtracker"],
    "mod15_ust":["Soundtracker"],
    "mod_adsc4":["Actionamics"],
    "mod_comp":["Protracker"],
    "mod_doc":["Soundtracker"],
    "mod_flt4":["Startrekker FLT8", "Startrekker AM"],
    "mod_ntk": ["Soundtracker"],
    "mod_ntk1":["Soundtracker"],
    "mod_ntk2":["Soundtracker"],
    "mod_ntkamp":["Soundtracker"],
    "mon_old": ["Hippel"],
    "mosh":  ["Delitracker Custom"],
    "mpro":  ["Promizer"],
    "mtp2":  ["Soundtracker Pro II"],
    "mug2":  ["Digital Mugician 2"],
    "mw":    ["Martin Walker"],
    "noisepacker2":["Noisepacker 2"],
    "noisepacker3":["Noisepacker 3"],
    "np":    ["Noisepacker 1", "Noisepacker 2", "Noisepacker 3"],
    "np1":   ["Noisepacker 1"],
    "np2":   ["Noisepacker 2"],
    "np3":   ["Noisepacker 3"],
    "npp":   ["Delitracker Custom"],
    "nru":   ["Promizer"],
    "ntpk":  ["Noise Tracker Packed"],
    "octamed":["OctaMED MMD0", "OctaMED MMD1", "OctaMED MMD2"],
    "p10":   ["Promizer 1.0c"],
    "p21":   ["Promizer 2.0"],
    "p30":   ["Promizer 3.0"],
    "p40a":  ["Promizer 4.0"],
    "p40b":  ["Promizer 4.0"],
    "p41a":  ["Promizer 4.0"],
    "p4x":   ["Promizer 4.0"],
    "p5a":   ["Promizer"],
    "p5x":   ["Promizer"],
    "p60":   ["The Player 6.0"],
    "p60a":  ["The Player 6.0a"],
    "p61":   ["The Player 6.1"],
    "p61a":  ["The Player 6.1a"],
    "p6x":   ["The Player 6.0", "The Player 6.1", "The Player 6.1a"],
    "pat":   ["Delitracker Custom"],
    "pm0":   ["Power Music"],
    "pm1":   ["Power Music"],
    "pm10c": ["Promizer 1.0c"],
    "pm18a": ["Promizer 1.8a"],
    "pm2":   ["Promizer 2.0"],
    "pm20":  ["Promizer 2.0"],
    "pm4":   ["Promizer 4.0"],
    "pm40":  ["Promizer 4.0"],
    "pmz":   ["Promizer"],
    "polk":  ["Polkamaster"],
    "powt":  ["Powertracker"],
    "pp20":  ["ProPacker 2.1"],
    "pp30":  ["ProPacker 3.0"],
    "ppk":   ["ProPacker 2.1", "ProPacker 3.0"],
    "pru":   ["ProRunner 1.0", "ProRunner 2.0"],
    "pru1":  ["ProRunner 1.0"],
    "prun":  ["ProRunner 1.0", "ProRunner 2.0"],
    "prun1": ["ProRunner 1.0"],
    "prun2": ["ProRunner 2.0"],
    "pwr":   ["Powertracker"],
    "pyg":   ["Pygmy Projects"],
    "pygm":  ["Pygmy Projects"],
    "pygmy": ["Pygmy Projects"],
    "qc":    ["Promizer"],
    "rj":    ["Delitracker Custom"],
    "rjp":   ["Richard Joseph"],
    "rkb":   ["Delitracker Custom"],
    "s-c":   ["Sean Connolly", "Sean Conran"],
    "s7g":   ["Hippel 7V"],
    "sa-p":  ["Sonic Arranger"],
    "sa_old":["Sonic Arranger"],
    "sct":   ["Soundtracker"],
    "sfx13": ["SoundFX 1.3"],
    "sid1":  ["SidMon 1"],
    "skyt":  ["Delitracker Custom"],
    "sm1":   ["FAC SoundTracker"],
    "sm2":   ["FAC SoundTracker"],
    "smn":   ["Delitracker Custom"],
    "sndmon":["BP SoundMon 1", "BP SoundMon 2", "BP SoundMon 3"],
    "snt":   ["Sonic Arranger"],
    "snt!":  ["Sonic Arranger"],
    "snx":   ["Sonix Music Driver"],
    "ss":    ["Delitracker Custom"],
    "st26":  ["Soundtracker 2.6"],
    "st30":  ["Soundtracker Pro II"],
    "star":  ["StarTrekker AM"],
    "stpk":  ["Soundtracker Packed"],
    "tfhd1.5":["TFMX ST"],
    "tfhd7v": ["TFMX ST"],
    "tfhdpro":["TFMX ST"],
    "tfmx":   ["TFMX"],
    "tfmx1.5":["TFMX"],
    "tfmx7v": ["TFMX"],
    "tfmxpro":["TFMX"],
    "thn":   ["Thomas Hermann"],
    "thx":   ["AHX"],
    "tits":  ["Delitracker Custom"],
    "tmk":   ["Tomy Tracker"],
    "tp1":   ["Tracker Packer 1"],
    "tp2":   ["Tracker Packer 2"],
    "tp3":   ["Tracker Packer 3"],
    "trc":   ["Tronic"],
    "tro":   ["Tronic"],
    "un2":   ["Unique Development"],
    "unic":  ["UNIC Tracker"],
    "unic2": ["UNIC Tracker"],
    "wn":    ["Wanton Packer"],
    "xan":   ["Xann Packer"],
}

# Non-UADE missing extensions and their search strategy
ADLIB_EXTS = {
    "a2t": {"suffix": True, "dirs": ["Adlib Tracker 2"]},
    "bmf": {"suffix": True, "dirs": ["BMF"]},
    "got": {"suffix": True, "dirs": ["Dynamics"]},
    "ha2": {"suffix": True, "dirs": ["HSC AdLib"]},
    "hsp": {"suffix": True, "dirs": ["HSC AdLib"]},
    "hsq": {"suffix": True, "dirs": ["Ad Lib"]},
    "mdi": {"suffix": True, "dirs": ["Ad Lib"]},
    "mdy": {"suffix": True, "dirs": ["Ad Lib"]},
    "mkf": {"suffix": True, "dirs": ["Ad Lib"]},
    "msc": {"suffix": True, "dirs": ["Ad Lib"]},
    "rac": {"suffix": True, "dirs": ["Ad Lib"]},
    "rix": {"suffix": True, "dirs": ["Ad Lib"]},
    "snd": {"suffix": True, "dirs": ["Ad Lib", "SNDHPlayer"]},
    "sop": {"suffix": True, "dirs": ["Ad Lib", "Note"]},
    "wlf": {"suffix": True, "dirs": ["Ad Lib"]},
}

OPENMPT_EXTS = {
    "fst": {"suffix": True, "dirs": ["FastTracker"]},
    "j2b": {"suffix": True, "dirs": []},
    "mmcmp":{"suffix": True, "dirs": []},
    "nst": {"suffix": True, "dirs": ["Soundtracker"]},
    "oxm": {"suffix": True, "dirs": []},
    "ppm": {"suffix": True, "dirs": []},
    "pt36":{"suffix": True, "dirs": ["Protracker"]},
    "stx": {"suffix": True, "dirs": ["Screamtracker 2", "Screamtracker 3"]},
    "xpk": {"suffix": True, "dirs": []},
}

RSN_EXTS = {
    "r64": {"suffix": True, "dirs": ["Nintendo SPC"]},
    "rdc": {"suffix": True, "dirs": ["Nintendo SPC"]},
    "rds": {"suffix": True, "dirs": ["Nintendo SPC"]},
    "rgs": {"suffix": True, "dirs": ["Nintendo SPC"]},
    "rps": {"suffix": True, "dirs": ["Nintendo SPC"]},
}

SC68_EXTS = {
    "snd": {"suffix": True, "dirs": ["SNDH", "sc68"]},
}

AO_EXTS = {
    "qsf": {"suffix": True, "dirs": ["Capcom Q-Sound Format"]},
}

QUARTET_EXTS = {
    "4q": {"suffix": True, "dirs": ["Quartet"]},
}


# ── Helpers ──────────────────────────────────────────────────────────────
def download_file(url, dest_path, timeout=30):
    """Download a file using curl. Returns True on success."""
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    cmd = ["curl", "-fsSL",
           "--connect-timeout", str(timeout),
           "--max-time", str(timeout * 3),
           "-o", dest_path, url]
    try:
        subprocess.run(cmd, check=True, capture_output=True, timeout=timeout*4)
        if os.path.exists(dest_path) and os.path.getsize(dest_path) > 0:
            return True
        if os.path.exists(dest_path):
            os.remove(dest_path)
        return False
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        if os.path.exists(dest_path):
            os.remove(dest_path)
        return False


def sanitize_filename(name):
    """Make a filename safe for the filesystem."""
    return name.replace('/', '_').replace('\\', '_')[:100]


# ── Build index of format dirs → files from allmods.txt ──────────────────
def build_index():
    """Build: format_dir (lowercase) → [(path, size)]"""
    # Collect all format directory names we care about
    needed_dirs = set()
    for dirs in UADE_EXT_TO_MODLAND_DIR.values():
        for d in dirs:
            needed_dirs.add(d.lower())
    for ext_map in [ADLIB_EXTS, OPENMPT_EXTS, RSN_EXTS, SC68_EXTS, AO_EXTS, QUARTET_EXTS]:
        for info in ext_map.values():
            for d in info.get("dirs", []):
                needed_dirs.add(d.lower())

    index = defaultdict(list)  # dir_lower -> [(path, size)]
    print(f"Building index from {ALLMODS} ...")
    print(f"Looking for {len(needed_dirs)} format directories")

    with open(ALLMODS, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.strip()
            if not line or '\t' not in line:
                continue
            parts = line.split('\t', 1)
            if len(parts) != 2:
                continue
            try:
                size = int(parts[0])
            except ValueError:
                continue
            path = parts[1]
            # Format dir is the first component
            fmt_dir = path.split('/')[0].lower()
            if fmt_dir in needed_dirs:
                index[fmt_dir].append((path, size))

    print(f"Indexed {sum(len(v) for v in index.values())} files across "
          f"{len(index)} format directories")
    return index


def find_candidates_for_ext(ext, target_folder, index):
    """Find download candidates for a missing extension.
    Returns list of (path, size, filename_to_save_as)."""
    candidates = []
    ext_lower = ext.lower()

    if target_folder == "uade" and ext_lower in UADE_EXT_TO_MODLAND_DIR:
        dirs = UADE_EXT_TO_MODLAND_DIR[ext_lower]
        for d in dirs:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                fname_lower = fname.lower()
                # Check prefix match (e.g. "p61a.songname", "thx.songname")
                prefix = fname_lower.split('.')[0] if '.' in fname_lower else ""
                suffix = fname_lower.rsplit('.', 1)[1] if '.' in fname_lower else ""

                # For TFMX, files are named mdat.xxx and smpl.xxx
                # For prefix formats, the prefix is the extension
                # For suffix formats, the suffix is the extension

                # Skip companion files (smpl.*, *.smp, *.ins, etc.)
                skip_prefixes = ["smpl", "smp", "ins", "sample"]
                if prefix in skip_prefixes:
                    continue
                skip_suffixes = ["smp", "smpl", "ins", "as", "info", "txt"]
                if suffix in skip_suffixes:
                    continue

                # Match by prefix or suffix
                if prefix == ext_lower or suffix == ext_lower:
                    # For test, save with the prefix/suffix form
                    candidates.append((path, size, fname))
                elif ext_lower.startswith("tfmx") or ext_lower.startswith("tfhd"):
                    # TFMX songs are named mdat.xxx  — we need them
                    if prefix == "mdat" or prefix == "mdst":
                        # Rename to ext.songname for test matching
                        songname = fname.split('.', 1)[1] if '.' in fname else fname
                        candidates.append((path, size, f"{ext_lower}.{songname}"))
                elif ext_lower in ("mod15", "mod15_mst", "mod15_ust",
                                    "mod15_st-iv", "mod_doc", "mod_comp",
                                    "mod_flt4", "mod_ntk", "mod_ntk1",
                                    "mod_ntk2", "mod_ntkamp", "mod_adsc4"):
                    # These are all variants of MOD — in Modland they're just
                    # "mod.songname" in Soundtracker/Protracker dirs
                    if prefix == "mod":
                        songname = fname.split('.', 1)[1] if '.' in fname else fname
                        candidates.append((path, size, f"{ext_lower}.{songname}"))
                elif ext_lower == "star":
                    # StarTrekker AM files
                    if suffix == "am" or prefix in ("mod", "mod_flt"):
                        candidates.append((path, size, f"star.{fname}"))
                elif ext_lower in ("sndmon",):
                    # BPSoundMon files have .bss extension normally
                    candidates.append((path, size, f"sndmon.{sanitize_filename(fname)}"))
                elif ext_lower in ("octamed",):
                    # OctaMED files have .mmd0/.mmd1/.mmd2 extensions
                    candidates.append((path, size, f"octamed.{sanitize_filename(fname)}"))
                else:
                    # Generic: just include the file
                    candidates.append((path, size, fname))

    elif target_folder == "adlib" and ext_lower in ADLIB_EXTS:
        info = ADLIB_EXTS[ext_lower]
        # Search by suffix match across relevant directories
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    elif target_folder == "openmpt" and ext_lower in OPENMPT_EXTS:
        info = OPENMPT_EXTS[ext_lower]
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    elif target_folder == "rsn" and ext_lower in RSN_EXTS:
        info = RSN_EXTS[ext_lower]
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    elif target_folder == "sc68" and ext_lower in SC68_EXTS:
        info = SC68_EXTS[ext_lower]
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    elif target_folder == "ao" and ext_lower in AO_EXTS:
        info = AO_EXTS[ext_lower]
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    elif target_folder == "4v" and ext_lower in QUARTET_EXTS:
        info = QUARTET_EXTS[ext_lower]
        for d in info["dirs"]:
            files = index.get(d.lower(), [])
            for path, size in files:
                fname = os.path.basename(path)
                if '.' in fname:
                    suffix = fname.rsplit('.', 1)[1].lower()
                    if suffix == ext_lower:
                        candidates.append((path, size, fname))

    return candidates


# ── Main ─────────────────────────────────────────────────────────────────
def main():
    # Build the full extension list
    all_missing = {}  # ext -> folder
    for folder, exts in {
        "4v":      ["4q"],
        "adlib":   ["a2t", "bmf", "got", "ha2", "hsp", "hsq", "mdi", "mdy",
                    "mkf", "msc", "rac", "rix", "snd", "sop", "wlf"],
        "ao":      ["qsf"],
        "openmpt": ["fst", "j2b", "mmcmp", "nst", "oxm", "ppm", "pt36", "stx",
                    "xpk"],
        "rsn":     ["r64", "rdc", "rds", "rgs", "rps"],
        "sc68":    ["snd"],
        "uade":    list(UADE_EXT_TO_MODLAND_DIR.keys()) + [
            # These had NO mapping above yet
        ],
    }.items():
        for ext in exts:
            all_missing[ext.lower()] = folder

    # Build index
    index = build_index()

    # Find candidates and download
    downloaded = defaultdict(int)
    not_found = []
    skipped_existing = 0

    for ext in sorted(all_missing.keys()):
        folder = all_missing[ext]
        dest_dir = os.path.join(TESTMUS, folder)
        os.makedirs(dest_dir, exist_ok=True)

        # Check if we already have files with this extension
        existing = []
        if os.path.isdir(dest_dir):
            for f in os.listdir(dest_dir):
                fl = f.lower()
                if '.' in fl:
                    suf = fl.rsplit('.', 1)[1]
                    pre = fl.split('.', 1)[0]
                    if suf == ext.lower() or pre == ext.lower():
                        existing.append(f)
        if existing:
            print(f"  [EXIST] {folder}/{ext}: already have {len(existing)} files")
            downloaded[ext] = len(existing)
            skipped_existing += 1
            continue

        candidates = find_candidates_for_ext(ext, folder, index)
        if not candidates:
            not_found.append(f"{folder}/{ext}")
            continue

        # Filter: prefer files between 500 bytes and 200KB
        good = [(p, s, n) for p, s, n in candidates if 500 < s < 200000]
        if not good:
            good = [(p, s, n) for p, s, n in candidates if s > 100]
        if not good:
            good = candidates

        # Sort by size
        good.sort(key=lambda x: x[1])

        # Pick up to MAX_PER_EXT
        picks = good[:MAX_PER_EXT] if len(good) <= MAX_PER_EXT else [
            good[i * len(good) // MAX_PER_EXT]
            for i in range(MAX_PER_EXT)
        ]

        for path, size, save_name in picks:
            if downloaded[ext] >= MAX_PER_EXT:
                break

            save_name = sanitize_filename(save_name)
            dest_path = os.path.join(dest_dir, save_name)

            if os.path.exists(dest_path):
                downloaded[ext] += 1
                continue

            encoded_path = urllib.parse.quote(path, safe='/')
            url = MODLAND_HTTP + encoded_path

            print(f"  [{folder}/{ext}] {save_name} ({size}B) ... ", end="", flush=True)
            if download_file(url, dest_path):
                print("OK")
                downloaded[ext] += 1
            else:
                # Try FTP
                url_ftp = "ftp://ftp.modland.com/pub/modules/" + encoded_path
                if download_file(url_ftp, dest_path, timeout=45):
                    print("OK (FTP)")
                    downloaded[ext] += 1
                else:
                    print("FAILED")

    # Report
    print("\n" + "=" * 60)
    print("DOWNLOAD SUMMARY")
    print("=" * 60)

    full, partial, none_ = 0, 0, 0
    for ext in sorted(all_missing.keys()):
        count = downloaded[ext]
        folder = all_missing[ext]
        if count >= MAX_PER_EXT:
            full += 1
        elif count > 0:
            partial += 1
            print(f"  ⚠ {folder}/{ext}: {count}/{MAX_PER_EXT}")
        else:
            none_ += 1
            print(f"  ✗ {folder}/{ext}: NOT FOUND")

    print(f"\n✓ Fully covered: {full}")
    print(f"⚠ Partially covered: {partial}")
    print(f"✗ Not found: {none_}")
    print(f"  (pre-existing skipped: {skipped_existing})")

    if not_found:
        print(f"\nExtensions not found in Modland:")
        for nf in not_found:
            print(f"  {nf}")

if __name__ == "__main__":
    main()
