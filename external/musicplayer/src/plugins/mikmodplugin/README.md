# mikmodplugin — MikMod UNITRK / UNIMOD

**MikMod UNITRK / UNIMOD** modules (`.uni`) — MikMod's own on-disk module format
(magic `UN0x`, e.g. `UN05`; legacy `APUN`), into which it could save any module
it loaded. The modland `MikMod UNITRK/` corpus is mostly FastTracker 2 tunes
converted this way.

Extensions: `.uni`

## Why a dedicated plugin

No other engine in the build has a UNIMOD loader — libopenmpt's
superficially-similar `Load_unic` is the unrelated *UNIC Tracker*, and libxmp and
libmodplug have none — so these files previously had no decoder at all.

## Engine

A vendored slice of **libmikmod**: just the player core, software mixer, UNI
loader, depackers and null driver, pulling PCM through libmikmod's virtual mixer.
The object is combined with `ld -r`.

Content-gated to the `UN0x` / `APUN` magic, so it claims only `.uni` and never
contests the mod-family extensions owned by the OpenMPT / UADE plugins.
