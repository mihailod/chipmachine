// cSID by Hermit (Mihaly Horvath) -- http://hermit.sidrip.com
// License: WTF -- Do what the fuck you want with this code, but please mention
// me as its original author.
//
// Library-ised C API over Hermit's cycle-based cSID (csid.c). The DSP and CPU
// code behind this header is Hermit's, unchanged; see csid_engine.c for the
// exact list of edits made to turn a standalone SDL command-line player into
// something linkable.

#ifndef CSID_ENGINE_H
#define CSID_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Metadata lifted from the PSID/RSID header by csid_load().
typedef struct
{
    int subtune_count;     // number of subtunes (>= 1)
    int default_subtune;   // 0-based
    int sid_amount;        // 1..3 (2SID/3SID tunes)
    int sid_model;         // 6581 or 8580, as preferred by the header
    int is_rsid;           // 1 when the file carries the "RSID" magic
    int play_address;      // header play address; 0 = tune installs its own IRQ
    char title[32];
    char author[32];
    char info[32];
} csid_info;

// Parse a PSID/RSID image and load it into the emulated C64 memory map.
// Returns 0 on success, non-zero when the data is not a SID file.
// `out_info` may be NULL.
int csid_load(const uint8_t* data, int length, int samplerate,
              csid_info* out_info);

// Select a subtune (0-based) and run its init routine. Must be called after
// csid_load(). Safe to call again to switch subtunes.
void csid_init_tune(int subtune);

// Render `frames` MONO samples. Returns `frames`.
int csid_render(int16_t* out, int frames);

#ifdef __cplusplus
}
#endif

#endif // CSID_ENGINE_H
