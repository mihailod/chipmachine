#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays Beepola .bbsong files (ZX Spectrum 1-bit beeper music). A .bbsong
// declares one of ~12 beeper engines; each is a small Z80 routine that
// synthesises sound by busy-toggling the speaker (bit 4 of port 0xFE). We pack
// the song into the engine's expected data layout, run the engine's player on
// an embedded Z80 core, and sample the speaker output. See BBSongPlugin.cpp.
//
// MVP scope: the engines Shiru authored and released as public domain --
// Tritone and QChan (sourced from 1tracker). Other engines parse but currently
// throw from fromFile until their packer/player is added.
class BBSongPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "BBSong"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
