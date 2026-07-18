#pragma once

#include "../../chipplugin.h"

namespace musix {

class KlystrackPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Klystrack Player"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
