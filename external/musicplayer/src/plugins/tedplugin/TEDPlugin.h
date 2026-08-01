#pragma once

#include "../../chipplugin.h"

namespace musix {

class TEDPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "Tedplay"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Commodore Plus/4 (TED)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

