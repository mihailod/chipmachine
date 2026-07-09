#ifndef HTPLAYER_H
#define HTPLAYER_H

#include "../../chipplugin.h"

namespace musix {

class HTPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "HTPlugin"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // HTPLAYER_H
