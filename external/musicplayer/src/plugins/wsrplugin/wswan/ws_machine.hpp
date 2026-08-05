// A WonderSwan, reduced to the parts a music rip needs.
//
// Written from the WSdev Wiki (https://ws.nesdev.org): "Memory map", "Mapper",
// "Interrupts", "Timers", "Display" and the sound pages. See README.md next to
// this file.
//
// A .wsr file is not a sequence format -- it is a cartridge image whose entry
// point takes a song number in AW. So this is a whole machine, minus everything
// that makes no sound: there is no PPU, no sprite/tile rendering, no keypad, no
// serial, no EEPROM. What remains is the CPU, the memory map with its bank
// registers, the two blank timers and their interrupts, the display's line
// counter (which is what drives those timers and the music tempo), general DMA,
// and the APU in ws_apu.c.

#ifndef WS_MACHINE_HPP
#define WS_MACHINE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

#include "../v30mz/v30mz.hpp"

extern "C" {
#include "ws_apu.h"
}

namespace wswan {

class Machine : public ares::V30MZ {
public:
    // Internal RAM is 64 KB on the Color; the mono model only decodes 16 KB of
    // it, but nothing here depends on the difference.
    static constexpr uint32_t IRamSize = 0x10000;
    static constexpr uint32_t SRamSize = 0x10000;

    // The 32-byte WSRF footer, from Mamiya's own format note.
    static constexpr size_t FooterSize = 0x20;
    static constexpr size_t FooterFirstSong = 0x05;

    Machine();

    // Validates the WSRF footer and takes a copy of the image. `rate` is the
    // output sample rate: the APU renders straight to it, so there is no
    // resampler anywhere in this plugin.
    bool load(const uint8_t* data, size_t size, uint32_t rate);

    // Boots the machine the way the format specifies: AW = song number,
    // CS:IP = FFFF:0000.
    void reset(unsigned song);

    unsigned firstSong() const { return firstSong_; }

    // Interleaved stereo, `frames` frames.
    void render(int16_t* out, unsigned frames);

    // --- V30MZ bus -------------------------------------------------------
    auto step(u32 clocks = 1) -> void override;
    auto width(n20 address) -> u32 override;
    auto speed(n20 address) -> n32 override;
    auto read(n20 address) -> n8 override;
    auto write(n20 address, n8 data) -> void override;
    auto in(n16 port) -> n8 override;
    auto out(n16 port, n8 data) -> void override;
    auto ioWidth(n16 port) -> u32 override;
    auto ioSpeed(n16 port) -> n32 override;

private:
    void hblank();
    void pollInterrupts();
    void raise(uint8_t bit);
    void generalDma();
    uint32_t romOffset(uint32_t address) const;
    uint32_t bankStart(uint8_t bank) const;
    static uint8_t dmaRead(void* ctx, uint32_t address);

    std::vector<uint8_t> rom_;
    unsigned firstSong_ = 0;

    uint8_t iram_[IRamSize] = {};
    uint8_t sram_[SRamSize] = {};
    uint8_t io_[0x100] = {};

    // Mapper registers $C0-$C3.
    uint8_t bankLinear_ = 0, bankRam_ = 0, bankRom0_ = 0, bankRom1_ = 0xFF;

    // Timers ($A2-$AB) and the display line counter ($02/$03).
    uint16_t hReload_ = 0, vReload_ = 0, hCount_ = 0, vCount_ = 0;
    uint8_t timerCtrl_ = 0;
    uint16_t line_ = 0;

    // Interrupts ($B0-$B6).
    uint8_t intBase_ = 0, intEnable_ = 0, intStatus_ = 0;

    WSAPU apu_ = {};

    uint64_t clock_ = 0;        // master cycles since reset
    uint32_t hblankAcc_ = 0;    // cycles towards the next 256-cycle line
    uint64_t sampleAcc_ = 0;    // 16.16 cycles per output sample, accumulated
    uint32_t cyclesPerSmpl_ = 0;
};

}

#endif
