// Stubs for the Mac live-audio + Finder-metadata entry points that the stock
// PlayerPRO build references on __APPLE__ (it auto-#defines _MAC_H, which keeps
// all the well-tested Mac IO/struct code paths). chipmachine never starts the
// CoreAudio backend -- it drives the engine offline with NoHardwareDriver +
// MADDirectSave -- and never writes module files, so these are inert. Providing
// trivial stubs avoids linking AudioToolbox/AudioUnit and the Objective-C
// CocoaFuncs.m purely to satisfy the symbol table.
//
// Public domain, like the rest of PlayerPRO.

#include "RDriver.h"
#include <CoreFoundation/CoreFoundation.h>

MADErr initCoreAudio(MADDriverRec* inMADDriver)
{
    (void)inMADDriver;
    return MADNoErr;
}

MADErr closeCoreAudio(MADDriverRec* inMADDriver)
{
    (void)inMADDriver;
    return MADNoErr;
}

// Sets a file's legacy HFS OSType on save; never reached (we only read).
void SetOSType(CFURLRef theURL, OSType theType)
{
    (void)theURL;
    (void)theType;
}
