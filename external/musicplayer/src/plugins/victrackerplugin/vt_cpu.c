// vt_cpu.h over MyLittle6502 (public domain / CC0). See README.md.
//
// This replaced the omarandlorraine fake6502 fork (GPLv2) that the plugin
// originally ran on. MyLittle6502 is a continuation of the SAME code that fork
// descends from -- Mike Chambers' Fake6502 v1.1 (2011), which he released into
// the public domain -- so going back to the public-domain line costs no accuracy.
// gek169's v1.3 in fact fixes decimal mode, the BIT opcode and interrupt masking,
// and keeps UNDOCUMENTED opcodes on, which is what an NMOS 6502 needs.
//
// The core is header-only and keeps its register/helper state static, so it is
// included ONCE, here. Its entry points and memory hooks are NOT static, though,
// and names like read6502 / step6502 / callexternal are too generic to expose in
// a link with ~60 other plugins -- so they are prefixed on the way in, the same
// precaution csidplugin takes with Hermit's unqualified globals.

#include "vt_cpu.h"

#define read6502 vtpd_read6502
#define write6502 vtpd_write6502
#define reset6502 vtpd_reset6502
#define step6502 vtpd_step6502
#define exec6502 vtpd_exec6502
#define irq6502 vtpd_irq6502
#define nmi6502 vtpd_nmi6502
#define hookexternal vtpd_hookexternal
#define callexternal vtpd_callexternal
#define loopexternal vtpd_loopexternal

#include "pd6502.h"

// The two functions the core requires the host to supply.
uint8 read6502(ushort address)
{
    return vt_mem_read((uint16_t)address);
}

void write6502(ushort address, uint8 value)
{
    vt_mem_write((uint16_t)address, (uint8_t)value);
}

void vtcpu_call(uint16_t entry, uint16_t trap, int maxInstructions)
{
    // reset6502() pulls the reset vector out of $FFFC, which is 0 in our image;
    // the entry point is set explicitly below instead.
    reset6502();
    sp = 0xFD;
    push_6502_16((ushort)(trap - 1)); // RTS -> trap
    pc = (ushort)entry;
    for (int i = 0; i < maxInstructions; i++) {
        if (pc == (ushort)trap) {
            return;
        }
        step6502();
    }
}
