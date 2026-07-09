#define HAVE_CONFIG_H
#include "Nes_Snd_Emu-0.1.7/Simple_Apu_PAL.h"

#include "apuwrap.h"

// wrapper stuff for Nes_Snd_Emu in NT2
// this file is heavily based off demo.cpp by blargg

const long sample_rate = 44100;
static Simple_Apu apu;

void (*playcallback)();

static int read_dmc( void*, cpu_addr_t addr )
{
    unsigned char *sptr = (unsigned char *)APUWRAP_SAMPLEPTR;

    if(sptr) {
        // addr is C000 -> ...
        // in tracker should always be in range C000, C000+4K
        // sample ptr might change between calls to this function
        sptr += (addr - 0xC000);
        return *sptr;
    }

    return 0;
}

extern "C"
void apuSetup(void (*playcallback)())
{
    // Set sample rate and check for out of memory error
    // (who cares about error checking!)
    apu.sample_rate( sample_rate );

    // Set function for APU to read memory with (required for DMC samples to play properly)
    apu.dmc_reader( read_dmc, NULL );

    ::playcallback = playcallback;
}

extern "C"
void apuShutdown()
{
    // nothing to do... :-)
}

extern "C"
void apuRender(void *buf, long len)
{
    // note: "apu" has internal buffering
    while(apu.samples_avail() < len) {
        playcallback(); // calls PlayNED
        apu.end_frame();
    }

    apu.read_samples((Simple_Apu::sample_t *)buf, len);
}

extern "C"
void apuWriteReg(int reg, int val)
{
    apu.write_register(reg, val);
}

extern "C"
int apuReadReg(int reg)
{
    if(reg == 0x4015) {
        return apu.read_status();
    }

    return 0;
}
