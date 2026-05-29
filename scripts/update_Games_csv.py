#!/usr/bin/env python3
"""
fetch_gb64_games.py
-------------------
Downloads the GameBase64 v19 SQLite database and exports the Games
table to a CSV matching chipmachine's data/Games.csv format.

Source: http://www.twinbirds.com/gamebase64browser/GBC_v19.sqlitedb.gz
This is a pre-converted SQLite version of GBC_V19.mdb maintained by
the GameBase64 Browser project — no mdbtools or 7-Zip required.

Usage:
    python3 fetch_gb64_games.py <output_path>

Example:
    python3 fetch_gb64_games.py chipmachine-as/chipmachine/data/Games_new.csv

Requirements: Python 3.6+ (stdlib only — sqlite3, gzip, csv, urllib)
No Homebrew packages needed.
"""

import sys
import os
import csv
import gzip
import shutil
import sqlite3
import tempfile
import urllib.request
import urllib.error

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SQLITE_GZ_URL = "http://www.twinbirds.com/gamebase64browser/GBC_v19.sqlitedb.gz"
TABLE         = "Games"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def download_with_progress(url: str, dest_path: str) -> None:
    print(f"Downloading: {url}")
    print(f"Destination: {dest_path}")
    try:
        with urllib.request.urlopen(url) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            chunk_size = 256 * 1024
            with open(dest_path, "wb") as f:
                while True:
                    buf = resp.read(chunk_size)
                    if not buf:
                        break
                    f.write(buf)
                    downloaded += len(buf)
                    if total:
                        pct = downloaded / total * 100
                        print(f"  {downloaded/1_048_576:.1f} / {total/1_048_576:.1f} MB"
                              f"  ({pct:.0f}%)", end="\r", flush=True)
            print()
    except urllib.error.URLError as e:
        print(f"\nERROR: Download failed: {e}")
        sys.exit(1)
    print(f"Download complete — {os.path.getsize(dest_path)/1_048_576:.1f} MB")


def decompress_gz(gz_path: str, out_path: str) -> None:
    print(f"Decompressing → {out_path} ...")
    with gzip.open(gz_path, "rb") as f_in, open(out_path, "wb") as f_out:
        shutil.copyfileobj(f_in, f_out)
    print(f"SQLite DB: {os.path.getsize(out_path)/1_048_576:.1f} MB")


def list_tables(db_path: str) -> list:
    con = sqlite3.connect(db_path)
    cur = con.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")
    tables = [row[0] for row in cur.fetchall()]
    con.close()
    return tables


def export_to_csv(db_path: str, table: str, output_path: str) -> None:
    """
    Export the SQLite table to CSV with quoting that exactly matches the
    original chipmachine Games.csv produced by the GameBase64 Access frontend.

    Quoting rule (derived by analysing the original file):
      - TEXT columns : always double-quoted, even when empty
      - INTEGER columns : never quoted, even when the value contains special chars
      - NULL/empty TEXT  : written as "" (quoted empty string)
      - NULL/empty INT   : written as  (unquoted empty field)
      - Embedded double-quotes in TEXT values are doubled ("") per RFC 4180

    TEXT columns (by index, 0-based):
      1  Name              3  Filename          4  FileToRun
      6  ScrnshotFilename  12 SidFilename       13 DateLastPlayed
      16 HighScore         32 MemoText          40 Gemus
      42 Comment           43 V_Comment         52 WebLink_Name
      53 WebLink_URL       54 V_WebLink_Name    55 V_WebLink_URL

    All other 46 columns are INTEGER.
    """
    # TEXT column indices (0-based) — derived from original file analysis
    TEXT_COLS = {1, 3, 4, 6, 12, 13, 16, 32, 40, 42, 43, 52, 53, 54, 55}

    print(f"Exporting '{table}' → {output_path} ...")

    # The SQLite DB contains Latin-1 encoded strings (e.g. German umlauts).
    # text_factory=bytes lets us read every row without decode errors; we
    # then decode each value ourselves: UTF-8 first, fall back to Latin-1.
    con = sqlite3.connect(db_path)
    con.text_factory = bytes
    con.row_factory = sqlite3.Row
    cur = con.execute(f"SELECT * FROM [{table}]")

    rows = cur.fetchall()
    if not rows:
        print(f"ERROR: Table '{table}' is empty or not found.")
        con.close()
        sys.exit(1)

    columns = [
        c.decode("utf-8") if isinstance(c, bytes) else c
        for c in rows[0].keys()
    ]

    def decode_val(v):
        """Decode raw bytes to str; return empty string for None."""
        if v is None:
            return ""
        if isinstance(v, bytes):
            try:
                return v.decode("utf-8")
            except UnicodeDecodeError:
                return v.decode("latin-1")
        return str(v)

    def quote_field(value: str, is_text: bool) -> str:
        """
        Produce the exact bytes that should appear in the CSV for this field.
        TEXT non-empty → wrapped in double-quotes; internal " doubled.
        TEXT empty     → bare empty (matches original: ,, not ,"",)
        INT            → bare value, no quotes.
        """
        if is_text and value != "":
            escaped = value.replace('"', '""')
            return f'"{escaped}"'
        else:
            return value

    with open(output_path, "w", newline="", encoding="utf-8") as f:
        # Header row — column names are never quoted (matches original)
        f.write(",".join(columns) + "\n")

        for row in rows:
            decoded = [decode_val(v) for v in row]
            fields = [
                quote_field(decoded[i], i in TEXT_COLS)
                for i in range(len(decoded))
            ]
            f.write(",".join(fields) + "\n")

    con.close()

    with open(output_path, "r", encoding="utf-8") as f:
        row_count = sum(1 for _ in f) - 1
    print(f"Rows written: {row_count:,}")


