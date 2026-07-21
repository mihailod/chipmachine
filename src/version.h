/*

Simulating a newer version without GitHub:

cd /Users/mihailod/Documents/chipmachine-as/build
echo "9.9.9" > DEBUG_UPDATE_VERSION
./chipmachine

After testing:
rm build/DEBUG_UPDATE_VERSION

*/

#pragma once
// PROGRAM_NAME is normally injected by CMake per build variant (see CM_VARIANT
// in CMakeLists.txt / variants.conf): "ChipMachine" for the Mac App Store build,
// "ChipMachinePlus" for the full/GitHub build. This fallback only applies to
// targets that don't receive the define (e.g. cmtest); it must stay in sync with
// the plus variant's PLUS_PROGRAM_NAME in variants.conf.
#ifndef PROGRAM_NAME
#define PROGRAM_NAME "ChipMachinePlus"
#endif
#define VERSION_STR "2.0.1"
