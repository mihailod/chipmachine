#pragma once

#include <juce_core/juce_core.h>

#include "../../../thirdParty/z80emulatorLibz80/mmu.h"
#include "../../../thirdParty/z80emulatorLibz80/z80.hpp"

namespace arkostracker
{

class PsgRegisters;
class Song;

/** Wrapper around the low-level Z80 emulator. */
class Z80SimpleEmulator
{
public:
    Z80SimpleEmulator() noexcept;

    /**
     * Injects the given MemoryBlock into the CPC memory.
     * @param memoryBlock the block to load.
     * @param address where to load the block. Must be < 0xffff.
     */
    void injectData(const juce::MemoryBlock& memoryBlock, unsigned short address) noexcept;

    /**
     * Writes a byte.
     * @param address where to load the block. Must be < 0xffff.
     * @param b the byte to write.
     */
    void writeData(unsigned short address, unsigned char b) noexcept;

    /**
     * Runs the Z80 emulator till the end address is reached.
     * The stack pointer is reset to an initial value.
     * @param pc the Program Counter to start with.
     * @param endAddress the end address.
     * @param sp the stack pointer.
     * @param maximumNops how many nops before giving up.
     * @return how many cycles have been spent, or -1 if an error occurred.
     */
    int runTill(unsigned short pc, unsigned short endAddress, unsigned short sp, int maximumNops) noexcept;

    z80emulator::MMU mmu;
    z80emulator::Z80 z80;
};

}   // namespace arkostracker
