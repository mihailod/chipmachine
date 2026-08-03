#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace musix::gsf {

// A GSF rip assembled back into the Game Boy Advance cartridge image it was cut
// from. `data` is indexed from cartridge base 0x08000000.
struct RomImage
{
    std::vector<uint8_t> data;
    // "GSF entry point" from the program header of the OUTERMOST file, i.e. the
    // last one applied. Effectively always 0x08000000 -- rips carry a real GBA
    // cartridge header whose first word is a branch to the driver -- but it is
    // reported so GSFPlayer can notice and log a rip that disagrees.
    uint32_t entryPoint = 0;
    // True when the program section is linked for EWRAM (0x02000000) rather
    // than the cartridge -- a rip of a game that runs its driver from RAM.
    // `data` is still built from offset 0; the flag only records which of
    // mGBA's two load paths the image has been sized for. See loadRom().
    bool multiboot = false;
    bool valid = false;
};

// Loads `path`, following its "_lib" chain, and returns the assembled image.
// Never throws; check `valid`.
RomImage loadRom(const std::string& path);

} // namespace musix::gsf
