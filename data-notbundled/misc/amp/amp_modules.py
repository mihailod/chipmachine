import csv
import time
import re
import os
import urllib.request
import urllib.parse

PROGRESS_FILE = "PROGRESS.csv"
MODULES_FILE = "MODULES.csv"
BASE_URL = "https://amp.dascene.net/newresult.php?request=module&search=&position={position}"
MAX_REQUESTS = 1000
STEP = 50
DELAY = 2
LIMIT = 178000

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
    "Accept-Encoding": "identity",
    "Connection": "keep-alive",
    "Referer": "https://amp.dascene.net/",
    "DNT": "1",
    "Upgrade-Insecure-Requests": "1",
}


def read_last_position():
    if not os.path.exists(PROGRESS_FILE) or os.path.getsize(PROGRESS_FILE) == 0:
        return 0
    last = 0
    with open(PROGRESS_FILE, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                last = int(row["POSITION"])
            except (ValueError, KeyError):
                pass
    return last


def append_position(position):
    file_exists = os.path.exists(PROGRESS_FILE) and os.path.getsize(PROGRESS_FILE) > 0
    with open(PROGRESS_FILE, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["POSITION"])
        if not file_exists:
            writer.writeheader()
        writer.writerow({"POSITION": position})


def append_modules(rows):
    fieldnames = ["MODULE_ID", "MODULE_TITLE", "COMPOSER", "COMPOSER_DETAIL_URL", "FORMAT", "SIZE", "DL"]
    file_exists = os.path.exists(MODULES_FILE) and os.path.getsize(MODULES_FILE) > 0
    with open(MODULES_FILE, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerows(rows)


def decode_html_entities(text):
    text = text.replace("&nbsp;", " ")
    text = text.replace("&amp;", "&")
    text = text.replace("&lt;", "<")
    text = text.replace("&gt;", ">")
    text = text.replace("&quot;", '"')
    text = text.replace("&#39;", "'")
    return text


def strip_tags(html):
    return re.sub(r"<[^>]+>", "", html)


def clean_title(text):
    text = decode_html_entities(text)
    text = strip_tags(text)
    return text.strip()


def parse_query_param(href, param):
    href = href.replace("&amp;", "&")
    parsed = urllib.parse.parse_qs(urllib.parse.urlparse(href).query)
    values = parsed.get(param, [None])
    return values[0] if values else None


def parse_modules(html):
    rows = []

    header_pattern = re.compile(
        r"<tr[^>]*>\s*<th>Module</th>\s*<th>Composer</th>", re.IGNORECASE
    )
    if not header_pattern.search(html):
        return rows

    row_pattern = re.compile(
        r'<tr\s+class="tr\d+">(.*?)</tr>', re.IGNORECASE | re.DOTALL
    )
    td_pattern = re.compile(r"<td[^>]*>(.*?)</td>", re.IGNORECASE | re.DOTALL)
    link_pattern = re.compile(r'<a\s+href="([^"]*)"[^>]*>(.*?)</a>', re.IGNORECASE | re.DOTALL)

    for row_match in row_pattern.finditer(html):
        row_html = row_match.group(1)
        tds = td_pattern.findall(row_html)
        if len(tds) < 5:
            continue

        module_td = tds[0]
        link_match = link_pattern.search(module_td)
        if not link_match:
            continue

        module_href = link_match.group(1)
        module_inner = link_match.group(2)
        module_id = parse_query_param(module_href, "index")
        module_title = clean_title(module_inner)

        composer_td = tds[1]
        composer_link = link_pattern.search(composer_td)
        if composer_link:
            composer_url = composer_link.group(1)
            composer = clean_title(composer_link.group(2))
        else:
            composer_url = ""
            composer = clean_title(strip_tags(composer_td))

        fmt = clean_title(strip_tags(tds[2]))
        size = clean_title(strip_tags(tds[3]))
        dl = clean_title(strip_tags(tds[4]))

        rows.append({
            "MODULE_ID": module_id,
            "MODULE_TITLE": module_title,
            "COMPOSER": composer,
            "COMPOSER_DETAIL_URL": composer_url,
            "FORMAT": fmt,
            "SIZE": size,
            "DL": dl,
        })

    return rows


def fetch(url):
    req = urllib.request.Request(url, headers=HEADERS)
    with urllib.request.urlopen(req, timeout=30) as resp:
        charset = resp.headers.get_content_charset() or "utf-8"
        return resp.read().decode(charset, errors="replace")


def main():
    position = read_last_position()
    print(f"Starting from position {position}")

    for i in range(MAX_REQUESTS):
        if position > LIMIT:
            print(f"[{LIMIT} Limit reached position {position}")
            break

        url = BASE_URL.format(position=position)
        print(f"[{i+1}/{MAX_REQUESTS}] Fetching position {position}")

        try:
            html = fetch(url)
        except Exception as e:
            print(f"  Request failed: {e}")
            break

        rows = parse_modules(html)
        if rows:
            append_modules(rows)
            print(f"  Parsed {len(rows)} module(s)")
        else:
            print("  No data rows found — may have reached the end")

        position += STEP
        append_position(position)

        if i < MAX_REQUESTS - 1:
            time.sleep(DELAY)

    print(f"Done. Next position is {position} (saved to {PROGRESS_FILE})")


if __name__ == "__main__":
    main()
