import sys
import os
import re

extensions = [
    (".hcs", "testmus/adlib"),
    (".hsp", "testmus/adlib"),
    (".mid", "testmus/adlib"),
    (".msc", "testmus/adlib"),
    (".rix", "testmus/adlib"),
    (".qsf", "testmus/ao"),
    (".psg", "testmus/zx"),
    (".emul", "testmus/gme"),
    (".gym", "testmus/gme"),
    (".nsf", "testmus/gme"),
    (".vgm", "testmus/gme"),
    (".gsf", "testmus/gsf"),
    (".dsf", "testmus/ht"),
    (".minidsf", "testmus/ht"),
    (".minissf", "testmus/ht"),
    (".ssf", "testmus/ht"),
    (".2sf", "testmus/nds"),
    (".669", "testmus/openmpt"),
    (".c67", "testmus/openmpt"),
    (".dbm", "testmus/openmpt"),
    (".digi", "testmus/openmpt"),
    (".dmf", "testmus/openmpt"),
    (".dsm", "testmus/openmpt"),
    (".dtm", "testmus/openmpt"),
    (".gdm", "testmus/openmpt"),
    (".ice", "testmus/openmpt"),
    (".imf", "testmus/openmpt"),
    (".it", "testmus/openmpt"),
    (".j2b", "testmus/openmpt"),
    (".m15", "testmus/openmpt"),
    (".mdl", "testmus/openmpt"),
    (".med", "testmus/openmpt"),
    (".mmcmp", "testmus/openmpt"),
    (".mms", "testmus/openmpt"),
    (".mo3", "testmus/openmpt"),
    (".mptm", "testmus/openmpt"),
    (".mt2", "testmus/openmpt"),
    (".mtm", "testmus/openmpt"),
    (".nst", "testmus/openmpt"),
    (".okt", "testmus/openmpt"),
]

matches = {}
with open("data/allmods.txt", "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        line = line.strip()
        if not line: continue
        parts = line.split("\t", 1)
        if len(parts) < 2:
            # Try splitting by multiple spaces
            parts = re.split(r'\s+', line, 1)
        if len(parts) < 2: continue
        
        path = parts[1]
        path_lower = path.lower()
        for ext, folder in extensions:
            if ext in matches: continue
            if path_lower.endswith(ext.lower()):
                matches[ext] = (path, folder)
                
for ext, folder in extensions:
    if ext in matches:
        path, folder = matches[ext]
        print(f"MATCH|{ext}|{folder}|{path}")
    else:
        print(f"MISS|{ext}")
