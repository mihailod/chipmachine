#pragma once

#include "../../chipplugin.h"

namespace musix {

class PTKPlugin : public ChipPlugin
{
public:
    PTKPlugin();
    virtual ~PTKPlugin() = default;
    std::string name() const override { return "ProTrekkr"; }
    bool canHandle(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& name) override;
};

} // namespace musix
