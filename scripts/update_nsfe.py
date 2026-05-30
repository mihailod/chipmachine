import os
import shutil
import struct
import urllib.parse

def parse_ape_tags(tag_path):
    """
    Parses APEv2 tags from the .tag binary files to extract Title and Artist.
    """
    tags = {}
    if not os.path.exists(tag_path):
        return tags
    
    try:
        with open(tag_path, 'rb') as f:
            data = f.read()
    except Exception:
        return tags
    
    if b'APETAGEX' not in data:
        return tags
    
    # Locate the APEv2 header
    start_pos = data.find(b'APETAGEX')
    if start_pos == -1:
        return tags
    
    header = data[start_pos:start_pos+32]
    if len(header) < 32:
        return tags
        
    try:
        # APEv2 header: bytes 16-20 contain the item count (little-endian)
        tag_count = struct.unpack('<I', header[16:20])[0]
    except Exception:
        return tags
    
    # Iterate through tag items
    curr = start_pos + 32
    for _ in range(tag_count):
        if curr + 8 >= len(data):
            break
        try:
            val_len = struct.unpack('<I', data[curr:curr+4])[0]
            flags = struct.unpack('<I', data[curr+4:curr+8])[0]
            curr += 8
            
            # Key is null-terminated
            end_key = data.find(b'\x00', curr)
            if end_key == -1:
                break
            key = data[curr:end_key].decode('utf-8', errors='ignore')
            curr = end_key + 1
            
            # Value is val_len bytes long
            val = data[curr:curr+val_len].decode('utf-8', errors='ignore')
            tags[key.lower()] = val
            curr += val_len
        except Exception:
            break
            
    return tags

def migrate():
    src_root = 'NSFE'
    dst_dir = 'chipmachine/music/Console'
    index_path = 'chipmachine/data/nsfe.txt'
    
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
        
    # 1. Load existing index to preserve current entries
    existing_entries = {}
    if os.path.exists(index_path):
        with open(index_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip('\n')
                parts = line.split('\t')
                # Format: ["", title, artist, "", filename]
                if len(parts) >= 5:
                    filename = parts[4]
                    existing_entries[filename] = line

    new_entries = []
    seen_dest_filenames = set(existing_entries.keys())
    
    # 2. Scan NSFE/ root recursively
    migrated_count = 0
    for root, dirs, files in os.walk(src_root):
        for file in files:
            if file.endswith('.nsfe'):
                nsfe_src = os.path.join(root, file)
                tag_path = nsfe_src + '.tag'
                
                # We require a .tag file to process the metadata
                if not os.path.exists(tag_path):
                    continue
                
                tags = parse_ape_tags(tag_path)
                title = tags.get('title', os.path.splitext(file)[0]).strip()
                artist = tags.get('artist', '<?>').strip()
                if not artist: artist = '<?>'
                
                # 3. Collision-safe flattening logic
                dest_filename = file
                dest_path = os.path.join(dst_dir, dest_filename)
                
                # If filename exists, prefix with parent folder name for uniqueness
                if dest_filename in seen_dest_filenames:
                    folder_name = os.path.basename(root)
                    dest_filename = f"{folder_name}_{file}"
                    dest_path = os.path.join(dst_dir, dest_filename)
                    
                    counter = 1
                    while dest_filename in seen_dest_filenames or os.path.exists(dest_path):
                        name, ext = os.path.splitext(dest_filename)
                        dest_filename = f"{name}_{counter}{ext}"
                        dest_path = os.path.join(dst_dir, dest_filename)
                        counter += 1

                seen_dest_filenames.add(dest_filename)
                
                # 4. Perform the copy
                shutil.copy2(nsfe_src, dest_path)
                
                # 5. Create entry in the 5-column tab format
                entry = f"\t{title}\t{artist}\t\t{dest_filename}"
                new_entries.append(entry)
                migrated_count += 1
                
    # 6. Merge existing with new and sort by Title
    final_entries = list(existing_entries.values()) + new_entries
    
    def get_sort_key(line):
        parts = line.split('\t')
        if len(parts) >= 2:
            return parts[1].lower()
        return line.lower()
        
    final_entries.sort(key=get_sort_key)
    
    # 7. Write the master index
    with open(index_path, 'w', encoding='utf-8') as f:
        for entry in final_entries:
            f.write(entry + '\n')
            
    print(f"Migrated {migrated_count} new tracks.")
    print(f"Total tracks in index: {len(final_entries)}.")

if __name__ == "__main__":
    migrate()
