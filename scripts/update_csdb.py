#!/usr/bin/env python3
"""update_csdb.py – generate an up‑to‑date ``csdb.xml`` for ChipMachine.

Queries the CSDb XML webservice endpoint:
    https://csdb.dk/webservice/?type=release&id=<ID>
and produces an XML file whose structure is byte‑for‑byte identical to the
original ChipMachine csdb.xml (root tag <ReleasesWithHVSC>, ISO-8859-1,
<ReleaseDate> with <Day>/<Month>/<Year>, <Group> with <ID>+<Group>, etc.).
"""

import argparse
import sys
import time
from pathlib import Path
from typing import List, Dict, Optional
from xml.etree import ElementTree as ET

import requests

# ── Constants ─────────────────────────────────────────────────────────────────
WEBSERVICE_URL    = "https://csdb.dk/webservice/"
DEFAULT_MAX_ID    = 300_000
DEFAULT_START_ID  = 10_000
DEFAULT_OUTPUT    = Path(__file__).resolve().parent.parent / "data" / "csdb.xml"
PROGRESS_INTERVAL = 1_000   # print a progress line every N IDs
RATE_LIMIT_EVERY  = 100     # sleep after every N requests
RATE_LIMIT_SLEEP  = 0.2     # seconds to sleep
DEBUG             = True


# ── Helpers ───────────────────────────────────────────────────────────────────

def txt_of(el: ET.Element, tag: str) -> str:
    """Return stripped text of a child tag within *el*, or '' if absent."""
    child = el.find(tag)
    return (child.text or "").strip() if child is not None else ""


# ── Webservice fetcher ────────────────────────────────────────────────────────

