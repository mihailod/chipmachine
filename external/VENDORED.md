# Vendored external dependencies

Previously separate sibling clones, vendored (flattened) into this repo on 2026-07-09.
Only the committed tree of each fork was imported (build artifacts excluded).
To re-sync a fork later: re-clone its URL at/after the recorded commit and re-import.

Paths in the `location` column are relative to this `external/` directory.
Single-plugin engines were relocated under their owning plugin on 2026-07-17;
shared/infra deps (`zxtune`, `apone`, `sol2`, `lua`, `musicplayer`) stay at the
`external/` root.

| dependency | location (under `external/`) | fork remote | branch | commit |
|---|---|---|---|---|
| `apone` | `apone` | git@github.com:mihailod/apone.git | master | `2ad8e72d6530a2c7408a6ed8f5cf315f439e450c` |
| `vice310` | `vice310` | https://github.com/mihailod/vice310.git | master | `2786cc4e61746f9854149c0dd858ab9fc07706fb` |
| `98fmplayer` | `98fmplayer` | https://github.com/mihailod/98fmplayer.git | master | `4fa914e4b2b994cb3ccf92d571a20a7cdf1fe36a` |
| `libpxtone` | `musicplayer/src/plugins/pxtoneplugin/libpxtone` | https://github.com/mihailod/libpxtone.git | master | `c6be751153f2f1bbc7aac5e84b72f4d5bee2e584` |
| `organya` | `musicplayer/src/plugins/orgplugin/organya` | https://github.com/mihailod/organya.h.git | master | `7fe762d4090233d392dfaacc48a7c26c6dd91e14` |
| `eupmini` | `eupmini` | https://github.com/mihailod/eupmini.git | master | `aeaec7196867334212b988558870110ac2903ff1` |
| `libkss` | `libkss` | https://github.com/mihailod/libkss.git | master | `751cd56267ba4eaca6b0e1b45f9184473620aebe` |
| `zingzong` | `musicplayer/src/plugins/quartetplugin/zingzong` | https://github.com/mihailod/zingzong.git | master | `487b5bd4e63bdefc2992c64d91ba75b6cfc1e1d2` |
| `audiodecoderwsr` | `audiodecoderwsr` | https://github.com/mihailod/audiodecoder.wsr.git | master | `992e976e7f30c505ce1ad38e222d11797e48b266` |
| `zxtune` | `zxtune` | https://github.com/mihailod/zxtune.git | master | `fa7e554309a578262c838d72324d6132a279b51b` |
| `protrekkr` | `musicplayer/src/plugins/ptkplugin/protrekkr` | https://github.com/mihailod/protrekkr.git | master | `9c909d77bfa1ce985b52b48f9b2b9925e0c6ed1d` |
| `soundsmith` | `soundsmith` | https://github.com/mihailod/soundsmith.git | master | `0f5bcc71ef5f2109aba7c66f440f50725492d897` |
| `arkostracker3` | `musicplayer/src/plugins/sksplugin/arkostracker3` | git@github.com:mihailod/arkostracker3.git | master | `3fc0f9dc7f6fa986a319576fa9c6c979dfffa29a` |
| `webixs` | `musicplayer/src/plugins/ixsplugin/webixs` | https://github.com/mihailod/webixs.git | master | `52de8ef68027316c9c9bda6e902b78933c0a6365` |
| `playerpro` | `playerpro` | https://github.com/mihailod/playerpro.git | master | `f2dd5d98b5d24d85261cf283c1bcfbd52ccc085c` |
| `jaytrax` | `jaytrax` | https://github.com/mihailod/jaytrax.git | master | `95691b2a17bac5dedbcaa8b24f445b13be11a189` |
| `famitracker-cx` | `famitracker-cx` | https://github.com/mihailod/famitracker-cx.git | master | `202a614bd42f516c84772722ae3e6bb28d263699` |
| `musicplayer` | `musicplayer` | git@github.com:mihailod/musicplayer.git | master | `9b9125e08b6a8883d48779e65ad18ae498050639` |
| `furnace` | `musicplayer/src/plugins/dmfplugin/furnace` | https://github.com/mihailod/furnace.git | master | `caccf29c6e719e48172496f5695597fb4bd2d0d4` |
| `vgmstream` | `musicplayer/src/plugins/vgmstreamplugin/vgmstream` | https://github.com/vgmstream/vgmstream | master | `7f1ceb3058f581ed42d265c8980e44a0a281b4f6` |
