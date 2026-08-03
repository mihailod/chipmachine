#ifndef ZXAY_PLUGIN_H
#define ZXAY_PLUGIN_H

#include "../../chipplugin.h"

namespace musix {

// ZX Spectrum AY tracker formats, without a line of GPL in sight.
//
// This exists so the Mac App Store build has a ZX AY player at all: the
// engine that used to own these 67k songs (Ayfly, GPL-2, plus a GPL-2 z80ex)
// cannot ship there. See LEGAL and README.md.
//
// Registered AFTER ayflyplugin so that in the plus build -- where both are
// present and both claim the same extensions -- Ayfly still wins every tie on
// registration order and that build is bit-for-bit unchanged. In the mas build
// ayflyplugin is not compiled at all and this is the only claimant.
class ZXAYPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "ZX AY"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override
    {
        return "ZX Spectrum AY (clean room)";
    }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // ZXAY_PLUGIN_H
