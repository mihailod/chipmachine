#pragma once

// 7501 facade for the TED machine.
//
// The 7501 is a 6502 with an on-chip I/O port at $00/$01; the port lives in the
// memory hooks, not in the core, so an ordinary NMOS 6502 is all this needs.
// ted_machine.cpp talks to this API rather than to the core directly -- the core
// is C (and header-only), this side is C++, and the boundary is what would make
// swapping cores a one-file change. ted_cpu.c implements it over MyLittle6502.
//
// Unlike victrackerplugin, which only ever calls a replay routine as a
// subroutine, this machine runs continuously and takes interrupts, so the API
// exposes single-stepping and the IRQ line rather than a call-and-wait.
//
// The implementation gets its memory from ted_mem_read/ted_mem_write, which
// ted_machine.cpp exports over the active machine's address space.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory hooks, supplied by ted_machine.cpp.
uint8_t ted_mem_read(uint16_t addr);
void ted_mem_write(uint16_t addr, uint8_t val);

void tedcpu_reset(void);

// Executes one instruction; returns the CPU cycles it took.
int tedcpu_step(void);

// Takes an interrupt if the I flag is clear. Returns the cycles consumed (7) or
// 0 if the CPU had interrupts masked.
int tedcpu_irq(void);
int tedcpu_nmi(void);

uint16_t tedcpu_get_pc(void);
void tedcpu_set_pc(uint16_t pc);
void tedcpu_set_sp(uint8_t sp);
void tedcpu_set_status(uint8_t status);

// Pushes a 16-bit value the way JSR does, so an RTS lands at value + 1.
void tedcpu_push16(uint16_t value);

// Pulls the way RTS does and returns the popped value; the caller adds one to
// get the resume address. Used by the KERNAL call traps in ted_machine.cpp.
uint16_t tedcpu_pull16(void);

#ifdef __cplusplus
}
#endif
