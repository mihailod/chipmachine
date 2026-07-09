#pragma once

#include "../../chipplugin.h"

namespace musix {

class QuartetPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Quartet"; }
    virtual bool canHandle(const std::string& name) override;
    virtual ChipPlayer* fromFile(const std::string& name) override;
    virtual std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override { return {"4v", "4q"}; }
};

} // namespace musix
