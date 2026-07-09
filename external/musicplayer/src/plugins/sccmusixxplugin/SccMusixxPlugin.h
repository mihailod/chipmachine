#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays SCC-Musixx .SNG files: tracker music for Konami's SCC wavetable chip,
// authored with Tyfoon-Software's SCC-MUSIXX (MSX, 1990). A .SNG is a raw MSX
// memory image; we run the original SCC-MUSIXX replay routine on an embedded
// Z80 core and route its SCC register writes into the emu2212 SCC emulator.
// See SccMusixxPlugin.cpp / scc_machine.cpp.
//
// These files carry the lowercase extension "sng", which is shared by several
// unrelated formats (AdLib SNGPlay/Faust/AdLib Tracker, GoatTracker, Amiga
// SoundMon/Richard Joseph). canHandle() therefore content-detects the SCC-
// Musixx waveform-bank layout and declines everything else.
class SccMusixxPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "SCC-Musixx"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
