#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays MaxTrax (.mxtx) Amiga modules -- the commercial sound engine used by
// e.g. Cyberdreams' Dark Seed (David A. Bean). A single .mxtx holds the tempo,
// MIDI-like score events and the sampled instruments; playback is driven by a
// vendored port of ScummVM's portable MaxTrax sequencer over its Paula mixer.
// See MaxTraxPlugin.cpp for details. UADE is not involved: its amifilemagic
// detects the MXTX magic but ships no eagleplayer for the format.
class MaxTraxPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "MaxTrax"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
