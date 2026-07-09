#ifndef GSOUND_H
#define GSOUND_H

// Minimal replacement for GoatTracker's gsound.h. The original wraps SDL audio,
// HardSID and CatWeasel output; the plugin drives reSID directly, so only the
// clock-rate constants used by gsid.cpp are kept here.

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

#endif
