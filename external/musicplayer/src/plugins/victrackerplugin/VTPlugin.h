#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays the modland "Vic-Tracker" corpus: Commodore VIC-20 music in Daniel
// Kahlin's VIC-TRACKER (.vt) format. A .vt file is a VIC-20 PRG ($3300 load
// address + a "T1"/"T0" tune struct) that the tracker's own 6502 replay routine
// interprets; we run that routine (vendored pre-assembled, BSD) on the fake6502
// core and route its VIC sound-register writes into the VIC-I core lifted from
// VICE. See vt_machine.cpp.
class VTPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "Vic-Tracker"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
