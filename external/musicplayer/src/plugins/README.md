# Music plugins

One directory per decoder. Each has its own `README.md` covering the formats it
claims, the engine behind it, extension routing / content gating, licensing and
build gating, and any known gaps.

The user-facing overview lives in the [top-level README](../../../../README.md);
everything technical lives here.

## By platform

| Platform / family | Plugins |
|---|---|
| PC & Amiga trackers | [openmpt](openmptplugin/README.md), [uade](uadeplugin/README.md), [mikmod](mikmodplugin/README.md), [med](medplugin/README.md), [hively](hivelyplugin/README.md), [maxtrax](maxtraxplugin/README.md), [jxs](jxsplugin/README.md), [ixs](ixsplugin/README.md), [ptk](ptkplugin/README.md), [quartet](quartetplugin/README.md), [v2](v2plugin/README.md) |
| Commodore 64 / 264 / VIC-20 | [csid](csidplugin/README.md), [mus](musplugin/README.md), [vicebridge](vicepluginbridge/README.md), [goattracker](goattrackerplugin/README.md), [tedcr](tedcrplugin/README.md), [victracker](victrackerplugin/README.md) |
| ZX Spectrum / Amstrad CPC / Sam Coupé | [zxay](zxayplugin/README.md), [zxtune](zxtuneplugin/README.md), [ayfly](ayflyplugin/README.md), [bbsong](bbsongplugin/README.md), [sks](sksplugin/README.md), [cop](copplugin/README.md) |
| Atari ST/STE & 8-bit | [sndh](sndhplugin/README.md), [sc68](sc68plugin/README.md), [stsound](stsoundplugin/README.md), [mgt](mgtplugin/README.md), [pokeynoise](pokeynoiseplugin/README.md) |
| MSX / Japanese PCs | [kss](kssplugin/README.md), [sccmusixx](sccmusixxplugin/README.md), [fmp](fmpplugin/README.md), [s98](s98plugin/README.md), [eup](eupplugin/README.md), [mdx](mdxplugin/README.md), [mdxcr](mdxcrplugin/README.md) |
| Consoles | [gme](gmeplugin/README.md), [famitracker](famitrackerplugin/README.md), [ned](nedplugin/README.md), [gsf](gsfplugin/README.md), [nds](ndsplugin/README.md), [usf](usfplugin/README.md), [ao](aoplugin/README.md), [ht](htplugin/README.md), [wsr](wsrplugin/README.md), [rsn](rsnplugin/README.md), [vgmstream](vgmstreamplugin/README.md), [libvgm](libvgmplugin/README.md), [dmf](dmfplugin/README.md), [dmfcr](dmfcrplugin/README.md) |
| Acorn / Apple / Mac / DOS | [musx](musxplugin/README.md), [coco](cocoplugin/README.md), [soundsmith](soundsmithplugin/README.md), [playerpro](playerproplugin/README.md), [fnk](fnkplugin/README.md), [sbstudio](sbstudioplugin/README.md), [monotone](monotoneplugin/README.md) |
| Softsynths & modern trackers | [sunvox](sunvoxplugin/README.md), [klystrack](klystrackplugin/README.md), [pxtone](pxtoneplugin/README.md), [org](orgplugin/README.md) |
| Compressed / streaming audio | [ffmpeg](ffmpegplugin/README.md), [mp3](mp3plugin/README.md) |

## Cross-cutting notes

* **Plugin order is a `stable_sort`** — ties break on *registration order*. A
  plain sort silently re-routed 6,618 `.sap` songs once. Diff with
  `cmtest priority_map`.
* **Shared extensions route by content.** Many extensions are claimed by two or
  three unrelated formats (`.mus`, `.ftm`, `.dmf`, `.dtm`, `.imf`, `.sng`,
  `.psm`, `.mad`, `.cop`, `.gtk`); each plugin's `canHandle` is magic-gated, and
  the per-plugin README says which side it takes.
* **Build gating** (`CM_HAVE_*` in `CMakeLists.txt`) removes copyleft or
  non-redistributable engines from the Mac App Store variant. Rationale per
  component is in [`LEGAL-PLUS`](../../../../LEGAL-PLUS); the catalog hides rows
  the build cannot play via `songHasNoPlayer()` / `songFormatHasNoPlayer()`.
* **Release builds do not define `NDEBUG`**, so asserts in vendored engines are
  live and `abort()` rather than misbehaving quietly.
