#include "Z80SimpleEmulator.h"

#include "../../../thirdParty/z80emulatorLibz80/z80.hpp"

namespace arkostracker 
{

// FIXME This should be a base emulator class.

// Raw class, so many warnings are silenced.

// memory read request per 1 byte from CPU
unsigned char readByteSE(void* arg, const uint16_t addr)     // FIXME Crappy rename, else the name clashed in CLI UTs because of the first emulator.
{
    // NOTE: implement switching procedure here if your MMU has bank switch feature
    return ((z80emulator::MMU*)arg)->RAM[addr];         // NOLINT(*)
}

// memory write request per 1 byte from CPU
void writeByteSE(void* arg, const uint16_t addr, const unsigned char value)
{
    // NOTE: implement switching procedure here if your MMU has bank switch feature
    ((z80emulator::MMU*)arg)->RAM[addr] = value;         // NOLINT(*)
}

// IN operand request from CPU
unsigned char inPortSE(void* arg, const unsigned char port)
{
    return ((z80emulator::MMU*)arg)->IO[port];         // NOLINT(*)
}

// OUT operand request from CPU
/*
void outPort(void* arg, unsigned char port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}
*/

// ==============================================================================


Z80SimpleEmulator::Z80SimpleEmulator() noexcept :
        mmu(),
        z80(readByteSE, writeByteSE, inPortSE,         // NOLINT(*)
            [&](const unsigned short /*port*/, const unsigned char /*value*/) { /* Nothing to do. */ },
            &mmu)
{
}

void Z80SimpleEmulator::injectData(const juce::MemoryBlock& memoryBlock, const unsigned short address) noexcept
{
    const auto size = memoryBlock.getSize();
    jassert((address + size) < sizeof(mmu.RAM));

    for (unsigned short i = 0; i < static_cast<unsigned short>(size); ++i) {
        auto c = static_cast<unsigned char>(memoryBlock[i]);
        z80.writeByte(address + i, c);         // NOLINT(*)
    }
}

void Z80SimpleEmulator::writeData(const unsigned short address, const unsigned char b) noexcept
{
    z80.writeByte(address, b);
}

int Z80SimpleEmulator::runTill(const unsigned short pc, const unsigned short endAddress, const unsigned short sp, const int maximumNops) noexcept
{
    z80.reg.PC = pc;
    z80.reg.SP = sp;
    z80.reg.IFF = 0;        // DI.

    z80.removeAllBreakPoints();

    z80.addBreakPoint(endAddress, [&] {
        z80.requestBreak();
    });

    return z80.execute(maximumNops);        // Runs for a long time, the break above should be encountered way before that.
}

}   // namespace arkostracker
