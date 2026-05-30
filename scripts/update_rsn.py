#!/usr/bin/env python3
"""
Scrapes snesmusic.org to regenerate chipmachine/data/rsn.txt
Format: Game Title\tComposer(s)\tSuper Nintendo\tfilename.rsn

Usage:
    python3 scrape_snesmusic.py
    python3 scrape_snesmusic.py --output rsn.txt
    python3 scrape_snesmusic.py --delay 1.5   # seconds between requests (default 1.0)
    python3 scrape_snesmusic.py --resume       # skip set IDs already in cache file
"""

import re
import sys
import time
import json
import argparse
import urllib.request
import urllib.error
from pathlib import Path
from html.parser import HTMLParser

BASE = "https://snesmusic.org/v2"
CACHE_FILE = "rsn_cache.json"  # intermediate cache so you can resume on failure
USER_AGENT = "Mozilla/5.0 (compatible; rsn-updater/1.0)"

# All alphabet chars the site uses
CHARS = ["n1-9"] + list("ABCDEFGHIJKLMNOPQRSTUVWXYZ")


def fetch(url, delay=1.0):
    """Fetch URL with polite delay and return HTML string."""
    time.sleep(delay)
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return r.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        print(f"  HTTP {e.code}: {url}", file=sys.stderr)
        return ""
    except Exception as e:
        print(f"  Error fetching {url}: {e}", file=sys.stderr)
        return ""


# ── HTML parsers ──────────────────────────────────────────────────────────────

class SetListParser(HTMLParser):
    """Parse select.php?view=sets listing — extract set IDs."""
    def __init__(self):
        super().__init__()
        self.ids = []

    def handle_starttag(self, tag, attrs):
        if tag == "a":
            d = dict(attrs)
            href = d.get("href", "")
            m = re.search(r"profile\.php\?profile=set&selected=(\d+)", href)
            if m:
                sid = int(m.group(1))
                if sid not in self.ids:
                    self.ids.append(sid)


def strip_tags(html):
    """Remove all HTML tags and decode common entities."""
    text = re.sub(r"<[^>]+>", "", html)
    for ent, ch in [("&amp;","&"),("&lt;","<"),("&gt;",">"),("&#039;","'"),("&quot;",'"'),("&nbsp;"," ")]:
        text = text.replace(ent, ch)
    return text.strip()


def parse_profile(html):
    """Extract title, composers, project from a raw profile page HTML string."""
    # Project name from download link (most reliable)
    m = re.search(r'download\.php\?spcNow=([A-Za-z0-9_-]+)', html)
    if not m:
        return None
    project = m.group(1)

    # Title: find h2 that contains a region img (alt='NTSC' etc.) followed by game name text
    title = ""
    region_re = re.compile(r'^(NTSC-J|NTSC|PAL|SatellaView|Beta|SGB)$', re.IGNORECASE)
    for h2_inner in re.findall(r'<h2[^>]*>(.*?)</h2>', html, re.DOTALL | re.IGNORECASE):
        # Check if the h2 contains a region image (alt="NTSC" etc.)
        img_alt = re.search(r'alt=[^>]*?([A-Za-z0-9-]+)[^>]*>', h2_inner, re.IGNORECASE)
        if img_alt and region_re.match(img_alt.group(1).strip()):
            # Strip all tags and the region word to get the title
            text = strip_tags(h2_inner).strip()
            # Remove leading region prefix if present as text too
            text = re.sub(r'^(NTSC-J|NTSC|PAL|SatellaView|Beta|SGB)\s*', '', text, flags=re.IGNORECASE).strip()
            if text:
                title = text
                break
    # Fallback: page <title> tag "Game profile: ActRaiser ~ SNESmusic.org"
    if not title:
        m2 = re.search(r'<title[^>]*>Game profile:\s*(.+?)\s*~', html, re.IGNORECASE)
        if m2:
            title = m2.group(1).strip()

    # Composers: label is a <td> (not <th>), value is the next <td> in the same <tr>
    # Pattern: <td>Composer: </td><td>...<a>Name</a>...</td>
    composers = []
    comp_m = re.search(
        r'<td>\s*Composers?\s*:?\s*</td>\s*<td[^>]*>(.*?)</td>',
        html, re.DOTALL | re.IGNORECASE
    )
    if comp_m:
        raw = strip_tags(comp_m.group(1))
        SKIP = {"n/a", "unknown", "?", "-", ""}
        composers = [c.strip() for c in raw.split(",") if c.strip() and c.strip().lower() not in SKIP]

    return {"project": project, "title": title, "composers": composers}


def scrape_set_ids(delay):
    """Return all set IDs from all alphabet listing pages, paginating as needed."""
    all_ids = []
    seen = set()
    for char in CHARS:
        offset = 0
        while True:
            url = f"{BASE}/select.php?view=sets&char={char}&limit={offset}"
            print(f"Listing page: {url}")
            html = fetch(url, delay)
            parser = SetListParser()
            parser.feed(html)
            new_ids = [sid for sid in parser.ids if sid not in seen]
            for sid in new_ids:
                seen.add(sid)
                all_ids.append(sid)
            # Paginate: look for "Results X-Y of Z"
            m = re.search(r'Results\s+\d+-(\d+)\s+of\s+(\d+)', html)
            if m:
                end, total = int(m.group(1)), int(m.group(2))
                if end < total:
                    offset = end
                    continue
            break
    print(f"Found {len(all_ids)} unique set IDs")
    return all_ids


