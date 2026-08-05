# playerproplugin — PlayerPRO (Macintosh)

**PlayerPRO**, Antoine Rosset's classic Macintosh tracker — the dominant Mac
module editor of the 1990s.

Extensions: `.mad` (`MADG` / `MADF` / `MADK`)

## Engine

A minimal slice of PlayerPRO's own **public-domain "MADDriver"** software synth,
driven offline at 44100 Hz. The sources assume a signed `char`, so the slice is
compiled with `-fsigned-char`.

## Routing

`.mad` is shared with AdPlug's unrelated **Mad Tracker 2** loader, which is
content-gated on magic `MAD+`, so PlayerPRO tunes route here. See
[adplugin](../adplugin/README.md).
