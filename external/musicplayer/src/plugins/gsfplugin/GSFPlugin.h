#ifndef GSFPLAYER_H
#define GSFPLAYER_H

#include "../../chipplugin.h"

namespace musix {

class GSFPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Gameboy Advance"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string &name) override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix

#endif // GSFPLAYER_H