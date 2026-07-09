/*
 * Minimal no-op libsndfile stub for the DMF (Furnace) plugin.
 *
 * Furnace's engine references libsndfile in two places: DivFilePlayer/SFWrapper
 * (loading external audio-file *instruments*) and the DivEngine::saveAudio()
 * export path. A DefleMask .dmf never uses audio-file instruments, and the
 * plugin drives DivEngine::nextBuf() directly rather than saveAudio(), so none
 * of this machinery is exercised at runtime -- but it must still *link*. Rather
 * than vendor the whole of libsndfile we build with HAVE_SNDFILE defined and
 * satisfy the referenced symbols with these no-op stubs (sf_open_virtual
 * returns NULL, so every real path fails cleanly).
 *
 * Covers exactly the sf_* symbols Furnace's engine references.
 */

#include "sndfile.h"

SNDFILE* sf_open_virtual(SF_VIRTUAL_IO* sfvirtual, int mode, SF_INFO* sfinfo, void* user_data)
{
    (void)sfvirtual; (void)mode; (void)user_data;
    if (sfinfo != NULL) {
        sfinfo->frames = 0;
        sfinfo->channels = 0;
        sfinfo->samplerate = 0;
        sfinfo->seekable = 0;
    }
    return NULL;
}

int sf_close(SNDFILE* sndfile) { (void)sndfile; return 0; }

int sf_error(SNDFILE* sndfile) { (void)sndfile; return 0; }

const char* sf_strerror(SNDFILE* sndfile) { (void)sndfile; return "sndfile stub"; }

const char* sf_error_number(int errnum) { (void)errnum; return "sndfile stub"; }

int sf_command(SNDFILE* sndfile, int command, void* data, int datasize)
{
    (void)sndfile; (void)command; (void)data; (void)datasize; return 0;
}

sf_count_t sf_seek(SNDFILE* sndfile, sf_count_t frames, int whence)
{
    (void)sndfile; (void)frames; (void)whence; return -1;
}

sf_count_t sf_readf_float(SNDFILE* sndfile, float* ptr, sf_count_t frames)
{
    (void)sndfile; (void)ptr; (void)frames; return 0;
}

sf_count_t sf_read_float(SNDFILE* sndfile, float* ptr, sf_count_t items)
{
    (void)sndfile; (void)ptr; (void)items; return 0;
}

sf_count_t sf_read_short(SNDFILE* sndfile, short* ptr, sf_count_t items)
{
    (void)sndfile; (void)ptr; (void)items; return 0;
}

sf_count_t sf_read_raw(SNDFILE* sndfile, void* ptr, sf_count_t bytes)
{
    (void)sndfile; (void)ptr; (void)bytes; return 0;
}

sf_count_t sf_writef_float(SNDFILE* sndfile, const float* ptr, sf_count_t frames)
{
    (void)sndfile; (void)ptr; (void)frames; return 0;
}

sf_count_t sf_writef_short(SNDFILE* sndfile, const short* ptr, sf_count_t frames)
{
    (void)sndfile; (void)ptr; (void)frames; return 0;
}

sf_count_t sf_write_raw(SNDFILE* sndfile, const void* ptr, sf_count_t bytes)
{
    (void)sndfile; (void)ptr; (void)bytes; return 0;
}
