#pragma once

#include "../../chipplugin.h"

namespace musix {

class KlystrackPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Klystrack Player"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "Klystrack"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
