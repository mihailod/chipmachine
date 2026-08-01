#pragma once

#include "../../chipplugin.h"

namespace musix {

class V2Plugin : public ChipPlugin
{
public:
    V2Plugin();
    std::string name() const override { return "V2Plugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Farbrausch V2"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

