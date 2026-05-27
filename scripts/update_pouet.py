import json
import urllib.request
import re
import gzip
import lzma
import os
import sys

opener = urllib.request.build_opener()
opener.addheaders = [('User-agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36')]
urllib.request.install_opener(opener)

BASE_URL = "https://data.pouet.net/"

print("Scraping live index from data.pouet.net to locate production dumps...")
try:
    req = urllib.request.Request(BASE_URL)
    with urllib.request.urlopen(req) as response:
        html = response.read().decode('utf-8')
except Exception as e:
    print(f"Network error accessing root repository index: {e}")
    sys.exit(1)

found_links = re.findall(r'href=["\']([^"\']+\.(?:gz|xz))["\']', html)
prod_links = [link for link in found_links if "prods" in link.lower()]

if not prod_links:
    print("Error: Could not isolate an active production dump link on the page index.")
    sys.exit(1)

target_link = sorted(list(set(prod_links)), reverse=True)[0]

if not target_link.startswith("http"):
    download_url = BASE_URL + target_link.lstrip('/')
else:
    download_url = target_link

local_filename = os.path.basename(download_url)
file_ext = "xz" if local_filename.endswith(".xz") else "gz"

print(f"Target located: {download_url}")
print(f"Downloading stream payload to local disk: {local_filename}...")

try:
    urllib.request.urlretrieve(download_url, local_filename)
except Exception as e:
    print(f"Failed to pull data archive from remote server: {e}")
    sys.exit(1)

print(f"Streaming data out of {local_filename} into ChipMachine schema...")
if not os.path.exists(local_filename):
    print("Verification failure: Downloaded asset missing from filesystem workspace.")
    sys.exit(1)

open_func = lzma.open if file_ext == "xz" else gzip.open

count = 0
try:
    with open_func(local_filename, "rt", encoding="utf-8") as f:
        data = json.load(f)
        
    prods_list = data.get("prods", []) if isinstance(data, dict) else data

    with open("pouet.txt", "w", encoding="utf-8") as out:
        for prod in prods_list:
            if not isinstance(prod, dict):
                continue
                
            all_candidate_urls = []
            
            # Extract basic flat download link string
            download_val = prod.get("download")
            if isinstance(download_val, str) and download_val:
                all_candidate_urls.append(download_val)

            # Extract target link arrays matching verified schema structures
            for key in ["downloadLinks", "externalLinks", "links"]:
                val = prod.get(key)
                if isinstance(val, list):
                    for item in val:
                        if isinstance(item, dict):
                            for sub_key in ["link", "url"]:
                                sub_val = item.get(sub_key)
                                if isinstance(sub_val, str):
                                    all_candidate_urls.append(sub_val)

            all_candidate_urls = list(set(all_candidate_urls))
            yt_url = next((link for link in all_candidate_urls if "youtube.com" in link or "youtu.be" in link), None)
            
            if not yt_url:
                continue
                
            title = prod.get("name", "Unknown").replace("\t", " ").strip()
            
            # Parse groups array
            groups_list = prod.get("groups", [])
            if isinstance(groups_list, list):
                group_name = "+".join([g.get("name", "") for g in groups_list if isinstance(g, dict) and g.get("name")])
            else:
                group_name = "Unknown"
            group_name = group_name.replace("\t", " ").strip() if group_name else "Unknown"
            
            # Parse platforms dictionary structure
            platform_names = []
            platforms_obj = prod.get("platforms")
            
            if isinstance(platforms_obj, dict):
                # Handle dictionary of dictionaries layout
                for p_id, p_info in platforms_obj.items():
                    if isinstance(p_info, dict) and p_info.get("name"):
                        platform_names.append(p_info.get("name"))
            elif isinstance(platforms_obj, list):
                # Fallback backup for varying legacy dump formats
                for p in platforms_obj:
                    if isinstance(p, dict) and p.get("name"):
                        platform_names.append(p.get("name"))
                    elif isinstance(p, str):
                        platform_names.append(p)
                        
            platform = ",".join(list(set(platform_names))) if platform_names else "Unknown"
            platform_str = f"Youtube ({platform})"
            
            try:
                prod_id = int(prod["id"])
            except (ValueError, KeyError, TypeError):
                continue
                
            dir_prefix = f"{prod_id // 1000:05d}"
            file_suffix = f"{prod_id:08d}.jpg"
            img_path = f"{dir_prefix}/{file_suffix}"
            
            # Double tab structural format matching parseStandard offsets
            out.write(f"{title}\t\t{group_name}\t{platform_str}\t{yt_url}\t{img_path}\n")
            count += 1

except Exception as e:
    print(f"\nProcessing exception thrown: {e}")
    sys.exit(1)

if os.path.exists(local_filename):
    os.remove(local_filename)

print(f"\nSuccess! Generated modern 'pouet.txt' containing {count} verified production indices.")
