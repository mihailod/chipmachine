# ayflyplugin — Ayfly (legacy)

ZX Spectrum AY formats via **Ayfly**.

## Status

Superseded. Ayfly was replaced as the primary ZX AY decoder by
[zxayplugin](../zxayplugin/README.md), which is copyleft-free and ships in
**both** builds.

Ayfly remains **ChipMachinePlus only** (see
[`LEGAL-PLUS`](../../../../../LEGAL-PLUS)) and is registered *ahead* of
zxayplugin there, so the Plus build's routing is byte-for-byte what it always
was.

Two things zxayplugin fixed rather than merely replicated:

* `.vt2` — Ayfly claimed the extension and then threw on every one of the
  catalog's 551 rows (Vortex Tracker II's binary save is really a PT3 module),
  which also stopped anything else from trying.
* `.ay` — Ayfly renders Amstrad CPC ZXAYEMUL rips silent, so that extension
  belongs to [gmeplugin](../gmeplugin/README.md) in both builds.
