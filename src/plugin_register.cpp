#include <vector>

/* vice bridge, ffmpeg and uade are now integrated */

extern "C" {
    void adplugin_register();      // Adlib / OPL
    void aoplugin_register();      // Adlib Objective
    void ayflyplugin_register();   // ZX Spectrum / CPC (AY-3-8910)
    void gmeplugin_register();     // Game Music Emu (NES, SNES, etc.)
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
    void vicepluginbridge_register();
    void ffmpegplugin_register();
    void uadeplugin_register();
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
    void cocoplugin_register();    // Coconizer / Acorn Archimedes (.coco) via libxmp coco_loader
    void mgtplugin_register();     // Megatracker / Atari ST (.mgt) via libxmp mgt_loader
    void sbstudioplugin_register(); // SBStudio / MS-DOS (.pac) via vendored libpac
    void maxtraxplugin_register(); // MaxTrax / Amiga (.mxtx) via ScummVM MaxTrax+Paula
    void sksplugin_register();     // STarKos / Amstrad CPC (.sks) via Arkos Tracker 3
    void nedplugin_register();     // NerdTracker II / NES (.ned) via blargg Nes_Snd_Emu
    void sccmusixxplugin_register(); // SCC-Musixx / MSX Konami SCC (.SNG) via Z80 + emu2212
    void playerproplugin_register(); // PlayerPRO / Macintosh tracker (.mad MADG/MADF/MADK) via vendored MADDriver
    void jxsplugin_register();     // JayTrax / cross-platform synth tracker (.jxs) via Rhino's replayer (C port)
    void monotoneplugin_register(); // MONOTONE / PC-speaker tracker (.mon) via vendored PTPlayer (BSD-3)
    void mikmodplugin_register();  // MikMod UNITRK / UNIMOD (.uni) via vendored libmikmod slice
}

void register_plugins() {
    adplugin_register();
    aoplugin_register();
    ayflyplugin_register();
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
    vicepluginbridge_register();
    ffmpegplugin_register();
    uadeplugin_register();
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
    cocoplugin_register();
    mgtplugin_register();
    sbstudioplugin_register();
    maxtraxplugin_register();
    sksplugin_register();
    nedplugin_register();
    sccmusixxplugin_register();
    playerproplugin_register();
    jxsplugin_register();
    monotoneplugin_register();
    mikmodplugin_register();
}
