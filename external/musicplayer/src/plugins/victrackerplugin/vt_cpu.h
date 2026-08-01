#pragma once

// 6502 facade for the VIC-TRACKER player.
//
// The plugin needs exactly one thing from a CPU core: run a subroutine to
// completion. vt_machine.cpp talks to this two-function API rather than to the
// core directly -- the core is C (and header-only), this side is C++, and the
// boundary is also what made swapping cores a one-file change (see README.md).
// vt_cpu.c implements it over MyLittle6502.
//
// The implementation gets its memory from vt_mem_read/vt_mem_write, which
// vt_machine.cpp exports over the active machine's 64K image.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory hooks, supplied by vt_machine.cpp.
uint8_t vt_mem_read(uint16_t addr);
void vt_mem_write(uint16_t addr, uint8_t val);

// Calls `entry` as a subroutine with a return address of `trap`, and runs until
// the PC reaches it or `maxInstructions` have executed (a hang backstop).
void vtcpu_call(uint16_t entry, uint16_t trap, int maxInstructions);

#ifdef __cplusplus
}
#endif
