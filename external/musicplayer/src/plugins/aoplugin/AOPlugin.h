#pragma once

#include "../../chipplugin.h"

namespace musix {

class AOPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Audio Overload"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "Capcom QSound / PS1 SPU"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string &name) override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
