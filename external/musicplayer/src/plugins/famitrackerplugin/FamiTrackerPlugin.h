#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays FamiTracker modules (.ftm) for the NES/Famicom 2A03 and its expansion
// chips (VRC6, VRC7, MMC5, FDS). The decode engine is a vendored, boost-free
// slice of nukep's cross-platform "FamiTracker CX" (repo-root famitracker-cx/),
// driven synchronously for chipmachine's pull-based host.
//
// NOTE on the extension collision: .ftm is overloaded. The Atari "Face The
// Music" format (magic "FTMN") is handled by OpenMPT; FamiTracker modules carry
// the magic "FamiTracker Module". canHandle() content-gates on that magic so the
// two formats coexist.
//
// Known gap: Namco 163 (N163) and Sunsoft 5B expansion modules are not driven
// (upstream never wired their channel handlers); those rare modules will fail
// to load rather than play wrong.
class FamiTrackerPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "FamiTracker"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
