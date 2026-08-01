#ifndef S98_PLAYER_H
#define S98_PLAYER_H

#include "../../chipplugin.h"

namespace musix {

class S98Plugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "S98"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const override { return "PC-98 FM (S98)"; }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // S98_PLAYER_H
