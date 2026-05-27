#!/usr/bin/env python3
import os
import sys

def parse_sid_header(filepath):
    try:
        with open(filepath, 'rb') as f:
            header = f.read(0x56)
            if len(header) < 0x56 or header[0:4] not in (b'PSID', b'RSID'):
                return None, None
            
            title = header[0x16:0x36].split(b'\x00')[0].decode('latin-1', errors='replace').strip()
            author = header[0x36:0x56].split(b'\x00')[0].decode('latin-1', errors='replace').strip()
            
            if not title: 
                title = os.path.splitext(os.path.basename(filepath))[0]
            if not author: 
                author = "Unknown"
            return title, author
    except Exception:
        return None, None

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 update_hvsc.py <path_to_HVSC_root> <target_output_file>", file=sys.stderr)
        sys.exit(1)
        
    hvsc_root = os.path.abspath(sys.argv[1])
    target_file = os.path.abspath(sys.argv[2])
    reference_file = os.path.abspath(os.path.join("chipmachine", "data", "hvsc.txt"))
    
    if not os.path.exists(reference_file):
        print(f"Error: Baseline reference not found at: {reference_file}", file=sys.stderr)
        sys.exit(1)

    existing_order = []
    
    print(f"Reading historical order layout from reference...")
    with open(reference_file, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            parts = line.strip('\n').split('\t')
            if parts and parts[-1]:
                rel_path = parts[-1].strip().replace('\\', '/')
                if rel_path:
                    existing_order.append(rel_path)

    print(f"Scanning disk assets in {hvsc_root}...")
    # Key: filename.lower(), Value: list of absolute paths matching that filename
    filename_disk_db = {}
    all_seen_disk_paths = set()
    
    for root, _, files in os.walk(hvsc_root):
        for file in files:
            if file.lower().endswith('.sid'):
                full_path = os.path.abspath(os.path.join(root, file))
                filename_lower = file.lower()
                filename_disk_db.setdefault(filename_lower, []).append(full_path)
                
                rel_path = os.path.relpath(full_path, hvsc_root).replace('\\', '/')
                all_seen_disk_paths.add(rel_path.lower())

    final_entries = []
    used_disk_paths = set()
    print("Resolving moved tracks and extracting metadata...")
    
    for rel_path in existing_order:
        filename = os.path.basename(rel_path)
        filename_lower = filename.lower()
        matched_full_path = None
        
        if filename_lower in filename_disk_db:
            candidates = filename_disk_db[filename_lower]
            if len(candidates) == 1:
                matched_full_path = candidates[0]
            else:
                # If there are multiple files with the same name, match by closest directory path match
                # (e.g. matching 'Nebula' in the path components)
                ref_parts = set(rel_path.lower().split('/'))
                best_score = -1
                for cand in candidates:
                    cand_rel = os.path.relpath(cand, hvsc_root).lower()
                    cand_parts = set(cand_rel.split('/'))
                    score = len(ref_parts.intersection(cand_parts))
                    if score > best_score:
                        best_score = score
                        matched_full_path = cand

        if matched_full_path:
            title, author = parse_sid_header(matched_full_path)
            if title is not None:
                title = title.replace('\t', ' ').replace('\n', '').replace('\r', '')
                author = author.replace('\t', ' ').replace('\n', '').replace('\r', '')
                display_rel_path = os.path.relpath(matched_full_path, hvsc_root).replace('\\', '/')
                final_entries.append((title, author, "Commodore 64", display_rel_path))
                used_disk_paths.add(display_rel_path.lower())
                continue
                
        # Safe fallback placeholder if file is genuinely completely gone from disk
        final_entries.append(("<?>", "<?>", "Commodore 64", rel_path))

    # Append completely new items that never existed in the old template file
    new_count = 0
    for filename_lower in sorted(filename_disk_db.keys()):
        for actual_path in filename_disk_db[filename_lower]:
            display_rel_path = os.path.relpath(actual_path, hvsc_root).replace('\\', '/')
            if display_rel_path.lower() not in used_disk_paths:
                title, author = parse_sid_header(actual_path)
                if title is not None:
                    title = title.replace('\t', ' ').replace('\n', '').replace('\r', '')
                    author = author.replace('\t', ' ').replace('\n', '').replace('\r', '')
                    final_entries.append((title, author, "Commodore 64", display_rel_path))
                    new_count += 1

    print(f"Writing updated index to {target_file}...")
    with open(target_file, 'w', encoding='utf-8') as out:
        for entry in final_entries:
            out.write(f"{entry[0]}\t\t{entry[1]}\t{entry[2]}\t{entry[3]}\n")

    print(f"Done. Total database size: {len(final_entries)} tracks (Appended {new_count} brand new tracks).")

if __name__ == "__main__":
    main()
