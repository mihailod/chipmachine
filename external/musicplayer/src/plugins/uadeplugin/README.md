# uadeplugin — UADE

Amiga "exotic" (Delitracker) formats, played by the **original 68k replayers**
under UADE.

## Build gating

**ChipMachinePlus only** (build gate `CM_HAVE_UADE`; `cmtest` coverage for it is
Plus-only). The Mac App Store build ships without UADE and without the
`data/uade` payload, and the catalog hides the Amiga custom-replayer formats UADE
alone handled. See [`LEGAL-PLUS`](../../../../../LEGAL-PLUS).

## Vendoring

Eagleplayers and the format database are vendored from **UADE 3.05**
(zakalwe.fi, 2024-10-06), which adds ~19 new replayers over the previous
2.13-era set (PreTracker, Protracker 4, TCB Tracker, AProSys, Delta Music 1.3,
the Prowizard pack family and more).

The integration is **data-driven**: adding a format means adding it to
`supported_ext` and to the UADE conf, not writing code.

## Replayers

ActionAmics AbyssHighestExperience ADPCM-mono AM-Composer AMOS ArtAndMagic
Alcatraz-Packer ArtOfNoise-4V ArtOfNoise-8V AudioSculpture BeathovenSynthesizer
BenDaglish BenDaglish-SID BladePacker ChipTracker Cinemaware CoreDesign custom
CustomMade DariusZendeh DaveLowe DaveLowe-Deli DaveLoweNew DavidHanney
DavidWhittaker DeltaMusic2.0 DeltaMusic1.3 Desire DIGI-Booster
DigitalSonixChrome DigitalSoundStudio DynamicSynthesizer EMS EMS-6
FashionTracker FutureComposer1.3 FutureComposer1.4 Fred FredGray
FutureComposer-BSI FuturePlayer ForgottenWorlds-Game GlueMon EarAche HowieDavies
JochenHippel-CoSo QuadraComposer ImagesMusicSystem Infogrames InStereo
InStereo2.0 JamCracker JankoMrsicFlogel JasonBrooke JasonPage JeroenTel
JesperOlsen JochenHippel JochenHippel-7V Jochen-Hippel-ST KrisHatlelid Laxity
LegglessMusicEditor ManiacsOfNoise MagneticFieldsPacker MajorTom Mark-Cooksey
Mark-Cooksey-Old MarkII MartinWalker Maximum-Effect MCMD MED Medley
MIDI-Loriciel MikeDavies MMDC Mugician MugicianII MusicAssembler MusicMaker-4V
MusicMaker-8V MultiMedia-Sound NovoTradePacker NTSP-system Octa-MED Oktalyzer
onEscapee PaulRobotham PaulShields PaulSummers PeterVerswyvelen PierreAdane
ProfessionalSoundArtists PTK-Prowiz PumaTracker RichardJoseph RiffRaff
RobHubbard SCUMM SeanConnolly SeanConran SIDMon1.0 SIDMon2.0 Silmarils
SonicArranger SonicArranger-pc-all SonixMusicDriver SoundProgrammingLanguage
SoundControl SoundFactory Sound-FX SoundImages SoundMaster SoundMon2.0
SoundMon2.2 SoundPlayer Special-FX Special-FX-ST SpeedyA1System SpeedySystem
SteveBarrett SteveTurner SUN-Tronic Synth SynthDream SynthPack SynTracker TFMX
TFMX-1.5-TFHD TFMX-7V TFMX-7V-TFHD TFMX-Pro TFMX-Pro-TFHD TFMX-ST ThomasHermann
TimFollin TheMusicalEnlightenment TomyTracker Tronic UFO UltimateSoundtracker
VoodooSupremeSynthesizer WallyBeben YM-2149 MusiclineEditor Soundtracker-IV
Sierra-AGI DirkBialluch Quartet Quartet-PSG Quartet-ST AProSys Anders-Oland
Andrew-Parton Ashley-Hogg GMC Janne-Salmijarvi-Optimizer Kim-Christensen
Mosh-Packer Nick-Pelling-Packer Paul-Tonge PreTracker Protracker4
RichardJoseph-Player RobHubbard-ST TCB-Tracker TimeTracker Titanics-Packer
ZoundMonitor

## Extensions

Matched as a filename **prefix or suffix** (modland names many of these
`prefix.song`, not `song.ext`):

