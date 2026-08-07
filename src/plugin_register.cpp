#include <vector>

/* vice bridge, ffmpeg and uade are now integrated */

extern "C" {
    void adplugin_register();      // Adlib / OPL
#ifndef CM_NO_AO
    // Audio Overload: Saturn/QSound/SPU AND PlayStation 1-2 PSF (which it took
    // over from the deleted heplugin -- AOSDK's PS1/PS2 engines are HLE and
    // need no Sony BIOS image). MAME 1997-2008 (non-commercial) + GPL-2 peops
    // SPU, so plus build only.
    void aoplugin_register();
#endif
#ifndef CM_NO_AYFLY
    // ZX Spectrum AY trackers -- GPL-2 (Ayfly + z80ex), plus build only.
    void ayflyplugin_register();
#endif
    void zxayplugin_register();    // the same formats, no copyleft; both builds
    void gmeplugin_register();     // Game Music Emu (NES, SNES, etc.)
    void libvgmplugin_register();  // OPL2/OPL3 VGM/VGZ (AdLib/SB) via libvgm
    void gsfplugin_register();     // Gameboy Advance
    void hivelyplugin_register();  // Amiga HivelyTracker
#ifndef CM_NO_HT
    void htplugin_register();      // Hudson Soft (TurboGrafx)
#endif
#ifndef CM_NO_MDX
    void mdxplugin_register();     // Sharp X68000 -- GPL-2 mdxmini, plus only
#endif
    void mp3plugin_register();     // MPEG Audio
#ifndef CM_NO_NDS
    void ndsplugin_register();     // Nintendo DS
#endif
    void openmptplugin_register(); // Trackers (MOD, XM, IT, S3M)
    void rsnplugin_register();     // Rar-packaged SNES
    void s98plugin_register();     // PC-98
    void fmpplugin_register();     // PC-98 FMP
    void sndhplugin_register();    // Atari ST/STE SNDH via AtariAudio (MIT)
#ifndef CM_NO_SC68
    void sc68plugin_register();    // Atari ST .sc68 -- GPL-3, plus build only
#endif
    void stsoundplugin_register(); // Atari ST (YM2149)
    void tedcrplugin_register();   // Commodore 264 series (TED)
#ifndef CM_NO_USF
    void usfplugin_register();     // Nintendo 64
#endif
    void v2plugin_register();      // Farbrausch V2
#ifndef CM_NO_VICE
    void vicepluginbridge_register(); // C64 Compute! Sidplayer .mus/.str -- GPL, plus build only
#endif
    void ffmpegplugin_register();
#ifndef CM_NO_UADE
    void uadeplugin_register();    // Amiga 68k replayers -- GPL, plus build only
#endif
    void pxtoneplugin_register();
    void ptkplugin_register();
    void orgplugin_register();     // Organya / Cave Story (.org)
    void sunvoxplugin_register();  // SunVox modular synth (.sunvox)
    void eupplugin_register();     // Euphony / FM Towns & PC-98 (.eup)
    void kssplugin_register();     // MGSDRV / MSX (.mgs) via libkss
    void quartetplugin_register(); // Microdeal Quartet / Atari ST (.4v, .4q)
    void wsrplugin_register();     // Bandai WonderSwan (.wsr), ares V30MZ + own machine
#ifndef CM_NO_ZXTUNE
    // ZX Spectrum Sound Tracker 1.1 / TFM / Chip Tracker -- GPL-3, plus only.
    void zxtuneplugin_register();
#endif
#ifndef CM_NO_POKEYNOISE
    void pokeynoiseplugin_register(); // Atari 800 PokeyNoise (.pn) via ASAP -- GPL-2, plus build only
#endif
    void bbsongplugin_register();  // Beepola .bbsong (ZX Spectrum beeper) via Z80 + speaker sampling
    void soundsmithplugin_register(); // Apple IIgs SoundSmith (bare song + .W wavebank) via Ensoniq 5503
    void ixsplugin_register();     // Ixalance (.ixs) via webixs (RE'd Shortcut Software synth tracker)
    void musxplugin_register();    // Acorn Archimedes Tracker (.musx) via libxmp arch_loader
    void fnkplugin_register();     // Funktracker (.fnk, MS-DOS, magic "Funk") via libxmp fnk_loader
    void cocoplugin_register();    // Coconizer / Acorn Archimedes (.coco) via libxmp coco_loader
    void mgtplugin_register();     // Megatracker / Atari ST (.mgt) via libxmp mgt_loader
    void medplugin_register();     // Old MED / Amiga "Music Editor" (.med, magic MED\x02..\x04) via libxmp med2/3/4_loader
    void sbstudioplugin_register(); // SBStudio / MS-DOS (.pac) via vendored libpac
#ifndef CM_NO_MAXTRAX
    // MaxTrax / Amiga (.mxtx) via ScummVM MaxTrax+Paula -- GPL-3 or later,
    // plus build only.
    void maxtraxplugin_register();
#endif
    void sksplugin_register();     // STarKos / Amstrad CPC (.sks) via Arkos Tracker 3
    void nedplugin_register();     // NerdTracker II / NES (.ned) via blargg Nes_Snd_Emu
    void sccmusixxplugin_register(); // SCC-Musixx / MSX Konami SCC (.SNG) via Z80 + emu2212
    void copplugin_register();     // Sam Coupe COP / SAA1099 (.cop) via GME Z80 + SAASound
    void playerproplugin_register(); // PlayerPRO / Macintosh tracker (.mad MADG/MADF/MADK) via vendored MADDriver
    void jxsplugin_register();     // JayTrax / cross-platform synth tracker (.jxs) via Rhino's replayer (C port)
    void monotoneplugin_register(); // MONOTONE / PC-speaker tracker (.mon) via vendored PTPlayer (BSD-3)
    void mikmodplugin_register();  // MikMod UNITRK / UNIMOD (.uni) via vendored libmikmod slice
#ifndef CM_NO_FAMITRACKER
    // FamiTracker (.ftm) NES 2A03 + expansions via vendored FamiTracker CX --
    // GPL-2 or later (jsr's engine, APU included), plus build only.
    void famitrackerplugin_register();
#endif
#ifndef CM_NO_GOATTRACKER
    void goattrackerplugin_register(); // GoatTracker (.sng) C64 SID via vendored GoatTracker player + reSID
#endif
    void dmfplugin_register();         // DefleMask (.dmf) multi-system chiptune via vendored Furnace engine
    void dmfcrplugin_register();       // DefleMask (.dmf) SEGA Genesis + Master System, clean-room parser + sequencer
    void vgmstreamplugin_register();   // vgmstream (.adx/.hca/.fsb/... hundreds of game-audio containers) via vendored vgmstream
    void victrackerplugin_register();  // VIC-TRACKER (.vt) Commodore VIC-20 via MyLittle6502 + our VIC-I sound
    void klystrackplugin_register();   // Klystrack (.kt) via vendored libksnd / klystron cyd synth
    void csidplugin_register();        // Commodore 64 SID (.sid/.rsid) via Hermit's cSID (WTFPL)
    void musplugin_register();         // Compute! Sidplayer (.mus/.str) -- clean-room sequencer on cSID
}

