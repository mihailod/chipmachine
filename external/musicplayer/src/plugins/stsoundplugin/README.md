# stsoundplugin

Atari ST **YM2149 register-dump** music (`.ym`), by **Arnaud Carré**
(Leonard/Oxygene) — the same author as the AtariAudio engine used by
[sndhplugin](../sndhplugin/README.md).

Extensions: `.ym` `.mix`

## Licensing / vendoring

The copy this project originally carried was a pre-2005 **LGPL-3** drop. It was
re-vendored from Arnaud Carré's **BSD-2-Clause** release, so all 13,210 `.ym`
songs ship in **both** builds and need no gate.

Note when auditing: the source files are ISO-8859 encoded, so a naive UTF-8
`grep` misses the licence text.

## Format

`.ym` files are LH5-packed AY/YM register streams — the same packing the ZX
`.vtx` files use (see [zxayplugin](../zxayplugin/README.md)).
