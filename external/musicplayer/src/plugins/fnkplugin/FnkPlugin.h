#pragma once

#include "../../chipplugin.h"

namespace musix {

// Funktracker (.fnk) -- Elias Ehlin's 1994-96 MS-DOS tracker (FunktrackerGOLD /
// Funktracker DOS32), a sample tracker aimed at funk/hiphop with 4-32 channels.
// Files begin with the ASCII magic "Funk". Played via libxmp's fnk_loader
// (vendored under zxtune/3rdparty/xmp); we build a minimal single-loader slice
// here and gate canHandle to .fnk, so we never overlap OpenMPT's formats.
class FnkPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Funktracker"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
