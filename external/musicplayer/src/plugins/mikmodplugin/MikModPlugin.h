#ifndef MIKMODPLUGIN_H
#define MIKMODPLUGIN_H

#include "../../chipplugin.h"

namespace musix {

// Plays MikMod UNITRK / UNIMOD on-disk modules (.uni, magic "UN0x" / "APUN")
// via libmikmod. Gated tightly to .uni so it does not contest the mod-family
// extensions already owned by ModPlugin / OpenMPTPlugin / UADE.
class MikModPlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "MikMod"; }
    virtual bool canHandle(const std::string& name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

#endif // MIKMODPLUGIN_H
