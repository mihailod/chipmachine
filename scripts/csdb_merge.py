import xml.etree.ElementTree as ET
from pathlib import Path

def merge_csdb_xml_files(file1_path: str, file2_path: str, output_path: str):
    """
    Parses and merges two CSDb XML files under a single root element,
    deduplicating <Release> elements based on their unique <ID>.
    """
    p1 = Path(file1_path)
    p2 = Path(file2_path)
    
    if not p1.exists() or not p2.exists():
        raise FileNotFoundError("One or both input XML files do not exist.")

    print(f"Parsing {p1.name}...")
    tree1 = ET.parse(p1)
    root1 = tree1.getroot()
    
    print(f"Parsing {p2.name}...")
    tree2 = ET.parse(p2)
    root2 = tree2.getroot()
    
    # Verify both root tags match
    if root1.tag != root2.tag:
        raise ValueError(f"Root tag mismatch: {root1.tag} vs {root2.tag}")
        
    # Map existing release IDs in the first file to prevent duplicates
    # and track unique records.
    seen_ids = set()
    for release in root1.findall('Release'):
        rel_id = release.find('ID')
        if rel_id is not None and rel_id.text:
            seen_ids.add(rel_id.text)
            
    print(f"Initial unique releases in file 1: {len(seen_ids)}")

    # Iterate through the second file and append unique entries to the first tree
    added_count = 0
    duplicate_count = 0
    
    for release in root2.findall('Release'):
        rel_id = release.find('ID')
        if rel_id is not None and rel_id.text:
            if rel_id.text not in seen_ids:
                # Deep copy not strictly necessary here unless modifying nodes inline,
                # but appending directly transfers the reference to the root1 tree.
                root1.append(release)
                seen_ids.add(rel_id.text)
                added_count += 1
            else:
                duplicate_count += 1
        else:
            # Fallback if a release lacks an ID tag entirely
            root1.append(release)
            added_count += 1

    print(f"Merged {added_count} new entries. Skipped {duplicate_count} duplicates.")
    print(f"Total entries in merged tree: {len(root1.findall('Release'))}")

    # Write out the combined data with proper XML declaration and encoding
    print(f"Writing merged output to {output_path}...")
   
    # no any formatting to reduce the file size 
    tree1.write(
        output_path, 
        encoding="ISO-8859-1", 
        xml_declaration=True
    )
    print("Merge operation completed successfully.")

if __name__ == "__main__":
    # Define file names based on provided context
    FILE_1 = "csdb.xml"
    FILE_2 = "csdb-174979-263000.xml"
    OUTPUT_FILE = "csdb_merged.xml"
    
    merge_csdb_xml_files(FILE_1, FILE_2, OUTPUT_FILE)
