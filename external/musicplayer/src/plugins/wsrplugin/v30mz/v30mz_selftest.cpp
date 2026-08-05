// Smoke test for the vendored ares V30MZ and the nall_compat.hpp shim it runs
// on. Boots the core the way a WSR file does -- far JMP at FFFF:0000 -- and
// exercises the paths a music driver actually uses. Run from cmtest
// ("V30MZ core" test case); see PROVENANCE.md.
//
// Two things worth knowing if this ever fails:
//
//  * Only SIXTEEN bytes live at FFFF:0000 before the 1 MB space wraps, which is
//    exactly why the WSR format puts a far JMP there and the code lower down.
//    A test program written straight to 0xFFFF0 runs off the end of memory.
//  * interrupt(vector) returns false unless PSW.IE *and* the pre-instruction
//    state.interrupt snapshot are both set, so a test that never issues STI
//    sees no interrupt at all. That is correct behaviour, not a fault.
//
// DAA is in here on purpose: it is the auxiliary-carry path, and the auxiliary
// carry is computed as `(u4)x + (u4)y + (u4)c >= 16`, so it fails immediately if
// the shim's sized integers stop masking on assignment.

#include "v30mz.hpp"

#include <cstring>

namespace {

struct TestCPU : ares::V30MZ {
    u8 ram[1 << 20] = {};
    u8 ports[65536] = {};

    auto step(u32 clocks = 1) -> void override { (void)clocks; }
    auto width(n20) -> u32 override { return 2; }
    auto speed(n20) -> n32 override { return 1; }
    auto read(n20 a) -> n8 override { return ram[a]; }
    auto write(n20 a, n8 d) -> void override { ram[a] = d; }
    auto in(n16 p) -> n8 override { return ports[p]; }
    auto out(n16 p, n8 d) -> void override { ports[p] = d; }
    auto ioWidth(n16) -> u32 override { return 1; }
    auto ioSpeed(n16) -> n32 override { return 1; }
};

} // namespace

bool v30mz_selftest()
{
    TestCPU cpu;

    static const u8 program[] = {
        0x31, 0xc0,             // XOR AW,AW
        0x8e, 0xd8,             // MOV DS0,AW    (DS = 0)
        0x8e, 0xc0,             // MOV DS1,AW    (ES = 0)
        0xbc, 0x00, 0x20,       // MOV SP,0x2000
        0xbe, 0x00, 0x30,       // MOV IX,0x3000 (SI)
        0xbf, 0x00, 0x40,       // MOV IY,0x4000 (DI)
        0xb9, 0x04, 0x00,       // MOV CW,4
        0xfc,                   // CLD
        0xf3, 0xa4,             // REP MOVSB
        0xb8, 0x34, 0x12,       // MOV AW,0x1234
        0x50,                   // PUSH AW
        0x5b,                   // POP BW
        0xb0, 0x09,             // MOV AL,9
        0x04, 0x08,             // ADD AL,8      (0x11, auxiliary carry set)
        0x27,                   // DAA           -> 0x17
        0x88, 0xc4,             // MOV AH,AL
        0xb0, 0x55,             // MOV AL,0x55
        0xe6, 0x42,             // OUT 0x42,AL
        0xb0, 0x00,             // MOV AL,0
        0xe4, 0x42,             // IN  AL,0x42   -> 0x55
        0xfb,                   // STI
        0xf4                    // HLT
    };
    static const u8 boot[] = { 0xea, 0x00, 0x06, 0x00, 0x00 };  // JMP FAR 0000:0600
    static const char source[] = "WSWN";

    std::memcpy(cpu.ram + 0xffff0, boot, sizeof(boot));
    std::memcpy(cpu.ram + 0x0600, program, sizeof(program));
    std::memcpy(cpu.ram + 0x3000, source, 4);

    cpu.power();
    for (int i = 0; i < 64 && !cpu.state.halt; i++) { cpu.instruction(); }

    if (std::memcmp(cpu.ram + 0x4000, source, 4) != 0) { return false; } // REP MOVSB
    if ((u16)cpu.BW != 0x1234) { return false; }                        // PUSH/POP
    if ((u8)cpu.AH != 0x17) { return false; }                           // DAA
    if ((u8)cpu.AL != 0x55) { return false; }                           // IN/OUT
    if (!cpu.state.halt) { return false; }                              // HLT

    // Interrupt vector 3 -> a handler that sets CW and returns.
    cpu.ram[3 * 4 + 0] = 0x00;
    cpu.ram[3 * 4 + 1] = 0x05;
    cpu.ram[3 * 4 + 2] = 0x00;
    cpu.ram[3 * 4 + 3] = 0x00;
    static const u8 handler[] = { 0xb9, 0xef, 0xbe, 0xcf };  // MOV CW,0xbeef ; IRET
    std::memcpy(cpu.ram + 0x0500, handler, sizeof(handler));

    if (!cpu.interrupt(3)) { return false; }
    for (int i = 0; i < 4; i++) { cpu.instruction(); }
    if ((u16)cpu.CW != 0xbeef) { return false; }

    return true;
}
