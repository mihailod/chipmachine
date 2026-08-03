#!/usr/bin/env python3
"""Reconstruct a Z80 binary from a MONS4D disassembly listing.

Bulba's ay.strangled.net player archives (PT1Player, PSCPlayer, SQTPlayer,
STPDocs) ship the original ZX Spectrum replay routines as MONS4D listings:

    82A2  EDB0           LDIR

i.e. a 4-hex-digit address, two spaces, then the instruction's bytes as hex,
then the mnemonic. The hex column alone is enough to rebuild the byte image
exactly, with no assembler dialect in the loop -- and the addresses let us
verify the result is contiguous and self-consistent.
"""
import re
import sys

# MONS4D writes an orphaned index prefix (a DD/FD with no instruction after it)
# as "DD*" -- the '*' is the disassembler flagging it, not a byte. The prefix
# itself is real and must go into the image, so accept the marker.
LINE = re.compile(r'^([0-9A-Fa-f]{4})\s\s+((?:[0-9A-Fa-f]{2})+)\*?(?:\s|$)')


def parse(path):
    """Returns {addr: byte} plus the list of (line_no, text) we skipped."""
    mem = {}
    skipped = []
    for n, raw in enumerate(open(path, 'rb').read().decode('cp437').splitlines(), 1):
        m = LINE.match(raw)
        if not m:
            if raw.strip():
                skipped.append((n, raw))
            continue
        addr = int(m.group(1), 16)
        data = bytes.fromhex(m.group(2))
        for i, b in enumerate(data):
            a = addr + i
            if a in mem and mem[a] != b:
                print(f'{path}:{n}: CONFLICT at {a:04X}: '
                      f'{mem[a]:02X} vs {b:02X}', file=sys.stderr)
            mem[a] = b
    return mem, skipped


def main():
    path, out = sys.argv[1], sys.argv[2]
    mem, skipped = parse(path)
    if not mem:
        sys.exit(f'{path}: no listing lines matched')
    lo, hi = min(mem), max(mem)
    gaps = [a for a in range(lo, hi + 1) if a not in mem]
    print(f'{path}: {lo:04X}-{hi:04X} ({hi - lo + 1} bytes), '
          f'{len(mem)} defined, {len(gaps)} gaps, {len(skipped)} non-listing lines')
    if gaps:
        # Report gaps as ranges so a real hole is obvious at a glance.
        runs, start = [], gaps[0]
        for a, b in zip(gaps, gaps[1:] + [None]):
            if b != (a or 0) + 1:
                runs.append((start, a))
                start = b
        print('  gaps: ' + ', '.join(f'{s:04X}-{e:04X}' for s, e in runs[:20]))
    img = bytes(mem.get(a, 0) for a in range(lo, hi + 1))
    open(out, 'wb').write(img)
    print(f'  -> {out} (org #{lo:04X})')


if __name__ == '__main__':
    main()
