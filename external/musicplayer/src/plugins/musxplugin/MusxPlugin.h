#pragma once

#include "../../chipplugin.h"

namespace musix {

// Archimedes Tracker (.musx) -- the native 8-channel "!Tracker" format from the
// Acorn Archimedes. Played via libxmp's arch_loader (vendored under
// zxtune/3rdparty/xmp). We build a minimal single-loader slice of libxmp here
// and gate canHandle to .musx so we never overlap OpenMPT's Amiga/PC formats.
class MusxPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "Archimedes Tracker"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
