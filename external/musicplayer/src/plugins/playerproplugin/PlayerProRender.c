// C bridge over the vendored PlayerPRO MADDriver. See PlayerProRender.h.
//
// The engine is driven exactly the way its own CoreAudio/PulseAudio backends
// drive it, only without a hardware sink: create a driver in NoHardwareDriver
// mode, attach a loaded MADMusic, MADPlayMusic(), then pull fixed-size PCM
// blocks with MADDirectSave() (which returns false once the song ends). We hold
// a small residual so callers can request any frame count.

#include "PlayerProRender.h"

#include "RDriver.h"
#include "MADDriver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MADDirectSave lives in RDriverInt.h, which pulls in a lot we don't need here.
extern bool MADDirectSave(char* myPtr, MADDriverSettings* driverType,
                          MADDriverRec* intDriver);

struct PPRender
{
    MADLibrary*   lib;
    MADDriverRec* driver;
    MADMusic*     music;

    char*  block;     // one MADDirectSave buffer (driver->BufSize bytes)
    size_t blockBytes;
    int    residOff;  // byte offset of unread data within block
    int    residLen;  // bytes of valid data within block
    int    ended;     // engine reported musicEnd
};

// PlayerPRO file types we route. MADG/MADF go through the embedded MAD-FG
// loader (registered as type "MADF" in PPEmbeddedPlugs.c); MADK is the native
// format loaded directly by the driver core.
static const char* plug_type_for_magic(const unsigned char* m)
{
    if (memcmp(m, "MADG", 4) == 0 || memcmp(m, "MADF", 4) == 0) {
        return "MADF";
    }
    if (memcmp(m, "MADK", 4) == 0) {
        return "MADK";
    }
    return NULL;
}

PPRender* pprender_open(const char* path)
{
    unsigned char magic[4];
    FILE* f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    if (fread(magic, 1, 4, f) != 4) {
        fclose(f);
        return NULL;
    }
    fclose(f);

    const char* type = plug_type_for_magic(magic);
    if (!type) {
        return NULL;
    }

    PPRender* r = (PPRender*)calloc(1, sizeof(PPRender));
    if (!r) {
        return NULL;
    }

    if (MADInitLibrary(NULL, &r->lib) != MADNoErr) {
        pprender_close(r);
        return NULL;
    }

    MADDriverSettings set;
    MADGetBestDriver(&set);
    set.driverMode   = NoHardwareDriver;
    set.outPutBits   = 16;
    set.outPutMode   = DeluxeStereoOutPut;
    set.outPutRate   = 44100;
    set.oversampling = 1;
    set.repeatMusic  = false;
    set.surround     = false;
    set.Reverb       = false;
    set.TickRemover  = false;
    // Match the stock "PlayerPRO Player" defaults exactly: oversampling off,
    // reverb/surround off, and the stereo micro-delay ("Stereo Delay") ON at
    // 30 ms -- this is the widening you hear in the real app. (MADGetBestDriver
    // leaves it at 25 ms; the shipping app overrides to 30.)
    set.MicroDelaySize = 30;

    if (MADCreateDriver(&set, r->lib, &r->driver) != MADNoErr || !r->driver) {
        pprender_close(r);
        return NULL;
    }

    {
        char  typeBuf[5];
        char  pathBuf[1024];
        strncpy(typeBuf, type, sizeof(typeBuf));
        typeBuf[4] = 0;
        strncpy(pathBuf, path, sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = 0;
        if (MADLoadMusicFileCString(r->lib, &r->music, typeBuf, pathBuf) !=
                MADNoErr ||
            !r->music) {
            pprender_close(r);
            return NULL;
        }
    }

    if (MADAttachDriverToMusic(r->driver, r->music, NULL) != MADNoErr) {
        pprender_close(r);
        return NULL;
    }
    if (MADPlayMusic(r->driver) != MADNoErr) {
        pprender_close(r);
        return NULL;
    }

    r->blockBytes = r->driver->BufSize;
    r->block = (char*)malloc(r->blockBytes);
    if (!r->block) {
        pprender_close(r);
        return NULL;
    }
    r->residOff = 0;
    r->residLen = 0;
    r->ended = 0;
    return r;
}

int pprender_hz(PPRender* r)
{
    (void)r;
    return 44100;
}

int pprender_fill(PPRender* r, short* out, int maxFrames)
{
    int framesOut = 0;
    const int frameBytes = 4; // 16-bit stereo

    while (framesOut < maxFrames) {
        if (r->residOff >= r->residLen) {
            if (r->ended) {
                break;
            }
            bool more = MADDirectSave(r->block, NULL, r->driver);
            r->residOff = 0;
            r->residLen = (int)r->blockBytes;
            if (!more) {
                // This block is the last; serve it, then stop next time.
                r->ended = 1;
            }
        }

        int availFrames = (r->residLen - r->residOff) / frameBytes;
        int want = maxFrames - framesOut;
        int take = availFrames < want ? availFrames : want;
        if (take <= 0) {
            break;
        }
        memcpy(out + (size_t)framesOut * 2, r->block + r->residOff,
               (size_t)take * frameBytes);
        r->residOff += take * frameBytes;
        framesOut += take;
    }
    return framesOut;
}

void pprender_close(PPRender* r)
{
    if (!r) {
        return;
    }
    if (r->music && r->driver) {
        MADStopMusic(r->driver);
        MADCleanDriver(r->driver);
        MADDisposeMusic(&r->music, r->driver);
    }
    if (r->driver) {
        MADDisposeDriver(r->driver);
    }
    if (r->lib) {
        MADDisposeLibrary(r->lib);
    }
    free(r->block);
    free(r);
}
