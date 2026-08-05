#ifndef __OPNINTF_H__
#define __OPNINTF_H__

#include "../EmuStructs.h"

// [chipmachine local patch] Core selection, following the pattern every other
// libvgm chip interface already uses (2413intf.h, 262intf.h, ...). Upstream has
// only the MAME core for the OPN family, so it defines no EC_ macros here.
// "YMFM" selects the BSD-3 ymfm cores wrapped by
// musicplayer/src/plugins/libvgmplugin/opn_ymfm.cpp -- that is what lets the
// Mac App Store build drop the GPL-2.0+ fmopn.c. Re-apply on revendor.
#ifndef SNDDEV_SELECT
// undefine one of the variables to disable the cores
#define EC_YM2203_MAME
#define EC_YM2608_MAME
#define EC_YM2610_MAME
#endif

// YM2610 cfg.flags: 0 = YM2610 mode (4 FM channels), 1 = YM2610B mode (6 FM channels)

#ifdef SNDDEV_YM2203
extern const DEV_DECL sndDev_YM2203;
#endif
#ifdef SNDDEV_YM2608
extern const DEV_DECL sndDev_YM2608;
#endif
#ifdef SNDDEV_YM2610
extern const DEV_DECL sndDev_YM2610;
#endif

#endif	// __OPNINTF_H__
