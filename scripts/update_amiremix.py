#!/usr/bin/env python3
import sys
import urllib.request
import re
import html
import time
import os

def scrape_amiremix(output_file):
    print("Initializing AmigaRemix Scraper...", flush=True)
    
    # Configure custom opener to mimic a real browser
    opener = urllib.request.build_opener()
    opener.addheaders = [('User-agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36')]
    urllib.request.install_opener(opener)

    base_url = "https://www.amigaremix.com/"
    
    # 1. Fetch the first page to get maximum page count
    print("Accessing homepage to determine total pages...", flush=True)
    try:
        req = urllib.request.Request(base_url)
        with urllib.request.urlopen(req, timeout=15) as response:
            first_page_html = response.read().decode('utf-8', errors='ignore')
    except Exception as e:
        print(f"Error accessing AmigaRemix homepage: {e}", flush=True)
        sys.exit(1)

    max_page = 43  # reasonable fallback
    page_match = re.search(r"/\s*(\d+)</span>\s*</div>\s*<div\s+class='col-4 justify-content-end", first_page_html)
    if page_match:
        max_page = int(page_match.group(1))
        print(f"Detected total pages to scrape: {max_page}", flush=True)
    else:
        # Try alternate select-based extraction if format changed slightly
        select_match = re.findall(r"<option value='(\d+)'>", first_page_html)
        if select_match:
            max_page = max(int(val) for val in select_match)
            print(f"Detected total pages via select: {max_page}", flush=True)
        else:
            print(f"Could not determine total pages. Falling back to default: {max_page}", flush=True)

    remixes = {}

    # 2. Iterate and scrape pages
    for page in range(1, max_page + 1):
        print(f"Scraping page {page}/{max_page}...", flush=True)
        page_url = f"{base_url}?p={page}"
        
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

        # Extract opening a-tags of class surfer (may contain newlines)
        a_tags = re.findall(r'<a\s+[^>]*?class=["\']surfer["\'][\s\S]*?>', page_html)
        page_count = 0
        
        for tag in a_tags:
            href_m = re.search(r'href=["\']([^"\']+)["\']', tag)
            id_m = re.search(r'data-id=["\'](\d+)["\']', tag)
            artist_m = re.search(r'data-track-artist=["\']([^"\']*)["\']', tag)
            title_m = re.search(r'data-track-title=["\']([^"\']*)["\']', tag)
            
            if href_m and id_m and artist_m and title_m:
                remix_id = int(id_m.group(1))
                rel_link = href_m.group(1)
                
                # Normalize relative links to match C++ provider expected source prefix stripping.
                # The scheme MUST match db.lua's amigaremix `source` exactly -- the strip in
                # MusicDatabase::parseStandard is a plain prefix match, so http:// here against
                # an https:// source would silently store the absolute URL as the song path.
                if rel_link.startswith('/'):
                    link = "https://www.amigaremix.com" + rel_link
                else:
                    link = rel_link
                
                # Unescape HTML entities
                title = html.unescape(title_m.group(1)).strip()
                artist = html.unescape(artist_m.group(1)).strip()
                
                remixes[remix_id] = (link, title, artist)
                page_count += 1
        
        print(f"  Extracted {page_count} remixes from page {page}.", flush=True)
        time.sleep(0.15) # Polite throttle delay

    # 3. Sort by ID ascending and write to file
    sorted_ids = sorted(remixes.keys())
    print(f"\nScraping finished. Total unique remixes found: {len(remixes)}", flush=True)
    print(f"Writing to output file: {output_file}...", flush=True)
    
    # Create parent directories if they don't exist
    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        for rid in sorted_ids:
            link, title, artist = remixes[rid]
            # Write tab-separated format
            f.write(f"{rid}\t{link}\t{title}\t{artist}\n")
            
    print(f"Successfully generated clean '{output_file}' with {len(sorted_ids)} entries!", flush=True)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 update_amiremix.py <output_file>")
        sys.exit(1)
        
    scrape_amiremix(sys.argv[1])
