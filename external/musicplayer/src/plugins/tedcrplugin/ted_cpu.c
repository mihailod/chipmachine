// ted_cpu.h over MyLittle6502 (public domain / CC0). See README.md.
//
// The core is header-only and keeps its register/helper state static, so it is
// included ONCE, here -- which is also why the accessors below live in this file
// rather than the machine: `pc`, `sp` and `status` are not visible outside this
// translation unit. Its entry points and memory hooks are NOT static, though,
// and names like read6502 / step6502 / callexternal are far too generic for a
// link with ~60 other plugins, so they are prefixed on the way in, exactly as
// victrackerplugin's vt_cpu.c does with the same header.
//
// pd6502.h here is byte-identical to victrackerplugin/pd6502.h. Keep the two in
// step if either is ever updated.

#include "ted_cpu.h"

#define read6502 tedpd_read6502
#define write6502 tedpd_write6502
#define reset6502 tedpd_reset6502
#define step6502 tedpd_step6502
#define exec6502 tedpd_exec6502
#define irq6502 tedpd_irq6502
#define nmi6502 tedpd_nmi6502
#define hookexternal tedpd_hookexternal
#define callexternal tedpd_callexternal
#define loopexternal tedpd_loopexternal

#include "pd6502.h"

// The two functions the core requires the host to supply.
uint8 read6502(ushort address)
{
    return ted_mem_read((uint16_t)address);
}

void write6502(ushort address, uint8 value)
{
    ted_mem_write((uint16_t)address, (uint8_t)value);
}

void tedcpu_reset(void)
{
    reset6502();
    sp = 0xFD;
    status = FLAG_CONSTANT; // interrupts enabled, as after a BASIC SYS
}

int tedcpu_step(void)
{
    return (int)step6502();
}

int tedcpu_irq(void)
{
    if (status & FLAG_INTERRUPT) {
        return 0;
    }
    irq6502();
    return 7;
}

int tedcpu_nmi(void)
{
    nmi6502();
    return 7;
}

uint16_t tedcpu_get_pc(void)
{
    return (uint16_t)pc;
}

void tedcpu_set_pc(uint16_t newpc)
{
    pc = (ushort)newpc;
}

void tedcpu_set_sp(uint8_t newsp)
{
    sp = (uint8)newsp;
}

void tedcpu_set_status(uint8_t newstatus)
{
    status = (uint8)(newstatus | FLAG_CONSTANT);
}

void tedcpu_push16(uint16_t value)
{
    push_6502_16((ushort)value);
}

uint16_t tedcpu_pull16(void)
{
    return (uint16_t)pull_6502_16();
}
