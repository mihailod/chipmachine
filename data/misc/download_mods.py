import urllib.parse
import subprocess
import os

tasks = [
    ("testmus/gme", "AY Emul/Agent X/agent-x1.emul"),
    ("testmus/gme", "Megadrive GYM/- unknown/Art Alive/art alive.gym"),
    ("testmus/gme", "Nintendo Sound Format/3-108/new rally-x.nsf"),
    ("testmus/gsf", "Gameboy Sound Format/Motoi Sakuraba/Tales Of Phantasia/01 yume wa owaranai.gsf"),
    ("testmus/ht", "Dreamcast Sound Format/Daisuke Ishiwatari/Guilty Gear X (Naomi)/ggx-68.dsf"),
    ("testmus/ht", "Dreamcast Sound Format/Daisuke Ishiwatari/Guilty Gear X (Naomi)/ggx-66-00-01.minidsf"),
    ("testmus/ht", "Saturn Sound Format/Greg Turner/Bug!/w00-00-25.minissf"),
    ("testmus/ht", "Saturn Sound Format/Akinaga Yoshida/Sangokushi Eiketsuden/02 palace.ssf"),
    ("testmus/nds", "Nintendo DS Sound Format/Hideki Naganuma/Sonic Rush/111 right there, ride on (blazy mix).2sf"),
    ("testmus/openmpt", "Composer 669/30 Second Chase/the lunar forest.669"),
    ("testmus/openmpt", "Digibooster Pro/1541/blipp blopp.dbm"),
    ("testmus/openmpt", "Digibooster/Black Dragon/children-dream.digi"),
    ("testmus/openmpt", "X-Tracker/Bomb20/4wd benzs and dickwhiping.dmf"),
    ("testmus/openmpt", "Digital Sound Interface Kit/Necros/andante.dsm"),
    ("testmus/openmpt", "Ad Lib/DeFy AdLib Tracker/Hector/dtm track 2.dtm"),
    ("testmus/openmpt", "General DigiMusic/Bishop/as the seasons change.gdm"),
    ("testmus/openmpt", "Ad Lib/Apogee/Bobby Prince/Bio Menace/antspant.imf"),
    ("testmus/openmpt", "Impulsetracker/006/sweat piano.it"),
    ("testmus/openmpt", "Digitrakker/Aphex/soorya namaskara.mdl"),
    ("testmus/openmpt", "Music Editor/Alex Van Starrex/synth.med"),
]

base_url = "ftp://ftp.modland.com/pub/modules/"

for folder, path in tasks:
    os.makedirs(folder, exist_ok=True)
    filename = os.path.basename(path)
    dest_path = os.path.join(folder, filename)
    
    # URL encode the path part
    encoded_path = urllib.parse.quote(path)
    url = base_url + encoded_path
    
    print(f"Downloading {url} to {dest_path}...")
    try:
        subprocess.run(["curl", "-L", "-o", dest_path, url], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to download {url}: {e}")

