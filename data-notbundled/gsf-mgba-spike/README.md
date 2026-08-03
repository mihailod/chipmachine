# GSF -> mGBA spike — RESOLVED 2026-08-01, kept only as a post-mortem

The swap SHIPPED. `gsfplugin` runs on mGBA (MPL-2.0) in both variants and
`playgsf/` is deleted. Everything worth knowing now lives with the code:
`external/musicplayer/src/plugins/gsfplugin/README.md`.

Nothing here is compiled, and `gsfspike.c` is superseded by the plugin's own
`GsfRom.cpp`. Do not start from this directory.

## What the blocker actually was

The 2026-08-01 spike reported that `GBACoreCreate()` returned a valid pointer
whose `init`/`reset` read as garbage, and recorded `-flto` and include paths as
the remaining suspects, having "ruled out" the defines because
`sizeof(struct mCore)` measured 1352 either way.

**The defines were the cause, and that measurement was the bug.** The spike
passed mGBA's `-D` list through a shell variable; the shell here is zsh, which
does not word-split unquoted parameters, so the whole list arrived as a single
argv token and *none* of the macros applied. 1352 is the no-defines layout; the
library's is 2432, and `offsetof(init)` moves 704 -> 1784. Both the segfault and
the "it isn't the defines" conclusion came from the same broken command line.

Second bug behind it, once the calls landed: `core->opts.volume` is
zero-initialised and `mCoreLoadConfig` only overwrites keys that exist, so the
GBA core set `masterVolume = 0` and every rip rendered silence.

## Lesson worth keeping

Trust the shell less. `cc $DEFINES ...` in zsh is one argument, not many; use an
array (`D=(-DA -DB); cc "${D[@]}"`). And when a struct-layout hypothesis is
"ruled out" by measuring inside the same TU that is suspected of being
misconfigured, it has not been ruled out.