void register_plugins() {
    adplugin_register();
#ifndef CM_NO_AO
    aoplugin_register();
#endif
#ifndef CM_NO_AYFLY
    ayflyplugin_register();
#endif
    // Before gmeplugin so OPL VGMs are claimed here first; the two canHandle
    // gates are disjoint (GME declines OPL) so order is belt-and-braces.
    libvgmplugin_register();
    gmeplugin_register();
    gsfplugin_register();
    hivelyplugin_register();
#ifndef CM_NO_HT
    // GPL-3 (Highly Theoretical SegaCore) -- plus build only.
    htplugin_register();
#endif
#ifndef CM_NO_MDX
    // GPL-2 (mdxmini / BouKiCHi's mdxplay derivative) -- excluded from the Mac
    // App Store build (CM_VARIANT=mas), where the mdxplugin target is not built
    // at all. Sole claimer of ".mdx", so all 6,913 rows drop from the mas index.
    mdxplugin_register();
#endif
    mp3plugin_register();
#ifndef CM_NO_NDS
    // GPL-2 (DeSmuME-derived vio2sf) -- plus build only.
    ndsplugin_register();
#endif
    openmptplugin_register();
    rsnplugin_register();
    s98plugin_register();
    fmpplugin_register();
    // AtariAudio (MIT) owns .sndh in BOTH variants. It declares priority 2 --
    // above SC68Plugin's 1 -- so registration order here is belt-and-braces:
    // what actually decides is the priority sort in ChipPlugin::createPlugins().
    sndhplugin_register();
#ifndef CM_NO_SC68
    // GPL-3 (libsc68 + emu68 + file68 + unice68) -- excluded from the Mac App
    // Store build (CM_VARIANT=mas), where the sc68plugin target is not built at
    // all. In the plus build it keeps .sc68 (which needs data/sc68/Replay's 95
    // prebuilt replay binaries and has no permissive equivalent) and stays
    // reachable as a .sndh fallback for anything AtariAudio declines.
    sc68plugin_register();
#endif
    stsoundplugin_register();
    tedcrplugin_register();
#ifndef CM_NO_USF
    // GPL-2 (lazyusf2 / Mupen64Plus) -- plus build only.
    usfplugin_register();
#endif
    v2plugin_register();
    // Before vicepluginbridge, so cSID (permissive) claims .sid/.rsid and VICE
    // is left with only the .mus/.str Compute! Sidplayer tunes it alone can
    // play. The two gates are disjoint anyway -- CSIDPlugin never claims
    // .mus/.str and VicePlugin no longer claims .sid/.rsid -- so the ordering
    // is belt-and-braces, and it is what makes the mas build (no VICE at all)
    // behave identically for every .sid in the catalog.
    csidplugin_register();
#ifndef CM_NO_VICE
    // GPLv2 -- excluded from the Mac App Store build (CM_VARIANT=mas), where the
    // vicepluginbridge target is not built at all. Only .mus/.str reach it.
    vicepluginbridge_register();
#endif
    // AFTER vicepluginbridge on purpose -- registration order is the variant
    // gate for Compute! Sidplayer. In the plus build VICE exists and claims
    // .mus/.str first, so that build is completely unchanged; in the mas build
    // vicepluginbridge is not linked at all and this permissive sequencer picks
    // them up, making ~6.5k Sidplayer songs playable there instead of hidden.
    musplugin_register();
    ffmpegplugin_register();
#ifndef CM_NO_UADE
    // GPLv2 -- excluded from the Mac App Store build (CM_VARIANT=mas), where the
    // uadeplugin target is not built at all. Registered AFTER openmptplugin, so
    // anything libopenmpt can decode (including via OpenMPTPlugin's content
    // probe) is already claimed by the time we get here and UADE only sees the
    // Amiga formats no portable decoder covers.
    uadeplugin_register();
#endif
    pxtoneplugin_register();
    ptkplugin_register();
    orgplugin_register();
    sunvoxplugin_register();
    eupplugin_register();
    kssplugin_register();
    quartetplugin_register();
    wsrplugin_register();
#ifndef CM_NO_ZXTUNE
    zxtuneplugin_register();
#endif
    // zxayplugin registers AFTER BOTH the engines it replaces -- ayflyplugin
    // above and zxtuneplugin here. All three claim overlapping extensions at
    // equal priority and ties break on registration order, so the plus build
    // keeps routing every ZX AY song exactly where it did before and stays
    // unchanged. In the mas build neither of the other two is compiled and
    // this is the only claimant.
    // See [[plugin-order-stable-sort]], CM_HAVE_AYFLY and CM_HAVE_ZXTUNE.
    zxayplugin_register();
#ifndef CM_NO_POKEYNOISE
    // GPL-2 (ASAP) -- excluded from the Mac App Store build (CM_VARIANT=mas),
    // where the pokeynoiseplugin target is not built at all. Only the 17 modland
    // ".pn" tunes are lost with it; ".sap" (the 6,617-song ASMA corpus) is
    // claimed by gmeplugin first in BOTH variants and is unaffected.
    pokeynoiseplugin_register();
#endif
    bbsongplugin_register();
    soundsmithplugin_register();
    ixsplugin_register();
    musxplugin_register();
    fnkplugin_register();
    cocoplugin_register();
    mgtplugin_register();
    medplugin_register();
    sbstudioplugin_register();
#ifndef CM_NO_MAXTRAX
    // GPL-3+ (ScummVM's maxtrax.{h,cpp} + paula.{h,cpp}) -- excluded from the
    // Mac App Store build (CM_VARIANT=mas), where the maxtraxplugin target is
    // not built at all. All 93 ".mxtx" rows are dropped from that variant's
    // index to match; the extension is sole-claimed, so no formatPlayer key is
    // needed -- see CM_HAVE_MAXTRAX in CMakeLists.txt.
    maxtraxplugin_register();
#endif
    sksplugin_register();
    nedplugin_register();
    sccmusixxplugin_register();
    copplugin_register();
    playerproplugin_register();
    jxsplugin_register();
    monotoneplugin_register();
    mikmodplugin_register();
#ifndef CM_NO_FAMITRACKER
    // GPL-2+ (the whole FamiTracker CX engine slice) -- excluded from the Mac
    // App Store build (CM_VARIANT=mas), where the famitrackerplugin target is
    // not built at all. Its 1,597 .ftm rows are dropped from that variant's
    // index by the formatPlayer keys in MusicDatabase.cpp; the 95 Atari "Face
    // The Music" .ftm rows are OpenMPT's and are untouched.
    famitrackerplugin_register();
#endif
#ifndef CM_NO_GOATTRACKER
    // GPLv2 twice over (GoatTracker's gplay.c/gsong.c + reSID) -- excluded from
    // the Mac App Store build (CM_VARIANT=mas), where the goattrackerplugin
    // target is not built at all. The 104 GoatTracker-format .sng rows are
    // dropped from the mas index to match; see songFormatHasNoPlayer().
    goattrackerplugin_register();
#endif
#ifndef CM_NO_DMF
    // The whole vendored Furnace engine is GPL-2.0-or-later (engine, all 80
    // DivPlatform wrappers, and several sound cores) -- excluded from the Mac
    // App Store build (CM_VARIANT=mas), where the dmfplugin target is not built
    // at all. The DefleMask-format .dmf rows that nothing else can play are
    // dropped from the mas index to match; see songFormatHasNoPlayer(). The
    // unrelated X-Tracker DDMF .dmf rows keep playing via libopenmpt in both
    // variants.
    //
    // BEFORE dmfcrplugin, and that ordering is the whole variant gate -- both
    // declare priority 1, so stable_sort leaves them in registration order and
    // MusicPlayer::fromFile takes the first that claims the file. In the plus
    // build Furnace therefore keeps every .dmf, including the Genesis ones
    // dmfcrplugin could play, so plus playback is untouched and stays usable as
    // the A/B reference. In mas this block is compiled out and dmfcrplugin is
    // the only claimant. Same arrangement as musplugin behind vicepluginbridge
    // and csidplugin ahead of it -- see the stable_sort note in chipplugin.h.
    dmfplugin_register();
#endif
    // Clean-room DefleMask player -- SEGA Genesis and SEGA Master System (see
    // dmfcrplugin/README.md). Built in BOTH variants; declines every other
    // DefleMask system, and any DMF version outside 0x11-0x18, so in the plus
    // build the files it cannot play fall through to Furnace rather than being
    // claimed and mis-played.
    dmfcrplugin_register();
    vgmstreamplugin_register();
    victrackerplugin_register();
    klystrackplugin_register();
}
