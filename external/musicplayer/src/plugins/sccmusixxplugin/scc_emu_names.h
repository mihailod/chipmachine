#pragma once
// emu2212 (Konami SCC emulator, by Mitsutaka Okazaki) is also compiled into
// kssplugin. To keep this plugin self-contained without duplicate-symbol
// clashes at final link, we compile our own copy with the public API renamed.
// This header is force-included into both our emu2212 translation unit
// (scc_emu.c) and the caller (scc_machine.cpp) so definitions and call sites
// agree on the renamed names.
#define SCC_new        sccmx_SCC_new
#define SCC_reset      sccmx_SCC_reset
#define SCC_set_rate   sccmx_SCC_set_rate
#define SCC_set_quality sccmx_SCC_set_quality
#define SCC_set_type   sccmx_SCC_set_type
#define SCC_delete     sccmx_SCC_delete
#define SCC_calc       sccmx_SCC_calc
#define SCC_write      sccmx_SCC_write
#define SCC_writeReg   sccmx_SCC_writeReg
#define SCC_read       sccmx_SCC_read
#define SCC_readReg    sccmx_SCC_readReg
#define SCC_setMask    sccmx_SCC_setMask
#define SCC_toggleMask sccmx_SCC_toggleMask
#define SCC_init       sccmx_SCC_init
#define SCC_close      sccmx_SCC_close
