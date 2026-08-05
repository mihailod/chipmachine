# openmptplugin

PC and Amiga tracker formats, via **libopenmpt** (vendored, 0.8.7).

## Formats

ProTracker, ScreamTracker III, FastTracker II, Impulse Tracker, OpenMPT,
ScreamTracker II, NoiseTracker, Soundtracker, Mod's Grave, UltraTracker,
Composer 669 / UNIS 669, MultiTracker, OctaMED, Farandole Composer, DigiTracker,
Extreme's Tracker, Velvet Studio, DSIK Format, DSMI, ASYLUM, Oktalyzer,
X-Tracker, PolyTracker, Epic Megagames, MASI, MadTracker 2, DigiBooster Pro,
DigiBooster, Imago Orpheus, Galaxy Sound System.

Added by the 0.8.7 upgrade: Symphonie / Symphonie Pro (Amiga "pseudo-DAW" with
software mixer + real-time echo DSP), Digital Symphony, Face The Music, Graoumf
Tracker 1 & 2, TCB Tracker, Real Tracker, Astroidea XMF, Composer 667, EasyTrax,
FM Tracker, CBA.

## Extensions

`.mod` `.xm` `.it` `.s3m` `.mptm` `.stm` `.nst` `.m15` `.stk` `.wow` `.ult`
`.669` `.mtm` `.med` `.far` `.mdl` `.ams` `.dsm` `.amf` `.okt` `.omf` `.dmf`
`.mt2` `.dbm` `.digi` `.imf` `.j2b` `.gdm` `.umx` `.mo3` `.symmod` `.dsym`
`.dsyn` `.dysn` `.ftm` `.gt2` `.gtk` `.tcb` `.rtm` `.xmf` `.667` `.etx` `.fmt`
`.cba` `.c67` `.fst` `.ice` `.mmcmp` `.mms` `.mus` `.oxm` `.plm` `.ppm` `.psm`
`.pt36` `.ptm` `.sfx` `.sfx2` `.stp` `.stx` `.xpk`

`canHandle` ends in a content probe, so shared extensions route by content.

## Shared-extension routing

* `.mus`, `.psm` and `.stp` are claimed by libopenmpt but a SID `.mus` falls
  through to the Compute!'s Sidplayer player (libvice in Plus, the clean-room
  [musplugin](../musplugin/README.md) in MAS), a ZX `.psm` to ZXTune, and a ZX
  `.stp` to the [ZX Spectrum AY engine](../zxayplugin/README.md).
* `.ftm` is shared with FamiTracker (magic `FamiTracker Module`); the Atari
  **Face The Music** files (magic `FTMN`) are ours. See
  [famitrackerplugin](../famitrackerplugin/README.md).
* `.tcb` is owned here; the plugin declines AM-only synth TCB variants.
* Some Amiga formats libopenmpt can also decode (Future Composer, Puma, Game
  Music Creator, Images Music System, …) are intentionally routed to
  [uadeplugin](../uadeplugin/README.md) instead, which uses the original 68k
  replayers.

## Local patches to vendored libopenmpt

* **`.dsm` v1 (DSIK "old" Internal Format).** `.dsm` covers three unrelated
  DSIK/Dynamic-Studio variants. libopenmpt natively plays the newer DSIK "RIFF"
  format (`RIFF…DSMF`) and Dynamic Studio (`DSm`), but not the original DSIK
  Internal Format (`DSM` + 0x10, e.g. the Necros tunes). Support was added in a
  local patch to `Load_dsm.cpp`, with the loader adapted from MilkyTracker's
  `LoaderDSMv1` (BSD-3-Clause).
* **`.dsyn` / `.dysn`.** modland's misspelled **Digital Symphony** extensions.
  Almost all of the `Digital Symphony/` corpus is `.dsym`, but 8 files in one
  composer dir use these spellings and so routed to no plugin at all. The bytes
  are ordinary Digital Symphony and `Load_dsym` decodes them unchanged, so
  `canHandle` claims both spellings — gated on the loader's own magic
  (`\x02\x01\x13\x13\x14\x12\x01\x0B`) so a misnamed non-DSym file skips cleanly
  instead of hard-failing.
* **`.omf` (Onyx Music File).** A MOD-like Amiga format from the 1993 musicdisk
  *Jangle* by Onyx (modland's `Onyx Music File/`, 24 tunes). It never had a
  standalone replayer — the tunes were only playable through the original
  musicdisk executable. Playback reuses libopenmpt's existing MOD engine.
* **`.670` (CDFM / Composer 670)** is handled as its own case, not by the `.c67`
  path.
