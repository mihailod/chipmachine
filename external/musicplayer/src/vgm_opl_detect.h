#pragma once

// Shared VGM chip probe: does a .vgm/.vgz log drive an OPL-family FM chip?
//
// GME's Vgm_Emu only decodes SN76489 (PSG), YM2413 (OPLL) and YM2612; it plays
// an OPL2 (YM3812) log SILENT and, worse, aborts on some of them (assertion in
// Blip_Buffer). So we route any OPL-carrying VGM to libvgmplugin (ValleyBell
// libvgm, which has real OPL cores) and make GME decline those same files.
//
// Header-only + zlib so both plugins share exactly one copy of the detection.
// zlib's gzread transparently returns raw bytes for a non-gzipped file, so the
// same path handles bare .vgm and gzipped .vgz without a magic-byte branch.

#include <cstdint>
#include <cstring>
#include <string>

#include <zlib.h>

namespace musix {

// VGM header clock-field offsets for the OPL family (VGM spec >= 1.51). A field
// is only present when it lies inside the header (before the data offset), so
// older VGMs that stop short simply report no OPL.
//   0x50  YM3812  (OPL2)
//   0x54  YM3526  (OPL / OPL1)
//   0x58  Y8950   (OPL + ADPCM, MSX-Audio)
//   0x5C  YMF262  (OPL3)
inline bool vgmHasOPL(const std::string& path)
{
    gzFile gf = gzopen(path.c_str(), "rb");
    if (gf == nullptr) { return false; }
    unsigned char h[0x100];
    int n = gzread(gf, h, sizeof(h));
    gzclose(gf);

    if (n < 0x60 || std::memcmp(h, "Vgm ", 4) != 0) { return false; }

    auto rd32 = [&](int o) -> uint32_t {
        return static_cast<uint32_t>(h[o]) |
               (static_cast<uint32_t>(h[o + 1]) << 8) |
               (static_cast<uint32_t>(h[o + 2]) << 16) |
               (static_cast<uint32_t>(h[o + 3]) << 24);
    };

    // Absolute start of VGM data. The relative field at 0x34 exists from v1.50;
    // when it is zero (older files) the data begins at the fixed 0x40.
    uint32_t dataRel = rd32(0x34);
    uint32_t dataAbs = dataRel != 0 ? 0x34 + dataRel : 0x40;

    auto clockAt = [&](int o) -> uint32_t {
        if (o + 4 > static_cast<int>(dataAbs) || o + 4 > n) { return 0; }
        // Mask off the dual-chip / flag bits (30-31); we only care that the
        // chip is present, not its clock rate.
        return rd32(o) & 0x3FFFFFFFu;
    };

    return clockAt(0x50) != 0 || clockAt(0x54) != 0 || clockAt(0x58) != 0 ||
           clockAt(0x5C) != 0;
}

// Broader routing gate: does this VGM drive ANY chip that GME's Vgm_Emu cannot
// decode, so it must go to libvgm (ValleyBell libvgm, which has every core)?
//
// GME's Vgm_Core competently renders only four chips: SN76489 (PSG), YM2413
// (OPLL, via the emu2413 core we vendored), YM2612 (Genesis FM) and AY8910
// (wired in for the Vectrex rips). That covers Sega Mega Drive / Master System /
// Game Gear / SG-1000 and AY logs -- everything else (OPL, YM2151, the YM2203/
// 2608/2610 OPN family, HuC6280, NES APU, GameBoy DMG, the Neo Geo / arcade
// sample chips: C140, QSound, K053260, K054539, SegaPCM, OKIM..., WonderSwan,
// SAA1099, ...) renders silent or aborts on GME. VGMRips is overwhelmingly those
// chips, so we probe the full VGM chip table and route anything outside GME's
// four to libvgm (and make GME decline the same files). Supersedes vgmHasOPL:
// the OPL chips are simply four of the many entries below.
inline bool vgmNeedsLibVGM(const std::string& path)
{
    gzFile gf = gzopen(path.c_str(), "rb");
    if (gf == nullptr) { return false; }
    unsigned char h[0x100];
    int n = gzread(gf, h, sizeof(h));
    gzclose(gf);

    if (n < 0x40 || std::memcmp(h, "Vgm ", 4) != 0) { return false; }

    auto rd32 = [&](int o) -> uint32_t {
        return static_cast<uint32_t>(h[o]) |
               (static_cast<uint32_t>(h[o + 1]) << 8) |
               (static_cast<uint32_t>(h[o + 2]) << 16) |
               (static_cast<uint32_t>(h[o + 3]) << 24);
    };

    uint32_t dataRel = rd32(0x34);
    uint32_t dataAbs = dataRel != 0 ? 0x34 + dataRel : 0x40;

    // VGM 1.70+ may place an optional extra header (chip clock / chip volume
    // tables) BETWEEN the normal header and the VGM data, at 0xBC + the relative
    // offset stored at 0xBC -- in practice 0xC0. So the normal header ends at
    // whichever comes first, the extra header or the data. Bounding by dataAbs
    // alone lets the extra header's size/offset dwords alias the 0xC0+ clock
    // slots and read back as phantom WonderSwan/VSU/SAA1099/ES5503/ES5506 chips
    // (e.g. testmus/libvgm/pc98-opn.vgz). Files with a genuine full 1.71 header
    // and no extra header are unaffected -- they keep their 0xC0+ chips.
    uint32_t hdrEnd = dataAbs;
    if (rd32(0x08) >= 0x170 && 0xBC + 4 <= static_cast<int>(dataAbs) &&
        0xBC + 4 <= n) {
        uint32_t xhRel = rd32(0xBC);
        uint32_t xhAbs = xhRel != 0 ? 0xBC + xhRel : 0;
        // Ignore a malformed offset pointing back into the fixed header:
        // shrinking hdrEnd too far would hide a REAL chip and misroute the file
        // to GME, which aborts on chips it cannot decode.
        if (xhAbs >= 0xC0 && xhAbs < hdrEnd) { hdrEnd = xhAbs; }
    }

    // A chip is "present" only when its clock field lies inside the header (before
    // the extra header / VGM data) and is non-zero (mask off the dual-chip / flag
    // bits 30-31).
    auto present = [&](int o) -> bool {
        if (o + 4 > static_cast<int>(hdrEnd) || o + 4 > n) { return false; }
        return (rd32(o) & 0x3FFFFFFFu) != 0;
    };

    // Every chip-clock offset in the VGM 1.71 header EXCEPT the four GME handles
    // (SN76489 @0x0C, YM2413 @0x10, YM2612 @0x2C, AY8910 @0x74). Interface/flag
    // sub-fields (0x3C SegaPCM reg, 0x94/0x95 sample-chip flags, ...) are not
    // clocks and are deliberately omitted.
    static const int kNonGmeChips[] = {
        0x30, // YM2151
        0x38, // SegaPCM
        0x40, // RF5C68
        0x44, // YM2203
        0x48, // YM2608
        0x4C, // YM2610/B
        0x50, // YM3812  (OPL2)
        0x54, // YM3526  (OPL)
        0x58, // Y8950
        0x5C, // YMF262  (OPL3)
        0x60, // YMF278B
        0x64, // YMF271
        0x68, // YMZ280B
        0x6C, // RF5C164
        0x70, // PWM
        0x80, // GameBoy DMG
        0x84, // NES APU
        0x88, // MultiPCM
        0x8C, // uPD7759
        0x90, // OKIM6258
        0x98, // OKIM6295
        0x9C, // K051649 (SCC)
        0xA0, // K054539
        0xA4, // HuC6280
        0xA8, // C140
        0xAC, // K053260
        0xB0, // Pokey
        0xB4, // QSound
        0xB8, // SCSP
        0xC0, // WonderSwan
        0xC4, // VSU
        0xC8, // SAA1099
        0xCC, // ES5503
        0xD0, // ES5506
        0xD8, // X1-010
        0xDC, // C352
        0xE0, // GA20
    };
    for (int o : kNonGmeChips) {
        if (present(o)) { return true; }
    }

    // Even for the four chips GME does handle, its Vgm_Emu instantiates only ONE
    // instance each, so a log that uses a SECOND instance (dual-chip bit 30 of
    // the clock, 0x40000000) drives the extra chip's writes out of range and
    // aborts -- e.g. Capcom arcade rips (1942) use two AY8910s, overflowing
    // Ay_Apu's 16 registers ("addr < reg_count" assert). Route any dual-chip log
    // to libvgm, which instantiates both. present() already masks bits 30-31, so
    // test the dual-chip bit explicitly here.
    static const int kGmeChips[] = {
        0x0C, // SN76489
        0x10, // YM2413
        0x2C, // YM2612
        0x74, // AY8910
    };
    for (int o : kGmeChips) {
        if (present(o) && (rd32(o) & 0x40000000u) != 0) { return true; }
    }
    return false;
}

} // namespace musix