`.smod` `.lion` `.okta` `.sid` `.ymst` `.jb` `.ast` `.ahx` `.thx` `.adpcm`
`.amc` `.nt` `.abk` `.aam` `.alp` `.aon` `.aon4` `.aon8` `.adsc` `.mod_adsc4`
`.bss` `.bd` `.BDS` `.uds` `.kris` `.cin` `.core` `.cus` `.cust` `.custom` `.cm`
`.rk` `.rkb` `.dz` `.mkiio` `.dl` `.dl_deli` `.dln` `.dh` `.dw` `.dwold` `.dlm2`
`.dm2` `.dlm1` `.dm1` `.dsr` `.db` `.digi` `.dsc` `.dss` `.dns` `.ems` `.emsv6`
`.ex` `.fc13` `.fc3` `.fc` `.fc14` `.fc4` `.fred` `.gray` `.bfc` `.bsi`
`.fc-bsi` `.fp` `.fw` `.glue` `.gm` `.ea` `.mg` `.hd` `.hipc` `.soc` `.emod`
`.qc` `.ims` `.dum` `.is` `.is20` `.jam` `.jc` `.jmf` `.jcb` `.jcbo` `.jpn`
`.jpnd` `.jp` `.jt` `.mon_old` `.jo` `.hip` `.mcmd` `.sog` `.hip7` `.s7g` `.hst`
`.kh` `.powt` `.pt` `.lme` `.mon` `.mfp` `.hn` `.mtp2` `.thn` `.mc` `.mcr`
`.mco` `.mk2` `.mkii` `.avp` `.mw` `.max` `.mcmd_org` `.med` `.mmd0` `.mmd1`
`.mmd2` `.mso` `.midi` `.md` `.mmdc` `.dmu` `.mug` `.dmu2` `.mug2` `.ma` `.mm4`
`.mm8` `.mms` `.ntp` `.two` `.octamed` `.okt` `.one` `.dat` `.ps` `.snk` `.pvp`
`.pap` `.psa` `.mod_doc` `.mod15` `.mod15_mst` `.mod_ntk` `.mod_ntk1`
`.mod_ntk2` `.mod_ntkamp` `.mod_flt4` `.mod` `.mod_comp` `.!pm!` `.40a` `.40b`
`.41a` `.50a` `.60a` `.61a` `.ac1` `.ac1d` `.aval` `.chan` `.cp` `.cplx` `.crb`
`.di` `.eu` `.fc-m` `.fcm` `.ft` `.fuz` `.fuzz` `.gmc` `.gv` `.hmc` `.hrt`
`.hrt!` `.ice` `.it1` `.kef` `.kef7` `.krs` `.ksm` `.lax` `.mexxmp` `.mpro`
`.np` `.np1` `.np2` `.noisepacker2` `.np3` `.noisepacker3` `.nr` `.nru` `.ntpk`
`.p10` `.p21` `.p30` `.p40a` `.p40b` `.p41a` `.p4x` `.p50a` `.p5a` `.p5x` `.p60`
`.p60a` `.p61` `.p61a` `.p6x` `.pha` `.pin` `.pm` `.pm0` `.pm01` `.pm1` `.pm10c`
`.pm18a` `.pm2` `.pm20` `.pm4` `.pm40` `.pmz` `.polk` `.pp10` `.pp20` `.pp21`
`.pp30` `.ppk` `.pr1` `.pr2` `.prom` `.pru` `.pru1` `.pru2` `.prun` `.prun1`
`.prun2` `.pwr` `.pyg` `.pygm` `.pygmy` `.skt` `.skyt` `.snt` `.snt!` `.st2`
`.st26` `.st30` `.star` `.stpk` `.tp` `.tp1` `.tp2` `.tp3` `.un2` `.unic`
`.unic2` `.wn` `.xan` `.xann` `.zen` `.puma` `.rjp` `.sng` `.riff` `.rh` `.rho`
`.sa-p` `.scumm` `.s-c` `.scn` `.scr` `.sid1` `.smn` `.sid2` `.mok` `.sa`
`.sonic` `.sa_old` `.smus` `.snx` `.tiny` `.spl` `.sc` `.sct` `.psf` `.sfx`
`.sfx13` `.tw` `.sm` `.sm1` `.sm2` `.sm3` `.smpro` `.bp` `.sndmon` `.bp3` `.sjs`
`.jd` `.doda` `.sas` `.ss` `.sb` `.jpo` `.jpold` `.sun` `.syn` `.sdr` `.osp`
`.st` `.synmod` `.tfmx1.5` `.tfhd1.5` `.tfmx7V` `.tfhd7V` `.mdat` `.tfmxpro`
`.tfhdpro` `.tfmx` `.mdst` `.thm` `.tf` `.tme` `.sg` `.dp` `.trc` `.tro`
`.tronic` `.ufo` `.mod15_ust` `.vss` `.wb` `.ym` `.ml` `.mod15_st-iv` `.agi`
`.tpu` `.qpa` `.sqt` `.qts` `.ftm` `.sdata` `.dux` `.aps` `.arp` `.ash` `.bye`
`.dm` `.hot` `.js` `.kim` `.mod3` `.mosh` `.mus` `.npp` `.pat` `.prt` `.ptm`
`.rj` `.sfx20` `.tcb` `.tits` `.tmk`

## Routing fixes and known cases

* **Hippel conf reroutes** — `.soc` (Hippel-ST), `.sog` (Hippel-ST vs TFMX),
  `.hipc` (CoSo, with an ST fallback), `.jo` (Jesper Olsen).
* **`SONG_END` loop** — `getSamples` returns -1 on `SONG_END` rather than
  spinning.
* **`EAGAIN` long intros** — a silent intro longer than 3 s forces audio out
  rather than being read as a dead tune.
* **Two-file companions** — RJP and SynthDream tunes need their sample
  companions fetched alongside the song. modland matches the prefix *before* the
  dot, which collides for e.g. `sonic.a2m`.
* **Formats declined here on purpose** — `.ast` (native All Sound Tracker saves),
  `.sm1`/`.sm2` (MSX BSAVE drumkit banks, see
  [kssplugin](../kssplugin/README.md)), `.doda` (Special FX ST; the stderr noise
  was benign).
* **`.med`** — pre-OctaMED "old MED" files crashed UADE and are handled by
  [medplugin](../medplugin/README.md) instead. `.ash` (Ashley Hogg) is a flat
  modland format.
* **`.sng`** is gated to Richard Joseph tunes only; C64 `.sng` goes to
  [goattrackerplugin](../goattrackerplugin/README.md).
* **`.mk2` / `.bss`** are lone Amiga outliers with player gaps (Mark II variant;
  katakis loading tune v1 renders silent).
* **`.thx`** is really AHX and is claimed by
  [hivelyplugin](../hivelyplugin/README.md).
* **TFMX** fixtures require the real `mdat.` / `smpl.` naming to load.