def fetch_release(release_id: int, session: requests.Session) -> Optional[Dict]:
    """Fetch one release from the CSDb webservice and return a normalised dict,
    or ``None`` if the ID does not exist / has no usable data.

    Actual API response structure (verified against live endpoint):
        <CSDbData>
          <Release>
            <ID>139711</ID>
            <Name>…</Name>
            <Type>REU Release</Type>
            <ReleaseDay>12</ReleaseDay>
            <ReleaseMonth>7</ReleaseMonth>
            <ReleaseYear>2015</ReleaseYear>
            <ScreenShot>https://…</ScreenShot>
            <Rating>9.25</Rating>
            <ReleasedBy>
              <Group><ID>149</ID><Name>Hoaxers</Name>…</Group>
              <!-- or -->
              <Scener><ID>…</ID><Handle>…</Handle>…</Scener>
            </ReleasedBy>
            <ReleasedAt>
              <Event><ID>2370</ID><Name>Solskogen 2015</Name></Event>
            </ReleasedAt>
            <Achievement>
              <Compo>Mixed Demo</Compo>
              <Place>1</Place>
            </Achievement>
            <UsedSIDs>
              <SID><HVSCPath>/MUSICIANS/…</HVSCPath>…</SID>
            </UsedSIDs>
          </Release>
        </CSDbData>

    Non‑existent IDs return an empty <CSDbData/> or contain an <Error> child.
    """
    try:
        resp = session.get(
            WEBSERVICE_URL,
            params={"type": "release", "id": release_id},
            timeout=15,
        )
        if resp.status_code != 200:
            if DEBUG:
                print(f"[DBG] ID {release_id} – HTTP {resp.status_code}", file=sys.stderr)
            return None
    except Exception as exc:
        if DEBUG:
            print(f"[DBG] ID {release_id} – request error: {exc}", file=sys.stderr)
        return None

    raw = resp.content  # bytes; the service uses UTF-8

    # ── Parse XML ──────────────────────────────────────────────────────────────
    try:
        root = ET.fromstring(raw)
    except ET.ParseError as exc:
        if DEBUG:
            print(f"[DBG] ID {release_id} – XML parse error: {exc}", file=sys.stderr)
        return None

    rel = root.find("Release")
    if rel is None:
        if DEBUG:
            print(f"[DBG] ID {release_id} – empty", file=sys.stderr)
        return None

    def txt(tag: str) -> str:
        el = rel.find(tag)
        return (el.text or "").strip() if el is not None else ""

    name = txt("Name")
    if not name:
        if DEBUG:
            print(f"[DBG] ID {release_id} – no Name field", file=sys.stderr)
        return None

    # ── Date fields ────────────────────────────────────────────────────────────
    day   = txt("ReleaseDay")
    month = txt("ReleaseMonth")
    year  = txt("ReleaseYear")

    # ── Screenshot  API tag: <ScreenShot> ─────────────────────────────────────
    screenshot = txt("ScreenShot")

    # ── Release type  API tag: <Type> ─────────────────────────────────────────
    release_type = txt("Type")

    # ── Rating  API tag: <Rating> ──────────────────────────────────────────────
    rating = txt("Rating")

    # ── ReleasedBy: Groups and Sceners ─────────────────────────────────────────
    groups:  List[Dict[str, str]] = []
    sceners: List[Dict[str, str]] = []
    released_by = rel.find("ReleasedBy")
    if released_by is not None:
        for grp in released_by.findall("Group"):
            gid   = txt_of(grp, "ID")
            gname = txt_of(grp, "Name")
            if gname:
                groups.append({"ID": gid, "Name": gname})
        for scener in released_by.findall("Scener"):
            scener_id = txt_of(scener, "ID")
            handle    = txt_of(scener, "Handle")
            if handle:
                sceners.append({"ID": scener_id, "Handle": handle})

    # ── Achievement (compo placement) ──────────────────────────────────────────
    # API structure: <Achievement> (Compo + Place) and <ReleasedAt> (Event) are
    # *sibling* elements under <Release>, NOT nested inside each other.
    achievement = None
    ach_el = rel.find("Achievement")
    if ach_el is not None:
        compo = txt_of(ach_el, "Compo")
        place = txt_of(ach_el, "Place")
        if compo or place:
            event_info = None
            released_at = rel.find("ReleasedAt")   # sibling of Achievement
            if released_at is not None:
                ev = released_at.find("Event")
                if ev is not None:
                    event_info = {
                        "ID":   txt_of(ev, "ID"),
                        "Name": txt_of(ev, "Name"),
                    }
            achievement = {"Compo": compo, "Place": place, "Event": event_info}

    # ── SID files  API tags: <UsedSIDs> / <SID> / <HVSCPath> ─────────────────
    sids: List[str] = []
    sid_files = rel.find("UsedSIDs")
    if sid_files is not None:
        for sf in sid_files.findall("SID"):
            path = txt_of(sf, "HVSCPath")
            if path:
                sids.append(path)

    return {
        "ID":          str(release_id),
        "Name":        name,
        "ReleaseType": release_type,
        "Day":         day,
        "Month":       month,
        "Year":        year,
        "Screenshot":  screenshot,
        "CSDbRating":  rating,
        "Groups":      groups,
        "Sceners":     sceners,
        "Achievement": achievement,
        "Sids":        sids,
    }


# ── XML builder ───────────────────────────────────────────────────────────────

