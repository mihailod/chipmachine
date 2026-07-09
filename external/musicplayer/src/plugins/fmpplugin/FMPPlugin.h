#pragma once
#include "../../chipplugin.h"
#include <set>
#include <string>

namespace musix {

class FMPPlugin : public ChipPlugin {
public:
    std::string name() const override { return "FMPPlugin"; }
    bool canHandle(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
    std::set<std::string> getSupportedExtensions() const override;
};

} // namespace musix
