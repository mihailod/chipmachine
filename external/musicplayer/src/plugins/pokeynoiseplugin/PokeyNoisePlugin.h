#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays Atari 8-bit PokeyNoise (.pn) music. These are raw Atari executables
// (6502 player code + data driven by a ~50Hz VBL interrupt); we run them on a
// vendored ASAP 6502+POKEY core. See PokeyNoisePlugin.cpp for the details.
class PokeyNoisePlugin : public ChipPlugin
{
public:
    std::string name() const override { return "PokeyNoise"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
