#pragma once

#include "../../chipplugin.h"

#include <string>

namespace musix {

// Commodore 64 SID playback on Hermit's cSID (WTFPL) -- a from-scratch
// 6581/8580 + 6510 + PSID player with no VICE/reSID lineage, which is why it can
// ship in the Mac App Store variant where vicepluginbridge (GPL) cannot.
//
// Claims .sid/.rsid ONLY. It deliberately does NOT claim Compute! Sidplayer
// .mus/.str: those are a note format rather than a SID-chip dump, cSID has no
// parser for them, and it would happily read their first bytes as a PSID header
// and emit garbage. Those stay with vicepluginbridge in the plus build and are
// dropped from the catalog in the mas build. See CSIDPlugin.cpp.
class CSIDPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "cSID"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Commodore 64 SID (cSID)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
