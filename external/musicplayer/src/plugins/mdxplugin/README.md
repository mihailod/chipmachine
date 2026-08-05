# mdxplugin

Sharp **X68000** MDX (Music Macro Language) music, via **mdxmini**.

Extensions: `.mdx` (with optional `.pdx` sample banks)

## Build gating

**ChipMachinePlus only.** mdxmini is not part of the Mac App Store build, and all
6,913 `.mdx` songs are hidden there — it is the sole claimer of the extension, so
there is no fallback decoder and the gate needs no format key, just the plain
extension test.

See the MDX entry in [`LEGAL-PLUS`](../../../../../LEGAL-PLUS) for the terms and
for why no permissive replacement exists off the shelf. (GAMDX is a
disassembly and was rejected on that basis.)

## Clean-room work in progress

A clean-room MDX engine is under development at
[mdxcrplugin](../mdxcrplugin/README.md); the chip emulation it needs is already
BSD-3 in tree.