def build_xml(releases: List[Dict]) -> ET.Element:
    """Build the <ReleasesWithHVSC> element tree matching csdb.xml exactly."""
    root = ET.Element("ReleasesWithHVSC")

    for rel in releases:
        rel_el = ET.SubElement(root, "Release")

        ET.SubElement(rel_el, "ID").text   = rel["ID"]
        ET.SubElement(rel_el, "Name").text = rel["Name"]

        if rel["Screenshot"]:
            ET.SubElement(rel_el, "Screenshot").text = rel["Screenshot"]

        if rel["ReleaseType"]:
            ET.SubElement(rel_el, "ReleaseType").text = rel["ReleaseType"]

        # <ReleaseDate> only when we have at least a year
        if rel["Year"]:
            date_el = ET.SubElement(rel_el, "ReleaseDate")
            if rel["Day"]:
                ET.SubElement(date_el, "Day").text   = rel["Day"]
            if rel["Month"]:
                ET.SubElement(date_el, "Month").text = rel["Month"]
            ET.SubElement(date_el, "Year").text      = rel["Year"]

        # <ReleasedBy> – Groups and/or Sceners
        if rel["Groups"] or rel["Sceners"]:
            rb_el = ET.SubElement(rel_el, "ReleasedBy")
            for g in rel["Groups"]:
                grp_el = ET.SubElement(rb_el, "Group")
                if g["ID"]:
                    ET.SubElement(grp_el, "ID").text    = g["ID"]
                # output tag is <Group> to mirror the original csdb.xml structure
                ET.SubElement(grp_el, "Group").text = g["Name"]
            for s in rel["Sceners"]:
                sc_el = ET.SubElement(rb_el, "Scener")
                if s["ID"]:
                    ET.SubElement(sc_el, "ID").text     = s["ID"]
                ET.SubElement(sc_el, "Handle").text = s["Handle"]

        # <Achievement> with optional nested <Event>
        ach = rel.get("Achievement")
        if ach:
            ach_el = ET.SubElement(rel_el, "Achievement")
            if ach.get("Event"):
                ev_el = ET.SubElement(ach_el, "Event")
                if ach["Event"].get("ID"):
                    ET.SubElement(ev_el, "ID").text   = ach["Event"]["ID"]
                if ach["Event"].get("Name"):
                    ET.SubElement(ev_el, "Name").text = ach["Event"]["Name"]
            if ach.get("Compo"):
                ET.SubElement(ach_el, "Compo").text = ach["Compo"]
            if ach.get("Place"):
                ET.SubElement(ach_el, "Place").text = ach["Place"]

        if rel["CSDbRating"]:
            ET.SubElement(rel_el, "CSDbRating").text = rel["CSDbRating"]

        if rel["Sids"]:
            sids_el = ET.SubElement(rel_el, "Sids")
            for path in rel["Sids"]:
                ET.SubElement(sids_el, "HVSCPath").text = path

    return root


# ── XML writer ────────────────────────────────────────────────────────────────

def write_xml(root: ET.Element, output_path: Path) -> None:
    """Write the element tree as ISO-8859-1 with an XML declaration,
    matching the original csdb.xml exactly."""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    tree = ET.ElementTree(root)
    tree.write(
        str(output_path),
        encoding="ISO-8859-1",
        xml_declaration=True,
    )


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a fresh csdb.xml for ChipMachine"
    )
    parser.add_argument(
        "--max-id",
        type=int,
        default=DEFAULT_MAX_ID,
        help="Maximum release ID to query (default %(default)s)",
    )
    parser.add_argument(
        "--start-id",
        type=int,
        default=DEFAULT_START_ID,
        help="First release ID to query (default %(default)s)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Path for generated csdb.xml (default %(default)s)",
    )
    # Positional override kept for backward compatibility
    parser.add_argument(
        "output_path",
        nargs="?",
        type=Path,
        help="Positional output file (overrides --output)",
    )
    args = parser.parse_args()

    if args.output_path:
        args.output = args.output_path

    releases:  List[Dict] = []
    collected: int        = 0
    rid:       int        = args.start_id
    start:     float      = time.time()

    print(f"[INFO] Scanning CSDb releases from ID {args.start_id} to {args.max_id}")

    session = requests.Session()
    session.headers.update({"User-Agent": "ChipMachine-updater/1.0"})

    try:
        for rid in range(args.start_id, args.max_id + 1):
            data = fetch_release(rid, session)

            if data:
                releases.append(data)
                collected += 1
                if DEBUG:
                    print(f"[DBG] ID {rid} – found: {data['Name']!r}", file=sys.stderr)

            if rid % PROGRESS_INTERVAL == 0:
                elapsed = time.time() - start
                print(
                    f"[PROGRESS] ID {rid:,} – collected {collected:,} releases "
                    f"(elapsed {elapsed:.1f}s)"
                )

            # Polite rate‑limiting: brief pause every N requests
            if rid % RATE_LIMIT_EVERY == 0:
                time.sleep(RATE_LIMIT_SLEEP)

    except KeyboardInterrupt:
        print("\n[INFO] Interrupted – writing partial results.", file=sys.stderr)

    finally:
        elapsed_total = time.time() - start
        print(
            f"[INFO] Finished scan: processed up to ID {rid:,}, "
            f"collected {collected:,} releases (total time {elapsed_total:.1f}s)."
        )
        print("[INFO] Building XML…")
        xml_root = build_xml(releases)
        write_xml(xml_root, args.output)
        print(f"[INFO] csdb.xml written to {args.output}")


if __name__ == "__main__":
    main()
