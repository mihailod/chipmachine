#include <vector>

/* vice bridge, ffmpeg and uade are now integrated */

extern "C" {
    void adplugin_register();      // Adlib / OPL
    void aoplugin_register();      // Adlib Objective
    void ayflyplugin_register();   // ZX Spectrum / CPC (AY-3-8910)
    void gmeplugin_register();     // Game Music Emu (NES, SNES, etc.)
    void libvgmplugin_register();  // OPL2/OPL3 VGM/VGZ (AdLib/SB) via libvgm
    void gsfplugin_register();     // Gameboy Advance
    void heplugin_register();      // PS1/PS2 Executables
    void hivelyplugin_register();  // Amiga HivelyTracker
    void htplugin_register();      // Hudson Soft (TurboGrafx)
    void mdxplugin_register();     // Sharp X68000
    void mp3plugin_register();     // MPEG Audio
    void ndsplugin_register();     // Nintendo DS
    void openmptplugin_register(); // Trackers (MOD, XM, IT, S3M)
    void rsnplugin_register();     // Rar-packaged SNES
    void s98plugin_register();     // PC-98
    void fmpplugin_register();     // PC-98 FMP
    void sc68plugin_register();    // Atari ST
    void stsoundplugin_register(); // Atari ST (YM2149)
    void tedplugin_register();     // Commodore Plus/4
    void usfplugin_register();     // Nintendo 64
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
    void wsrplugin_register();     // Bandai WonderSwan (.wsr) via in_wsr
    void zxtuneplugin_register();  // ZX Spectrum Sound Tracker 1.1 (.st11) via ZXTune
    void pokeynoiseplugin_register(); // Atari 800 PokeyNoise (.pn) via ASAP (6502+POKEY)
    void bbsongplugin_register();  // Beepola .bbsong (ZX Spectrum beeper) via Z80 + speaker sampling
    void soundsmithplugin_register(); // Apple IIgs SoundSmith (bare song + .W wavebank) via Ensoniq 5503
    void ixsplugin_register();     // Ixalance (.ixs) via webixs (RE'd Shortcut Software synth tracker)
    void musxplugin_register();    // Acorn Archimedes Tracker (.musx) via libxmp arch_loader
    void fnkplugin_register();     // Funktracker (.fnk, MS-DOS, magic "Funk") via libxmp fnk_loader
    void cocoplugin_register();    // Coconizer / Acorn Archimedes (.coco) via libxmp coco_loader
    void mgtplugin_register();     // Megatracker / Atari ST (.mgt) via libxmp mgt_loader
    void medplugin_register();     // Old MED / Amiga "Music Editor" (.med, magic MED\x02..\x04) via libxmp med2/3/4_loader
    void sbstudioplugin_register(); // SBStudio / MS-DOS (.pac) via vendored libpac
    void maxtraxplugin_register(); // MaxTrax / Amiga (.mxtx) via ScummVM MaxTrax+Paula
    void sksplugin_register();     // STarKos / Amstrad CPC (.sks) via Arkos Tracker 3
    void nedplugin_register();     // NerdTracker II / NES (.ned) via blargg Nes_Snd_Emu
    void sccmusixxplugin_register(); // SCC-Musixx / MSX Konami SCC (.SNG) via Z80 + emu2212
    void copplugin_register();     // Sam Coupe COP / SAA1099 (.cop) via GME Z80 + SAASound
    void playerproplugin_register(); // PlayerPRO / Macintosh tracker (.mad MADG/MADF/MADK) via vendored MADDriver
    void jxsplugin_register();     // JayTrax / cross-platform synth tracker (.jxs) via Rhino's replayer (C port)
    void monotoneplugin_register(); // MONOTONE / PC-speaker tracker (.mon) via vendored PTPlayer (BSD-3)
    void mikmodplugin_register();  // MikMod UNITRK / UNIMOD (.uni) via vendored libmikmod slice
    void famitrackerplugin_register(); // FamiTracker (.ftm) NES 2A03 + expansions via vendored FamiTracker CX
#ifndef CM_NO_GOATTRACKER
    void goattrackerplugin_register(); // GoatTracker (.sng) C64 SID via vendored GoatTracker player + reSID
#endif
    void dmfplugin_register();         // DefleMask (.dmf) multi-system chiptune via vendored Furnace engine
    void vgmstreamplugin_register();   // vgmstream (.adx/.hca/.fsb/... hundreds of game-audio containers) via vendored vgmstream
    void victrackerplugin_register();  // VIC-TRACKER (.vt) Commodore VIC-20 via fake6502 + VICE VIC-I sound
    void klystrackplugin_register();   // Klystrack (.kt) via vendored libksnd / klystron cyd synth
    void csidplugin_register();        // Commodore 64 SID (.sid/.rsid) via Hermit's cSID (WTFPL)
    void musplugin_register();         // Compute! Sidplayer (.mus/.str) -- clean-room sequencer on cSID
}

void register_plugins() {
    adplugin_register();
    aoplugin_register();
    ayflyplugin_register();
    // Before gmeplugin so OPL VGMs are claimed here first; the two canHandle
    // gates are disjoint (GME declines OPL) so order is belt-and-braces.
    libvgmplugin_register();
    gmeplugin_register();
    gsfplugin_register();
    heplugin_register();
    hivelyplugin_register();
    htplugin_register();
    mdxplugin_register();
    mp3plugin_register();
    ndsplugin_register();
    openmptplugin_register();
    rsnplugin_register();
    s98plugin_register();
    fmpplugin_register();
    sc68plugin_register();
    stsoundplugin_register();
    tedplugin_register();
    usfplugin_register();
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
    zxtuneplugin_register();
    pokeynoiseplugin_register();
    bbsongplugin_register();
    soundsmithplugin_register();
    ixsplugin_register();
    musxplugin_register();
    fnkplugin_register();
    cocoplugin_register();
    mgtplugin_register();
    medplugin_register();
    sbstudioplugin_register();
    maxtraxplugin_register();
    sksplugin_register();
    nedplugin_register();
    sccmusixxplugin_register();
    copplugin_register();
    playerproplugin_register();
    jxsplugin_register();
    monotoneplugin_register();
    mikmodplugin_register();
    famitrackerplugin_register();
#ifndef CM_NO_GOATTRACKER
    // GPLv2 twice over (GoatTracker's gplay.c/gsong.c + reSID) -- excluded from
    // the Mac App Store build (CM_VARIANT=mas), where the goattrackerplugin
    // target is not built at all. The 104 GoatTracker-format .sng rows are
    // dropped from the mas index to match; see songFormatHasNoPlayer().
    goattrackerplugin_register();
#endif
    dmfplugin_register();
    vgmstreamplugin_register();
    victrackerplugin_register();
    klystrackplugin_register();
}
