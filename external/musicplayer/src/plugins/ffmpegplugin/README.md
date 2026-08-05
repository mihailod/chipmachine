# ffmpegplugin

Compressed and streaming audio — MP3, AAC, M4A/MP4, Ogg/Vorbis, Opus, FLAC, WAV,
AIFF, AC3, MP2, WMA and IFF-8SVX — decoded **in-process** by FFmpeg's LGPL
`libav*` libraries. No command-line `ffmpeg` is spawned or bundled.

Extensions: `.m4a` `.aac` `.mp3` `.mp4` `.ogg` `.opus` `.flac` `.wav` `.aiff`
`.aif` `.mp2` `.mpeg` `.ac3` `.wma`

Local files, progressive downloads and HTTP(S) radio streams all route here. A
decode-only, LGPL build of **FFmpeg 8.1.1** is vendored at
`external/ffmpeg-lgpl/`.

## Streaming notes

* **Progressive streaming** runs curl → fifo → libav. `SIGPIPE` on the fifo is
  suppressed with `F_SETNOSIGPIPE`.
* **`.aif` / `.mp4` / `.m4a` cannot be demuxed from `pipe:0`** (they need seeks),
  so those route via `streamUrl` instead of the fifo path.
* **YouTube** streams need libav's reconnect flags; a TLS `-9806` failure means
  googlevideo dropped the socket.
* The packaged dylibs live in `Contents/Frameworks` and bake
  `@executable_path` rpaths.
