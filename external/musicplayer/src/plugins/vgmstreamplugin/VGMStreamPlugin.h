#pragma once

#include "../../chipplugin.h"

namespace musix {

class VGMStreamPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "vgmstream"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
