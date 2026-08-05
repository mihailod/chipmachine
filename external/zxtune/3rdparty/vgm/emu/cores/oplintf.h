#ifndef __OPLINTF_H__
#define __OPLINTF_H__

#include "../EmuStructs.h"

#if ! defined(SNDDEV_SELECT) && defined(SNDDEV_YM3812)
// undefine one of the variables to disable the cores
#define EC_YM3812_MAME		// enable YM3812 core from MAME
#define EC_YM3812_ADLIBEMU	// enable AdLibEmu core (from DOSBox)
#define EC_YM3812_NUKED		// enable Nuked OPL3 core
#endif

// [chipmachine local patch] Core selection for the YM3526 and the Y8950,
// following the pattern the YM3812 above already uses. Upstream has only the
// MAME core (fmopl.c) for those two chips, so it defines no EC_ macros for them
// and reaches into fmopl directly under SNDDEV_YM3526 / SNDDEV_Y8950. "YMFM"
// selects the BSD-3 ymfm cores wrapped by
// musicplayer/src/plugins/libvgmplugin/opl_ymfm.cpp -- that is what lets the Mac
// App Store build drop the GPL-2.0+ fmopl.c. Re-apply on revendor; oplintf.c.orig
// and oplintf.h.orig sit next to these files.
#ifndef SNDDEV_SELECT
#ifdef SNDDEV_YM3526
#define EC_YM3526_MAME
#endif
#ifdef SNDDEV_Y8950
#define EC_Y8950_MAME
#endif
#endif


#ifdef SNDDEV_YM3812
extern const DEV_DECL sndDev_YM3812;
#endif
#ifdef SNDDEV_YM3526
extern const DEV_DECL sndDev_YM3526;
#endif
#ifdef SNDDEV_Y8950
extern const DEV_DECL sndDev_Y8950;
#endif

#endif	// __OPLINTF_H__
