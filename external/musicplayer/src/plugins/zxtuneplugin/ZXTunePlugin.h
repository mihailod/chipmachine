#pragma once

#include "../../chipplugin.h"

namespace musix {

// ZX Spectrum AY/YM tracker playback via the vendored ZXTune engine
// (src lives in chipmachine-as/zxtune/, a checkout of djdron/zxtune @ cmake).
//
// Currently scoped to Modland's Sound Tracker 1.1 collection (.st11): those
// files are an "ZXAYST11" container wrapping a raw, uncompiled Sound Tracker
// v1.x module at offset 0x38 that no other engine in this project decodes
// (libayfly/ayfly only handles the *compiled* STC variant). ZXTune's raw
// container scanner locates the embedded module and its ST1 player renders it.
class ZXTunePlugin : public ChipPlugin {
public:
    virtual std::string name() const override { return "ZX Spectrum (ZXTune)"; }
    virtual bool canHandle(const std::string &name) override;
    virtual std::set<std::string> getSupportedExtensions() const override;
    virtual ChipPlayer *fromFile(const std::string &fileName) override;
};

} // namespace musix
