#!/usr/bin/env python3
import sys
import urllib.request
import urllib.parse
import re
import html
import time
import os

def scrape_rko(output_file):
    print("Initializing Remix.Kwed.Org (RKO) Scraper...", flush=True)
    
    # Configure custom opener to mimic a real browser
    opener = urllib.request.build_opener()
    opener.addheaders = [('User-agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36')]
    urllib.request.install_opener(opener)

    base_url = "https://remix.kwed.org/"
    
    # 1. Fetch the first page to determine maximum page count
    print("Accessing RKO homepage to determine total pages...", flush=True)
    try:
        req = urllib.request.Request(base_url)
        with urllib.request.urlopen(req, timeout=15) as response:
            first_page_html = response.read().decode('utf-8', errors='ignore')
    except Exception as e:
        print(f"Error accessing RKO homepage: {e}", flush=True)
        sys.exit(1)

    max_page = 105  # reasonable fallback
    page_matches = re.findall(r"page=(\d+)", first_page_html)
    if page_matches:
        max_page = max(int(p) for p in page_matches)
        print(f"Detected total pages to scrape: {max_page}", flush=True)
    else:
        print(f"Could not determine total pages. Falling back to default: {max_page}", flush=True)

    remixes = {}

    # 2. Iterate and scrape pages
    for page in range(1, max_page + 1):
        print(f"Scraping page {page}/{max_page}...", flush=True)
        page_url = f"{base_url}?chart=&view=date&page={page}"
        
        try:
            req = urllib.request.Request(page_url)
            with urllib.request.urlopen(req, timeout=15) as response:
                page_html = response.read().decode('utf-8', errors='ignore')
        except Exception as e:
            print(f"Warning: Failed to fetch page {page}: {e}. Retrying in 2 seconds...", flush=True)
            time.sleep(2)
            try:
                with urllib.request.urlopen(req, timeout=15) as response:
                    page_html = response.read().decode('utf-8', errors='ignore')
            except Exception as retry_e:
                print(f"Error: Retrying page {page} failed. Skipping. Error: {retry_e}", flush=True)
                continue

        # Extract table rows starting with tr id="..."
        rows = re.findall(r'<tr id="[^"]+">[\s\S]*?</tr>', page_html)
        page_count = 0
        
        for row in rows:
            # 1. Extract RKO ID from the download link
            id_m = re.search(r'/download\.php/(\d+)/', row)
            if not id_m:
                # Try fallback standard download link pattern without trailing slash
                id_m = re.search(r'/download\.php/(\d+)', row)
                
            if id_m:
                remix_id = int(id_m.group(1))
                
                # 2. Extract title
                title = "Unknown"
                title_m = re.search(r'class=["\']download["\'][^>]*>([\s\S]*?)</a>', row)
                if title_m:
                    title = html.unescape(title_m.group(1)).strip()
                
                # 3. Extract Remixer/Arranger
                artist = "Unknown"
                artist_m = re.search(r'class=["\']artistEmail["\'][^>]*>([\s\S]*?)</a>', row)
                if artist_m:
                    artist = html.unescape(artist_m.group(1)).strip()
                
                # 4. Extract Rating (from smiley rating image name)
                rating = "4"  # default standard rating
                rating_m = re.search(r'smiley_(\d+)\.svg', row)
                if rating_m:
                    rating = rating_m.group(1)
                
                # 5. Extract SID Path and Subtune from DeepSID URL
                sid_path = "GAMES/C64_Track.sid"  # default placeholder if no SID link exists
                subtune = "1"
                
                sid_m = re.search(r'deepsid\.chordian\.net/\?file=([^"\'&]+)', row)
                if sid_m:
                    sid_path = urllib.parse.unquote(sid_m.group(1)).lstrip('/')
                    subtune_m = re.search(r'subtune=(\d+)', row)
                    if subtune_m:
                        subtune = subtune_m.group(1)
                
                remixes[remix_id] = (sid_path, subtune, title, artist, rating)
                page_count += 1
        
        print(f"  Extracted {page_count} remixes from page {page}.", flush=True)
        time.sleep(0.08) # Polite throttle delay

    # 3. Sort by ID ascending and write to file in ISO-8859-1 (Latin1)
    sorted_ids = sorted(remixes.keys())
    print(f"\nScraping finished. Total unique remixes found: {len(remixes)}", flush=True)
    print(f"Writing to output file (ISO-8859-1): {output_file}...", flush=True)
    
    # Create parent directories if they don't exist
    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    
    with open(output_file, 'w', encoding='iso-8859-1', errors='replace') as f:
        for rid in sorted_ids:
            sid_path, subtune, title, artist, rating = remixes[rid]
            # Format matches rko.txt: ID \t SID_PATH \t SUBTUNE \t TITLE \t ARTIST \t RATING
            f.write(f"{rid}\t{sid_path}\t{subtune}\t{title}\t{artist}\t{rating}\n")
            
    print(f"Successfully generated clean '{output_file}' with {len(sorted_ids)} entries!", flush=True)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 update_rko.py <output_file>")
        sys.exit(1)
        
    scrape_rko(sys.argv[1])
