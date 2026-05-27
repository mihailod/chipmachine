#!/usr/bin/env python3
import os
import argparse

def parse_sap_header(file_path):
    title = "<?>"
    author = "<?>"
    try:
        with open(file_path, "rb") as f:
            header_bytes = f.read(2048)
        header_text = header_bytes.decode("latin-1")
        for line in header_text.splitlines():
            line = line.strip()
            if not line:
                continue
            if line.startswith("MUTE") or line.startswith("TYPE") or not any(c.isalnum() for c in line[:4]):
                break
            if line.startswith("NAME"):
                title = line.replace("NAME", "", 1).strip().strip('"')
            elif line.startswith("AUTHOR"):
                author = line.replace("AUTHOR", "", 1).strip().strip('"')
    except Exception:
        pass
    return title, author

def load_original_order(original_file_path):
    """
    Reads the original asma.txt file to capture its exact structural sorting order.
    Returns a dictionary mapping relative_path -> sequence_index.
    """
    order_map = {}
    if not os.path.isfile(original_file_path):
        return order_map
    try:
        with open(original_file_path, "r", encoding="utf-8") as f:
            for idx, line in enumerate(f):
                parts = line.strip().split("\t")
                if len(parts) >= 4:
                    rel_path = parts[-1]
                    order_map[rel_path] = idx
    except Exception:
        pass
    return order_map

def main():
    parser = argparse.ArgumentParser(description="Compile an ASMA folder tree into a Chipmachine TSV index file.")
    parser.add_argument("source_dir", help="Path to the extracted ASMA database folder root")
    parser.add_argument("output_file", help="Path to the output text file destination")
    
    args = parser.parse_args()

    source_dir = os.path.abspath(os.path.expanduser(args.source_dir))
    output_file = os.path.abspath(os.path.expanduser(args.output_file))

    # Read original order from chipmachine/data/asma.txt if it exists to preserve ordering matches
    original_reference = os.path.join("chipmachine", "data", "asma.txt")
    order_map = load_original_order(original_reference)

    if not os.path.isdir(source_dir):
        print(f"Error: Source directory '{source_dir}' does not exist.")
        return

    print(f"Scanning ASMA library inside: {source_dir}")
    records = []
    target_folders = ["Composers", "Games", "Groups", "Misc", "Unknown"]

    for folder in target_folders:
        folder_path = os.path.join(source_dir, folder)
        if not os.path.isdir(folder_path):
            continue
            
        for root, _, files in os.walk(folder_path):
            for file in files:
                if file.lower().endswith(".sap"):
                    full_path = os.path.join(root, file)
                    title, author = parse_sap_header(full_path)
                    relative_path = os.path.relpath(full_path, source_dir)
                    records.append((title, author, "Atari 8Bit", relative_path))

    # Sorting logic: Use original index position if found. New files go to the end.
    # If both are new, fall back to standard string sorting.
    def sort_key(x):
        rel_path = x[3]
        return (order_map.get(rel_path, float('inf')), rel_path)

    records.sort(key=sort_key)

    output_dir = os.path.dirname(output_file)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(output_file, "w", encoding="utf-8") as out:
        for record in records:
            out.write(f"{record[0]}\t\t{record[1]}\t{record[2]}\t{record[3]}\n")

    print(f"Success. Wrote {len(records)} verified records to '{output_file}'")

if __name__ == "__main__":
    main()
