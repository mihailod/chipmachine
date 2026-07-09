#pragma once

// From https://github.com/suzukiplan/z80

#include <cstring>

namespace z80emulator
{

class MMU
{
public:
    MMU() :
            RAM(),
            IO()
    {
        memset(&RAM, 0, sizeof(RAM));
        memset(&IO, 0, sizeof(IO));
    }

    unsigned char RAM[0x10000]; // 64KB memory (minimum)
    unsigned char IO[0x1000]; // 64Kb port
};

}   // namespace z80emulator