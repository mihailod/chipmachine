#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays NerdTracker II (.ned) modules -- Michel Iwaniec's MS-DOS NES/Famicom
// tracker. The replay engine is the player core of thefox's "NerdTracker 2 SDL
// port", driving blargg's Nes_Snd_Emu 2A03 (APU) emulation. See NEDPlugin.cpp
// and ned/ned_engine.c.
class NEDPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "NerdTracker2"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
