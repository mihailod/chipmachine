#pragma once

#include "../../chipplugin.h"

namespace musix {

class AdPlugin : public ChipPlugin
{
public:
    explicit AdPlugin(const std::string& configDir) : configDir(configDir) {}
    std::string name() const override { return "AdPlug"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "AdLib / OPL (AdPlug)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    std::vector<std::string>
    getSecondaryFiles(const std::string& file) override;

private:
    std::string configDir;
};

} // namespace musix

