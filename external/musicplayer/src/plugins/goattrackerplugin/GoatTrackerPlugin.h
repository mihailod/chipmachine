#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays GoatTracker native song files (.sng) -- the C64 SID tracker by Lasse
// Oorni -- across every version: GTS! (v1), GTS2 (3-table) and GTS3/GTS4/GTS5.
// Wraps GoatTracker's own playback core (gplay.c sequencer + gsid.cpp) driving
// vendored reSID. ".sng" is heavily overloaded, so the format is claimed by
// content magic only.
class GoatTrackerPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "GoatTracker"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