def scrape_profile(sid, delay):
    """Fetch profile page for set ID; return dict or None."""
    url = f"{BASE}/profile.php?profile=set&selected={sid}"
    html = fetch(url, delay)
    if not html:
        return None

    rec = parse_profile(html)
    if not rec:
        return None

    return {
        "id": sid,
        "title": rec["title"],
        "composers": rec["composers"],
        "project": rec["project"],
        "rsn": rec["project"] + ".rsn",
    }


def format_composers(composers, max_len=30):
    """Join composers with ', ', truncating to fit the original style."""
    s = ", ".join(composers)
    if len(s) > max_len and len(composers) > 1:
        # Truncate like the original: "Jun Funahashi, Masanari Iwata, M"
        s = s[:max_len]
        # Don't end mid-word badly; keep it as-is (matches original style)
    return s


def build_rsn_txt(records):
    """Return the rsn.txt content from a list of record dicts."""
    lines = []
    for r in sorted(records, key=lambda x: x["title"].lower()):
        title = r["title"]
        composers = format_composers(r["composers"])
        rsn = r["rsn"]
        line = f"\t{title}\t{composers}\tSuper Nintendo\t{rsn}"
        lines.append(line)
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser(description="Regenerate rsn.txt from snesmusic.org")
    ap.add_argument("--output", default="rsn_new.txt", help="Output file (default: rsn_new.txt)")
    ap.add_argument("--delay", type=float, default=1.0, help="Delay between requests in seconds")
    ap.add_argument("--resume", action="store_true", help="Resume from cache file if it exists")
    ap.add_argument("--ids-only", action="store_true", help="Only collect set IDs (first pass)")
    ap.add_argument("--limit", type=int, default=0, help="Stop after this many profiles (0 = all)")
    ap.add_argument("--from-index", type=int, default=0, help="Start at this 0-based index in the ID list")
    ap.add_argument("--debug-id", type=int, default=0, help="Fetch one set ID, print raw HTML + parsed result, then exit")
    args = ap.parse_args()

    if args.debug_id:
        url = f"{BASE}/profile.php?profile=set&selected={args.debug_id}"
        print(f"Fetching: {url}")
        html = fetch(url, delay=0)
        print("\n--- All 'composer' occurrences ---")
        for m in re.finditer(r'composer', html, re.IGNORECASE):
            start = max(0, m.start() - 100)
            print(repr(html[start:m.start() + 300]))
            print("---")
        print("\n--- First table HTML ---")
        tbl = re.search(r'<table[^>]*>(.*?)</table>', html, re.DOTALL | re.IGNORECASE)
        if tbl:
            print(repr(tbl.group()[:2000]))
        print("\n--- parse_profile result ---")
        print(parse_profile(html))
        return

    cache_path = Path(CACHE_FILE)

    # Load or create cache
    cache = {}
    if args.resume and cache_path.exists():
        with open(cache_path) as f:
            cache = json.load(f)
        print(f"Loaded {len(cache)} cached records")

    # Get all set IDs
    ids_cache_file = Path("rsn_ids.json")
    if args.resume and ids_cache_file.exists():
        with open(ids_cache_file) as f:
            all_ids = json.load(f)
        print(f"Loaded {len(all_ids)} set IDs from cache")
    else:
        all_ids = scrape_set_ids(args.delay)
        with open(ids_cache_file, "w") as f:
            json.dump(all_ids, f)

    if args.ids_only:
        print(f"IDs saved to {ids_cache_file}")
        return

    # Scrape each profile
    records = list(cache.values())
    cached_ids = set(str(r["id"]) for r in records)
    todo = [sid for sid in all_ids if str(sid) not in cached_ids]
    if args.from_index:
        todo = todo[args.from_index:]
        print(f"Skipping first {args.from_index} uncached IDs (--from-index)")
    if args.limit:
        todo = todo[:args.limit]
    print(f"Will scrape {len(todo)} profiles ({len(cached_ids)} already cached)")

    for i, sid in enumerate(todo):
        print(f"  [{i+1}/{len(todo)}] set {sid}", end=" ")
        rec = scrape_profile(sid, args.delay)
        if rec:
            print(f"  -> {rec['project']}.rsn  '{rec['title']}'")
            cache[str(sid)] = rec
        else:
            print("→ skipped (no project name found)")

        # Save cache every 50 records
        if (i + 1) % 50 == 0:
            with open(cache_path, "w") as f:
                json.dump(cache, f, indent=2)
            print(f"  Cache saved ({len(cache)} records)")

    # Final cache save
    with open(cache_path, "w") as f:
        json.dump(cache, f, indent=2)

    # Build output — deduplicate by project name (multiple set IDs can share one .rsn)
    seen_projects = set()
    unique_records = []
    for r in cache.values():
        p = r.get("project", "")
        if p and p not in seen_projects:
            seen_projects.add(p)
            unique_records.append(r)

    content = build_rsn_txt(unique_records)
    with open(args.output, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"\nWrote {len(unique_records)} unique entries ({len(cache)} total set IDs) to {args.output}")
    print(f"Run: diff chipmachine/data/rsn.txt {args.output}  to see changes")


if __name__ == "__main__":
    main()
