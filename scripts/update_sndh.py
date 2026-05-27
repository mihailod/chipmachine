#!/usr/bin/env python3
import os
import sys

def parse_sndh_header(file_path):
    """
    Parses the ASCII metadata header blocks from an SNDH chiptune file.
    Looks specifically for 'TITL' (Title) and 'COMM' (Composer).
    """
    title = "Unknown"
    composer = "Unknown"
    
    try:
        with open(file_path, 'rb') as f:
            chunk = f.read(2048)
            
            if b'SNDH' not in chunk:
                base = os.path.splitext(os.path.basename(file_path))[0]
                return base.replace('_', ' '), "Unknown"

            tags = {
                b'TITL': 'title',
                b'COMM': 'composer'
            }
            
            for tag, attr in tags.items():
                idx = chunk.find(tag)
                if idx != -1:
                    start = idx + 4
                    end = chunk.find(b'\x00', start)
                    if end != -1:
                        val = chunk[start:end].decode('latin-1', errors='replace').strip()
                        if val:
                            if attr == 'title':
                                title = val
                            elif attr == 'composer':
                                composer = val
    except Exception:
        base = os.path.splitext(os.path.basename(file_path))[0]
        title = base.replace('_', ' ')

    return title, composer

def generate_legacy_ordered_index(source_dir, output_file):
    print(f"Scanning source directory: {source_dir}")
    records = []
    
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.lower().endswith(('.sndh', '.snd')):
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, source_dir)
                
                title, composer = parse_sndh_header(full_path)
                
                if composer == "Unknown" or not composer:
                    dir_parts = rel_path.split(os.sep)
                    if len(dir_parts) > 1:
                        composer = dir_parts[0].replace('_', ' ')
                
                # Sanitize tabs to maintain structural field columns
                title = title.replace('\t', ' ')
                composer = composer.replace('\t', ' ')
                
                # Rigid 5-column architecture using double tabs
                line = f"{title}\t\t{composer}\tAtari ST\t{rel_path}\n"
                
                # To match legacy sorting, the sort key must prioritize the relative path, 
                # forcing files in 4-Mat/* to group before files in 505/*
                sort_key = rel_path.lower()
                records.append((sort_key, line))
                
    print("Sorting entries by archive path hierarchy to match legacy layout...")
    records.sort(key=lambda x: x[0])
    
    try:
        with open(output_file, 'w', encoding='utf-8') as out:
            for _, line in records:
                out.write(line)
    except IOError as e:
        print(f"Error writing to output file {output_file}: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Index complete. Generated {len(records)} entries inside {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 update_sndh.py <path_to_sndh_folder> <output_file_name>", file=sys.stderr)
        sys.exit(1)
        
    src_folder = os.path.abspath(os.path.expanduser(sys.argv[1]))
    out_name = sys.argv[2]
    
    if not os.path.exists(src_folder) or not os.path.isdir(src_folder):
        print(f"Error: Target directory '{src_folder}' is invalid.", file=sys.stderr)
        sys.exit(1)
        
    generate_legacy_ordered_index(src_folder, out_name)
