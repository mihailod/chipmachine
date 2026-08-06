// Synthetic YRW801 wavetable for the OPL4 (YMF278B).
//
// MSX MoonSound VGM logs carry only their own sample-RAM upload (VGM data block
// 0x87); the instruments they play come from the MoonSound cartridge's internal
// 2 MB Yamaha YRW801 wave ROM, which the format does not embed and which we
// cannot ship. Without it the chip plays silence at best -- see cm_yrw801.c for
// what the bank is, how its tuning was derived and what it is not.
//
// Returns a 0x200000-byte image in YRW801 layout (384 twelve-byte wave headers
// followed by sample data), built once on first call and shared by every OPL4
// instance afterwards. NULL only if the allocation fails, in which case the
// caller keeps its silent stub.

#ifndef CM_YRW801_H
#define CM_YRW801_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CM_YRW801_SIZE 0x200000

const unsigned char* cm_yrw801_bank(void);

#ifdef __cplusplus
}
#endif

#endif	// CM_YRW801_H
