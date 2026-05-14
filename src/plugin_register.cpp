#include <vector>

/* vice uad ffmpeg -- TOODOO ADD THESE ONCE YOU FIGURE OUT HOW */

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
    void sc68plugin_register();    // Atari ST
    void stsoundplugin_register(); // Atari ST (YM2149)
    void tedplugin_register();     // Commodore Plus/4
    void usfplugin_register();     // Nintendo 64
    void v2plugin_register();      // Farbrausch V2
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
    sc68plugin_register();
    stsoundplugin_register();
    tedplugin_register();
    usfplugin_register();
    v2plugin_register();
}
