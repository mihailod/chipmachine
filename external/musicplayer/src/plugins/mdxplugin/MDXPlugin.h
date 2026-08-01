#pragma once

#include "../../chipplugin.h"

namespace musix {

class MDXPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "MDX"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "Sharp X68000 (MDX)"; }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string& name) override;
};

} // namespace musix

