#ifndef STPLAYER_H
#define STPLAYER_H

#include "../../chipplugin.h"

namespace musix {

class StSoundPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "StSound"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Atari ST YM (StSound)"; }

    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // STPLAYER_H
