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
// VERSION_STR is the MARKETING version -- what the user sees in the title bar,
// the help screen and the GitHub release tag, and what CheckForUpdate.mm
// compares against the latest tag. It becomes CFBundleShortVersionString.
#define VERSION_STR "2.1.1"

// BUILD_STR is the BUILD number, and becomes CFBundleVersion. It is deliberately
// SEPARATE from VERSION_STR, and it is not shown anywhere in the UI.
//
// Why it exists: App Store Connect rejects an upload whose CFBundleVersion it
// has already seen. When review bounces a build and you resubmit the same
// marketing version, the marketing version must NOT change (2.1 is still 2.1)
// but the build number MUST. With the two fields tied together there was no way
// to express that except by faking a version bump.
//
// RULE: bump this on EVERY App Store upload, including a resubmission of an
// otherwise identical build. Never reset it, never reuse a value -- Apple only
// requires it to increase, not to relate to VERSION_STR, so a plain monotonic
// counter is the simplest thing that can never collide. It does NOT need to be
// bumped for a plus/GitHub release (nothing there consumes it), but bumping it
// anyway is harmless.
//
// Apple accepts up to three dot-separated integers here; a single integer is
// intentional so it never has to be reset when VERSION_STR changes.
#define BUILD_STR "3"
