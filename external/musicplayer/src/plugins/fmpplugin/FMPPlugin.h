#pragma once
#include "../../chipplugin.h"
#include <set>
#include <string>

namespace musix {

class FMPPlugin : public ChipPlugin {
public:
    std::string name() const override { return "FMPPlugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "PC-98 FMP"; }
    bool canHandle(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    std::set<std::string> getSupportedExtensions() const override;
};

} // namespace musix
