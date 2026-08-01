#ifndef HIVELY_PLAYER_H
#define HIVELY_PLAYER_H

#include "../../chipplugin.h"

namespace musix {

class HivelyPlugin : public ChipPlugin
{
public:
    HivelyPlugin();
    std::string name() const override { return "HivelyPlugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "HivelyTracker (AHX/HVL)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // HIVELY_PLAYER_H
