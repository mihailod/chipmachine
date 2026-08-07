#pragma once

#include "../../chipplugin.h"

namespace musix {

// Commodore 264 series (C16 / 116 / Plus/4) TED music: the HVTC corpus, and the
// 264 executables from the party/demoscene collections. Clean-room -- see
// README.md for what it replaces and why.
class TEDCRPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "TED"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Commodore Plus/4 (TED) (Clean Room)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
