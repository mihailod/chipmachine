# Vendored in_wsr WonderSwan replayer

This directory is a verbatim copy of the `lib/in_wsr/` core from Kodi's
**audiodecoder.wsr** addon (https://github.com/xbmc/audiodecoder.wsr), branch
`Piers`. It is a small, self-contained WonderSwan / WonderSwan Color sound-rip
player: a NEC V30MZ CPU core (`nec/`, derived from MAME/Oswan) driving the
WonderSwan sound chip (`ws_audio.c`), originating from Mamiya's `in_wsr` Winamp
plugin / `foo_input_wsr`.

Only `WSRPlayerSetUp()` (declared in `wsr_player.h`) is called from
`../WSRPlugin.cpp`. Every other symbol in this core is renamed to a `wsr__*`
prefix at compile time (see `../CMakeLists.txt`) so its very generic global
names (`ROM`, `SampleRate`, `sample_buffer`, `nec_reset`, ...) cannot collide
with the other vendored plugins at the final static link.

## License

GPL-2.0-or-later. See the project `LEGAL` file and the upstream `LICENSE.md` in
the audiodecoder.wsr repository. The combined chipmachine binary is distributed
under GPL-3.0-or-later, with which this is compatible.
