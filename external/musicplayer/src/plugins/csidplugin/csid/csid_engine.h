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

// ---------------------------------------------------------------------------
// Register-level API, used by musplugin's Compute! Sidplayer sequencer.
//
// A .mus is note-sequence DATA, not 6502 code, so its player needs the SID CHIP
// emulation but none of the CPU/PSID machinery: it writes SID registers itself
// once per tick and asks for samples in between. These three entry points give
// exactly that and bypass csid_load()/the 6502 loop entirely.
// ---------------------------------------------------------------------------

// sid_count is 1 or 2; sid2_base is the second chip's base address (Stereo
// Sidplayer routes its voices 4-6 there).
void csid_chip_init(int rate, int sid_count, unsigned int sid2_base);
void csid_poke(unsigned int addr, unsigned char val);
int csid_chip_render(int16_t* out, int frames);

// Interleaved L/R. With two chips the FIRST goes left and the SECOND right,
// which is how VICE positions a Stereo Sidplayer pair (its L and R differ by up
// to half full scale). With one chip both channels carry the same signal.
int csid_chip_render_stereo(int16_t* out, int frames);

#ifdef __cplusplus
}
#endif

#endif // CSID_ENGINE_H
