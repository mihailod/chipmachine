#pragma once

#include "../../chipplugin.h"

namespace musix {

class PxTonePlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "PxTone Collage Player"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "PxTone Collage"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