def patch_weblinks(output_path: str) -> None:
    """
    The SQLite DB has empty WebLink_Name / WebLink_URL columns.
    We reconstruct them to match the original:
      WebLink_Name = "www.gb64.com"
      WebLink_URL  = "http://www.gb64.com/game.php?id=<GA_Id>"

    Both are TEXT columns so they are always quoted. We do this by reading
    the already-written CSV with the stdlib reader (for correct field splitting),
    then re-writing only those two fields using the same custom quoting rule,
    so the rest of the line's quoting is untouched.
    """
    print("Patching WebLink_Name / WebLink_URL ...")

    with open(output_path, "r", newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        rows = list(reader)

    header = rows[0]
    try:
        ga_id_col        = header.index("GA_Id")
        weblink_name_col = header.index("WebLink_Name")
        weblink_url_col  = header.index("WebLink_URL")
    except ValueError as e:
        print(f"WARNING: Could not find column for weblinks: {e}")
        return

    patched = 0
    for row in rows[1:]:
        if row[weblink_name_col] == "" and row[weblink_url_col] == "":
            row[weblink_name_col] = "www.gb64.com"
            row[weblink_url_col]  = f"http://www.gb64.com/game.php?id={row[ga_id_col]}"
            patched += 1

    # TEXT column indices — must match export_to_csv exactly
    TEXT_COLS = {1, 3, 4, 6, 12, 13, 16, 32, 40, 42, 43, 52, 53, 54, 55}

    def quote_field(value: str, is_text: bool) -> str:
        if is_text and value != "":
            return '"' + value.replace('"', '""') + '"'
        return value

    with open(output_path, "w", newline="", encoding="utf-8") as f:
        # Header (unquoted column names)
        f.write(",".join(rows[0]) + "\n")
        for row in rows[1:]:
            fields = [quote_field(row[i], i in TEXT_COLS) for i in range(len(row))]
            f.write(",".join(fields) + "\n")

    print(f"Patched {patched:,} rows with WebLink data.")


def verify_and_report(output_path: str) -> None:
    with open(output_path, "r", encoding="utf-8") as f:
        lines = [next(f, "") for _ in range(3)]

    print("\n--- Header ---")
    print(lines[0].rstrip())
    if lines[1]:
        print("\n--- First data row ---")
        print(lines[1].rstrip())
    print("--------------\n")

    if "GA_Id" in lines[0]:
        print("OK: GA_Id column present.")
    else:
        print("WARNING: GA_Id not found in header — check column names above.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) != 2:
        print(f"Usage: python3 {sys.argv[0]} <output_path>")
        print(f"Example: python3 {sys.argv[0]} data/Games_new.csv")
        sys.exit(1)

    output_path = sys.argv[1]
    output_dir  = os.path.dirname(os.path.abspath(output_path))
    os.makedirs(output_dir, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="gb64_") as tmpdir:

        # 1. Download the gzipped SQLite DB (~20 MB compressed)
        gz_path = os.path.join(tmpdir, "GBC_v19.sqlitedb.gz")
        download_with_progress(SQLITE_GZ_URL, gz_path)

        # 2. Decompress
        db_path = os.path.join(tmpdir, "GBC_v19.sqlite")
        decompress_gz(gz_path, db_path)

        # 3. Show tables
        tables = list_tables(db_path)
        print(f"\nTables in DB ({len(tables)}):")
        for t in tables:
            print(f"  {t}")
        print()

        if TABLE not in tables:
            print(f"ERROR: '{TABLE}' not found. Available: {tables}")
            sys.exit(1)

        # 4. Export
        export_to_csv(db_path, TABLE, output_path)

    # 5. Patch WebLinks (empty in SQLite DB, reconstructed from GA_Id)
    patch_weblinks(output_path)

    # 6. Verify
    verify_and_report(output_path)

    abs_out = os.path.abspath(output_path)
    print(f"\nDone → {abs_out}\n")
    print("Structural comparison commands:")
    print(f"  # Column count (should match)")
    print(f"  head -1 data/Games.csv       | tr ',' '\\n' | wc -l")
    print(f"  head -1 {output_path} | tr ',' '\\n' | wc -l")
    print(f"  # Row count")
    print(f"  wc -l data/Games.csv  &&  wc -l {output_path}")
    print(f"  # Column name diff (empty = structure matches)")
    print(f"  diff <(head -1 data/Games.csv | tr ',' '\\n') \\")
    print(f"       <(head -1 {output_path} | tr ',' '\\n')")


if __name__ == "__main__":
    main()
