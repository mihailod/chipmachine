#pragma once

#include "../../chipplugin.h"

namespace musix {

class HEPlugin : public ChipPlugin
{
public:
    explicit HEPlugin(const std::string& biosFileName)
        : biosFileName(biosFileName)
    {}
    std::string name() const override { return "HEPlugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "PlayStation 1/2 (PSF)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;

private:
    std::string biosFileName;
    bool biosLoaded = false;
};

} // namespace musix

