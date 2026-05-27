#!/usr/bin/env python3
import os
import sys

def get_author(path_parts):
    if path_parts[0] == 'musicians':
        base_author = path_parts[1].replace('_', ' ')
        if len(path_parts) > 3:
            return f"{base_author} ({path_parts[2].replace('_', ' ')})"
        return base_author
    return "Unknown"

def format_title(filename):
    # Remove extension and replace underscores with spaces
    raw_title = os.path.splitext(filename)[0].replace('_', ' ')
    
    # Capitalize first letter of every word
    title = raw_title.title()
    
    # Fix acronyms and specific casing that .title() mangles
    title = title.replace('Mtv', 'MTV').replace('Sfx', 'SFX').replace('Ii', 'II')
    
    # Ensure 3D is always 3D
    if title.lower().startswith('3d '):
        title = '3D ' + title[3:]
        
    return title

def main():
    if len(sys.argv) != 3:
        sys.exit(1)

    hvtc_dir = os.path.abspath(sys.argv[1])
    output_file = sys.argv[2]
    categories = ['demos', 'games', 'musicians', 'other']
    final_output = []

    for cat in categories:
        cat_path = os.path.join(hvtc_dir, cat)
        if not os.path.exists(cat_path): continue
        
        cat_entries = []
        for root, _, files in os.walk(cat_path):
            for file in files:
                if file.lower().endswith('.prg'):
                    rel_path = os.path.relpath(os.path.join(root, file), hvtc_dir)
                    path_parts = rel_path.split(os.sep)
                    
                    title = format_title(file)
                    author = get_author(path_parts)
                    cat_entries.append(f"{title}\t\t{author}\tTED\t{rel_path}")
        
        # Sort by title, case-insensitive
        cat_entries.sort(key=lambda x: x.split('\t')[0].lower())
        final_output.extend(cat_entries)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(final_output) + '\n')

    print(f"Done. Categorized and processed {len(final_output)} tracks.")

if __name__ == "__main__":
    main()
