#!/usr/bin/env python3
"""Quick scan to see which missing extensions exist in Modland (dry run)."""

import os
from collections import defaultdict

PROJ   = os.path.dirname(os.path.abspath(__file__))
DATA   = os.path.join(os.path.dirname(PROJ), "chipmachine", "data")
ALLMODS = os.path.join(DATA, "allmods.txt")

MISSING = {
    "4v":      ["4q"],
    "adlib":   ["a2t", "bmf", "got", "ha2", "hsp", "hsq", "mdi", "mdy",
                "mkf", "msc", "rac", "rix", "snd", "sop", "wlf"],
    "ao":      ["qsf"],
    "openmpt": ["fst", "j2b", "mmcmp", "nst", "oxm", "ppm", "pt36", "stx",
                "xpk"],
    "rsn":     ["r64", "rdc", "rds", "rgs", "rps"],
    "sc68":    ["snd"],
    "uade":    [
        "!pm!", "40a", "40b", "41a", "50a", "60a", "61a", "ac1", "ac1d",
        "adpcm", "agi", "aon4", "aon8", "aval", "bfc", "bye", "cin",
        "cplx", "crb", "custom", "di", "dl_deli", "dlm1", "dlm2", "dmu2",
        "dwold", "emsv6", "eu", "fc-bsi", "fc-m", "fc3", "fc4", "fcm",
        "fuz", "gm", "gv", "hmc", "hn", "hrt", "hrt!", "it1", "jcb",
        "jcbo", "jp", "jpnd", "jpold", "js", "kef7", "kim", "krs", "lax",
        "mcmd_org", "mdst", "mexxmp", "mkiio", "mod15", "mod15_mst",
        "mod15_st-iv", "mod15_ust", "mod_adsc4", "mod_comp", "mod_doc",
        "mod_flt4", "mod_ntk", "mod_ntk1", "mod_ntk2", "mod_ntkamp",
        "mon_old", "mosh", "mpro", "mtp2", "mug2", "mw", "noisepacker2",
        "noisepacker3", "np", "np1", "np2", "np3", "npp", "nru", "ntpk",
        "octamed", "p10", "p21", "p30", "p40a", "p40b", "p41a", "p4x",
        "p5a", "p5x", "p60", "p60a", "p61", "p61a", "p6x", "pat", "pm0",
        "pm1", "pm10c", "pm18a", "pm2", "pm20", "pm4", "pm40", "pmz",
        "polk", "powt", "pp20", "pp30", "ppk", "pru", "pru1", "prun",
        "prun1", "prun2", "pwr", "pyg", "pygm", "pygmy", "qc", "rj",
        "rjp", "rkb", "s-c", "s7g", "sa-p", "sa_old", "sct", "sfx13",
        "sid1", "skyt", "sm1", "sm2", "smn", "sndmon", "snt", "snt!",
        "snx", "ss", "st26", "st30", "star", "stpk", "tfhd1.5", "tfhd7v",
        "tfhdpro", "tfmx", "tfmx1.5", "tfmx7v", "tfmxpro", "thn", "thx",
        "tits", "tmk", "tp1", "tp2", "tp3", "trc", "tro", "un2", "unic",
        "unic2", "wn", "xan",
    ],
}

EXT_TO_FOLDER = {}
for folder, exts in MISSING.items():
    for ext in exts:
        EXT_TO_FOLDER[ext.lower()] = folder

ALL_MISSING = set(EXT_TO_FOLDER.keys())

results = defaultdict(list)  # ext -> [(path, size)]

print(f"Scanning {ALLMODS} ({os.path.getsize(ALLMODS)/1e6:.1f} MB) ...")
linecount = 0
with open(ALLMODS, 'r', encoding='utf-8', errors='replace') as f:
    for line in f:
        linecount += 1
        line = line.strip()
        if not line or '\t' not in line:
            continue
        parts = line.split('\t', 1)
        if len(parts) != 2:
            continue
        size_str, path = parts
        try:
            size = int(size_str)
        except ValueError:
            continue

        filename = os.path.basename(path)
        # Suffix: file.ext
        if '.' in filename:
            suf = filename.rsplit('.', 1)[1].lower()
            pre = filename.split('.', 1)[0].lower()
        else:
            continue

        for ext in [suf, pre]:
            if ext in ALL_MISSING:
                if len(results[ext]) < 20:  # cap at 20 candidates
                    results[ext].append((path, size))

print(f"Scanned {linecount} lines.\n")

found_count = 0
not_found = []
for ext in sorted(ALL_MISSING):
    folder = EXT_TO_FOLDER[ext]
    matches = results.get(ext, [])
    if matches:
        found_count += 1
        # Show first match as example
        example = matches[0]
        print(f"  ✓ {folder}/{ext}: {len(matches)} files  (e.g. {example[0][:80]})")
    else:
        not_found.append(f"{folder}/{ext}")

print(f"\n{'='*60}")
print(f"Found in Modland: {found_count}/{len(ALL_MISSING)}")
print(f"NOT found: {len(not_found)}")
for nf in not_found:
    print(f"  ✗ {nf}")
